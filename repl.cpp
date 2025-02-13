#include <pybind11/embed.h>

int repl()
{
    pybind11::exec(R"(
import code,readline,rlcompleter
from genpack_init import get_block_device_info, get_partition_info, parted, mkfs, mkswap
from genpack_init import boot_path, root_path, ro_path, rw_path, chown, chmod
from genpack_init import is_raspberry_pi, is_qemu, read_qemu_firmware_config
from genpack_init import enable_systemd_service, disable_systemd_service
history_file = os.path.expanduser("~/.genpack_init_history")
if os.path.exists(history_file):
    readline.read_history_file(history_file)
readline.parse_and_bind("tab: complete")
readline.set_completer(rlcompleter.Completer(globals()).complete)
try:
    code.interact(local=globals())
finally:
    readline.write_history_file(history_file)
)");
    return 0;
}

#ifdef TEST
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "No test for repl() function." << std::endl;
    return 0;
}
#endif