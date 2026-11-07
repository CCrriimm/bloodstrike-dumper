#pragma once
#include "analysis/disassembler.hpp"
#include "core/image.hpp"
#include "core/pattern.hpp"
#include "offsets/offset_table.hpp"

namespace dumper::offsets {

	struct ResolveContext {
		const core::Image& image;
		const analysis::Disassembler& disassembler;
		OffsetTable& offsets;
	};

	// Resolvers locate a function and then read values out of the decoded instructions, never from fixed byte positions.
	class Resolver {
	public:
		virtual ~Resolver() = default;

		[[nodiscard]] virtual std::string_view name() const noexcept = 0;
		virtual void resolve(ResolveContext& context) const = 0;

	protected:
		void record(ResolveContext& context, std::string_view offset, std::uint64_t value) const;
		void record(ResolveContext& context, std::string_view offset, std::uint64_t value, std::string_view source) const;
		void fail(ResolveContext& context, std::string_view reason) const;
		void warn(ResolveContext& context, std::string_view reason) const;

		[[nodiscard]] std::optional<Rva> find_unique(ResolveContext& context, const core::Pattern& pattern) const;
		[[nodiscard]] std::optional<std::int64_t> origin_shift(ResolveContext& context, std::span<const analysis::Instruction> code, analysis::Register base) const;
	};

} // namespace dumper::offsets
