/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_UAPI_H
#define KFI_UAPI_H

#include <linux/types.h>
#include <linux/uaccess.h>

#include "../include/uapi/linux/kfi.h"

int kfi_uapi_copy_request(void *destination, size_t expected_size,
			  const void __user *source, u32 allowed_flags);
int kfi_uapi_copy_response(void __user *destination, const void *source,
			   size_t size);
bool kfi_uapi_reserved_is_zero(const __u64 *values, size_t count);

#endif
