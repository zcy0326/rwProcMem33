// SPDX-License-Identifier: MIT
#ifndef KFI_DETAIL_ENUMERATE_HPP
#define KFI_DETAIL_ENUMERATE_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include <linux/kfi.h>

namespace kfi::detail {

template <typename Entry, typename Invoke>
std::vector<Entry> enumerate_pages(std::uint64_t session_id,
				   unsigned long command,
				   const char *operation,
				   Invoke &&invoke,
				   std::uint32_t page_capacity = 128)
{
	constexpr std::size_t max_total_entries = 1024U * 1024U;
	if (!session_id || !page_capacity || page_capacity > KFI_ENUM_MAX_ENTRIES)
		throw std::invalid_argument("invalid enumeration arguments");

	std::vector<Entry> result;
	std::uint64_t cursor = 0;
	for (;;) {
		std::vector<Entry> page(page_capacity);
		kfi_enumerate request{};
		request.header.struct_size = sizeof(request);
		request.session_id = session_id;
		request.user_buffer =
			reinterpret_cast<std::uintptr_t>(page.data());
		request.cursor = cursor;
		request.capacity = page_capacity;

		invoke(command, &request, operation);

		if (request.returned > page_capacity)
			throw std::runtime_error(
				"kernel returned an invalid enumeration count");
		if (request.result_flags & ~KFI_ENUM_RESULT_END)
			throw std::runtime_error(
				"kernel returned unknown enumeration flags");
		if (result.size() > max_total_entries - request.returned)
			throw std::length_error(
				"kernel enumeration exceeded userspace limit");

		result.insert(result.end(), page.begin(),
			      page.begin() + request.returned);
		if (request.result_flags & KFI_ENUM_RESULT_END)
			break;
		if (!request.returned || request.next_cursor <= cursor)
			throw std::runtime_error(
				"kernel enumeration made no progress");
		cursor = request.next_cursor;
	}
	return result;
}

} // namespace kfi::detail

#endif
