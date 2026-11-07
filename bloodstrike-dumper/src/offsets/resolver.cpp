#include "pch.hpp"
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	void Resolver::record(ResolveContext& context, std::string_view offset, std::uint64_t value) const {
		record(context, offset, value, name());
	}

	void Resolver::record(ResolveContext& context, std::string_view offset, std::uint64_t value, std::string_view source) const {
		context.offsets.record(offset, value, source);
	}

	void Resolver::fail(ResolveContext& context, std::string_view reason) const {
		context.offsets.add_error(std::format("{}: {}", name(), reason));
	}

	void Resolver::warn(ResolveContext& context, std::string_view reason) const {
		context.offsets.add_warning(std::format("{}: {}", name(), reason));
	}

	std::optional<Rva> Resolver::find_unique(ResolveContext& context, const core::Pattern& pattern) const {
		const auto matches = context.image.find(pattern);
		if (matches.size() == 1) return matches.front();

		fail(context, matches.empty() ? std::string("signature not found")
									  : std::format("signature matched {} times", matches.size()));
		return std::nullopt;
	}

	std::optional<std::int64_t> Resolver::origin_shift(ResolveContext& context,
		std::span<const analysis::Instruction> code, analysis::Register base) const {
		constexpr std::int64_t kUnset = 0x7FFFFFFF;

		std::vector<std::int64_t> displacements;
		for (const auto& instruction : code)
			if (instruction.is(ZYDIS_MNEMONIC_CMP) && instruction.accesses(base) && instruction.immediate(1) == kUnset)
				displacements.push_back(*instruction.displacement());
		if (displacements.empty()) return std::nullopt;

		std::ranges::sort(displacements);
		const auto [first, last] = std::ranges::unique(displacements);
		displacements.erase(first, last);

		const bool contiguous = displacements.size() == 3 &&
			displacements[1] == displacements[0] + 4 && displacements[2] == displacements[0] + 8;
		if (!contiguous) warn(context, "origin shift checks are not three contiguous ints");

		return displacements.front();
	}

} // namespace dumper::offsets
