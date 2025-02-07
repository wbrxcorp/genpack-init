#include <functional>
#include <string>

namespace logging {
    void set_info(const std::function<void(const std::string&)> _info);
    void set_warning(const std::function<void(const std::string&)> _warning);
    void set_debug(const std::function<void(const std::string&)> _debug);
    void set_error(const std::function<void(const std::string&)> _error);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void debug(const std::string& msg);
    void error(const std::string& msg);
}