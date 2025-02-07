#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "native/logging.h"
#include "native/coldplug.h"
#include "native/disk.h"
#include "native/filesystem.h"
#include "native/platform.h"
#include "native/systemd.h"
#include "native/formatter.h"

#include "repl.h"

using namespace pybind11::literals;

template <auto F>
class PathLike {
    std::filesystem::path path;
public:
    PathLike(pybind11::args args) {
        std::vector<std::filesystem::path> paths;
        for (const auto& arg: args) {
            paths.push_back(arg.cast<std::filesystem::path>());
        }
        path = F({paths.begin(), paths.end()});
    }
    std::string __fspath__() const {
        return path.string();
    }
    std::string __repr__() const {
        return std::format("PathLike({})", path);
    }
    PathLike<F>* ensure_dir_exists() {
        ::ensure_dir_exists(path);
        return this;
    }
};

void setup_genpack_init_module()
{
    auto modules = pybind11::module_::import("sys").attr("modules");
    if (modules.contains("genpack_init")) return;
    //else
    auto dynamic_mod = pybind11::module_::create_extension_module(
        "genpack_init", nullptr, new pybind11::module::module_def()
    );
    modules["genpack_init"] = dynamic_mod;

    // coldplug functions
    dynamic_mod.def("coldplug", coldplug);
    // for backward compatibility
    auto builtins = pybind11::module_::import("builtins");
    builtins.attr("coldplug") = pybind11::cpp_function(coldplug);

    // disk functions
    dynamic_mod.def("get_block_device_info", [](const std::filesystem::path& path) -> pybind11::object {
        auto info = get_block_device_info(path);
        if (!info) return pybind11::none();
        //else
        pybind11::dict d;
        d["name"] = path;
        d["logical_sector_size"] = info->logical_sector_size;
        d["physical_sector_size"] = info->physical_sector_size;
        d["num_logical_sectors"] = info->num_logical_sectors;
        return d;
    }, "path"_a);
    dynamic_mod.def("get_partition_info", [](const std::filesystem::path& path) -> pybind11::object {
        auto info = get_partition_info(path);
        if (!info) return pybind11::none();
        //else
        pybind11::dict d;
        d["name"] = path;
        d["uuid"] = info->uuid;
        d["type"] = info->type;
        return d;
    }, "path"_a);
    dynamic_mod.def("parted", parted, "disk"_a, "command"_a);
    dynamic_mod.def("mkfs", mkfs, "device"_a, "fstype"_a, "label"_a = pybind11::none());
    dynamic_mod.def("mkswap", mkswap, "device"_a, "label"_a = pybind11::none());
    dynamic_mod.def("mount", mount, "device"_a, "mountpoint"_a, pybind11::kw_only(), "fstype"_a = pybind11::none(), "options"_a = pybind11::none());
    dynamic_mod.def("umount", umount, "mountpoint"_a);

    // platform functions
    dynamic_mod.def("is_raspberry_pi", is_raspberry_pi);
    dynamic_mod.def("is_qemu", is_qemu);
    dynamic_mod.def("read_qemu_firmware_config", read_qemu_firmware_config, "name"_a);
    
    // systemd functions
    dynamic_mod.def("enable_systemd_service", enable_systemd_service, "name"_a);
    dynamic_mod.def("disable_systemd_service", disable_systemd_service, "name"_a);

    // filesystem functions
    dynamic_mod.def("ensure_dir_exists", [](const std::string& path) {
        ensure_dir_exists(path);
    });
    dynamic_mod.def("chown", [](const std::string& user, pybind11::args paths, const std::optional<std::string>& group, bool recursive) {
        std::vector<std::filesystem::path> paths_;
        for (const auto& path: paths) {
            paths_.push_back(path.cast<std::filesystem::path>());
        }
        return chown(user, paths_, group, recursive);
    }, "user"_a, pybind11::kw_only(), "group"_a = pybind11::none(), "recursive"_a = false);

    dynamic_mod.def("chgrp", [](const std::string& group, pybind11::args paths, bool recursive) {
        std::vector<std::filesystem::path> paths_;
        for (const auto& path: paths) {
            paths_.push_back(path.cast<std::filesystem::path>());
        }
        return chgrp(group, paths_, recursive);
    }, "user"_a, pybind11::kw_only(), "recursive"_a = false);

    dynamic_mod.def("chmod", [](const std::string& mode, pybind11::args paths, bool recursive) {
        std::vector<std::filesystem::path> paths_;
        for (const auto& path: paths) {
            paths_.push_back(path.cast<std::filesystem::path>());
        }
        return chmod(mode, paths_, recursive);
    }, "mode"_a, pybind11::kw_only(), "recursive"_a = false);

    auto os = pybind11::module_::import("os");
    auto PathLike_register = os.attr("PathLike").attr("register");

    PathLike_register(
        pybind11::class_<PathLike<root_path>>(dynamic_mod, "RootPath")
            .def(pybind11::init<pybind11::args>())
            .def("__repr__", &PathLike<root_path>::__repr__)
            .def("__fspath__", &PathLike<root_path>::__fspath__)
            .def("ensure_dir_exists", &PathLike<root_path>::ensure_dir_exists)
    );

    PathLike_register(
        pybind11::class_<PathLike<boot_path>>(dynamic_mod, "BootPath")
            .def(pybind11::init<pybind11::args>())
            .def("__repr__", &PathLike<boot_path>::__repr__)
            .def("__fspath__", &PathLike<boot_path>::__fspath__)
            .def("ensure_dir_exists", &PathLike<boot_path>::ensure_dir_exists)
    );

    PathLike_register(
        pybind11::class_<PathLike<ro_path>>(dynamic_mod, "ROPath")
            .def(pybind11::init<pybind11::args>())
            .def("__repr__", &PathLike<ro_path>::__repr__)
            .def("__fspath__", &PathLike<ro_path>::__fspath__)
            // ensure_dir_exists is not applicable to ROPath
    );

    PathLike_register(
        pybind11::class_<PathLike<rw_path>>(dynamic_mod, "RWPath")
            .def(pybind11::init<pybind11::args>())
            .def("__repr__", &PathLike<rw_path>::__repr__)
            .def("__fspath__", &PathLike<rw_path>::__fspath__)
            .def("ensure_dir_exists", &PathLike<rw_path>::ensure_dir_exists)
    );
}

#ifdef TEST
int main()
{
    pybind11::scoped_interpreter guard{};
    setup_genpack_init_module();
    repl();
    return 0;
}
#endif