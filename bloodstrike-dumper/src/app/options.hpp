#pragma once

namespace dumper::app {

	struct Options {
		std::wstring process_name = L"BloodStrike.exe";
		std::optional<std::filesystem::path> file;
		bool pause = true;
		bool show_help = false;

		[[nodiscard]] static Options parse(std::span<const wchar_t* const> arguments);
		[[nodiscard]] static std::string_view usage() noexcept;
	};

} // namespace dumper::app
