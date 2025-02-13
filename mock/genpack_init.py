import os,pathlib,logging

_coldplug_called = False
_root_path = "."
_boot_path = "."
_ro_path = "."
_rw_path = "."

def coldplug():
    global _coldplug_called
    if _coldplug_called:
        logging.info("coldplug already called")
    else:
        logging.debug("coldplug called")
        _coldplug_called = True

def get_block_device_info():
    pass

def get_partition_info():
    pass

def parted():
    pass

def mkfs():
    pass

def mkswap():
    pass

def _create_posix_path(base, *args) -> pathlib.PosixPath:
    new_args = [base]
    if len(args) > 0:
        new_args.append(args[0].lstrip("/"))
    new_args += args[1:]
    return pathlib.PosixPath(*new_args)

def root_path(*args) -> pathlib.PosixPath:
    return _create_posix_path(_root_path, *args)

def boot_path(*args) -> pathlib.PosixPath:
    return _create_posix_path(_boot_path, *args)

def ro_path(*args) -> pathlib.PosixPath:
    return _create_posix_path(_ro_path, *args)

def rw_path(*args) -> pathlib.PosixPath:
    return _create_posix_path(_rw_path, *args)

def chown():
    pass

def chmod():
    pass

def is_raspberry_pi():
    return False

def is_qemu():
    return False

def read_qemu_firmware_config():
    pass

def enable_systemd_service(service):
    logging.info(f"Enabling systemd service {service}")

def disable_systemd_service(service):
    logging.info(f"Disabling systemd service {service}")
