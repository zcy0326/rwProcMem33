// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"
#include "kfi/detail/enumerate.hpp"
#include "kfi/detail/syscalls.hpp"

#include <algorithm>
#include <cerrno>
#include <limits>
#include <stdexcept>
#include <utility>

#include <fcntl.h>

namespace kfi {
namespace {

void checked_ioctl(int fd, unsigned long request, void *argument,
		   const char *operation)
{
	if (detail::syscalls().ioctl(fd, request, argument) == -1)
		throw std::system_error(errno, std::generic_category(), operation);
}

std::size_t transfer_memory(int fd, std::uint64_t session_id,
			    std::uint64_t remote_address, void *buffer,
			    std::size_t size, std::uint32_t max_io_size,
			    bool write)
{
	if (!session_id || !buffer || size == 0 || max_io_size == 0)
		throw std::invalid_argument("invalid memory transfer arguments");
	if (size > std::numeric_limits<std::uint64_t>::max() - remote_address)
		throw std::overflow_error("remote memory range overflows");

	auto *bytes = static_cast<unsigned char *>(buffer);
	std::size_t completed = 0;
	const char *operation = write ? "KFI_IOC_WRITE_MEMORY" :
				       "KFI_IOC_READ_MEMORY";

	while (completed < size) {
		const std::size_t chunk = std::min<std::size_t>(
			size - completed, max_io_size);
		kfi_memory_io request{};
		request.header.struct_size = sizeof(request);
		request.session_id = session_id;
		request.remote_address = remote_address + completed;
		request.user_buffer = reinterpret_cast<std::uintptr_t>(bytes + completed);
		request.requested_size = static_cast<std::uint32_t>(chunk);

		const int result = detail::syscalls().ioctl(
			fd, write ? KFI_IOC_WRITE_MEMORY : KFI_IOC_READ_MEMORY,
			&request);
		const int saved_errno = errno;
		const std::size_t chunk_completed =
			std::min<std::size_t>(request.completed_size, chunk);
		completed += chunk_completed;

		if (result == -1)
			throw MemoryTransferError(
				std::error_code(saved_errno, std::generic_category()),
				completed, operation);
		if (chunk_completed == 0)
			throw MemoryTransferError(
				std::error_code(EIO, std::generic_category()),
				completed, operation);
		if (chunk_completed < chunk)
			return completed;
	}

	return completed;
}

} // namespace

MemoryTransferError::MemoryTransferError(std::error_code error,
					 std::size_t completed,
					 const char *operation)
	: std::system_error(error, operation), completed_(completed)
{
}

std::size_t MemoryTransferError::completed() const noexcept
{
	return completed_;
}

Session::Session(int fd, std::uint64_t id, std::uint32_t max_io_size)
	: fd_(fd), id_(id), max_io_size_(max_io_size)
{
}

Session::~Session()
{
	close_noexcept();
}

Session::Session(Session &&other) noexcept
	: fd_(std::exchange(other.fd_, -1)),
	  id_(std::exchange(other.id_, 0)),
	  max_io_size_(std::exchange(other.max_io_size_, 0))
{
}

Session &Session::operator=(Session &&other) noexcept
{
	if (this == &other)
		return *this;
	close_noexcept();
	fd_ = std::exchange(other.fd_, -1);
	id_ = std::exchange(other.id_, 0);
	max_io_size_ = std::exchange(other.max_io_size_, 0);
	return *this;
}

std::uint64_t Session::id() const noexcept
{
	return id_;
}

bool Session::valid() const noexcept
{
	return fd_ != -1 && id_ != 0;
}

void Session::close()
{
	if (!valid())
		return;

	kfi_close_session request{};
	request.header.struct_size = sizeof(request);
	request.session_id = id_;
	if (detail::syscalls().ioctl(fd_, KFI_IOC_CLOSE_SESSION, &request) == -1 &&
	    errno != ENOENT)
		throw std::system_error(errno, std::generic_category(),
					"KFI_IOC_CLOSE_SESSION");

	(void)detail::syscalls().close(fd_);
	fd_ = -1;
	id_ = 0;
	max_io_size_ = 0;
}

void Session::close_noexcept() noexcept
{
	if (!valid())
		return;

	kfi_close_session request{};
	request.header.struct_size = sizeof(request);
	request.session_id = id_;
	(void)detail::syscalls().ioctl(fd_, KFI_IOC_CLOSE_SESSION, &request);
	(void)detail::syscalls().close(fd_);
	fd_ = -1;
	id_ = 0;
	max_io_size_ = 0;
}

std::size_t Session::read(std::uint64_t remote_address, void *buffer,
			  std::size_t size) const
{
	return transfer(remote_address, buffer, size, false);
}

std::size_t Session::write(std::uint64_t remote_address, const void *buffer,
			   std::size_t size) const
{
	return transfer(remote_address, const_cast<void *>(buffer), size, true);
}

std::vector<kfi_thread_entry> Session::threads() const
{
	if (!valid())
		throw std::system_error(EBADF, std::generic_category(),
					"closed session");
	return detail::enumerate_pages<kfi_thread_entry>(
		id_, KFI_IOC_ENUM_THREADS, "KFI_IOC_ENUM_THREADS",
		[this](unsigned long command, void *argument,
		       const char *operation) {
			if (detail::syscalls().ioctl(fd_, command, argument) == -1)
				throw std::system_error(
					errno, std::generic_category(), operation);
		});
}

std::vector<kfi_map_entry> Session::maps() const
{
	if (!valid())
		throw std::system_error(EBADF, std::generic_category(),
					"closed session");
	return detail::enumerate_pages<kfi_map_entry>(
		id_, KFI_IOC_ENUM_MAPS, "KFI_IOC_ENUM_MAPS",
		[this](unsigned long command, void *argument,
		       const char *operation) {
			if (detail::syscalls().ioctl(fd_, command, argument) == -1)
				throw std::system_error(
					errno, std::generic_category(), operation);
		});
}

std::size_t Session::transfer(std::uint64_t remote_address, void *buffer,
			      std::size_t size, bool write) const
{
	if (!valid())
		throw std::system_error(EBADF, std::generic_category(),
					"closed session");
	return transfer_memory(fd_, id_, remote_address, buffer, size,
			       max_io_size_, write);
}

Client::Client() : Client(Endpoint::Auto())
{
}

Client::Client(const std::string &device_path)
	: Client(Endpoint::Device(device_path))
{
}

Client::Client(const Endpoint &endpoint) : endpoint_(endpoint.resolve())
{
	fd_ = detail::syscalls().open(endpoint_.path.c_str(), O_RDWR | O_CLOEXEC);
	if (fd_ == -1)
		throw std::system_error(errno, std::generic_category(),
					"open " + endpoint_.path);
}

Client::~Client()
{
	if (fd_ != -1)
		(void)detail::syscalls().close(fd_);
}

Client::Client(Client &&other) noexcept
	: fd_(std::exchange(other.fd_, -1)),
	  endpoint_(std::move(other.endpoint_))
{
}

Client &Client::operator=(Client &&other) noexcept
{
	if (this == &other)
		return *this;
	if (fd_ != -1)
		(void)detail::syscalls().close(fd_);
	fd_ = std::exchange(other.fd_, -1);
	endpoint_ = std::move(other.endpoint_);
	return *this;
}

kfi_version Client::version() const
{
	kfi_version result{};
	result.header.struct_size = sizeof(result);
	checked_ioctl(fd_, KFI_IOC_GET_VERSION, &result, "KFI_IOC_GET_VERSION");
	return result;
}

kfi_caps Client::capabilities() const
{
	kfi_caps result{};
	result.header.struct_size = sizeof(result);
	checked_ioctl(fd_, KFI_IOC_GET_CAPS, &result, "KFI_IOC_GET_CAPS");
	return result;
}

kfi_runtime_info Client::runtime_info() const
{
	kfi_runtime_info result{};
	result.header.struct_size = sizeof(result);
	checked_ioctl(fd_, KFI_IOC_GET_RUNTIME_INFO, &result,
		      "KFI_IOC_GET_RUNTIME_INFO");
	return result;
}

std::uint64_t Client::open_process(std::int32_t pid) const
{
	kfi_open_process request{};
	request.header.struct_size = sizeof(request);
	request.pid = pid;
	checked_ioctl(fd_, KFI_IOC_OPEN_PROCESS, &request,
		      "KFI_IOC_OPEN_PROCESS");
	return request.session_id;
}

Session Client::open_process_session(std::int32_t pid) const
{
	const auto id = open_process(pid);
	try {
		const auto caps = capabilities();
		if (!caps.max_io_size)
			throw std::runtime_error("kernel reported zero max_io_size");

		const int duplicated_fd = detail::syscalls().dup_cloexec(fd_);
		if (duplicated_fd == -1) {
			const int saved_errno = errno;
			throw std::system_error(saved_errno, std::generic_category(),
						"duplicate KFI session descriptor");
		}
		return Session(duplicated_fd, id, caps.max_io_size);
	} catch (...) {
		try {
			close_session(id);
		} catch (...) {
		}
		throw;
	}
}

void Client::close_session(std::uint64_t session_id) const
{
	kfi_close_session request{};
	request.header.struct_size = sizeof(request);
	request.session_id = session_id;
	checked_ioctl(fd_, KFI_IOC_CLOSE_SESSION, &request,
		      "KFI_IOC_CLOSE_SESSION");
}

std::size_t Client::read_memory(std::uint64_t session_id,
				std::uint64_t remote_address, void *buffer,
				std::size_t size) const
{
	const auto caps = capabilities();
	return transfer_memory(fd_, session_id, remote_address, buffer, size,
			       caps.max_io_size, false);
}

std::size_t Client::write_memory(std::uint64_t session_id,
				 std::uint64_t remote_address, const void *buffer,
				 std::size_t size) const
{
	const auto caps = capabilities();
	return transfer_memory(fd_, session_id, remote_address,
			       const_cast<void *>(buffer), size,
			       caps.max_io_size, true);
}

void Client::hide_module() const
{
	kfi_visibility_control request{};
	request.header.struct_size = sizeof(request);
	request.operations = KFI_VISIBILITY_FLAG_HIDE_MODULE;
	checked_ioctl(fd_, KFI_IOC_HIDE_MODULE, &request, "KFI_IOC_HIDE_MODULE");
}

const ResolvedEndpoint &Client::endpoint() const noexcept
{
	return endpoint_;
}

} // namespace kfi
