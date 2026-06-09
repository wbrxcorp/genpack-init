#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <format>

#include <bpf/libbpf.h>
#include <bpf/btf.h>

#include "exec_guard.skel.h"
#include "exec_guard.h"
#include "native/logging.h"

static struct exec_guard_bpf *skel = nullptr;

// Returns byte offset of __upperdentry from &ovl_inode.vfs_inode, or -1 if unknown.
static int64_t find_upperdentry_offset(struct btf *vmlinux_btf)
{
    auto search = [](struct btf *btf) -> int64_t {
        if (!btf) return -1;
        __s32 id = btf__find_by_name_kind(btf, "ovl_inode", BTF_KIND_STRUCT);
        if (id < 0) return -1;
        const struct btf_type *t = btf__type_by_id(btf, id);
        if (!t) return -1;
        const struct btf_member *m = btf_members(t);
        int n = btf_vlen(t);
        int64_t vfs_off = -1, upper_off = -1;
        for (int i = 0; i < n; i++) {
            const char *name = btf__name_by_offset(btf, m[i].name_off);
            if (!name) continue;
            if (strcmp(name, "vfs_inode") == 0) vfs_off = m[i].offset;
            else if (strcmp(name, "__upperdentry") == 0) upper_off = m[i].offset;
        }
        if (vfs_off < 0 || upper_off < 0) return -1;
        return (upper_off - vfs_off) / 8;
    };

    // overlay is typically a module; its BTF appears at load time
    struct btf *mod_btf = btf__load_module_btf("overlay", vmlinux_btf);
    if (mod_btf) {
        int64_t off = search(mod_btf);
        btf__free(mod_btf);
        if (off >= 0) return off;
    }

    // Fallback: overlay built into kernel
    return search(vmlinux_btf);
}

bool setup_exec_guard(const char* lower_path)
{
    struct stat st;
    if (stat(lower_path, &st) != 0) {
        logging::warning(std::format("exec_guard: stat({}) failed: {}", lower_path, strerror(errno)));
        return false;
    }

    struct btf *vmlinux_btf = btf__load_vmlinux_btf();
    if (!vmlinux_btf) {
        logging::warning("exec_guard: failed to load vmlinux BTF");
    }

    int64_t upper_off = find_upperdentry_offset(vmlinux_btf);
    if (vmlinux_btf) btf__free(vmlinux_btf);

    if (upper_off >= 0) {
        logging::info(std::format("exec_guard: ovl_inode.__upperdentry offset from vfs_inode: {} bytes", upper_off));
    } else {
        logging::warning("exec_guard: ovl_inode not found in BTF; overlayfs check disabled (degraded mode)");
    }

    skel = exec_guard_bpf__open_and_load();
    if (!skel) {
        logging::warning(std::format("exec_guard: failed to load BPF program (errno={})", errno));
        return false;
    }

    uint32_t key = 0;
    int64_t val = upper_off;
    bpf_map__update_elem(skel->maps.ovl_config, &key, sizeof(key), &val, sizeof(val), BPF_ANY);

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
