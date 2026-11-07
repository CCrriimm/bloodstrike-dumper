#pragma once
#include "app/options.hpp"
#include "core/image.hpp"
#include "core/process.hpp"
#include "offsets/offset_table.hpp"

namespace dumper::app {

	enum class ExitCode : int {
		Success = 0,    // every offset resolved
		Failure = 1,    // couldn't load the target
		Incomplete = 2, // ran, but some offsets are missing or conflicting
	};

	class Application {
	public:
		explicit Application(Options options) noexcept : options_(std::move(options)) {}

		[[nodiscard]] ExitCode run();

	private:
		struct Target {
			std::string name;
			core::Image image;
			std::optional<core::Process> process;
		};

		[[nodiscard]] Target load_target() const;
		[[nodiscard]] Target load_from_file(const std::filesystem::path& path) const;
		[[nodiscard]] Target load_from_process() const;
		void write_results(const Target& target, const offsets::OffsetTable& offsets, std::optional<std::string> live_check) const;

		[[nodiscard]] static std::filesystem::path output_directory();

		Options options_;
	};

} // namespace dumper::app
