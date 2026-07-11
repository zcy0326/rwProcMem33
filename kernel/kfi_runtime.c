// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/utsname.h>
#include <linux/version.h>

#include "kfi_internal.h"
#include "kfi_memory.h"

static u64 kfi_runtime_build_flags(void)
{
	u64 flags = 0;

#ifdef CONFIG_COMPAT
	flags |= KFI_RUNTIME_COMPAT;
#endif
#ifdef CONFIG_MODVERSIONS
	flags |= KFI_RUNTIME_MODVERSIONS;
#endif
#if defined(CONFIG_CFI) || defined(CONFIG_CFI_CLANG)
	flags |= KFI_RUNTIME_CFI;
#endif
#ifdef CONFIG_KCFI
	flags |= KFI_RUNTIME_KCFI;
#endif
#ifdef CONFIG_LTO_CLANG
	flags |= KFI_RUNTIME_LTO_CLANG;
#endif
#ifdef CONFIG_KPROBES
	flags |= KFI_RUNTIME_KPROBES;
#endif
#ifdef CONFIG_HAVE_HW_BREAKPOINT
	flags |= KFI_RUNTIME_HW_BREAKPOINT;
#endif
#ifdef KFI_ENABLE_TRANSPORT_CHAR
	flags |= KFI_RUNTIME_TRANSPORT_CHAR;
#endif
#ifdef KFI_ENABLE_TRANSPORT_PROC_PRIVATE
	flags |= KFI_RUNTIME_TRANSPORT_PROC_PRIVATE;
#endif

	return flags;
}

void kfi_runtime_get(struct kfi_runtime_info *info)
{
	const struct new_utsname *name = utsname();

	memset(info, 0, sizeof(*info));
	info->header.struct_size = sizeof(*info);
	info->linux_version_code = LINUX_VERSION_CODE;
	info->page_size = PAGE_SIZE;
	info->page_shift = PAGE_SHIFT;
	info->build_flags = kfi_runtime_build_flags();
	info->transport_caps = kfi_transport_capabilities();
	info->active_caps = KFI_CAP_CLIENT_ISOLATION |
		KFI_CAP_OPAQUE_SESSIONS | KFI_CAP_RUNTIME_INFO |
		KFI_CAP_SESSION_REFS | info->transport_caps |
		kfi_memory_capabilities() | kfi_visibility_capabilities();
	strscpy(info->release, name->release, sizeof(info->release));
	strscpy(info->machine, name->machine, sizeof(info->machine));
	strscpy(info->profile, KFI_PROFILE_NAME, sizeof(info->profile));
}
