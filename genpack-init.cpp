#include <sys/reboot.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <functional>
#include <format>

#include <pybind11/embed.h>
#include <argparse/argparse.hpp>

#include "native/logging.h"

#include "module.h"
#include "repl.h"

using namespace pybind11::literals;

auto load_inifile(const std::filesystem::path& path)
{
    auto configparser = pybind11::module_::import("configparser").attr("ConfigParser")();
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

int run_as_init()
{
    auto sys = pybind11::module_::import("sys");
    // disable writing of .pyc files
    sys.attr("dont_write_bytecode") = true;

    const auto inifile_dir = std::filesystem::path(
        std::filesystem::is_directory("/run/initramfs/boot")? 
            "/run/initramfs/boot":"/run/initramfs/rw"
    );
    auto inifile = load_inifile(inifile_dir / "system.ini");
    auto debug = inifile.attr("getboolean")("_default", "debug", "fallback"_a = false).cast<bool>();

    // setup logging
    auto logging = pybind11::module_::import("logging");
    pybind11::list handlers;
    handlers.append(logging.attr("FileHandler")("/var/log/genpack-init.log", "mode"_a = "w"));
    handlers.append(logging.attr("StreamHandler")());
    logging.attr("basicConfig")("level"_a = logging.attr(debug? "DEBUG":"INFO"), 
        "format"_a = "%(asctime)s %(levelname)s %(filename)s:%(lineno)d %(message)s",
        "handlers"_a = handlers,
        "force"_a = true);
    logging::set_info([](const std::string& msg){ pybind11::module_::import("logging").attr("info")(msg); });
    logging::set_debug([](const std::string& msg){ pybind11::module_::import("logging").attr("debug")(msg); });
    logging::set_warning([](const std::string& msg){ pybind11::module_::import("logging").attr("warning")(msg); });
    logging::set_error([](const std::string& msg){ pybind11::module_::import("logging").attr("error")(msg); });

    if (debug) {
        logging::debug("Debug mode enabled");
    }

    setup_genpack_init_module();

    // load and run every .py file in the inifile_dir
    if (!std::filesystem::is_directory("/usr/lib/genpack-init")) {
        logging::error("Directory /usr/lib/genpack-init not found.");
        return 1;
    }
    //else

    auto machinery = pybind11::module::import("importlib.machinery");
    auto signature = pybind11::module::import("inspect").attr("signature");
    for (const auto& entry: std::filesystem::directory_iterator("/usr/lib/genpack-init")) {
        if (entry.path().extension() != ".py") continue;
        //else
        try {
            auto sourceFileLoader = machinery.attr("SourceFileLoader")("mymodule", entry.path().c_str());
            auto _module = sourceFileLoader.attr("load_module")();
            if (!pybind11::hasattr(_module, "configure")) {
                logging::info("No configure function found in " + entry.path().string() + ". Skipping.");
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
            logging::error(entry.path().string() + ": " + e.what());
        }
    }
    return 0;
}

#if 0
int run(const std::filesystem::path& script_file, 
    const std::filesystem::path& inifile)
{
    // disable writing of .pyc files
    pybind11::module_::import("sys").attr("dont_write_bytecode") = true;

    // setup logging
    auto logging = pybind11::module_::import("logging");
    logging.attr("basicConfig")(pybind11::arg("level") = "DEBUG");
    
    setup_genpack_init_module(root_path, boot_path, ro_path, rw_path);

    auto machinery = pybind11::module::import("importlib.machinery");
    auto signature = pybind11::module::import("inspect").attr("signature");
    auto sourceFileLoader = machinery.attr("SourceFileLoader")("mymodule", script_file.c_str());
    auto module_ = sourceFileLoader.attr("load_module")();
    if (!pybind11::hasattr(module_, "configure")) {
        throw std::runtime_error("No configure function found in " + script_file.string() + ".");
    }
    //else
    auto configure_func = module_.attr("configure");
    auto parameters = signature(configure_func).attr("parameters").cast<pybind11::dict>();
    auto arglen = pybind11::len(parameters);
    if (arglen == 1) {
        configure_func(load_inifile(inifile));
    } else if (arglen == 0) {
        configure_func();
    } else {
        throw std::runtime_error("configure function must have 0 or 1 argument.");
    }
    return 0;
}
#endif

int main(int argc, char* argv[])
{
    auto running_as_init = (getpid() == 1 && getuid() == 0);
    if (running_as_init) {
        {
            pybind11::scoped_interpreter guard{};
            try {
                run_as_init();
            }
            catch (const std::exception& e) {
                // guard must be still alive here because the exception may be thrown from python interpreter
                std::cerr << e.what() << std::endl;
            }
        }
        // exec /sbin/init or /usr/bin/init
        execl("/sbin/init", "/sbin/init", nullptr);
        //else
        execl("/usr/bin/init", "/usr/bin/init", nullptr);
        //else there is nothing we can do
        reboot(RB_HALT_SYSTEM);
    }
    // else 
    pybind11::scoped_interpreter guard{};

    try {
        setup_genpack_init_module();
        std::string red_begin = "\033[31m";
        std::string red_end = "\033[0m";
        return repl();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 1;
}