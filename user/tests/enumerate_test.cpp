// SPDX-License-Identifier: MIT
#include "kfi/detail/enumerate.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

} // namespace

int main()
{
	try {
		std::vector<std::uint64_t> cursors;
		int call = 0;
		auto values = kfi::detail::enumerate_pages<kfi_thread_entry>(
			0x100000001ULL, KFI_IOC_ENUM_THREADS,
			"KFI_IOC_ENUM_THREADS",
			[&](unsigned long command, void *argument, const char *) {
				require(command == KFI_IOC_ENUM_THREADS,
					"unexpected command");
				auto *request =
					static_cast<kfi_enumerate *>(argument);
				cursors.push_back(request->cursor);
				auto *entries = reinterpret_cast<kfi_thread_entry *>(
					request->user_buffer);
				if (call++ == 0) {
					entries[0].tid = 10;
					entries[0].flags =
						KFI_THREAD_FLAG_LEADER;
					entries[1].tid = 11;
					request->returned = 2;
					request->next_cursor = 2;
				} else {
					entries[0].tid = 12;
					request->returned = 1;
					request->next_cursor = 3;
					request->result_flags =
						KFI_ENUM_RESULT_END;
				}
			},
			2);
		require(values.size() == 3, "pagination lost entries");
		require(values[0].tid == 10 && values[2].tid == 12,
			"pagination order mismatch");
		require((values[0].flags & KFI_THREAD_FLAG_LEADER) != 0,
			"leader flag was lost");
		require(cursors == std::vector<std::uint64_t>({0, 2}),
			"cursor progression mismatch");

		bool no_progress_rejected = false;
		try {
			(void)kfi::detail::enumerate_pages<kfi_thread_entry>(
				1, KFI_IOC_ENUM_THREADS, "test",
				[](unsigned long, void *argument, const char *) {
					auto *request =
						static_cast<kfi_enumerate *>(
							argument);
					request->returned = 1;
					request->next_cursor = request->cursor;
				},
				1);
		} catch (const std::runtime_error &) {
			no_progress_rejected = true;
		}
		require(no_progress_rejected,
			"non-progressing enumeration was accepted");

		bool count_rejected = false;
		try {
			(void)kfi::detail::enumerate_pages<kfi_map_entry>(
				1, KFI_IOC_ENUM_MAPS, "test",
				[](unsigned long, void *argument, const char *) {
					auto *request =
						static_cast<kfi_enumerate *>(
							argument);
					request->returned =
						request->capacity + 1;
					request->result_flags =
						KFI_ENUM_RESULT_END;
				},
				2);
		} catch (const std::runtime_error &) {
			count_rejected = true;
		}
		require(count_rejected, "invalid result count was accepted");
	} catch (const std::exception &error) {
		std::cerr << "enumerate_test: " << error.what() << '\n';
		return 1;
	}
	return 0;
}
