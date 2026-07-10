// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>

#include "kfi_compat.h"
#include "kfi_internal.h"

static dev_t kfi_dev;
static struct cdev kfi_cdev;
static struct class *kfi_class;
static struct device *kfi_device;

static const struct file_operations kfi_fops = {
	.owner = THIS_MODULE,
	.open = kfi_client_open,
	.release = kfi_client_release,
	.unlocked_ioctl = kfi_client_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = kfi_compat_ioctl,
#endif
	.llseek = no_llseek,
};

static int __init kfi_init(void)
{
	int error;

	error = kfi_selftest_run();
	if (error) {
		pr_err("kfi: selftest failed: %d\n", error);
		return error;
	}

	error = alloc_chrdev_region(&kfi_dev, 0, 1, KFI_DEVICE_NAME);
	if (error)
		return error;

	cdev_init(&kfi_cdev, &kfi_fops);
	kfi_cdev.owner = THIS_MODULE;
	error = cdev_add(&kfi_cdev, kfi_dev, 1);
	if (error)
		goto unregister_region;

	kfi_class = kfi_compat_class_create(KFI_DEVICE_NAME);
	if (IS_ERR(kfi_class)) {
		error = PTR_ERR(kfi_class);
		goto delete_cdev;
	}

	kfi_device = device_create(kfi_class, NULL, kfi_dev, NULL,
				   KFI_DEVICE_NAME);
	if (IS_ERR(kfi_device)) {
		error = PTR_ERR(kfi_device);
		goto destroy_class;
	}

	pr_info("kfi: ABI %u.%u ready on major %u\n",
		KFI_ABI_VERSION_MAJOR, KFI_ABI_VERSION_MINOR, MAJOR(kfi_dev));
	return 0;

destroy_class:
	class_destroy(kfi_class);
delete_cdev:
	cdev_del(&kfi_cdev);
unregister_region:
	unregister_chrdev_region(kfi_dev, 1);
	return error;
}

static void __exit kfi_exit(void)
{
	device_destroy(kfi_class, kfi_dev);
	class_destroy(kfi_class);
	cdev_del(&kfi_cdev);
	unregister_chrdev_region(kfi_dev, 1);
	pr_info("kfi: unloaded\n");
}

module_init(kfi_init);
module_exit(kfi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KFI contributors");
MODULE_DESCRIPTION("Kernel-assisted instrumentation interface");
MODULE_VERSION("1.1.0");
MODULE_INFO(kfi_abi, "1.1");
