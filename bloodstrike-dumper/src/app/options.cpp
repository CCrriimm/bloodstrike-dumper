#include "pch.hpp"
#include "app/options.hpp"

#include "core/strings.hpp"

namespace dumper::app {

	Options Options::parse(std::span<const wchar_t* const> arguments) {
		Options options;

		for (auto it = arguments.begin(); it != arguments.end(); ++it) {
			const std::wstring_view argument = *it;
			auto next_value = [&]() -> std::wstring_view {
				if (std::next(it) == arguments.end())
					throw std::invalid_argument(std::format("{} needs a value", core::to_utf8(argument)));
				return *++it;
			};

			if (argument == L"--process") options.process_name = next_value();
			else if (argument == L"--file") options.file = std::filesystem::path(next_value());
			else if (argument == L"--no-pause") options.pause = false;
			else if (argument == L"--help" || argument == L"-h") options.show_help = true;
			else throw std::invalid_argument(std::format("unknown argument {}", core::to_utf8(argument)));
		}
		return options;
	}

	std::string_view Options::usage() noexcept {
		return
			"usage: bloodstrike-dumper [options]\n"
			"\n"
			"  (no options)        attach to BloodStrike.exe\n"
			"  --process <name>    attach to a different process name\n"
			"  --file <path>       analyse a PE file on disk instead of a running process\n"
			"  --no-pause          don't wait for enter before exiting\n"
			"  --help              show this text\n";
	}

} // namespace dumper::app
