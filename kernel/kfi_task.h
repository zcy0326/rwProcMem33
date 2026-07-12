/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_TASK_H
#define KFI_TASK_H

#include <linux/types.h>
#include <linux/uaccess.h>

struct kfi_client;

int kfi_task_ioctl_enumerate(struct kfi_client *client, void __user *argument);
u64 kfi_task_capabilities(void);

#endif
