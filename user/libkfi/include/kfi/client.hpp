// SPDX-License-Identifier: MIT
#ifndef KFI_CLIENT_HPP
#define KFI_CLIENT_HPP

#include <cstdint>
#include <string>

#include <linux/kfi.h>

namespace kfi {

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
	std::uint64_t open_process(std::int32_t pid) const;
	void close_session(std::uint64_t session_id) const;

private:
	int fd_ = -1;
};

} // namespace kfi

#endif
