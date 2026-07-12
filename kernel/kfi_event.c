// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include "kfi_event.h"
#include "kfi_internal.h"
#include "kfi_uapi.h"

int kfi_event_client_init(struct kfi_client *client)
{
	if (!client)
		return -EINVAL;

	client->event_ring = kcalloc(KFI_EVENT_RING_CAPACITY,
				     sizeof(*client->event_ring), GFP_KERNEL);
	if (!client->event_ring)
		return -ENOMEM;

	client->event_capacity = KFI_EVENT_RING_CAPACITY;
	client->event_sequence = 1;
	spin_lock_init(&client->event_lock);
	mutex_init(&client->event_read_lock);
	init_waitqueue_head(&client->event_wait);
	return 0;
}

void kfi_event_client_shutdown(struct kfi_client *client)
{
	unsigned long irq_flags;

	if (!client)
		return;
	spin_lock_irqsave(&client->event_lock, irq_flags);
	client->event_shutdown = true;
	spin_unlock_irqrestore(&client->event_lock, irq_flags);
	wake_up_interruptible_poll(&client->event_wait,
				   EPOLLIN | EPOLLRDNORM | EPOLLHUP);
}

void kfi_event_client_destroy(struct kfi_client *client)
{
	if (!client)
		return;
	kvfree(client->event_ring);
	client->event_ring = NULL;
	client->event_capacity = 0;
}

int kfi_event_emit(struct kfi_client *client, const struct kfi_event *event)
{
	struct kfi_event value;
	unsigned long irq_flags;
	int result = 0;

	if (!client || !event || !event->type || !client->event_ring)
		return -EINVAL;

	value = *event;
	value.size = sizeof(value);
	value.flags = KFI_EVENT_FLAG_NONE;
	value.timestamp_ns = ktime_get_ns();
	value.cpu = raw_smp_processor_id();
	value.reserved0 = 0;
	memset(value.reserved, 0, sizeof(value.reserved));

	spin_lock_irqsave(&client->event_lock, irq_flags);
	value.sequence = client->event_sequence++;
	if (client->event_shutdown) {
		result = -ESHUTDOWN;
	} else if (client->event_count == client->event_capacity) {
		client->event_lost++;
		result = -ENOSPC;
	} else {
		client->event_ring[client->event_head] = value;
		client->event_head = (client->event_head + 1) %
				     client->event_capacity;
		client->event_count++;
	}
	spin_unlock_irqrestore(&client->event_lock, irq_flags);

	if (!result)
		wake_up_interruptible_poll(&client->event_wait,
					   EPOLLIN | EPOLLRDNORM);
	return result;
}

static bool kfi_event_ready(const struct kfi_client *client)
{
	return READ_ONCE(client->event_count) != 0 ||
	       READ_ONCE(client->event_shutdown);
}

ssize_t kfi_event_read(struct file *file, char __user *buffer, size_t size)
{
	struct kfi_client *client = file->private_data;
	struct kfi_event *events;
	unsigned long irq_flags;
	u32 available;
	u32 index;
	u32 count;
	int error;

	if (!client || !buffer)
		return -EINVAL;
	if (size < sizeof(*events))
		return -EINVAL;

	count = min_t(size_t, size / sizeof(*events), KFI_EVENT_READ_MAX);
	events = kvmalloc_array(count, sizeof(*events), GFP_KERNEL);
	if (!events)
		return -ENOMEM;

	error = mutex_lock_interruptible(&client->event_read_lock);
	if (error)
		goto out_free;

	for (;;) {
		if (kfi_event_ready(client))
			break;
		if (file->f_flags & O_NONBLOCK) {
			error = -EAGAIN;
			goto out_unlock;
		}
		error = wait_event_interruptible(client->event_wait,
						 kfi_event_ready(client));
		if (error)
			goto out_unlock;
	}

	spin_lock_irqsave(&client->event_lock, irq_flags);
	available = min(count, client->event_count);
	for (index = 0; index < available; index++)
		events[index] = client->event_ring[
			(client->event_tail + index) % client->event_capacity];
	spin_unlock_irqrestore(&client->event_lock, irq_flags);

	if (!available) {
		error = 0;
		goto out_unlock;
	}
	if (copy_to_user(buffer, events, available * sizeof(*events))) {
		error = -EFAULT;
		goto out_unlock;
	}

	spin_lock_irqsave(&client->event_lock, irq_flags);
	client->event_tail = (client->event_tail + available) %
			     client->event_capacity;
	client->event_count -= available;
	spin_unlock_irqrestore(&client->event_lock, irq_flags);
	error = available * sizeof(*events);

out_unlock:
	mutex_unlock(&client->event_read_lock);
out_free:
	kvfree(events);
	return error;
}

__poll_t kfi_event_poll(struct file *file, poll_table *wait)
{
	struct kfi_client *client = file->private_data;
	unsigned long irq_flags;
	__poll_t result = 0;

	if (!client)
		return EPOLLERR;
	poll_wait(file, &client->event_wait, wait);
	spin_lock_irqsave(&client->event_lock, irq_flags);
	if (client->event_count)
		result |= EPOLLIN | EPOLLRDNORM;
	if (client->event_shutdown)
		result |= EPOLLHUP;
	spin_unlock_irqrestore(&client->event_lock, irq_flags);
	return result;
}

int kfi_event_ioctl_get_stats(struct kfi_client *client, void __user *argument)
{
	struct kfi_event_stats stats;
	unsigned long irq_flags;
	u64 request_id;
	int error;

	error = kfi_uapi_copy_request(&stats, sizeof(stats), argument,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (stats.queued || stats.capacity || stats.lost || stats.next_sequence ||
	    stats.flags || stats.reserved0 ||
	    !kfi_uapi_reserved_is_zero(stats.reserved,
				       ARRAY_SIZE(stats.reserved)))
		return -EINVAL;

	request_id = stats.header.request_id;
	memset(&stats, 0, sizeof(stats));
	stats.header.struct_size = sizeof(stats);
	stats.header.request_id = request_id;
	spin_lock_irqsave(&client->event_lock, irq_flags);
	stats.queued = client->event_count;
	stats.capacity = client->event_capacity;
	stats.lost = client->event_lost;
	stats.next_sequence = client->event_sequence;
	if (client->event_shutdown)
		stats.flags |= KFI_EVENT_STATS_FLAG_SHUTDOWN;
	spin_unlock_irqrestore(&client->event_lock, irq_flags);
	return kfi_uapi_copy_response(argument, &stats, sizeof(stats));
}

u64 kfi_event_capabilities(void)
{
	return KFI_CAP_POLL_EVENTS | KFI_CAP_EVENT_STATS;
}
