// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/overflow.h>
#include <linux/sched/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "kfi_internal.h"
#include "kfi_memory.h"
#include "kfi_session.h"
#include "kfi_uapi.h"

#define KFI_MEMORY_CHUNK_SIZE (64U * 1024U)

static int kfi_memory_validate_request(const struct kfi_memory_io *request)
{
	u64 end;

	if (!request->session_id || !request->user_buffer ||
	    !request->requested_size ||
	    request->requested_size > KFI_MAX_IO_SIZE || request->completed_size ||
	    !kfi_uapi_reserved_is_zero(request->reserved,
				       ARRAY_SIZE(request->reserved)))
		return -EINVAL;
	if (check_add_overflow(request->remote_address,
			       (u64)request->requested_size, &end) ||
	    end <= request->remote_address || request->remote_address > ULONG_MAX ||
	    end - 1 > ULONG_MAX)
		return -EOVERFLOW;
	if (!access_ok(u64_to_user_ptr(request->user_buffer),
		       request->requested_size))
		return -EFAULT;
	return 0;
}

static int kfi_memory_transfer(struct kfi_client *client,
			       struct kfi_memory_io *request, bool write)
{
	struct kfi_session *session;
	struct task_struct *task;
	void __user *user_buffer = u64_to_user_ptr(request->user_buffer);
	unsigned char *buffer;
	u32 completed = 0;
	int result = 0;

	session = kfi_session_lookup_get(client, request->session_id);
	if (!session)
		return -ENOENT;
	task = kfi_session_get_task(session);
	if (!task) {
		result = -ESRCH;
		goto put_session;
	}

	buffer = kmalloc(KFI_MEMORY_CHUNK_SIZE, GFP_KERNEL);
	if (!buffer) {
		result = -ENOMEM;
		goto put_task;
	}

	while (completed < request->requested_size) {
		u32 remaining = request->requested_size - completed;
		u32 chunk = min_t(u32, remaining, KFI_MEMORY_CHUNK_SIZE);
		int transferred;

		if (write && copy_from_user(buffer,
					    (unsigned char __user *)user_buffer + completed,
					    chunk)) {
			result = -EFAULT;
			break;
		}

		transferred = access_process_vm(
			task, (unsigned long)(request->remote_address + completed),
			buffer, chunk, write ? FOLL_WRITE : 0);
		if (transferred <= 0) {
			result = completed ? 0 : -EFAULT;
			break;
		}

		if (!write && copy_to_user(
				      (unsigned char __user *)user_buffer + completed,
				      buffer, transferred)) {
			result = -EFAULT;
			break;
		}

		completed += transferred;
		if (transferred != chunk)
			break;
	}

	kfree(buffer);
put_task:
	put_task_struct(task);
put_session:
	kfi_session_put(session);
	request->completed_size = completed;
	return result;
}

static int kfi_memory_ioctl(struct kfi_client *client, void __user *argument,
			    bool write)
{
	struct kfi_memory_io request;
	int result;
	int copy_result;

	result = kfi_uapi_copy_request(&request, sizeof(request), argument,
				       KFI_MEM_FLAG_NONE);
	if (result)
		return result;
	result = kfi_memory_validate_request(&request);
	if (!result)
		result = kfi_memory_transfer(client, &request, write);

	copy_result = kfi_uapi_copy_response(argument, &request, sizeof(request));
	return copy_result ? copy_result : result;
}

int kfi_memory_ioctl_read(struct kfi_client *client, void __user *argument)
{
	return kfi_memory_ioctl(client, argument, false);
}

int kfi_memory_ioctl_write(struct kfi_client *client, void __user *argument)
{
	return kfi_memory_ioctl(client, argument, true);
}

u64 kfi_memory_capabilities(void)
{
	return KFI_CAP_READ_MEMORY | KFI_CAP_WRITE_MEMORY;
}
