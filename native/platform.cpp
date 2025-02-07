#include <filesystem>
#include <fstream>

#include "platform.h"
#include "filesystem.h"
#include "logging.h"

bool is_raspberry_pi() {
    std::filesystem::path sys_model("/sys/firmware/devicetree/base/model");
    if (std::filesystem::exists(sys_model)) {
        std::ifstream ifs(sys_model);
        std::string model;
        std::getline(ifs, model);
        logging::debug(std::format("Model: '{}'", model));
        return model.starts_with("Raspberry Pi");
    } else {
        logging::debug(std::format("{} not found.", sys_model.string()));
    }
    return false;
}

bool is_qemu() {
    std::filesystem::path sys_vendor("/sys/class/dmi/id/sys_vendor");
    if (std::filesystem::exists(sys_vendor)) {
        std::ifstream ifs(sys_vendor);
        std::string vendor;
        std::getline(ifs, vendor);
        logging::debug(std::format("Vendor: '{}'", vendor));
        return vendor == "QEMU";
    } else {
        logging::debug(std::format("{} not found.", sys_vendor.string()));
    }
    return false;
}

std::optional<std::string> read_qemu_firmware_config(const std::filesystem::path& name)
{
    auto path = root_path({"/sys/firmware/qemu_fw_cfg/by_name", name, "raw"});
    if (!std::filesystem::exists(path)) {
        logging::debug(std::format("{} not found.", path.string()));
        return std::nullopt;
    }
    //else
    // read whole content from file
    std::ifstream ifs(path);
    if (!ifs) {
        throw std::runtime_error(std::format("Failed to open {}", path.string()));
    }
    //else
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}
