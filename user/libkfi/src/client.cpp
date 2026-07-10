// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"

#include <cerrno>
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

} // namespace

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
	checked_ioctl(fd_, KFI_IOC_GET_VERSION, &result, "KFI_IOC_GET_VERSION");
	return result;
}

kfi_caps Client::capabilities() const
{
	kfi_caps result{};
	checked_ioctl(fd_, KFI_IOC_GET_CAPS, &result, "KFI_IOC_GET_CAPS");
	return result;
}

kfi_runtime_info Client::runtime_info() const
{
	kfi_runtime_info result{};
	checked_ioctl(fd_, KFI_IOC_GET_RUNTIME_INFO, &result,
		      "KFI_IOC_GET_RUNTIME_INFO");
	return result;
}

std::uint64_t Client::open_process(std::int32_t pid) const
{
	kfi_open_process request{};
	request.pid = pid;
	checked_ioctl(fd_, KFI_IOC_OPEN_PROCESS, &request,
		      "KFI_IOC_OPEN_PROCESS");
	return request.session_id;
}

void Client::close_session(std::uint64_t session_id) const
{
	kfi_close_session request{};
	request.session_id = session_id;
	checked_ioctl(fd_, KFI_IOC_CLOSE_SESSION, &request,
		      "KFI_IOC_CLOSE_SESSION");
}

const ResolvedEndpoint &Client::endpoint() const noexcept
{
	return endpoint_;
}

} // namespace kfi
