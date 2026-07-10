// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/version.h>

#include "kfi_compat.h"

struct class *kfi_compat_class_create(const char *name)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	return class_create(name);
#else
	return class_create(THIS_MODULE, name);
#endif
}
