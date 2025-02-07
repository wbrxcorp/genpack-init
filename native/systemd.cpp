#include <format>

#include "subprocess.h"
#include "filesystem.h"
#include "logging.h"

int enable_systemd_service(const std::string& name)
{
    logging::info(std::format("Enabling systemd service {}.", name));
    return run_subprocess({"systemctl", "enable", name});
}

int disable_systemd_service(const std::string& name)
{
    logging::info(std::format("Disabling systemd service {}.", name));
    return run_subprocess({"systemctl", "disable", name});
}
