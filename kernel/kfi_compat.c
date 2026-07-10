// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "kfi_compat.h"
#include "kfi_internal.h"

struct class *kfi_compat_class_create(const char *name)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	return class_create(name);
#else
	return class_create(THIS_MODULE, name);
#endif
}

#ifdef CONFIG_COMPAT
long kfi_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return kfi_client_ioctl(file, cmd,
				(unsigned long)compat_ptr(arg));
}
#endif
