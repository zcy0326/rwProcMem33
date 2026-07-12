// SPDX-License-Identifier: MIT
#ifndef KFI_DETAIL_SYSCALLS_HPP
#define KFI_DETAIL_SYSCALLS_HPP

namespace kfi::detail {

struct Syscalls {
	int (*open)(const char *path, int flags);
	int (*ioctl)(int fd, unsigned long request, void *argument);
	int (*dup_cloexec)(int fd);
	int (*close)(int fd);
};

const Syscalls &syscalls() noexcept;
void set_syscalls_for_testing(const Syscalls *replacement) noexcept;

} // namespace kfi::detail

#endif
