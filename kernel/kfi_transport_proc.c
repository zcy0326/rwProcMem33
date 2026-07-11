// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/proc_fs.h>

#include "generated/kfi_private_config.h"
#include "kfi_internal.h"
#include "kfi_visibility.h"
#include "kfi_transport.h"

static struct proc_dir_entry *kfi_proc_directory;
static struct proc_dir_entry *kfi_proc_endpoint;

static int kfi_proc_open(struct inode *inode, struct file *file)
{
	return kfi_transport_open(inode, file, KFI_TRANSPORT_PROC_PRIVATE);
}

static const struct proc_ops kfi_proc_ops = {
	.proc_open = kfi_proc_open,
	.proc_release = kfi_transport_release,
	.proc_ioctl = kfi_transport_ioctl,
#ifdef CONFIG_COMPAT
	.proc_compat_ioctl = kfi_transport_compat_ioctl,
#endif
	.proc_read = kfi_transport_read,
	.proc_poll = kfi_transport_poll,
};

int kfi_transport_proc_init(void)
{
	kfi_proc_directory = proc_mkdir(KFI_PRIVATE_PROC_NAME, NULL);
	if (!kfi_proc_directory)
		return -ENOMEM;

	kfi_proc_endpoint = proc_create(KFI_PRIVATE_PROC_NAME, 0600,
					kfi_proc_directory, &kfi_proc_ops);
	if (!kfi_proc_endpoint) {
		proc_remove(kfi_proc_directory);
		kfi_proc_directory = NULL;
		return -ENOMEM;
	}

	{
		int error = kfi_visibility_proc_hide_start(KFI_PRIVATE_PROC_NAME);
		if (error) {
			proc_remove(kfi_proc_endpoint);
			proc_remove(kfi_proc_directory);
			kfi_proc_endpoint = NULL;
			kfi_proc_directory = NULL;
			return error;
		}
	}

	pr_info("kfi: private proc transport registered (instance %s)\n",
		KFI_PRIVATE_BUILD_INSTANCE_ID);
	return 0;
}

u64 kfi_transport_proc_capabilities(void)
{
	return kfi_visibility_proc_hide_active() ?
		KFI_CAP_TRANSPORT_PROC_HIDDEN : 0;
}

void kfi_transport_proc_exit(void)
{
	kfi_visibility_proc_hide_stop();
	proc_remove(kfi_proc_endpoint);
	proc_remove(kfi_proc_directory);
	kfi_proc_endpoint = NULL;
	kfi_proc_directory = NULL;
}
