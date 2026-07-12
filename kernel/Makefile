KFI_PROFILE ?= generic
KFI_TRANSPORT_CHAR ?= y
KFI_TRANSPORT_PROC_PRIVATE ?= n

ifeq ($(KFI_PROFILE),generic)
ccflags-y += -DKFI_PROFILE_GENERIC
else ifeq ($(KFI_PROFILE),android12-5.10)
ccflags-y += -DKFI_PROFILE_ANDROID12_5_10
else ifeq ($(KFI_PROFILE),android13-5.15)
ccflags-y += -DKFI_PROFILE_ANDROID13_5_15
else ifeq ($(KFI_PROFILE),android14-6.1)
ccflags-y += -DKFI_PROFILE_ANDROID14_6_1
else ifeq ($(KFI_PROFILE),android15-6.6)
ccflags-y += -DKFI_PROFILE_ANDROID15_6_6
else ifeq ($(KFI_PROFILE),android16-6.12)
ccflags-y += -DKFI_PROFILE_ANDROID16_6_12
else
$(error Unsupported KFI_PROFILE '$(KFI_PROFILE)')
endif

obj-m += kfi.o
kfi-y := kfi_main.o kfi_client.o kfi_compat.o kfi_runtime.o \
	 kfi_selftest.o kfi_transport.o kfi_visibility.o kfi_memory.o \
	 kfi_session.o kfi_uapi.o kfi_task.o kfi_maps.o kfi_event.o

ifeq ($(KFI_TRANSPORT_CHAR),y)
ccflags-y += -DKFI_ENABLE_TRANSPORT_CHAR
kfi-y += kfi_transport_char.o
endif

ifeq ($(KFI_TRANSPORT_PROC_PRIVATE),y)
ifeq ($(wildcard $(src)/generated/kfi_private_config.h),)
$(error Run scripts/gen_private_config.py before enabling the private proc transport)
endif
ccflags-y += -DKFI_ENABLE_TRANSPORT_PROC_PRIVATE
kfi-y += kfi_transport_proc.o
endif

ifeq ($(KFI_TRANSPORT_CHAR)$(KFI_TRANSPORT_PROC_PRIVATE),nn)
$(error At least one KFI transport must be enabled)
endif

ccflags-y += -I$(src)/../include/uapi
