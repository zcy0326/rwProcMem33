// SPDX-License-Identifier: MIT
#ifndef KFI_CLIENT_HPP
#define KFI_CLIENT_HPP

#include <cstdint>
#include <string>

#include "kfi/endpoint.hpp"

#include <linux/kfi.h>

namespace kfi {

static_assert(sizeof(kfi_version) == 72, "unexpected kfi_version layout");
static_assert(sizeof(kfi_caps) == 128, "unexpected kfi_caps layout");
static_assert(sizeof(kfi_open_process) == 64, "unexpected open layout");
static_assert(sizeof(kfi_close_session) == 64, "unexpected close layout");
static_assert(sizeof(kfi_runtime_info) == 256, "unexpected runtime layout");

class Client final {
public:
	Client();
	explicit Client(const Endpoint &endpoint);
	explicit Client(const std::string &device_path);
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
	void hide_module() const;

	const ResolvedEndpoint &endpoint() const noexcept;

private:
	int fd_ = -1;
	ResolvedEndpoint endpoint_{EndpointKind::Device, {}, {}};
};

} // namespace kfi

#endif
