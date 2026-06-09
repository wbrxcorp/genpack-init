// SPDX-License-Identifier: GPL-2.0
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#define EPERM 1
#define OVERLAYFS_SUPER_MAGIC 0x794c7630

// Trusted squashfs device numbers
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 16);
    __type(key, __u64);
    __type(value, __u8);
} trusted_devs SEC(".maps");

// [0]: byte offset of __upperdentry from &ovl_inode.vfs_inode, or -1 if unknown
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __s64);
} ovl_config SEC(".maps");

SEC("lsm/bprm_check_security")
int BPF_PROG(exec_guard_check, struct linux_binprm *bprm, int ret)
{
    if (ret) return ret;

    struct file *file = BPF_CORE_READ(bprm, file);
    if (!file) return 0;

    struct inode *inode = BPF_CORE_READ(file, f_inode);
    struct super_block *sb = BPF_CORE_READ(inode, i_sb);
    unsigned long magic = BPF_CORE_READ(sb, s_magic);

    if (magic == OVERLAYFS_SUPER_MAGIC) {
        __u32 key = 0;
        __s64 *off = bpf_map_lookup_elem(&ovl_config, &key);
        // Degraded mode: offset unknown, allow to avoid breaking the system
        if (!off || *off < 0 || *off > 65536) return 0;

        struct dentry *upper = NULL;
        bpf_probe_read_kernel(&upper, sizeof(upper), (char *)inode + *off);
        return upper ? -EPERM : 0;
    }

    __u64 dev = (__u64)BPF_CORE_READ(sb, s_dev);
    __u8 *found = bpf_map_lookup_elem(&trusted_devs, &dev);
    return found ? 0 : -EPERM;
}

char LICENSE[] SEC("license") = "GPL";
