// bloodstrike-dumper: finds the BloodStrike offsets and writes offsets.hpp next to the exe.
// See Options::usage() for the command line.
#include "pch.hpp"

#include "app/application.hpp"
#include "output/console.hpp"

int wmain(int argc, wchar_t** argv) {
	using namespace dumper;
	using output::Console;

	Console::initialize();

	bool pause = true;
	auto exit_code = app::ExitCode::Failure;
	try {
		const auto options = app::Options::parse(std::span<const wchar_t* const>(argv + 1, argv + argc));
		pause = options.pause;

		if (options.show_help) {
			std::cout << app::Options::usage();
			return 0;
		}
		exit_code = app::Application(options).run();
	} catch (const std::exception& exception) {
		Console::error("{}", exception.what());
	}

	if (pause) Console::wait_for_enter();
	return static_cast<int>(exit_code);
}
