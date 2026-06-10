#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <format>

#include <bpf/libbpf.h>

#include "exec_guard.skel.h"
#include "exec_guard.h"
#include "native/logging.h"

static struct exec_guard_bpf *skel = nullptr;

bool setup_exec_guard(const char* lower_path)
{
    struct stat st;
    if (stat(lower_path, &st) != 0) {
        logging::warning(std::format("exec_guard: stat({}) failed: {}", lower_path, strerror(errno)));
        return false;
    }

    skel = exec_guard_bpf__open_and_load();
    if (!skel) {
        // Likely cause: overlay module not loaded or CONFIG_DEBUG_INFO_BTF_MODULES not set
        logging::warning(std::format("exec_guard: failed to load BPF program (errno={})", errno));
        return false;
    }

    uint64_t dev = static_cast<uint64_t>(st.st_dev);
    uint8_t one = 1;
    bpf_map__update_elem(skel->maps.trusted_devs, &dev, sizeof(dev), &one, sizeof(one), BPF_ANY);

    int err = exec_guard_bpf__attach(skel);
    if (err) {
        logging::warning(std::format("exec_guard: failed to attach BPF LSM: {}", err));
        exec_guard_bpf__destroy(skel);
        skel = nullptr;
        return false;
    }

    logging::info(std::format("exec_guard: active. trusted dev=0x{:x}", dev));
    return true;
}
