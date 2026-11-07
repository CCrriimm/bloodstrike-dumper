#pragma once
#include "offsets/offset_table.hpp"

namespace dumper::output {

	class ConsoleReport {
	public:
		explicit ConsoleReport(const offsets::OffsetTable& offsets) noexcept : offsets_(offsets) {}

		void print() const;

	private:
		void print_offset(const offsets::OffsetDefinition& definition) const;

		const offsets::OffsetTable& offsets_;
	};

} // namespace dumper::output
