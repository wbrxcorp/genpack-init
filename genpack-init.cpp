#include <sys/reboot.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>

#include <pybind11/embed.h>

#include "coldplug.h"

using namespace pybind11::literals;

auto load_inifile(const std::filesystem::path& path)
{
    auto configparser = pybind11::module::import("configparser").attr("ConfigParser")();
    std::ifstream ifs(path);
    if (ifs) {
        std::string ini_content = "[_default]\n" + std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
        try {
            configparser.attr("read_string")(ini_content);
        }
        catch (const std::exception& e) {
            // No use logging here, as logging is not yet configured
            std::cerr << "Error parsing ini file '" + path.string() + "'. Proceeding with empty configuration." << std::endl;
        }
    } else {
        std::cerr << "No ini file found. Proceeding with empty configuration." << std::endl;
    }
    return configparser;
}

struct Options {
    std::filesystem::path inifile;
    std::filesystem::path modules_dir;
    std::optional<std::filesystem::path> logfile;
    bool coldplug_mock = false;
};

int run(const Options& options = {})
{
    auto sys = pybind11::module::import("sys");
    // disable writing of .pyc files
    sys.attr("dont_write_bytecode") = true;

    auto inifile = load_inifile(options.inifile);
    auto debug = inifile.attr("getboolean")("_default", "debug", "fallback"_a = false).cast<bool>();

    // setup logging
    auto logging = pybind11::module::import("logging");
    pybind11::list handlers;
    if (options.logfile) handlers.append(logging.attr("FileHandler")(options.logfile->c_str(), "mode"_a = "w"));
    handlers.append(logging.attr("StreamHandler")());
    logging.attr("basicConfig")("level"_a = logging.attr(debug? "DEBUG":"INFO"), 
        "format"_a = "%(asctime)s %(levelname)s %(filename)s:%(lineno)d %(message)s",
        "handlers"_a = handlers,
        "force"_a = true);
    auto logging_info = logging.attr("info");
    auto logging_debug = logging.attr("debug");
    auto logging_error = logging.attr("error");
    if (debug) {
        logging_debug("Debug mode enabled");
    }

    // register coldplug function
    auto modules = sys.attr("modules");
    if (!modules.contains("genpack_init")) {
        auto dynamic_mod = pybind11::module_::create_extension_module(
            "genpack_init", nullptr, new pybind11::module::module_def()
        );
        modules["genpack_init"] = dynamic_mod;
        dynamic_mod.def("coldplug", &(options.coldplug_mock? coldplug_mock: coldplug));
    }
    auto builtins = pybind11::module::import("builtins");
    builtins.attr("coldplug") = pybind11::cpp_function(&(options.coldplug_mock? coldplug_mock: coldplug));

    // load and run every .py file in the inifile_dir
    if (!std::filesystem::is_directory(options.modules_dir)) {
        logging_error("No such directory: " + options.modules_dir.string());
        return 1;
    }
    //else

    auto machinery = pybind11::module::import("importlib.machinery");
    auto signature = pybind11::module::import("inspect").attr("signature");
    for (const auto& entry: std::filesystem::directory_iterator(options.modules_dir)) {
        if (entry.path().extension() != ".py") continue;
        //else
        try {
            auto sourceFileLoader = machinery.attr("SourceFileLoader")("mymodule", entry.path().c_str());
            auto _module = sourceFileLoader.attr("load_module")();
            if (!pybind11::hasattr(_module, "configure")) {
                logging_info("No configure function found in " + entry.path().string() + ". Skipping.");
                continue;
            }
            //else
            auto configure_func = _module.attr("configure");
            auto parameters = signature(configure_func).attr("parameters").cast<pybind11::dict>();
            auto arglen = pybind11::len(parameters);
            if (arglen == 1) {
                configure_func(inifile);
            } else if (arglen == 0) {
                configure_func();
            } else {
                throw std::runtime_error("configure function must have 0 or 1 argument.");
            }
        }
        catch (const std::exception& e) {
            logging_error(entry.path().string() + ": " + e.what());
        }
    }
    return 0;
}

int main(int argc, char* argv[])
{
    auto running_as_init = (getpid() == 1 && getuid() == 0);
    if (running_as_init) {
        {
            pybind11::scoped_interpreter guard{};
            const auto inifile_dir = std::filesystem::path(
                std::filesystem::is_directory("/run/initramfs/boot")? 
                  "/run/initramfs/boot":"/run/initramfs/rw"
            );
            try {
                run({.inifile = inifile_dir / "system.ini", 
                    .modules_dir = "/usr/lib/genpack-init",
                    .logfile = "/var/log/genpack-init.log"});
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
        }
        // exec /sbin/init or /usr/bin/init
        execl("/sbin/init", "/sbin/init", nullptr);
        //else
        execl("/usr/bin/init", "/usr/bin/init", nullptr);
        //else there is nothing we can do
        reboot(RB_HALT_SYSTEM);
    } else {
        pybind11::scoped_interpreter guard{};
        try {
            return run({.inifile = "test/test.ini", .modules_dir = "./test", .coldplug_mock = true});
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    return 1;
}