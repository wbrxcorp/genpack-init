#ifdef TEST
//
#include <pybind11/embed.h>
#include "native/logging.h"

using namespace pybind11::literals;

struct LoggingOptions {
    std::optional<std::string> logfile;
    bool debug = false;
};

static void setup_logging(LoggingOptions options = {})
{
    auto logging_module = pybind11::module_::import("logging");
    pybind11::list handlers;
    if (options.logfile) handlers.append(logging_module.attr("FileHandler")(options.logfile->c_str(), "mode"_a = "w"));
    handlers.append(logging_module.attr("StreamHandler")());
    logging_module.attr("basicConfig")("level"_a = logging_module.attr(options.debug? "DEBUG":"INFO"), 
        "format"_a = "%(asctime)s %(levelname)s %(filename)s:%(lineno)d %(message)s",
        "handlers"_a = handlers,
        "force"_a = true);
    logging::set_info([](const std::string& msg){ pybind11::module_::import("logging").attr("info")(msg); });
    logging::set_debug([](const std::string& msg){ pybind11::module_::import("logging").attr("debug")(msg); });
    logging::set_warning([](const std::string& msg){ pybind11::module_::import("logging").attr("warning")(msg); });
    logging::set_error([](const std::string& msg){ pybind11::module_::import("logging").attr("error")(msg); });
}

int main()
{
    pybind11::scoped_interpreter guard{};
    setup_logging({.debug = true});
    logging::info("info");
    logging::debug("debug");
    logging::warning("warning");
    logging::error("error");
    return 0;
}
#endif