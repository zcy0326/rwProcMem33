// SPDX-License-Identifier: MIT
#ifndef KFI_ENDPOINT_HPP
#define KFI_ENDPOINT_HPP

#include <string>
#include <string_view>

namespace kfi {

enum class EndpointKind {
	Auto,
	Device,
	Proc,
};

struct ResolvedEndpoint {
	EndpointKind kind;
	std::string path;
	std::string source;
};

class Endpoint final {
public:
	static Endpoint Auto();
	static Endpoint Device(std::string path);
	static Endpoint Proc(std::string path);
	static Endpoint Parse(std::string_view specification);

	EndpointKind kind() const noexcept;
	const std::string &path() const noexcept;
	ResolvedEndpoint resolve() const;

private:
	Endpoint(EndpointKind kind, std::string path);

	EndpointKind kind_;
	std::string path_;
};

const char *endpoint_kind_name(EndpointKind kind) noexcept;

} // namespace kfi

#endif
