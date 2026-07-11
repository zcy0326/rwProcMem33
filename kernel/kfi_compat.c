// SPDX-License-Identifier: GPL-2.0
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/version.h>

#include "kfi_compat.h"

struct class *kfi_compat_class_create(const char *name)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	return class_create(name);
#else
	return class_create(THIS_MODULE, name);
#endif
}

int kfi_compat_walk_vmas(struct mm_struct *mm, unsigned long start,
			 kfi_vma_visitor_t visitor, void *context)
{
	struct vm_area_struct *vma;
	int result = 0;

	if (!mm || !visitor)
		return -EINVAL;
	mmap_read_lock(mm);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	{
		VMA_ITERATOR(iterator, mm, start);

		for_each_vma(iterator, vma) {
			result = visitor(vma, context);
			if (result)
				break;
		}
	}
#else
	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (vma->vm_end <= start)
			continue;
		result = visitor(vma, context);
		if (result)
			break;
	}
#endif
	mmap_read_unlock(mm);
	return result < 0 ? result : 0;
}
