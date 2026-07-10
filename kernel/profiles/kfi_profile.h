/* SPDX-License-Identifier: GPL-2.0 */
#ifndef KFI_PROFILE_H
#define KFI_PROFILE_H

#if defined(KFI_PROFILE_ANDROID12_5_10)
#include "android12_5_10.h"
#elif defined(KFI_PROFILE_ANDROID13_5_15)
#include "android13_5_15.h"
#elif defined(KFI_PROFILE_ANDROID14_6_1)
#include "android14_6_1.h"
#elif defined(KFI_PROFILE_ANDROID15_6_6)
#include "android15_6_6.h"
#elif defined(KFI_PROFILE_ANDROID16_6_12)
#include "android16_6_12.h"
#elif defined(KFI_PROFILE_GENERIC)
#include "generic.h"
#else
#error "KFI build profile is not selected"
#endif

#endif
