// SPDX-License-Identifier: MIT
#include "kfi/detail/syscalls.hpp"

#include <atomic>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace kfi::detail {
namespace {

int system_open(const char *path, int flags)
{
	return ::open(path, flags);
}

int system_ioctl(int fd, unsigned long request, void *argument)
{
	return ::ioctl(fd, request, argument);
}

int system_dup_cloexec(int fd)
{
	return ::fcntl(fd, F_DUPFD_CLOEXEC, 0);
}

int system_close(int fd)
{
	return ::close(fd);
}

const Syscalls default_syscalls{
	&system_open,
	&system_ioctl,
	&system_dup_cloexec,
	&system_close,
};

std::atomic<const Syscalls *> active_syscalls{&default_syscalls};

} // namespace

const Syscalls &syscalls() noexcept
{
	return *active_syscalls.load(std::memory_order_acquire);
}

void set_syscalls_for_testing(const Syscalls *replacement) noexcept
{
	active_syscalls.store(replacement ? replacement : &default_syscalls,
			      std::memory_order_release);
}

} // namespace kfi::detail
