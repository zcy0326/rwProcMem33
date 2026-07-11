// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/overflow.h>
#include <linux/rcupdate.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include "kfi_internal.h"
#include "kfi_session.h"
#include "kfi_task.h"
#include "kfi_uapi.h"

static u32 kfi_task_state(const struct task_struct *task)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	return (u32)READ_ONCE(task->__state);
#else
	return (u32)READ_ONCE(task->state);
#endif
}

static void kfi_task_fill_entry(struct kfi_thread_entry *entry,
				struct task_struct *task)
{
	entry->tid = task_pid_nr(task);
	entry->tgid = task_tgid_nr(task);
	entry->state = kfi_task_state(task);
	get_task_comm(entry->comm, task);
}

u64 kfi_task_capabilities(void)
{
	return KFI_CAP_ENUM_THREADS;
}

int kfi_task_ioctl_enumerate(struct kfi_client *client,
			     void __user *argument)
{
	struct kfi_enumerate request;
	struct kfi_thread_entry *entries;
	struct kfi_session *session;
	struct task_struct *leader;
	struct task_struct *thread;
	size_t bytes;
	u64 ordinal = 0;
	u32 count = 0;
	bool exhausted = true;
	int error;

	error = kfi_uapi_copy_request(&request, sizeof(request), argument,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (!request.session_id || !request.user_buffer || !request.capacity ||
	    request.capacity > KFI_ENUM_MAX_ENTRIES || request.returned ||
	    request.next_cursor || request.result_flags || request.reserved0)
		return -EINVAL;
	if (check_mul_overflow((size_t)request.capacity, sizeof(*entries),
			       &bytes) ||
	    !access_ok(u64_to_user_ptr(request.user_buffer), bytes))
		return -EFAULT;

	session = kfi_session_lookup_get(client, request.session_id);
	if (!session)
		return -ENOENT;
	leader = kfi_session_get_task(session);
	if (!leader) {
		error = -ESRCH;
		goto out_session;
	}
	entries = kcalloc(request.capacity, sizeof(*entries), GFP_KERNEL);
	if (!entries) {
		error = -ENOMEM;
		goto out_task;
	}

	rcu_read_lock();
	if (ordinal++ >= request.cursor)
		kfi_task_fill_entry(&entries[count++], leader);
	for_each_thread(leader, thread) {
		if (thread == leader)
			continue;
		if (ordinal++ < request.cursor)
			continue;
		if (count == request.capacity) {
			exhausted = false;
			break;
		}
		kfi_task_fill_entry(&entries[count++], thread);
	}
	rcu_read_unlock();

	if (copy_to_user(u64_to_user_ptr(request.user_buffer),
			 entries, count * sizeof(*entries))) {
		error = -EFAULT;
		goto out_entries;
	}
	request.returned = count;
	request.next_cursor = request.cursor + count;
	if (exhausted)
		request.result_flags |= KFI_ENUM_RESULT_END;
	error = kfi_uapi_copy_response(argument, &request, sizeof(request));

out_entries:
	kfree(entries);
out_task:
	put_task_struct(leader);
out_session:
	kfi_session_put(session);
	return error;
}
