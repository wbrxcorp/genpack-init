#include <format>

#include "subprocess.h"
#include "filesystem.h"
#include "logging.h"

int enable_systemd_service(const std::string& name)
{
    int rst = run_subprocess({"systemctl", "enable", name});
    if (rst == 0) {
        logging::info(std::format("Systemd service {} enabled.", name));
    } else {   
        logging::error(std::format("Failed to enable systemd service {}.", name));
    }
    return rst;
}

int disable_systemd_service(const std::string& name)
{
    int rst = run_subprocess({"systemctl", "disable", name});
    if (rst == 0) {
        logging::info(std::format("Systemd service {} disabled.", name));
    } else {   
        logging::error(std::format("Failed to disable systemd service {}.", name));
    }
    return rst;
}
