// SPDX-License-Identifier: MIT
#ifndef KFI_CLIENT_HPP
#define KFI_CLIENT_HPP

#include <cstdint>
#include <string>

#include <linux/kfi.h>

namespace kfi {

static_assert(sizeof(kfi_version) == 56, "unexpected kfi_version layout");
static_assert(sizeof(kfi_caps) == 64, "unexpected kfi_caps layout");
static_assert(sizeof(kfi_open_process) == 40, "unexpected open layout");
static_assert(sizeof(kfi_close_session) == 32, "unexpected close layout");
static_assert(sizeof(kfi_runtime_info) == 176, "unexpected runtime layout");

class Client final {
public:
	explicit Client(const std::string &path = KFI_DEVICE_PATH);
	~Client();

	Client(const Client &) = delete;
	Client &operator=(const Client &) = delete;
	Client(Client &&other) noexcept;
	Client &operator=(Client &&other) noexcept;

	kfi_version version() const;
	kfi_caps capabilities() const;
	kfi_runtime_info runtime_info() const;
	std::uint64_t open_process(std::int32_t pid) const;
	void close_session(std::uint64_t session_id) const;

private:
	int fd_ = -1;
};

} // namespace kfi

#endif
