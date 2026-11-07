#include "pch.hpp"
#include "output/console.hpp"

namespace dumper::output {

	void Console::initialize() noexcept {
		SetConsoleOutputCP(CP_UTF8);
		const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD mode = 0;
		if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}

	void Console::write(Color color, std::string_view prefix, std::string_view text) {
		std::cout << escape(color) << prefix << text << escape(Color::Default) << '\n';
	}

	void Console::wait_for_enter() {
		std::cout << "\npress enter to exit...";
		std::cin.get();
	}

	std::string_view Console::escape(Color color) noexcept {
		switch (color) {
		case Color::Red:    return "\x1b[91m";
		case Color::Green:  return "\x1b[92m";
		case Color::Yellow: return "\x1b[93m";
		case Color::Cyan:   return "\x1b[96m";
		case Color::Gray:   return "\x1b[90m";
		default:            return "\x1b[0m";
		}
	}

} // namespace dumper::output
