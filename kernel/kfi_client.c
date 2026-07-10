// SPDX-License-Identifier: GPL-2.0
#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "kfi_internal.h"

static atomic64_t kfi_next_client_id = ATOMIC64_INIT(0);

static bool kfi_reserved_is_zero(const __u64 *values, size_t count)
{
	size_t i;

	for (i = 0; i < count; i++) {
		if (values[i])
			return false;
	}

	return true;
}

static void kfi_session_destroy(struct kfi_session *session)
{
	if (!session)
		return;

	put_pid(session->tgid);
	kfree(session);
}

static bool kfi_client_authorized(const struct kfi_client *client)
{
	return uid_eq(current_euid(), client->owner_euid) ||
	       capable(CAP_SYS_PTRACE);
}

int kfi_client_open(struct inode *inode, struct file *file)
{
	struct kfi_client *client;

	if (!capable(CAP_SYS_PTRACE))
		return -EPERM;

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	client->id = atomic64_inc_return(&kfi_next_client_id);
	client->owner_euid = current_euid();
	client->opener_pid = task_pid_nr(current);
	client->opener_tgid = task_tgid_nr(current);
	mutex_init(&client->lock);
	idr_init(&client->sessions);
	file->private_data = client;

	return nonseekable_open(inode, file);
}

int kfi_client_release(struct inode *inode, struct file *file)
{
	struct kfi_client *client = file->private_data;
	struct kfi_session *session;
	int id;

	if (!client)
		return 0;

	mutex_lock(&client->lock);
	idr_for_each_entry(&client->sessions, session, id)
		kfi_session_destroy(session);
	idr_destroy(&client->sessions);
	mutex_unlock(&client->lock);

	file->private_data = NULL;
	kfree(client);
	return 0;
}

static long kfi_get_version(struct kfi_client *client, unsigned long arg)
{
	struct kfi_version version = {
		.major = KFI_ABI_VERSION_MAJOR,
		.minor = KFI_ABI_VERSION_MINOR,
		.patch = 0,
		.abi_header_size = sizeof(struct kfi_version),
		.module_version = KFI_MODULE_VERSION,
		.client_id = client->id,
	};

	if (copy_to_user((void __user *)arg, &version, sizeof(version)))
		return -EFAULT;

	return 0;
}

static long kfi_get_caps(unsigned long arg)
{
	struct kfi_caps caps = {
		.flags = KFI_CAP_CLIENT_ISOLATION | KFI_CAP_OPAQUE_SESSIONS |
			 KFI_CAP_RUNTIME_INFO,
		.max_sessions = KFI_MAX_SESSIONS,
		.max_io_size = KFI_MAX_IO_SIZE,
	};

	if (copy_to_user((void __user *)arg, &caps, sizeof(caps)))
		return -EFAULT;

	return 0;
}

static long kfi_get_runtime_info(unsigned long arg)
{
	struct kfi_runtime_info info;

	kfi_runtime_get(&info);
	if (copy_to_user((void __user *)arg, &info, sizeof(info)))
		return -EFAULT;

	return 0;
}

static long kfi_open_process(struct kfi_client *client, unsigned long arg)
{
	struct kfi_open_process request;
	struct kfi_session *session;
	struct task_struct *task;
	struct pid *pid;
	int id;
	u32 generation;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;
	if (request.pid <= 0 || request.flags || request.session_id ||
	    !kfi_reserved_is_zero(request.reserved,
				  ARRAY_SIZE(request.reserved)))
		return -EINVAL;

	pid = find_get_pid(request.pid);
	if (!pid)
		return -ESRCH;

	task = get_pid_task(pid, PIDTYPE_PID);
	put_pid(pid);
	if (!task)
		return -ESRCH;

	session = kzalloc(sizeof(*session), GFP_KERNEL);
	if (!session) {
		put_task_struct(task);
		return -ENOMEM;
	}

	session->tgid = get_task_pid(task, PIDTYPE_TGID);
	session->opened_pid = request.pid;
	put_task_struct(task);
	if (!session->tgid) {
		kfree(session);
		return -ESRCH;
	}

	mutex_lock(&client->lock);
	id = idr_alloc(&client->sessions, session, 1, KFI_MAX_SESSIONS + 1,
		       GFP_KERNEL);
	if (id >= 0) {
		generation = ++client->session_generation;
		if (!generation)
			generation = ++client->session_generation;
		session->id = ((u64)generation << 32) | (u32)id;
	}
	mutex_unlock(&client->lock);
	if (id < 0) {
		kfi_session_destroy(session);
		return id;
	}

	request.session_id = session->id;
	if (copy_to_user((void __user *)arg, &request, sizeof(request))) {
		mutex_lock(&client->lock);
		session = idr_remove(&client->sessions, id);
		mutex_unlock(&client->lock);
		kfi_session_destroy(session);
		return -EFAULT;
	}

	return 0;
}

static long kfi_close_session(struct kfi_client *client, unsigned long arg)
{
	struct kfi_close_session request;
	struct kfi_session *session;
	u32 slot;

	if (copy_from_user(&request, (void __user *)arg, sizeof(request)))
		return -EFAULT;
	slot = (u32)request.session_id;
	if (!request.session_id || !slot || slot > KFI_MAX_SESSIONS ||
	    !kfi_reserved_is_zero(request.reserved,
				  ARRAY_SIZE(request.reserved)))
		return -EINVAL;

	mutex_lock(&client->lock);
	session = idr_find(&client->sessions, slot);
	if (!session || session->id != request.session_id) {
		mutex_unlock(&client->lock);
		return -ENOENT;
	}
	idr_remove(&client->sessions, slot);
	mutex_unlock(&client->lock);

	kfi_session_destroy(session);
	return 0;
}

long kfi_client_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct kfi_client *client = file->private_data;

	if (!client)
		return -ENODEV;
	if (_IOC_TYPE(cmd) != KFI_IOC_MAGIC)
		return -ENOTTY;
	if (!kfi_client_authorized(client))
		return -EPERM;

	switch (cmd) {
	case KFI_IOC_GET_VERSION:
		return kfi_get_version(client, arg);
	case KFI_IOC_GET_CAPS:
		return kfi_get_caps(arg);
	case KFI_IOC_GET_RUNTIME_INFO:
		return kfi_get_runtime_info(arg);
	case KFI_IOC_OPEN_PROCESS:
		return kfi_open_process(client, arg);
	case KFI_IOC_CLOSE_SESSION:
		return kfi_close_session(client, arg);
	default:
		return -ENOTTY;
	}
}
