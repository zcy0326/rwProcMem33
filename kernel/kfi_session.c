// SPDX-License-Identifier: GPL-2.0
#include <linux/atomic.h>
#include <linux/errno.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/kref.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "kfi_internal.h"
#include "kfi_event.h"
#include "kfi_session.h"
#include "kfi_uapi.h"

static void kfi_session_release(struct kref *reference)
{
	struct kfi_session *session =
		container_of(reference, struct kfi_session, reference);

	put_pid(session->tgid);
	kfree(session);
}

void kfi_session_put(struct kfi_session *session)
{
	if (session)
		kref_put(&session->reference, kfi_session_release);
}

u64 kfi_session_id(const struct kfi_session *session)
{
	return session->id;
}

struct task_struct *kfi_session_get_task(struct kfi_session *session)
{
	if (!session)
		return NULL;
	return get_pid_task(session->tgid, PIDTYPE_TGID);
}

struct kfi_session *kfi_session_lookup_get(struct kfi_client *client, u64 id)
{
	struct kfi_session *session;
	u32 slot = (u32)id;

	if (!client || !id || !slot || slot > KFI_MAX_SESSIONS)
		return NULL;

	mutex_lock(&client->lock);
	session = idr_find(&client->sessions, slot);
	if (!session || session->id != id || atomic_read(&session->closing) ||
	    !kref_get_unless_zero(&session->reference))
		session = NULL;
	mutex_unlock(&client->lock);

	return session;
}

static struct kfi_session *kfi_session_remove_locked(
	struct kfi_client *client, u64 id)
{
	struct kfi_session *session;
	u32 slot = (u32)id;

	if (!id || !slot || slot > KFI_MAX_SESSIONS)
		return NULL;
	session = idr_find(&client->sessions, slot);
	if (!session || session->id != id)
		return NULL;
	idr_remove(&client->sessions, slot);
	atomic_set(&session->closing, 1);
	return session;
}

static int kfi_session_remove(struct kfi_client *client, u64 id, bool notify)
{
	struct kfi_session *session;

	mutex_lock(&client->lock);
	session = kfi_session_remove_locked(client, id);
	mutex_unlock(&client->lock);
	if (!session)
		return -ENOENT;
	if (notify) {
		struct kfi_event event = {
			.type = KFI_EVENT_TYPE_SESSION_CLOSED,
			.session_id = session->id,
			.pid = session->target_tgid,
			.tid = session->opened_pid,
		};

		(void)kfi_event_emit(client, &event);
	}
	kfi_session_put(session);
	return 0;
}

int kfi_session_ioctl_open(struct kfi_client *client, void __user *argument)
{
	struct kfi_open_process request;
	struct kfi_session *session;
	struct task_struct *task;
	struct pid *pid;
	u32 generation;
	int slot;
	int error;

	error = kfi_uapi_copy_request(&request, sizeof(request), argument,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (request.pid <= 0 || request.reserved0 || request.session_id ||
	    !kfi_uapi_reserved_is_zero(request.reserved,
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
	kref_init(&session->reference);
	atomic_set(&session->closing, 0);
	session->tgid = get_task_pid(task, PIDTYPE_TGID);
	session->opened_pid = request.pid;
	session->target_tgid = task_tgid_nr(task);
	put_task_struct(task);
	if (!session->tgid) {
		kfi_session_put(session);
		return -ESRCH;
	}

	mutex_lock(&client->lock);
	if (client->closing) {
		slot = -ESHUTDOWN;
	} else {
		slot = idr_alloc(&client->sessions, session, 1,
				 KFI_MAX_SESSIONS + 1, GFP_KERNEL);
	}
	if (slot >= 0) {
		generation = ++client->session_generation;
		if (!generation)
			generation = ++client->session_generation;
		session->id = ((u64)generation << 32) | (u32)slot;
	}
	mutex_unlock(&client->lock);
	if (slot < 0) {
		kfi_session_put(session);
		return slot;
	}

	request.session_id = session->id;
	error = kfi_uapi_copy_response(argument, &request, sizeof(request));
	if (error) {
		kfi_session_remove(client, session->id, false);
	} else {
		struct kfi_event event = {
			.type = KFI_EVENT_TYPE_SESSION_OPENED,
			.session_id = session->id,
			.pid = session->target_tgid,
			.tid = session->opened_pid,
		};

		(void)kfi_event_emit(client, &event);
	}
	return error;
}

int kfi_session_ioctl_close(struct kfi_client *client, void __user *argument)
{
	struct kfi_close_session request;
	int error;

	error = kfi_uapi_copy_request(&request, sizeof(request), argument,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (!request.session_id ||
	    !kfi_uapi_reserved_is_zero(request.reserved,
				       ARRAY_SIZE(request.reserved)))
		return -EINVAL;

	error = kfi_session_remove(client, request.session_id, true);
	if (error)
		return error;
	request.session_id = 0;
	return kfi_uapi_copy_response(argument, &request, sizeof(request));
}

void kfi_session_shutdown_all(struct kfi_client *client)
{
	struct kfi_session *session;
	int slot;

	for (;;) {
		slot = 0;
		mutex_lock(&client->lock);
		session = idr_get_next(&client->sessions, &slot);
		if (session) {
			idr_remove(&client->sessions, slot);
			atomic_set(&session->closing, 1);
		}
		mutex_unlock(&client->lock);
		if (!session)
			break;
		kfi_session_put(session);
	}

	idr_destroy(&client->sessions);
}
