/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_COMPAT_H
#define KFI_COMPAT_H

#include <linux/device.h>
#include <linux/fs.h>

struct class *kfi_compat_class_create(const char *name);

#ifdef CONFIG_COMPAT
long kfi_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
#endif

#endif
