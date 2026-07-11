// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"

#include <cerrno>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace kfi {
namespace {

void checked_ioctl(int fd, unsigned long request, void *argument,
		   const char *operation)
{
	if (::ioctl(fd, request, argument) == -1)
		throw std::system_error(errno, std::generic_category(), operation);
}

std::size_t transfer_memory(int fd, std::uint64_t session_id,
				std::uint64_t remote_address, void *buffer,
				std::size_t size, bool write)
{
	if (!buffer || size == 0 || size > std::numeric_limits<std::uint32_t>::max())
		throw std::invalid_argument("invalid memory transfer buffer or size");

	kfi_memory_io request{};
	request.header.struct_size = sizeof(request);
	request.session_id = session_id;
	request.remote_address = remote_address;
	request.user_buffer = reinterpret_cast<std::uintptr_t>(buffer);
	request.requested_size = static_cast<std::uint32_t>(size);
	checked_ioctl(fd, write ? KFI_IOC_WRITE_MEMORY : KFI_IOC_READ_MEMORY,
			      &request, write ? "KFI_IOC_WRITE_MEMORY" : "KFI_IOC_READ_MEMORY");
	return request.completed_size;
}

template <typename Entry>
std::vector<Entry> enumerate(int fd, std::uint64_t session_id,
                             unsigned long command, const char *operation)
{
	constexpr std::uint32_t page_capacity = 128;
	std::vector<Entry> result;
	std::uint64_t cursor = 0;
	for (;;) {
		std::vector<Entry> page(page_capacity);
		kfi_enumerate request{};
		request.header.struct_size = sizeof(request);
		request.session_id = session_id;
		request.user_buffer = reinterpret_cast<std::uintptr_t>(page.data());
		request.cursor = cursor;
		request.capacity = page_capacity;
		checked_ioctl(fd, command, &request, operation);
		if (request.returned > page_capacity)
			throw std::runtime_error("kernel returned an invalid enumeration count");
		result.insert(result.end(), page.begin(), page.begin() + request.returned);
		if (request.result_flags & KFI_ENUM_RESULT_END)
			break;
		if (!request.returned || request.next_cursor == cursor)
			throw std::runtime_error("kernel enumeration made no progress");
		cursor = request.next_cursor;
	}
	return result;
}

} // namespace

Session::Session(int fd, std::uint64_t id) : fd_(fd), id_(id)
{
}

Session::~Session()
{
	close();
}

Session::Session(Session &&other) noexcept
	: fd_(std::exchange(other.fd_, -1)), id_(std::exchange(other.id_, 0))
{
}

Session &Session::operator=(Session &&other) noexcept
{
	if (this == &other)
		return *this;
	close();
	fd_ = std::exchange(other.fd_, -1);
	id_ = std::exchange(other.id_, 0);
	return *this;
}

void Session::close() noexcept
{
	if (fd_ == -1)
		return;
	kfi_close_session request{};
	request.header.struct_size = sizeof(request);
	request.session_id = id_;
	(void)::ioctl(fd_, KFI_IOC_CLOSE_SESSION, &request);
	::close(fd_);
	fd_ = -1;
	id_ = 0;
}

std::uint64_t Session::id() const noexcept
{
	return id_;
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

std::size_t Session::transfer(std::uint64_t remote_address, void *buffer,
				      std::size_t size, bool write) const
{
	if (fd_ == -1)
		throw std::system_error(EBADF, std::generic_category(), "closed session");
	return transfer_memory(fd_, id_, remote_address, buffer, size, write);
}

std::vector<kfi_thread_entry> Session::threads() const
{
	if (fd_ == -1)
		throw std::system_error(EBADF, std::generic_category(), "closed session");
	return enumerate<kfi_thread_entry>(fd_, id_, KFI_IOC_ENUM_THREADS,
					   "KFI_IOC_ENUM_THREADS");
}

std::vector<kfi_map_entry> Session::maps() const
{
	if (fd_ == -1)
		throw std::system_error(EBADF, std::generic_category(), "closed session");
	return enumerate<kfi_map_entry>(fd_, id_, KFI_IOC_ENUM_MAPS,
					"KFI_IOC_ENUM_MAPS");
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
	fd_ = ::open(endpoint_.path.c_str(), O_RDWR | O_CLOEXEC);
	if (fd_ == -1)
		throw std::system_error(errno, std::generic_category(),
					"open " + endpoint_.path);
}

Client::~Client()
{
	if (fd_ != -1)
		::close(fd_);
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
		::close(fd_);
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
	const int duplicated_fd = ::fcntl(fd_, F_DUPFD_CLOEXEC, 0);
	if (duplicated_fd == -1) {
		const int duplicate_error = errno;
		try {
			close_session(id);
		} catch (...) {
		}
		throw std::system_error(duplicate_error, std::generic_category(),
					"duplicate KFI session descriptor");
	}
	return Session(duplicated_fd, id);
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
	return transfer_memory(fd_, session_id, remote_address, buffer, size, false);
}

std::size_t Client::write_memory(std::uint64_t session_id,
				 std::uint64_t remote_address, const void *buffer,
				 std::size_t size) const
{
	return transfer_memory(fd_, session_id, remote_address,
				       const_cast<void *>(buffer), size, true);
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
