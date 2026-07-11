// SPDX-License-Identifier: MIT
#include <cstddef>
#include <iostream>

#include <linux/kfi.h>

static_assert(sizeof(kfi_request_header) == 16);
static_assert(alignof(kfi_request_header) == 8);
static_assert(offsetof(kfi_request_header, request_id) == 8);

static_assert(sizeof(kfi_version) == 72);
static_assert(alignof(kfi_version) == 8);
static_assert(offsetof(kfi_version, major) == 16);
static_assert(offsetof(kfi_version, client_id) == 32);

static_assert(sizeof(kfi_caps) == 128);
static_assert(offsetof(kfi_caps, flags) == 16);
static_assert(offsetof(kfi_caps, reserved) == 48);

static_assert(sizeof(kfi_open_process) == 64);
static_assert(offsetof(kfi_open_process, pid) == 16);
static_assert(offsetof(kfi_open_process, session_id) == 24);

static_assert(sizeof(kfi_close_session) == 64);
static_assert(offsetof(kfi_close_session, session_id) == 16);

static_assert(sizeof(kfi_runtime_info) == 256);
static_assert(offsetof(kfi_runtime_info, build_flags) == 32);
static_assert(offsetof(kfi_runtime_info, release) == 56);
static_assert(offsetof(kfi_runtime_info, profile) == 152);

static_assert(sizeof(kfi_memory_io) == 64);
static_assert(offsetof(kfi_memory_io, remote_address) == 24);
static_assert(offsetof(kfi_memory_io, requested_size) == 40);

static_assert(sizeof(kfi_visibility_control) == 64);
static_assert(offsetof(kfi_visibility_control, operations) == 16);
static_assert(sizeof(kfi_enumerate) == 64);
static_assert(sizeof(kfi_thread_entry) == 64);
static_assert(sizeof(kfi_map_entry) == 320);

int main()
{
	std::cout << "KFI ABI layout checks passed\n";
	return 0;
}
