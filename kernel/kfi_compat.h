/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_COMPAT_H
#define KFI_COMPAT_H

#include <linux/device.h>

struct mm_struct;
struct vm_area_struct;
typedef int (*kfi_vma_visitor_t)(struct vm_area_struct *vma, void *context);

struct class *kfi_compat_class_create(const char *name);
int kfi_compat_walk_vmas(struct mm_struct *mm, unsigned long start,
			 kfi_vma_visitor_t visitor, void *context);

#endif
