/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_EVENT_H
#define KFI_EVENT_H

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/uaccess.h>

struct kfi_client;
struct kfi_event;

int kfi_event_client_init(struct kfi_client *client);
void kfi_event_client_shutdown(struct kfi_client *client);
void kfi_event_client_destroy(struct kfi_client *client);

int kfi_event_emit(struct kfi_client *client, const struct kfi_event *event);
ssize_t kfi_event_read(struct file *file, char __user *buffer, size_t size);
__poll_t kfi_event_poll(struct file *file, poll_table *wait);
int kfi_event_ioctl_get_stats(struct kfi_client *client, void __user *argument);
u64 kfi_event_capabilities(void);

#endif
