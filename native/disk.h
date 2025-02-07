#include <optional>
#include <map>
#include <string>
#include <cstdint>

struct BlockDeviceInfo {
    uint32_t logical_sector_size;
    uint32_t physical_sector_size;
    uint64_t num_logical_sectors;
};

struct PartitionInfo {
    std::optional<std::string> uuid;
    std::optional<std::string> label;
    std::optional<std::string> type;
};

std::optional<BlockDeviceInfo> get_block_device_info(const std::filesystem::path& path);
std::optional<PartitionInfo> get_partition_info(const std::filesystem::path& path);
int parted(const std::filesystem::path& disk, const std::string& command);
int mkfs(const std::filesystem::path& device, const std::string& fstype, const std::optional<std::string>& label = std::nullopt);
int mkswap(const std::filesystem::path& device, const std::optional<std::string>& label = std::nullopt);
int mount(const std::filesystem::path& device, const std::filesystem::path& mountpoint, const std::optional<std::string>& fstype, const std::optional<std::string>& options);
int umount(const std::filesystem::path& mountpoint);