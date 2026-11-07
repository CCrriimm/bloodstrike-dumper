#include "pch.hpp"
#include "offsets/offset_table.hpp"

namespace dumper::offsets {

	void OffsetTable::record(std::string_view name, std::uint64_t value, std::string_view source) {
		if (std::ranges::find(kOffsetDefinitions, name, &OffsetDefinition::name) == kOffsetDefinitions.end())
			throw std::logic_error(std::format("resolver '{}' recorded unknown offset '{}'", source, name));

		auto it = hits_.find(name);
		if (it == hits_.end()) it = hits_.emplace(std::string(name), std::vector<OffsetHit>{}).first;
		it->second.push_back(OffsetHit{ .value = value, .source = std::string(source) });
	}

	std::span<const OffsetHit> OffsetTable::hits(std::string_view name) const {
		const auto it = hits_.find(name);
		if (it == hits_.end()) return {};
		return it->second;
	}

	bool OffsetTable::has_conflict(std::string_view name) const {
		const auto found = hits(name);
		return !found.empty() &&
			!std::ranges::all_of(found, [&](const OffsetHit& hit) { return hit.value == found.front().value; });
	}

	std::optional<std::uint64_t> OffsetTable::value(std::string_view name) const {
		const auto found = hits(name);
		if (found.empty() || has_conflict(name)) return std::nullopt;
		return found.front().value;
	}

	std::size_t OffsetTable::resolved_count() const {
		return static_cast<std::size_t>(std::ranges::count_if(kOffsetDefinitions,
			[&](const OffsetDefinition& definition) { return value(definition.name).has_value(); }));
	}

	std::vector<std::string_view> OffsetTable::unresolved() const {
		std::vector<std::string_view> names;
		for (const auto& definition : kOffsetDefinitions)
			if (!value(definition.name)) names.push_back(definition.name);
		return names;
	}

} // namespace dumper::offsets
