/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_INTERNAL_H
#define KFI_INTERNAL_H

#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/types.h>

#include "../include/uapi/linux/kfi.h"

#define KFI_MODULE_VERSION 0x00010000U
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
	struct mutex lock;
	struct idr sessions;
	u32 session_generation;
};

int kfi_client_open(struct inode *inode, struct file *file);
int kfi_client_release(struct inode *inode, struct file *file);
long kfi_client_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#endif
