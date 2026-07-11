/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_TRANSPORT_H
#define KFI_TRANSPORT_H

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/types.h>

enum kfi_transport_kind {
	KFI_TRANSPORT_CHAR = 1,
	KFI_TRANSPORT_PROC_PRIVATE = 2,
};

int kfi_transport_open(struct inode *inode, struct file *file,
		       enum kfi_transport_kind kind);
int kfi_transport_release(struct inode *inode, struct file *file);
long kfi_transport_ioctl(struct file *file, unsigned int cmd,
			 unsigned long arg);
#ifdef CONFIG_COMPAT
long kfi_transport_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg);
#endif
ssize_t kfi_transport_read(struct file *file, char __user *buffer,
			   size_t size, loff_t *offset);
__poll_t kfi_transport_poll(struct file *file, poll_table *wait);

int kfi_transport_init_all(void);
void kfi_transport_exit_all(void);
u64 kfi_transport_capabilities(void);
u64 kfi_transport_proc_capabilities(void);

int kfi_transport_char_init(void);
void kfi_transport_char_exit(void);
int kfi_transport_proc_init(void);
void kfi_transport_proc_exit(void);

#endif
