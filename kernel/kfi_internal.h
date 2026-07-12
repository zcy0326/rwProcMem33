/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_INTERNAL_H
#define KFI_INTERNAL_H

#include <linux/atomic.h>
#include <linux/build_bug.h>
#include <linux/idr.h>
#include <linux/kref.h>
#include <linux/mutex.h>
#include <linux/pid.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/types.h>

#include "../include/uapi/linux/kfi.h"
#include "profiles/kfi_profile.h"
#include "kfi_transport.h"
#include "kfi_visibility.h"

#define KFI_MODULE_VERSION 0x00010300U
#define KFI_MAX_SESSIONS 4096U
#define KFI_MAX_IO_SIZE (1024U * 1024U)
#define KFI_EVENT_RING_CAPACITY 256U
#define KFI_EVENT_READ_MAX 256U

struct kfi_session {
	struct kref reference;
	atomic_t closing;
	u64 id;
	struct pid *tgid;
	pid_t opened_pid;
	pid_t target_tgid;
};

struct kfi_client {
	u64 id;
	kuid_t owner_euid;
	pid_t opener_pid;
	pid_t opener_tgid;
	enum kfi_transport_kind transport;
	struct mutex lock;
	struct idr sessions;
	u32 session_generation;
	bool closing;

	struct kfi_event *event_ring;
	u32 event_capacity;
	u32 event_head;
	u32 event_tail;
	u32 event_count;
	u64 event_sequence;
	u64 event_lost;
	bool event_shutdown;
	spinlock_t event_lock;
	struct mutex event_read_lock;
	wait_queue_head_t event_wait;
};

int kfi_client_create(struct file *file,
			 enum kfi_transport_kind transport);
void kfi_client_destroy(struct file *file);
long kfi_dispatch_ioctl(struct kfi_client *client, unsigned int cmd,
			unsigned long arg);
bool kfi_client_authorized(const struct kfi_client *client);
void kfi_runtime_get(struct kfi_runtime_info *info);
int kfi_selftest_run(void);

static_assert(sizeof(struct kfi_request_header) == 16);
static_assert(sizeof(struct kfi_version) == 72);
static_assert(sizeof(struct kfi_caps) == 128);
static_assert(sizeof(struct kfi_open_process) == 64);
static_assert(sizeof(struct kfi_close_session) == 64);
static_assert(sizeof(struct kfi_runtime_info) == 256);
static_assert(sizeof(struct kfi_memory_io) == 64);
static_assert(sizeof(struct kfi_enumerate) == 64);
static_assert(sizeof(struct kfi_thread_entry) == 64);
static_assert(sizeof(struct kfi_map_entry) == 320);
static_assert(sizeof(struct kfi_visibility_control) == 64);
static_assert(sizeof(struct kfi_event) == 128);
static_assert(sizeof(struct kfi_event_stats) == 64);

#endif
