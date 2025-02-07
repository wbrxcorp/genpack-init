#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>

#include <memory>
#include <cstring>
#include <filesystem>

#include <blkid/blkid.h>

#include "disk.h"
#include "logging.h"
#include "subprocess.h"
#include "formatter.h"

std::optional<BlockDeviceInfo> get_block_device_info(const std::filesystem::path& path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            logging::debug(std::format("Block device {} not found", path));
            return std::nullopt;
        }
        logging::error(std::format("open({}) failed: {}", path, std::string(strerror(errno))));
        return std::nullopt;
    }
    //else
    uint32_t logical_sector_size;
    if (ioctl(fd, BLKSSZGET, &logical_sector_size) < 0) {
        logging::error(std::format("ioctl(BLKSSZGET) failed: {}", path, std::string(strerror(errno))));
        close(fd);
        return std::nullopt;
    }

    uint32_t physical_sector_size;
    if (ioctl(fd, BLKPBSZGET, &physical_sector_size) < 0) {
        logging::error(std::format("ioctl(BLKPBSZGET) failed: {}", path, std::string(strerror(errno))));
        close(fd);
        return std::nullopt;
    }

    uint64_t disk_size;
    if (ioctl(fd, BLKGETSIZE64, &disk_size) < 0) {
        logging::error(std::format("ioctl(BLKGETSIZE64) failed: {}", path, std::string(strerror(errno))));
        close(fd);
        return std::nullopt;
    }

    close(fd);

    return BlockDeviceInfo({
        .logical_sector_size = logical_sector_size,
        .physical_sector_size = physical_sector_size,
        .num_logical_sectors = disk_size / logical_sector_size
    });
}

std::optional<PartitionInfo> get_partition_info(const std::filesystem::path& path)
{
    auto _probe = blkid_new_probe_from_filename(path.c_str());
    if (!_probe) {
        logging::debug(std::format("Partition {} cannot be probed", path));
        return std::nullopt;
    }
    std::shared_ptr<blkid_struct_probe> probe(_probe, blkid_free_probe);

    // デバイスのスキャンを実行
    blkid_do_probe(probe.get());

    // 各属性を取得
    const char *uuid = NULL, *label = NULL, *type = NULL;

    blkid_probe_lookup_value(probe.get(), "UUID", &uuid, NULL);
    blkid_probe_lookup_value(probe.get(), "LABEL", &label, NULL);
    blkid_probe_lookup_value(probe.get(), "TYPE", &type, NULL);

    return std::optional<PartitionInfo>({
        .uuid = uuid? std::optional<std::string>(uuid): std::nullopt,
        .label = label? std::optional<std::string>(label): std::nullopt,
        .type = type? std::optional<std::string>(type): std::nullopt
    });
}

int parted(const std::filesystem::path& disk, const std::string& command)
{
    return run_subprocess({"parted", disk, command});
}

int mkfs(const std::filesystem::path& device, const std::string& fstype, const std::optional<std::string>& label)
{
    std::vector<std::string> cmdline = {"mkfs." + fstype};
    if (label) {
        cmdline.push_back("-L");
        cmdline.push_back(*label);
    }
    cmdline.push_back(device);

    return run_subprocess(cmdline);
}

int mkswap(const std::filesystem::path& device, const std::optional<std::string>& label)
{
    std::vector<std::string> cmdline = {"mkswap"};
    if (label) {
        cmdline.push_back("-L");
        cmdline.push_back(*label);
    }
    cmdline.push_back(device);
    return run_subprocess(cmdline);
}

int mount(const std::filesystem::path& device, const std::filesystem::path& mountpoint, const std::optional<std::string>& fstype, const std::optional<std::string>& options)
{
    std::vector<std::string> cmdline = {"mount"};
    if (fstype) {
        cmdline.push_back("-t");
        cmdline.push_back(*fstype);
    }
    if (options) {
        cmdline.push_back("-o");
        cmdline.push_back(*options);
    }
    cmdline.push_back(device);
    cmdline.push_back(mountpoint);
    return run_subprocess(cmdline);
}

int umount(const std::filesystem::path& mountpoint)
{
    return run_subprocess({"umount", mountpoint});
}
