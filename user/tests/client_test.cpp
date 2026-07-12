// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"
#include "kfi/detail/syscalls.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <vector>

namespace {

struct State {
	std::vector<std::uint32_t> chunks;
	std::vector<std::uint64_t> addresses;
	int memory_calls = 0;
	int fail_memory_call = -1;
	std::uint32_t fail_completed = 0;
	int close_session_errno = 0;
	int close_session_calls = 0;
	int close_fd_calls = 0;
};

State state;

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

int mock_open(const char *, int)
{
	return 42;
}

int mock_dup(int)
{
	return 43;
}

int mock_close(int)
{
	++state.close_fd_calls;
	return 0;
}

ssize_t mock_read(int, void *, std::size_t)
{
	errno = EAGAIN;
	return -1;
}

int mock_poll(struct pollfd *, nfds_t, int)
{
	return 0;
}

int mock_ioctl(int, unsigned long request, void *argument)
{
	if (request == KFI_IOC_OPEN_PROCESS) {
		auto *value = static_cast<kfi_open_process *>(argument);
		value->session_id = 0x100000001ULL;
		return 0;
	}
	if (request == KFI_IOC_GET_CAPS) {
		auto *value = static_cast<kfi_caps *>(argument);
		value->max_io_size = 4;
		return 0;
	}
	if (request == KFI_IOC_CLOSE_SESSION) {
		++state.close_session_calls;
		if (state.close_session_errno) {
			errno = state.close_session_errno;
			return -1;
		}
		return 0;
	}
	if (request == KFI_IOC_READ_MEMORY || request == KFI_IOC_WRITE_MEMORY) {
		auto *value = static_cast<kfi_memory_io *>(argument);
		state.chunks.push_back(value->requested_size);
		state.addresses.push_back(value->remote_address);
		const int call = state.memory_calls++;
		if (call == state.fail_memory_call) {
			value->completed_size = state.fail_completed;
			errno = EFAULT;
			return -1;
		}
		if (request == KFI_IOC_READ_MEMORY)
			std::memset(reinterpret_cast<void *>(value->user_buffer), 0x5a,
				    value->requested_size);
		value->completed_size = value->requested_size;
		return 0;
	}
	errno = ENOTTY;
	return -1;
}

const kfi::detail::Syscalls mock_syscalls{
	&mock_open,
	&mock_ioctl,
	&mock_dup,
	&mock_close,
	&mock_read,
	&mock_poll,
};

} // namespace

int main()
{
	try {
		kfi::detail::set_syscalls_for_testing(&mock_syscalls);
		kfi::Client client(kfi::Endpoint::Device("/dev/mock-kfi"));
		auto session = client.open_process_session(1234);
		require(session.valid(), "session is not valid");
		require(session.id() == 0x100000001ULL, "session id mismatch");

		unsigned char buffer[10]{};
		const auto completed = session.read(0x1000, buffer, sizeof(buffer));
		require(completed == sizeof(buffer), "full read did not complete");
		require(state.chunks == std::vector<std::uint32_t>({4, 4, 2}),
			"memory request was not chunked");
		require(state.addresses == std::vector<std::uint64_t>({0x1000, 0x1004, 0x1008}),
			"remote addresses did not advance");
		for (const auto byte : buffer)
			require(byte == 0x5a, "read buffer was not populated");

		state.chunks.clear();
		state.addresses.clear();
		state.memory_calls = 0;
		state.fail_memory_call = 1;
		state.fail_completed = 2;
		try {
			(void)session.read(0x2000, buffer, 8);
			throw std::runtime_error("partial failure did not throw");
		} catch (const kfi::MemoryTransferError &error) {
			require(error.completed() == 6,
				"partial completion count was lost");
		}

		state.close_session_errno = EBUSY;
		try {
			session.close();
			throw std::runtime_error("close failure did not throw");
		} catch (const std::system_error &) {
			require(session.valid(), "failed close invalidated session");
		}
		state.close_session_errno = 0;
		session.close();
		require(!session.valid(), "successful close kept session valid");
	} catch (const std::exception &error) {
		std::cerr << "client_test: " << error.what() << '\n';
		kfi::detail::set_syscalls_for_testing(nullptr);
		return 1;
	}

	kfi::detail::set_syscalls_for_testing(nullptr);
	return 0;
}
