/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_MAPS_H
#define KFI_MAPS_H

#include <linux/uaccess.h>
struct kfi_client;
int kfi_maps_ioctl_enumerate(struct kfi_client *client,
			     void __user *argument);
u64 kfi_maps_capabilities(void);
#endif
