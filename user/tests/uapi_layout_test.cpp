// SPDX-License-Identifier: MIT
#include <cstddef>
#include <iostream>

#include <linux/kfi.h>

static_assert(sizeof(kfi_request_header) == 16);
static_assert(alignof(kfi_request_header) == 8);
static_assert(offsetof(kfi_request_header, request_id) == 8);

static_assert(sizeof(kfi_version) == 72);
static_assert(sizeof(kfi_caps) == 128);
static_assert(sizeof(kfi_open_process) == 64);
static_assert(sizeof(kfi_close_session) == 64);
static_assert(sizeof(kfi_runtime_info) == 256);
static_assert(sizeof(kfi_memory_io) == 64);
static_assert(sizeof(kfi_visibility_control) == 64);

static_assert(sizeof(kfi_enumerate) == 64);
static_assert(offsetof(kfi_enumerate, cursor) == 32);
static_assert(offsetof(kfi_enumerate, next_cursor) == 48);
static_assert(sizeof(kfi_thread_entry) == 64);
static_assert(offsetof(kfi_thread_entry, comm) == 16);
static_assert(sizeof(kfi_map_entry) == 320);
static_assert(offsetof(kfi_map_entry, path) == 48);
static_assert(sizeof(kfi_event) == 128);
static_assert(offsetof(kfi_event, sequence) == 8);
static_assert(offsetof(kfi_event, data) == 48);
static_assert(sizeof(kfi_event_stats) == 64);
static_assert(offsetof(kfi_event_stats, lost) == 24);

int main()
{
	std::cout << "KFI ABI layout checks passed\n";
	return 0;
}
