#include <iostream>

#include "logging.h"

static std::function<void(const std::string&)> info = [](const std::string& msg){std::cout << "INFO: " << msg << std::endl;};
static std::function<void(const std::string&)> warning = [](const std::string& msg){std::cerr << "WARNING: " << msg << std::endl;};
static std::function<void(const std::string&)> debug = [](const std::string& msg){std::cout << "DEBUG: " << msg << std::endl;};
static std::function<void(const std::string&)> error = [](const std::string& msg){std::cerr << "ERROR: " << msg << std::endl;};

namespace logging {
    void set_info(const std::function<void(const std::string&)> _info) {
        ::info = _info;
    }
    void set_warning(const std::function<void(const std::string&)> _warning) {
        ::warning = _warning;
    }
    void set_debug(const std::function<void(const std::string&)> _debug) {
        ::debug = _debug;
    }
    void set_error(const std::function<void(const std::string&)> _error) {
        ::error = _error;
    }
    void info(const std::string& msg) {
        ::info(msg);
    }
    void warning(const std::string& msg) {
        ::warning(msg);
    }
    void debug(const std::string& msg) {
        ::debug(msg);
    }
    void error(const std::string& msg) {
        ::error(msg);
    }
}