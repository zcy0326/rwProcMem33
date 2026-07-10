// SPDX-License-Identifier: GPL-2.0
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/module.h>

#include "kfi_compat.h"
#include "kfi_internal.h"
#include "kfi_transport.h"

static dev_t kfi_char_dev;
static struct cdev kfi_char_cdev;
static struct class *kfi_char_class;
static struct device *kfi_char_device;

static int kfi_char_open(struct inode *inode, struct file *file)
{
	return kfi_transport_open(inode, file, KFI_TRANSPORT_CHAR);
}

static const struct file_operations kfi_char_fops = {
	.owner = THIS_MODULE,
	.open = kfi_char_open,
	.release = kfi_transport_release,
	.unlocked_ioctl = kfi_transport_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = kfi_transport_compat_ioctl,
#endif
	.read = kfi_transport_read,
	.poll = kfi_transport_poll,
	.llseek = no_llseek,
};

int kfi_transport_char_init(void)
{
	int error;

	error = alloc_chrdev_region(&kfi_char_dev, 0, 1, KFI_DEVICE_NAME);
	if (error)
		return error;

	cdev_init(&kfi_char_cdev, &kfi_char_fops);
	kfi_char_cdev.owner = THIS_MODULE;
	error = cdev_add(&kfi_char_cdev, kfi_char_dev, 1);
	if (error)
		goto unregister_region;

	kfi_char_class = kfi_compat_class_create(KFI_DEVICE_NAME);
	if (IS_ERR(kfi_char_class)) {
		error = PTR_ERR(kfi_char_class);
		kfi_char_class = NULL;
		goto delete_cdev;
	}

	kfi_char_device = device_create(kfi_char_class, NULL, kfi_char_dev, NULL,
					KFI_DEVICE_NAME);
	if (IS_ERR(kfi_char_device)) {
		error = PTR_ERR(kfi_char_device);
		kfi_char_device = NULL;
		goto destroy_class;
	}

	pr_info("kfi: character transport registered on major %u\n",
		MAJOR(kfi_char_dev));
	return 0;

destroy_class:
	class_destroy(kfi_char_class);
	kfi_char_class = NULL;
delete_cdev:
	cdev_del(&kfi_char_cdev);
unregister_region:
	unregister_chrdev_region(kfi_char_dev, 1);
	return error;
}

void kfi_transport_char_exit(void)
{
	device_destroy(kfi_char_class, kfi_char_dev);
	class_destroy(kfi_char_class);
	kfi_char_device = NULL;
	kfi_char_class = NULL;
	cdev_del(&kfi_char_cdev);
	unregister_chrdev_region(kfi_char_dev, 1);
}
