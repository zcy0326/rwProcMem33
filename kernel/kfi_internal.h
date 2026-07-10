/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_INTERNAL_H
#define KFI_INTERNAL_H

#include <linux/build_bug.h>
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/types.h>

#include "../include/uapi/linux/kfi.h"
#include "profiles/kfi_profile.h"
#include "kfi_transport.h"

#define KFI_MODULE_VERSION 0x00010100U
#define KFI_MAX_SESSIONS 4096U
#define KFI_MAX_IO_SIZE (1024U * 1024U)

struct kfi_session {
	u64 id;
	struct pid *tgid;
	pid_t opened_pid;
};

struct kfi_client {
	u64 id;
	kuid_t owner_euid;
	pid_t opener_pid;
	pid_t opener_tgid;
	enum kfi_transport_kind transport;
	struct mutex lock;
	struct idr sessions;
	u32 session_generation;
};

int kfi_client_create(struct file *file,
		      enum kfi_transport_kind transport);
void kfi_client_destroy(struct file *file);
long kfi_dispatch_ioctl(struct kfi_client *client, unsigned int cmd,
			unsigned long arg);
void kfi_runtime_get(struct kfi_runtime_info *info);
int kfi_selftest_run(void);

static_assert(sizeof(struct kfi_version) == 56);
static_assert(sizeof(struct kfi_caps) == 64);
static_assert(sizeof(struct kfi_open_process) == 40);
static_assert(sizeof(struct kfi_close_session) == 32);
static_assert(sizeof(struct kfi_runtime_info) == 176);

#endif
