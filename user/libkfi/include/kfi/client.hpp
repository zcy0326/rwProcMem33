// SPDX-License-Identifier: MIT
#ifndef KFI_CLIENT_HPP
#define KFI_CLIENT_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

#include "kfi/endpoint.hpp"

#include <linux/kfi.h>

namespace kfi {

static_assert(sizeof(kfi_version) == 72, "unexpected kfi_version layout");
static_assert(sizeof(kfi_caps) == 128, "unexpected kfi_caps layout");
static_assert(sizeof(kfi_open_process) == 64, "unexpected open layout");
static_assert(sizeof(kfi_close_session) == 64, "unexpected close layout");
static_assert(sizeof(kfi_runtime_info) == 256, "unexpected runtime layout");
static_assert(sizeof(kfi_memory_io) == 64, "unexpected memory layout");
static_assert(sizeof(kfi_enumerate) == 64, "unexpected enumerate layout");
static_assert(sizeof(kfi_thread_entry) == 64, "unexpected thread layout");
static_assert(sizeof(kfi_map_entry) == 320, "unexpected map layout");
static_assert(sizeof(kfi_visibility_control) == 64,
	      "unexpected visibility layout");

class MemoryTransferError final : public std::system_error {
public:
	MemoryTransferError(std::error_code error, std::size_t completed,
			    const char *operation);

	std::size_t completed() const noexcept;

private:
	std::size_t completed_;
};

class Session final {
public:
	Session() = delete;
	~Session();

	Session(const Session &) = delete;
	Session &operator=(const Session &) = delete;
	Session(Session &&other) noexcept;
	Session &operator=(Session &&other) noexcept;

	std::uint64_t id() const noexcept;
	bool valid() const noexcept;
	void close();
	std::size_t read(std::uint64_t remote_address, void *buffer,
			 std::size_t size) const;
	std::size_t write(std::uint64_t remote_address, const void *buffer,
			  std::size_t size) const;
	std::vector<kfi_thread_entry> threads() const;
	std::vector<kfi_map_entry> maps() const;

private:
	friend class Client;
	Session(int fd, std::uint64_t id, std::uint32_t max_io_size);
	void close_noexcept() noexcept;
	std::size_t transfer(std::uint64_t remote_address, void *buffer,
			     std::size_t size, bool write) const;

	int fd_ = -1;
	std::uint64_t id_ = 0;
	std::uint32_t max_io_size_ = 0;
};

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
	Session open_process_session(std::int32_t pid) const;
	void close_session(std::uint64_t session_id) const;
	std::size_t read_memory(std::uint64_t session_id,
				std::uint64_t remote_address, void *buffer,
				std::size_t size) const;
	std::size_t write_memory(std::uint64_t session_id,
				 std::uint64_t remote_address, const void *buffer,
				 std::size_t size) const;
	void hide_module() const;

	const ResolvedEndpoint &endpoint() const noexcept;

private:
	int fd_ = -1;
	ResolvedEndpoint endpoint_{EndpointKind::Device, {}, {}};
};

} // namespace kfi

#endif
