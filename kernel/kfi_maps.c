// SPDX-License-Identifier: GPL-2.0
#include <linux/dcache.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/limits.h>
#include <linux/mm.h>
#include <linux/overflow.h>
#include <linux/path.h>
#include <linux/sched/mm.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "kfi_compat.h"
#include "kfi_internal.h"
#include "kfi_maps.h"
#include "kfi_session.h"
#include "kfi_uapi.h"

struct kfi_maps_context {
	struct kfi_map_entry *entries;
	u32 capacity;
	u32 count;
	bool exhausted;
	char *path_buffer;
};

static int kfi_maps_visit(struct vm_area_struct *vma, void *opaque)
{
	struct kfi_maps_context *context = opaque;
	struct kfi_map_entry *entry;
	struct inode *inode;
	char *resolved;
	size_t length;

	if (context->count == context->capacity) {
		context->exhausted = false;
		return 1;
	}

	entry = &context->entries[context->count++];
	entry->start = vma->vm_start;
	entry->end = vma->vm_end;
	entry->offset = (u64)vma->vm_pgoff << PAGE_SHIFT;
	if (vma->vm_flags & VM_READ)
		entry->prot |= KFI_PROT_READ;
	if (vma->vm_flags & VM_WRITE)
		entry->prot |= KFI_PROT_WRITE;
	if (vma->vm_flags & VM_EXEC)
		entry->prot |= KFI_PROT_EXEC;
	entry->flags |= (vma->vm_flags & VM_SHARED) ?
		KFI_MAP_FLAG_SHARED : KFI_MAP_FLAG_PRIVATE;
	if (!vma->vm_file)
		return 0;

	entry->flags |= KFI_MAP_FLAG_FILE;
	inode = file_inode(vma->vm_file);
	entry->inode = inode->i_ino;
	entry->dev_major = MAJOR(inode->i_sb->s_dev);
	entry->dev_minor = MINOR(inode->i_sb->s_dev);
	resolved = d_path(&vma->vm_file->f_path, context->path_buffer, PAGE_SIZE);
	if (IS_ERR(resolved))
		return 0;
	length = strlen(resolved);
	if (length >= sizeof(entry->path))
		entry->flags |= KFI_MAP_FLAG_PATH_TRUNCATED;
	strscpy(entry->path, resolved, sizeof(entry->path));
	return 0;
}

u64 kfi_maps_capabilities(void)
{
	return KFI_CAP_QUERY_MAPS | KFI_CAP_ENUM_MAPS_PAGED;
}

int kfi_maps_ioctl_enumerate(struct kfi_client *client,
			     void __user *argument)
{
	struct kfi_maps_context context = { .exhausted = true };
	struct kfi_enumerate request;
	struct kfi_session *session;
	struct task_struct *task;
	struct mm_struct *mm;
	size_t bytes;
	int error;

	error = kfi_uapi_copy_request(&request, sizeof(request), argument,
				      KFI_REQUEST_FLAGS_NONE);
	if (error)
		return error;
	if (!request.session_id || !request.user_buffer || !request.capacity ||
	    request.capacity > KFI_ENUM_MAX_ENTRIES || request.returned ||
	    request.next_cursor || request.result_flags || request.reserved0)
		return -EINVAL;
	if (request.cursor > (u64)ULONG_MAX)
		return -EOVERFLOW;
	if (check_mul_overflow((size_t)request.capacity,
			       sizeof(struct kfi_map_entry), &bytes) ||
	    !access_ok(u64_to_user_ptr(request.user_buffer), bytes))
		return -EFAULT;

	session = kfi_session_lookup_get(client, request.session_id);
	if (!session)
		return -ENOENT;
	task = kfi_session_get_task(session);
	if (!task) {
		error = -ESRCH;
		goto out_session;
	}
	mm = get_task_mm(task);
	put_task_struct(task);
	if (!mm) {
		error = -ENXIO;
		goto out_session;
	}

	context.capacity = request.capacity;
	context.entries = kcalloc(context.capacity, sizeof(*context.entries),
				  GFP_KERNEL);
	context.path_buffer = (char *)__get_free_page(GFP_KERNEL);
	if (!context.entries || !context.path_buffer) {
		error = -ENOMEM;
		goto out_buffers;
	}
	error = kfi_compat_walk_vmas(mm, (unsigned long)request.cursor,
				     kfi_maps_visit, &context);
	if (error)
		goto out_buffers;
	if (copy_to_user(u64_to_user_ptr(request.user_buffer),
			 context.entries,
			 context.count * sizeof(*context.entries))) {
		error = -EFAULT;
		goto out_buffers;
	}

	request.returned = context.count;
	request.next_cursor = context.count ?
		context.entries[context.count - 1].end : request.cursor;
	if (context.exhausted)
		request.result_flags |= KFI_ENUM_RESULT_END;
	error = kfi_uapi_copy_response(argument, &request, sizeof(request));

out_buffers:
	if (context.path_buffer)
		free_page((unsigned long)context.path_buffer);
	kfree(context.entries);
	mmput(mm);
out_session:
	kfi_session_put(session);
	return error;
}
