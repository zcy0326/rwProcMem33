// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"
#include "kfi/detail/syscalls.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <stdexcept>

namespace {

struct State {
	bool ready = true;
	bool malformed_bytes = false;
	bool malformed_size = false;
	int read_calls = 0;
};

State state;

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

int mock_open(const char *, int)
{
	return 51;
}

int mock_ioctl(int, unsigned long request, void *argument)
{
	if (request == KFI_IOC_GET_EVENT_STATS) {
		auto *stats = static_cast<kfi_event_stats *>(argument);
		stats->queued = 2;
		stats->capacity = 256;
		stats->lost = 3;
		stats->next_sequence = 10;
		return 0;
	}
	errno = ENOTTY;
	return -1;
}

int mock_dup(int)
{
	return 52;
}

int mock_close(int)
{
	return 0;
}

ssize_t mock_read(int, void *buffer, std::size_t size)
{
	++state.read_calls;
	if (size < 2 * sizeof(kfi_event)) {
		errno = EINVAL;
		return -1;
	}
	auto *events = static_cast<kfi_event *>(buffer);
	std::memset(events, 0, 2 * sizeof(*events));
	events[0].type = KFI_EVENT_TYPE_SESSION_OPENED;
	events[0].size = state.malformed_size ? 64 : sizeof(kfi_event);
	events[0].sequence = 8;
	events[0].session_id = 0x100000001ULL;
	events[1].type = KFI_EVENT_TYPE_SESSION_CLOSED;
	events[1].size = sizeof(kfi_event);
	events[1].sequence = 9;
	if (state.malformed_bytes)
		return sizeof(kfi_event) + 1;
	return 2 * sizeof(kfi_event);
}

int mock_poll(struct pollfd *fds, nfds_t count, int)
{
	require(count == 1, "unexpected poll descriptor count");
	fds[0].revents = state.ready ? POLLIN : 0;
	return state.ready ? 1 : 0;
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

		const auto stats = client.event_stats();
		require(stats.queued == 2 && stats.capacity == 256,
			"event stats counts mismatch");
		require(stats.lost == 3 && stats.next_sequence == 10,
			"event stats sequence mismatch");

		require(client.wait_for_events(0), "ready event stream timed out");
		const auto events = client.read_events(4);
		require(events.size() == 2, "event batch size mismatch");
		require(events[0].type == KFI_EVENT_TYPE_SESSION_OPENED,
			"first event type mismatch");
		require(events[1].sequence == 9, "event sequence mismatch");
		require(std::string(kfi::event_type_name(events[1].type)) ==
			"session-closed", "event type name mismatch");

		state.ready = false;
		require(!client.wait_for_events(0), "empty event stream was ready");
		state.ready = true;

		state.malformed_bytes = true;
		bool malformed_rejected = false;
		try {
			(void)client.read_events(4);
		} catch (const std::runtime_error &) {
			malformed_rejected = true;
		}
		require(malformed_rejected, "malformed byte count was accepted");
		state.malformed_bytes = false;

		state.malformed_size = true;
		bool size_rejected = false;
		try {
			(void)client.read_events(4);
		} catch (const std::runtime_error &) {
			size_rejected = true;
		}
		require(size_rejected, "unsupported event size was accepted");
	} catch (const std::exception &error) {
		std::cerr << "event_test: " << error.what() << '\n';
		kfi::detail::set_syscalls_for_testing(nullptr);
		return 1;
	}
	kfi::detail::set_syscalls_for_testing(nullptr);
	return 0;
}
