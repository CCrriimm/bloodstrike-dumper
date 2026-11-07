#include "pch.hpp"
#include "app/application.hpp"

#include "app/live_check.hpp"
#include "core/strings.hpp"
#include "offsets/resolver_pipeline.hpp"
#include "output/console.hpp"
#include "output/console_report.hpp"
#include "output/offsets_header.hpp"

namespace dumper::app {

	using output::Console;

	ExitCode Application::run() {
		const auto started = std::chrono::steady_clock::now();

		const auto target = load_target();

		offsets::OffsetTable offsets;
		offsets::ResolverPipeline::standard().run(target.image, offsets);

		std::optional<std::string> live_check;
		if (target.process) {
			live_check = LiveCheck(*target.process, offsets).run();
			if (live_check) Console::info("live check: {}", *live_check);
		}

		output::ConsoleReport(offsets).print();
		write_results(target, offsets, std::move(live_check));

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started);
		Console::success("{}/{} offsets in {}", offsets.resolved_count(), offsets::kOffsetDefinitions.size(), elapsed);

		return offsets.complete() ? ExitCode::Success : ExitCode::Incomplete;
	}

	Application::Target Application::load_target() const {
		return options_.file ? load_from_file(*options_.file) : load_from_process();
	}

	Application::Target Application::load_from_file(const std::filesystem::path& path) const {
		Console::info("loading {}", path.string());
		return Target{ .name = path.string(), .image = core::Image::from_file(path), .process = std::nullopt };
	}

	Application::Target Application::load_from_process() const {
		auto process = core::Process::attach(options_.process_name);
		const auto& module = process.module();
		Console::info("{} pid {} base 0x{:X} size 0x{:X}", core::to_utf8(options_.process_name), module.pid, module.base, module.size);

		auto snapshot = process.snapshot();
		Console::info("snapshot: 0x{:X} / 0x{:X} bytes readable", snapshot.readable_bytes, module.size);

		return Target{
			.name = core::to_utf8(options_.process_name),
			.image = std::move(snapshot.image),
			.process = std::move(process),
		};
	}

	void Application::write_results(const Target& target, const offsets::OffsetTable& offsets, std::optional<std::string> live_check) const {
		output::DumpTarget info{
			.name = target.name,
			.pid = target.process ? std::optional(target.process->module().pid) : std::nullopt,
			.image_base = target.image.base(),
			.image_size = target.image.size(),
			.pe_timestamp = target.image.timestamp(),
		};

		const auto path = output_directory() / "offsets.hpp";
		output::OffsetsHeader(offsets, std::move(info), std::move(live_check)).save(path);
		Console::success("wrote {}", path.string());
	}

	std::filesystem::path Application::output_directory() {
		std::wstring buffer(MAX_PATH, L'\0');
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		buffer.resize(length);
		return std::filesystem::path(buffer).parent_path();
	}

} // namespace dumper::app
