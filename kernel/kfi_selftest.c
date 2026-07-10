// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/log2.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/version.h>

#include "kfi_internal.h"

int kfi_selftest_run(void)
{
	struct kfi_runtime_info info;

	kfi_runtime_get(&info);
	if (info.struct_size != sizeof(info))
		return -EINVAL;
	if (!is_power_of_2(info.page_size))
		return -EINVAL;
	if (info.page_size != (1UL << info.page_shift))
		return -EINVAL;
	if (!info.release[0] || !info.machine[0] || !info.profile[0])
		return -EINVAL;

#if KFI_PROFILE_ENFORCE_VERSION
	if ((LINUX_VERSION_CODE >> 16) != KFI_PROFILE_LINUX_MAJOR ||
	    ((LINUX_VERSION_CODE >> 8) & 0xff) != KFI_PROFILE_LINUX_MINOR)
		return -EPROTONOSUPPORT;
#endif

	pr_info("kfi: selftest ok release=%s machine=%s page_size=%u profile=%s\n",
		info.release, info.machine, info.page_size, info.profile);
	return 0;
}
