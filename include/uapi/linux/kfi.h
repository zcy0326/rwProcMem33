/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_KFI_H
#define _UAPI_LINUX_KFI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define KFI_ABI_VERSION_MAJOR 1
#define KFI_ABI_VERSION_MINOR 0
#define KFI_DEVICE_NAME "kfi"
#define KFI_DEVICE_PATH "/dev/kfi"
#define KFI_IOC_MAGIC 0xB7

#define KFI_CAP_CLIENT_ISOLATION (1ULL << 0)
#define KFI_CAP_OPAQUE_SESSIONS  (1ULL << 1)
#define KFI_CAP_READ_MEMORY      (1ULL << 2)
#define KFI_CAP_WRITE_MEMORY     (1ULL << 3)
#define KFI_CAP_QUERY_MAPS       (1ULL << 4)
#define KFI_CAP_ENUM_THREADS     (1ULL << 5)
#define KFI_CAP_HWBKPT_EXEC      (1ULL << 6)
#define KFI_CAP_HWBKPT_RW        (1ULL << 7)
#define KFI_CAP_MODIFY_REGS      (1ULL << 8)
#define KFI_CAP_POLL_EVENTS      (1ULL << 9)

struct kfi_version {
	__u16 major;
	__u16 minor;
	__u16 patch;
	__u16 abi_header_size;
	__u32 module_version;
	__u32 reserved0;
	__u64 client_id;
	__u64 reserved[4];
};

struct kfi_caps {
	__u64 flags;
	__u32 max_sessions;
	__u32 max_io_size;
	__u32 num_brps;
	__u32 num_wrps;
	__u32 event_size;
	__u32 reserved0;
	__u64 reserved[4];
};

struct kfi_open_process {
	__s32 pid;
	__u32 flags;
	__u64 session_id;
	__u64 reserved[3];
};

struct kfi_close_session {
	__u64 session_id;
	__u64 reserved[3];
};

#define KFI_IOC_GET_VERSION \
	_IOR(KFI_IOC_MAGIC, 0x00, struct kfi_version)
#define KFI_IOC_GET_CAPS \
	_IOR(KFI_IOC_MAGIC, 0x01, struct kfi_caps)
#define KFI_IOC_OPEN_PROCESS \
	_IOWR(KFI_IOC_MAGIC, 0x10, struct kfi_open_process)
#define KFI_IOC_CLOSE_SESSION \
	_IOW(KFI_IOC_MAGIC, 0x11, struct kfi_close_session)

#endif
