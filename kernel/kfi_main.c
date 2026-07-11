// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>

#include "kfi_internal.h"
#include "kfi_transport.h"

static int __init kfi_init(void)
{
	int error;

	error = kfi_selftest_run();
	if (error) {
		pr_err("kfi: selftest failed: %d\n", error);
		return error;
	}

	error = kfi_transport_init_all();
	if (error) {
		pr_err("kfi: no transport available: %d\n", error);
		return error;
	}

	pr_info("kfi: ABI %u.%u ready\n", KFI_ABI_VERSION_MAJOR,
		KFI_ABI_VERSION_MINOR);
	return 0;
}

static void __exit kfi_exit(void)
{
	kfi_transport_exit_all();
	pr_info("kfi: unloaded\n");
}

module_init(kfi_init);
module_exit(kfi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KFI contributors");
MODULE_DESCRIPTION("Kernel-assisted instrumentation interface");
MODULE_VERSION("1.2.0");
MODULE_INFO(kfi_abi, "1.2");
