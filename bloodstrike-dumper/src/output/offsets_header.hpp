#pragma once
#include "offsets/offset_table.hpp"

namespace dumper::output {

	struct DumpTarget {
		std::string name;
		std::optional<DWORD> pid;
		std::uint64_t image_base = 0;
		std::size_t image_size = 0;
		std::uint32_t pe_timestamp = 0;
	};

	class OffsetsHeader {
	public:
		OffsetsHeader(const offsets::OffsetTable& offsets, DumpTarget target, std::optional<std::string> live_check);

		[[nodiscard]] std::string render() const;
		void save(const std::filesystem::path& path) const;

	private:
		[[nodiscard]] std::string render_banner() const;
		[[nodiscard]] std::string render_offset(const offsets::OffsetDefinition& definition) const;

		const offsets::OffsetTable& offsets_;
		DumpTarget target_;
		std::optional<std::string> live_check_;
	};

} // namespace dumper::output
