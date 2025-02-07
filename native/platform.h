#include <string>

bool is_raspberry_pi();
bool is_qemu();
std::optional<std::string> read_qemu_firmware_config(const std::filesystem::path& name);
