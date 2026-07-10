// SPDX-License-Identifier: MIT
#include "kfi/client.hpp"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
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
	std::cerr << "usage: " << program << " <version|caps|runtime|attach PID>\n";
}

} // namespace

int main(int argc, char **argv)
{
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	try {
		kfi::Client client;
		const std::string_view command(argv[1]);

		if (command == "version" && argc == 2) {
			const auto version = client.version();
			std::cout << "KFI ABI " << version.major << '.' << version.minor
				  << '.' << version.patch << " client=" << version.client_id
				  << '\n';
			return 0;
		}

		if (command == "caps" && argc == 2) {
			const auto caps = client.capabilities();
			std::cout << "flags=0x" << std::hex << caps.flags << std::dec
				  << " max_sessions=" << caps.max_sessions
				  << " max_io_size=" << caps.max_io_size << '\n';
			return 0;
		}

		if (command == "runtime" && argc == 2) {
			const auto info = client.runtime_info();
			std::cout << "release=" << info.release
				  << " machine=" << info.machine
				  << " page_size=" << info.page_size
				  << " page_shift=" << info.page_shift
				  << " build_flags=0x" << std::hex << info.build_flags
				  << std::dec << " profile=" << info.profile << '\n';
			return 0;
		}

		if (command == "attach" && argc == 3) {
			const auto session = client.open_process(parse_pid(argv[2]));
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
