// SPDX-License-Identifier: MIT
#include "kfi/endpoint.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

template <typename Function>
void require_invalid(Function function, const char *message)
{
	try {
		function();
	} catch (const std::invalid_argument &) {
		return;
	}
	throw std::runtime_error(message);
}

} // namespace

int main()
{
	try {
		const auto device = kfi::Endpoint::Parse("dev:/dev/kfi-test").resolve();
		require(device.kind == kfi::EndpointKind::Device,
			"device kind mismatch");
		require(device.path == "/dev/kfi-test", "device path mismatch");

		const auto proc =
			kfi::Endpoint::Parse("proc:/proc/kfi_test/kfi_test").resolve();
		require(proc.kind == kfi::EndpointKind::Proc, "proc kind mismatch");
		require(proc.path == "/proc/kfi_test/kfi_test",
			"proc path mismatch");

		require_invalid([] { kfi::Endpoint::Parse("/dev/kfi"); },
				"untyped endpoint was accepted");
		require_invalid([] { kfi::Endpoint::Proc("/tmp/kfi"); },
				"non-proc path was accepted");
		require_invalid([] { kfi::Endpoint::Proc("/proc/../tmp/kfi"); },
				"parent traversal was accepted");
		require_invalid([] { kfi::Endpoint::Proc("/proc//kfi"); },
				"empty path component was accepted");
		require_invalid([] { kfi::Endpoint::Proc("/proc/kfi/"); },
				"trailing empty component was accepted");

		::setenv("KFI_ENDPOINT", "proc:/proc/from_env/from_env", 1);
		const auto automatic = kfi::Endpoint::Auto().resolve();
		require(automatic.kind == kfi::EndpointKind::Proc,
			"environment endpoint kind mismatch");
		require(automatic.source == "KFI_ENDPOINT",
			"environment endpoint source mismatch");
		::unsetenv("KFI_ENDPOINT");

		const auto fallback = kfi::Endpoint::Auto().resolve();
		require(fallback.kind == kfi::EndpointKind::Device,
			"automatic fallback kind mismatch");
		require(fallback.source == "default",
			"automatic fallback source mismatch");
	} catch (const std::exception &error) {
		std::cerr << "endpoint_test: " << error.what() << '\n';
		return 1;
	}

	return 0;
}
