#include "pch.hpp"
#include "output/console_report.hpp"

#include "output/console.hpp"

namespace dumper::output {

	void ConsoleReport::print() const {
		std::string_view group;
		for (const auto& definition : offsets::kOffsetDefinitions) {
			if (definition.group != group) {
				group = definition.group;
				Console::write(Color::Gray, "", std::format("\n  [{}]", group));
			}
			print_offset(definition);
		}

		std::cout << '\n';
		for (const auto& warning : offsets_.warnings()) Console::warning("{}", warning);
		for (const auto& error : offsets_.errors()) Console::error("{}", error);
	}

	void ConsoleReport::print_offset(const offsets::OffsetDefinition& definition) const {
		const auto label = std::format("    {:<18} ", definition.name);
		if (const auto value = offsets_.value(definition.name))
			Console::write(Color::Green, label, std::format("0x{:X}", *value));
		else if (offsets_.has_conflict(definition.name))
			Console::write(Color::Yellow, label, "conflict");
		else
			Console::write(Color::Red, label, "not found");
	}

} // namespace dumper::output
