// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/poll.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#include "kfi_internal.h"
#include "kfi_transport.h"

static bool kfi_char_active;
static bool kfi_proc_active;

int kfi_transport_open(struct inode *inode, struct file *file,
		       enum kfi_transport_kind kind)
{
	int error;

	error = kfi_client_create(file, kind);
	if (error)
		return error;

	error = nonseekable_open(inode, file);
	if (error)
		kfi_client_destroy(file);

	return error;
}

int kfi_transport_release(struct inode *inode, struct file *file)
{
	(void)inode;
	kfi_client_destroy(file);
	return 0;
}

long kfi_transport_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg)
{
	struct kfi_client *client = file->private_data;

	if (!client)
		return -ENODEV;

	return kfi_dispatch_ioctl(client, cmd, arg);
}

#ifdef CONFIG_COMPAT
long kfi_transport_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	return kfi_transport_ioctl(file, cmd,
				   (unsigned long)compat_ptr(arg));
}
#endif

ssize_t kfi_transport_read(struct file *file, char __user *buffer,
			   size_t size, loff_t *offset)
{
	(void)file;
	(void)buffer;
	(void)size;
	(void)offset;
	return -EOPNOTSUPP;
}

__poll_t kfi_transport_poll(struct file *file, poll_table *wait)
{
	(void)file;
	(void)wait;
	return EPOLLERR;
}

u64 kfi_transport_capabilities(void)
{
	u64 flags = 0;

	if (kfi_char_active)
		flags |= KFI_CAP_TRANSPORT_CHAR;
	if (kfi_proc_active)
		flags |= KFI_CAP_TRANSPORT_PROC_PRIVATE;

	return flags;
}

int kfi_transport_init_all(void)
{
	int error = -ENODEV;

#ifdef KFI_ENABLE_TRANSPORT_CHAR
	error = kfi_transport_char_init();
	if (!error)
		kfi_char_active = true;
	else
		pr_warn("kfi: character transport unavailable: %d\n", error);
#endif

#ifdef KFI_ENABLE_TRANSPORT_PROC_PRIVATE
	error = kfi_transport_proc_init();
	if (!error)
		kfi_proc_active = true;
	else
		pr_warn("kfi: private proc transport unavailable: %d\n", error);
#endif

	if (!kfi_char_active && !kfi_proc_active)
		return error;

	return 0;
}

void kfi_transport_exit_all(void)
{
	if (kfi_proc_active) {
		kfi_transport_proc_exit();
		kfi_proc_active = false;
	}
	if (kfi_char_active) {
		kfi_transport_char_exit();
		kfi_char_active = false;
	}
}
