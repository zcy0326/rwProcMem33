// SPDX-License-Identifier: MIT
#include "kfi/endpoint.hpp"

#include <cstdlib>
#include <stdexcept>
#include <utility>

#include <linux/kfi.h>

namespace kfi {
namespace {

void validate_absolute_path(const std::string &path, const char *kind)
{
	if (path.empty() || path.front() != '/')
		throw std::invalid_argument(std::string(kind) +
					    " endpoint path must be absolute");
}

} // namespace

Endpoint::Endpoint(EndpointKind kind, std::string path)
	: kind_(kind), path_(std::move(path))
{
}

Endpoint Endpoint::Auto()
{
	return Endpoint(EndpointKind::Auto, {});
}

Endpoint Endpoint::Device(std::string path)
{
	validate_absolute_path(path, "device");
	return Endpoint(EndpointKind::Device, std::move(path));
}

Endpoint Endpoint::Proc(std::string path)
{
	validate_absolute_path(path, "proc");
	if (path.rfind("/proc/", 0) != 0)
		throw std::invalid_argument("proc endpoint must be below /proc");
	return Endpoint(EndpointKind::Proc, std::move(path));
}

Endpoint Endpoint::Parse(std::string_view specification)
{
	if (specification == "auto")
		return Auto();
	if (specification.rfind("dev:", 0) == 0)
		return Device(std::string(specification.substr(4)));
	if (specification.rfind("proc:", 0) == 0)
		return Proc(std::string(specification.substr(5)));
	throw std::invalid_argument(
		"endpoint must be auto, dev:/path, or proc:/proc/path");
}

EndpointKind Endpoint::kind() const noexcept
{
	return kind_;
}

const std::string &Endpoint::path() const noexcept
{
	return path_;
}

ResolvedEndpoint Endpoint::resolve() const
{
	if (kind_ == EndpointKind::Device)
		return {kind_, path_, "explicit"};
	if (kind_ == EndpointKind::Proc)
		return {kind_, path_, "explicit"};

	const char *environment = std::getenv("KFI_ENDPOINT");
	if (environment && *environment) {
		Endpoint configured = Parse(environment);
		if (configured.kind() == EndpointKind::Auto)
			throw std::invalid_argument(
				"KFI_ENDPOINT must resolve to dev: or proc:");
		ResolvedEndpoint resolved = configured.resolve();
		resolved.source = "KFI_ENDPOINT";
		return resolved;
	}

	return {EndpointKind::Device, KFI_DEVICE_PATH, "default"};
}

const char *endpoint_kind_name(EndpointKind kind) noexcept
{
	switch (kind) {
	case EndpointKind::Auto:
		return "auto";
	case EndpointKind::Device:
		return "device";
	case EndpointKind::Proc:
		return "proc";
	}
	return "unknown";
}

} // namespace kfi
