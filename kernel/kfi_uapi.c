// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "kfi_uapi.h"

int kfi_uapi_copy_request(void *destination, size_t expected_size,
			  const void __user *source, u32 allowed_flags)
{
	struct kfi_request_header *header = destination;

	if (!destination || !source || expected_size < sizeof(*header))
		return -EINVAL;
	if (copy_from_user(destination, source, expected_size))
		return -EFAULT;
	if (header->struct_size != expected_size)
		return -EMSGSIZE;
	if (header->flags & ~allowed_flags)
		return -EINVAL;

	return 0;
}

int kfi_uapi_copy_response(void __user *destination, const void *source,
			   size_t size)
{
	if (!destination || !source || !size)
		return -EINVAL;
	if (copy_to_user(destination, source, size))
		return -EFAULT;
	return 0;
}

bool kfi_uapi_reserved_is_zero(const __u64 *values, size_t count)
{
	size_t index;

	for (index = 0; index < count; index++) {
		if (values[index])
			return false;
	}
	return true;
}
