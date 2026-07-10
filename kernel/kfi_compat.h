/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_COMPAT_H
#define KFI_COMPAT_H

#include <linux/device.h>

struct class *kfi_compat_class_create(const char *name);

#endif
