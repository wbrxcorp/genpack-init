// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define EPERM 1
#define PROT_EXEC 0x4
#define OVERLAYFS_SUPER_MAGIC 0x794c7630

// Minimal declaration for CO-RE access; libbpf resolves actual offsets
// from /sys/kernel/btf/overlay at load time.
struct ovl_inode {
    union { void *cache; const char *redirect; };
    const char *lowerdata_redirect;
    __u64 version;
    unsigned long flags;
    struct inode vfs_inode;
    struct dentry *__upperdentry;
} __attribute__((preserve_access_index));

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 16);
    __type(key, __u64);
    __type(value, __u8);
} trusted_devs SEC(".maps");

static __always_inline int check_file(struct file *file)
{
    if (!file) return 0;

    struct inode *inode = BPF_CORE_READ(file, f_inode);
    struct super_block *sb = BPF_CORE_READ(inode, i_sb);
    unsigned long magic = BPF_CORE_READ(sb, s_magic);

    if (magic == OVERLAYFS_SUPER_MAGIC) {
        // container_of: __builtin_offsetof with preserve_access_index generates
        // a CO-RE relocation resolved against overlay module BTF at load time.
        struct ovl_inode *oi = (struct ovl_inode *)((char *)inode
            - __builtin_offsetof(struct ovl_inode, vfs_inode));
        struct dentry *upper = BPF_CORE_READ(oi, __upperdentry);
        return upper ? -EPERM : 0;
    }

    __u64 dev = (__u64)BPF_CORE_READ(sb, s_dev);
    __u8 *found = bpf_map_lookup_elem(&trusted_devs, &dev);
    return found ? 0 : -EPERM;
}

SEC("lsm/bprm_check_security")
int BPF_PROG(exec_guard_check, struct linux_binprm *bprm, int ret)
{
    if (ret) return ret;
    return check_file(BPF_CORE_READ(bprm, file));
}

SEC("lsm/mmap_file")
int BPF_PROG(mmap_guard, struct file *file, unsigned long reqprot,
             unsigned long prot, unsigned long flags, int ret)
{
    if (ret) return ret;
    if (!(prot & PROT_EXEC)) return 0;
    return check_file(file);
}

char LICENSE[] SEC("license") = "GPL";
