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
#include "kfi_memory.h"
#include "kfi_session.h"
#include "kfi_transport.h"
#include "kfi_uapi.h"

static atomic64_t kfi_next_client_id = ATOMIC64_INIT(0);

bool kfi_client_authorized(const struct kfi_client *client)
{
	return uid_eq(current_euid(), client->owner_euid) ||
	       capable(CAP_SYS_PTRACE);
}

int kfi_client_create(struct file *file,
		      enum kfi_transport_kind transport)
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
	client->transport = transport;
	mutex_init(&client->lock);
	idr_init(&client->sessions);
	file->private_data = client;
	return 0;
}

void kfi_client_destroy(struct file *file)
{
	struct kfi_client *client = file->private_data;

	if (!client)
		return;

	mutex_lock(&client->lock);
	client->closing = true;
	mutex_unlock(&client->lock);
	kfi_session_shutdown_all(client);

	file->private_data = NULL;
	kfree(client);
}

static long kfi_get_version(struct kfi_client *client, unsigned long arg)
{
	struct kfi_version version;
	u64 request_id;
	int error;

	error = kfi_uapi_copy_request(&version, sizeof(version),
				      (void __user *)arg,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (version.reserved0 ||
	    !kfi_uapi_reserved_is_zero(version.reserved,
				       ARRAY_SIZE(version.reserved)))
		return -EINVAL;
	request_id = version.header.request_id;
	memset(&version, 0, sizeof(version));
	version = (struct kfi_version) {
		.header = {
			.struct_size = sizeof(version),
			.request_id = request_id,
		},
		.major = KFI_ABI_VERSION_MAJOR,
		.minor = KFI_ABI_VERSION_MINOR,
		.patch = 0,
		.abi_header_size = sizeof(struct kfi_request_header),
		.module_version = KFI_MODULE_VERSION,
		.client_id = client->id,
	};

	return kfi_uapi_copy_response((void __user *)arg, &version,
				      sizeof(version));
}

static long kfi_get_caps(unsigned long arg)
{
	struct kfi_caps caps;
	u64 request_id;
	int error;

	error = kfi_uapi_copy_request(&caps, sizeof(caps), (void __user *)arg,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (caps.reserved0 ||
	    !kfi_uapi_reserved_is_zero(caps.reserved,
				       ARRAY_SIZE(caps.reserved)))
		return -EINVAL;
	request_id = caps.header.request_id;
	memset(&caps, 0, sizeof(caps));
	caps = (struct kfi_caps) {
		.header = {
			.struct_size = sizeof(caps),
			.request_id = request_id,
		},
		.flags = KFI_CAP_CLIENT_ISOLATION | KFI_CAP_OPAQUE_SESSIONS |
			 KFI_CAP_RUNTIME_INFO | KFI_CAP_SESSION_REFS |
			 kfi_transport_capabilities() |
			 kfi_memory_capabilities() |
			 kfi_visibility_capabilities(),
		.max_sessions = KFI_MAX_SESSIONS,
		.max_io_size = KFI_MAX_IO_SIZE,
	};

	return kfi_uapi_copy_response((void __user *)arg, &caps, sizeof(caps));
}

static long kfi_get_runtime_info(unsigned long arg)
{
	struct kfi_runtime_info info;
	u64 request_id;
	int error;

	error = kfi_uapi_copy_request(&info, sizeof(info), (void __user *)arg,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (!kfi_uapi_reserved_is_zero(info.reserved,
				       ARRAY_SIZE(info.reserved)))
		return -EINVAL;
	request_id = info.header.request_id;
	kfi_runtime_get(&info);
	info.header.struct_size = sizeof(info);
	info.header.request_id = request_id;
	return kfi_uapi_copy_response((void __user *)arg, &info, sizeof(info));
}

static long kfi_open_process(struct kfi_client *client, unsigned long arg)
{
	return kfi_session_ioctl_open(client, (void __user *)arg);
}

static long kfi_close_session(struct kfi_client *client, unsigned long arg)
{
	return kfi_session_ioctl_close(client, (void __user *)arg);
}

static long kfi_hide_module(unsigned long arg)
{
	struct kfi_visibility_control request;
	int error;

	error = kfi_uapi_copy_request(&request, sizeof(request),
				      (void __user *)arg,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (request.operations != KFI_VISIBILITY_FLAG_HIDE_MODULE ||
	    request.reserved0 ||
	    !kfi_uapi_reserved_is_zero(request.reserved,
				       ARRAY_SIZE(request.reserved)))
		return -EINVAL;

	error = kfi_visibility_hide_module();
	if (error)
		return error;
	return kfi_uapi_copy_response((void __user *)arg, &request,
				      sizeof(request));
}

long kfi_dispatch_ioctl(struct kfi_client *client, unsigned int cmd,
			unsigned long arg)
{
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
	case KFI_IOC_READ_MEMORY:
		return kfi_memory_ioctl_read(client, (void __user *)arg);
	case KFI_IOC_WRITE_MEMORY:
		return kfi_memory_ioctl_write(client, (void __user *)arg);
	case KFI_IOC_HIDE_MODULE:
		return kfi_hide_module(arg);
	default:
		return -ENOTTY;
	}
}
