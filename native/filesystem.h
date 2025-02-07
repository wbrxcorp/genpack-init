#include <filesystem>
#include <vector>

std::filesystem::path boot_path(const std::vector<std::filesystem::path>& path);
std::filesystem::path root_path(const std::vector<std::filesystem::path>& path);
std::filesystem::path ro_path(const std::vector<std::filesystem::path>& path);
std::filesystem::path rw_path(const std::vector<std::filesystem::path>& path);
void ensure_dir_exists(const std::filesystem::path& path);
int chown(const std::string& user, const std::vector<std::filesystem::path>& paths, const std::optional<std::string>& group = std::nullopt, bool recursive = false);
int chgrp(const std::string& group, const std::vector<std::filesystem::path>& paths, bool recursive = false);
int chmod(const std::string& mode, const std::vector<std::filesystem::path>& paths, bool recursive = false);
