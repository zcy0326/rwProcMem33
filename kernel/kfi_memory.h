/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_MEMORY_H
#define KFI_MEMORY_H

#include <linux/types.h>
#include <linux/uaccess.h>

struct kfi_client;

int kfi_memory_ioctl_read(struct kfi_client *client, void __user *argument);
int kfi_memory_ioctl_write(struct kfi_client *client, void __user *argument);
u64 kfi_memory_capabilities(void);

#endif
