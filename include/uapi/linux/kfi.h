/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_KFI_H
#define _UAPI_LINUX_KFI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define KFI_ABI_VERSION_MAJOR 1
#define KFI_ABI_VERSION_MINOR 2
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
#define KFI_CAP_RUNTIME_INFO     (1ULL << 10)
#define KFI_CAP_TRANSPORT_CHAR   (1ULL << 11)
#define KFI_CAP_TRANSPORT_PROC_PRIVATE (1ULL << 12)
#define KFI_CAP_SESSION_REFS     (1ULL << 13)
#define KFI_CAP_TRANSPORT_PROC_HIDDEN (1ULL << 14)
#define KFI_CAP_MODULE_HIDING    (1ULL << 15)
#define KFI_CAP_ENUM_MAPS_PAGED  (1ULL << 16)

#define KFI_RUNTIME_COMPAT        (1ULL << 0)
#define KFI_RUNTIME_MODVERSIONS   (1ULL << 1)
#define KFI_RUNTIME_CFI           (1ULL << 2)
#define KFI_RUNTIME_KCFI          (1ULL << 3)
#define KFI_RUNTIME_LTO_CLANG     (1ULL << 4)
#define KFI_RUNTIME_KPROBES       (1ULL << 5)
#define KFI_RUNTIME_HW_BREAKPOINT (1ULL << 6)
#define KFI_RUNTIME_TRANSPORT_CHAR (1ULL << 7)
#define KFI_RUNTIME_TRANSPORT_PROC_PRIVATE (1ULL << 8)

#define KFI_REQUEST_FLAGS_NONE 0U
#define KFI_MEM_FLAG_NONE      0U
#define KFI_VISIBILITY_FLAG_HIDE_MODULE (1U << 0)

#define KFI_ENUM_RESULT_END (1U << 0)
#define KFI_ENUM_MAX_ENTRIES 256U
#define KFI_THREAD_FLAG_LEADER (1U << 0)

#define KFI_PROT_READ  (1U << 0)
#define KFI_PROT_WRITE (1U << 1)
#define KFI_PROT_EXEC  (1U << 2)

#define KFI_MAP_FLAG_SHARED         (1U << 0)
#define KFI_MAP_FLAG_PRIVATE        (1U << 1)
#define KFI_MAP_FLAG_FILE           (1U << 2)
#define KFI_MAP_FLAG_PATH_TRUNCATED (1U << 3)

struct kfi_request_header {
	__u32 struct_size;
	__u32 flags;
	__u64 request_id;
};

struct kfi_version {
	struct kfi_request_header header;
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
	struct kfi_request_header header;
	__u64 flags;
	__u32 max_sessions;
	__u32 max_io_size;
	__u32 num_brps;
	__u32 num_wrps;
	__u32 event_size;
	__u32 reserved0;
	__u64 reserved[10];
};

struct kfi_open_process {
	struct kfi_request_header header;
	__s32 pid;
	__u32 reserved0;
	__u64 session_id;
	__u64 reserved[4];
};

struct kfi_close_session {
	struct kfi_request_header header;
	__u64 session_id;
	__u64 reserved[5];
};

struct kfi_runtime_info {
	struct kfi_request_header header;
	__u32 linux_version_code;
	__u32 page_size;
	__u32 page_shift;
	__u32 profile_id;
	__u64 build_flags;
	__u64 active_caps;
	__u64 transport_caps;
	char release[64];
	char machine[32];
	char profile[32];
	__u64 reserved[9];
};

struct kfi_memory_io {
	struct kfi_request_header header;
	__u64 session_id;
	__u64 remote_address;
	__u64 user_buffer;
	__u32 requested_size;
	__u32 completed_size;
	__u64 reserved[2];
};

struct kfi_visibility_control {
	struct kfi_request_header header;
	__u32 operations;
	__u32 reserved0;
	__u64 reserved[5];
};

struct kfi_enumerate {
	struct kfi_request_header header;
	__u64 session_id;
	__u64 user_buffer;
	__u64 cursor;
	__u32 capacity;
	__u32 returned;
	__u64 next_cursor;
	__u32 result_flags;
	__u32 reserved0;
};

struct kfi_thread_entry {
	__s32 tid;
	__s32 tgid;
	__u32 state;
	__u32 flags;
	char comm[32];
	__u64 reserved[2];
};

struct kfi_map_entry {
	__u64 start;
	__u64 end;
	__u64 offset;
	__u64 inode;
	__u32 prot;
	__u32 flags;
	__u32 dev_major;
	__u32 dev_minor;
	char path[256];
	__u64 reserved[2];
};

#define KFI_IOC_GET_VERSION \
	_IOWR(KFI_IOC_MAGIC, 0x00, struct kfi_version)
#define KFI_IOC_GET_CAPS \
	_IOWR(KFI_IOC_MAGIC, 0x01, struct kfi_caps)
#define KFI_IOC_GET_RUNTIME_INFO \
	_IOWR(KFI_IOC_MAGIC, 0x02, struct kfi_runtime_info)
#define KFI_IOC_OPEN_PROCESS \
	_IOWR(KFI_IOC_MAGIC, 0x10, struct kfi_open_process)
#define KFI_IOC_CLOSE_SESSION \
	_IOWR(KFI_IOC_MAGIC, 0x11, struct kfi_close_session)
#define KFI_IOC_READ_MEMORY \
	_IOWR(KFI_IOC_MAGIC, 0x20, struct kfi_memory_io)
#define KFI_IOC_WRITE_MEMORY \
	_IOWR(KFI_IOC_MAGIC, 0x21, struct kfi_memory_io)
#define KFI_IOC_ENUM_THREADS \
	_IOWR(KFI_IOC_MAGIC, 0x22, struct kfi_enumerate)
#define KFI_IOC_ENUM_MAPS \
	_IOWR(KFI_IOC_MAGIC, 0x23, struct kfi_enumerate)
#define KFI_IOC_HIDE_MODULE \
	_IOWR(KFI_IOC_MAGIC, 0x30, struct kfi_visibility_control)

#endif
