#include <regex>

#include "logging.h"
#include "formatter.h"
#include "filesystem.h"
#include "subprocess.h"

int chown(const std::string& user, const std::vector<std::filesystem::path>& paths, const std::optional<std::string>& group, bool recursive)
{
    std::vector<std::string> cmdline = {"chown"};
    if (recursive) {
        cmdline.push_back("-R");
    }
    cmdline.push_back(user + (group? ":" + *group: ""));
    for (const auto& path: paths) {
        cmdline.push_back(path.string());
    }
    return run_subprocess(cmdline);
}

int chgrp(const std::string& group, const std::vector<std::filesystem::path>& paths, bool recursive)
{
    std::vector<std::string> cmdline = {"chgrp"};
    if (recursive) {
        cmdline.push_back("-R");
    }
    cmdline.push_back(group);
    for (const auto& path: paths) {
        cmdline.push_back(path.string());
    }
    return run_subprocess(cmdline);
}

int chmod(const std::string& mode, const std::vector<std::filesystem::path>& paths, bool recursive)
{
    std::vector<std::string> cmdline = {"chmod"};
    if (recursive) {
        cmdline.push_back("-R");
    }
    cmdline.push_back(mode);
    for (const auto& path: paths) {
        cmdline.push_back(path.string());
    }
    return run_subprocess(cmdline);
}
