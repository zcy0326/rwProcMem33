/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_SESSION_H
#define KFI_SESSION_H

#include <linux/sched.h>
#include <linux/types.h>
#include <linux/uaccess.h>

struct kfi_client;
struct kfi_session;

int kfi_session_ioctl_open(struct kfi_client *client, void __user *argument);
int kfi_session_ioctl_close(struct kfi_client *client, void __user *argument);
struct kfi_session *kfi_session_lookup_get(struct kfi_client *client, u64 id);
void kfi_session_put(struct kfi_session *session);
struct task_struct *kfi_session_get_task(struct kfi_session *session);
void kfi_session_shutdown_all(struct kfi_client *client);
u64 kfi_session_id(const struct kfi_session *session);

#endif
