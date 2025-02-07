#include <regex>

#include "logging.h"
#include "formatter.h"
#include "filesystem.h"
#include "subprocess.h"

static std::string remove_leading_slashes(const std::string& str)
{
    return std::regex_replace(str, std::regex("^/+"), "");
}

std::filesystem::path boot_path(const std::vector<std::filesystem::path>& path)
{
    std::filesystem::path complete_path("/run/initramfs/boot");
    for (const auto& p: path) {
        complete_path /= remove_leading_slashes(p.string());
    }
    return complete_path;
}

std::filesystem::path root_path(const std::vector<std::filesystem::path>& path)
{
    std::filesystem::path complete_path("/");
    for (const auto& p: path) {
        complete_path /= remove_leading_slashes(p.string());
    }
    return complete_path;
}

std::filesystem::path ro_path(const std::vector<std::filesystem::path>& path)
{
    std::filesystem::path complete_path("/run/initramfs/ro");
    for (const auto& p: path) {
        complete_path /= remove_leading_slashes(p.string());
    }
    return complete_path;
}

std::filesystem::path rw_path(const std::vector<std::filesystem::path>& path)
{
    std::filesystem::path complete_path("/run/initramfs/rw");
    for (const auto& p: path) {
        complete_path /= remove_leading_slashes(p.string());
    }
    return complete_path;
}


void ensure_dir_exists(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}

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
