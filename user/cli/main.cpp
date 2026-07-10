// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::int32_t parse_pid(std::string_view text)
{
	std::int32_t pid = 0;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), pid);
	if (result.ec != std::errc{} || result.ptr != text.data() + text.size() ||
	    pid <= 0)
		throw std::invalid_argument("invalid PID");
	return pid;
}

void usage(const char *program)
{
	std::cerr
		<< "usage: " << program
		<< " [--endpoint auto|dev:/path|proc:/proc/path] "
		   "<endpoint-info|version|caps|runtime|attach PID>\n";
}

} // namespace

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	try {
		kfi::Endpoint endpoint = kfi::Endpoint::Auto();
		int command_index = 1;
		if (std::string_view(argv[command_index]) == "--endpoint") {
			if (argc < 4) {
				usage(argv[0]);
				return 2;
			}
			endpoint = kfi::Endpoint::Parse(argv[command_index + 1]);
			command_index += 2;
		}

		const std::string_view command(argv[command_index]);
		const int arguments = argc - command_index - 1;
		if (command == "endpoint-info" && arguments == 0) {
			const auto resolved = endpoint.resolve();
			std::cout << "kind=" << kfi::endpoint_kind_name(resolved.kind)
				  << " path=" << resolved.path
				  << " source=" << resolved.source << '\n';
			return 0;
		}

		kfi::Client client(endpoint);

		if (command == "version" && arguments == 0) {
			const auto version = client.version();
			std::cout << "KFI ABI " << version.major << '.' << version.minor
				  << '.' << version.patch << " client=" << version.client_id
				  << '\n';
			return 0;
		}

		if (command == "caps" && arguments == 0) {
			const auto caps = client.capabilities();
			std::cout << "flags=0x" << std::hex << caps.flags << std::dec
				  << " max_sessions=" << caps.max_sessions
				  << " max_io_size=" << caps.max_io_size << '\n';
			return 0;
		}

		if (command == "runtime" && arguments == 0) {
			const auto info = client.runtime_info();
			std::cout << "release=" << info.release
				  << " machine=" << info.machine
				  << " page_size=" << info.page_size
				  << " page_shift=" << info.page_shift
				  << " build_flags=0x" << std::hex << info.build_flags
				  << std::dec << " profile=" << info.profile << '\n';
			return 0;
		}

		if (command == "attach" && arguments == 1) {
			const auto session = client.open_process(
				parse_pid(argv[command_index + 1]));
			std::cout << "session=" << session << '\n';
			client.close_session(session);
			return 0;
		}

		usage(argv[0]);
		return 2;
	} catch (const std::exception &error) {
		std::cerr << "kfi: " << error.what() << '\n';
		return 1;
	}
}
