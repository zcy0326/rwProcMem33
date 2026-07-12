// SPDX-License-Identifier: MIT
#ifndef KFI_DETAIL_SYSCALLS_HPP
#define KFI_DETAIL_SYSCALLS_HPP

#include <cstddef>
#include <poll.h>
#include <sys/types.h>

namespace kfi::detail {

struct Syscalls {
	int (*open)(const char *path, int flags);
	int (*ioctl)(int fd, unsigned long request, void *argument);
	int (*dup_cloexec)(int fd);
	int (*close)(int fd);
	ssize_t (*read)(int fd, void *buffer, std::size_t size);
	int (*poll)(struct pollfd *fds, nfds_t count, int timeout_ms);
};

const Syscalls &syscalls() noexcept;
void set_syscalls_for_testing(const Syscalls *replacement) noexcept;

} // namespace kfi::detail

#endif
