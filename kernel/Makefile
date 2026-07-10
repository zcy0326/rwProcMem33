KFI_PROFILE ?= generic

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
	 kfi_selftest.o

ccflags-y += -I$(src)/../include/uapi
