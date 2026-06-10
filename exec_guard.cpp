#include <sys/stat.h>
#include <sys/mount.h>
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

    // Pin links to BPF filesystem so they survive execl(systemd).
    // Without pinning, all BPF link fds close on exec and the programs are released.
    // /sys/fs/bpf is not yet mounted at this point (systemd does it later), so mount it ourselves.
    if (mount("none", "/sys/fs/bpf", "bpf", 0, "") != 0 && errno != EBUSY) {
        logging::warning(std::format("exec_guard: failed to mount bpf fs: {}", strerror(errno)));
    }
    mkdir("/sys/fs/bpf/exec_guard", 0700);
    if (bpf_link__pin(skel->links.exec_guard_check, "/sys/fs/bpf/exec_guard/exec_check") ||
        bpf_link__pin(skel->links.mmap_guard,        "/sys/fs/bpf/exec_guard/mmap_guard")) {
        logging::warning(std::format("exec_guard: failed to pin BPF links ({}); programs will be inactive after exec", strerror(errno)));
    }

    logging::info(std::format("exec_guard: active. trusted dev=0x{:x}", dev));
    return true;
}
