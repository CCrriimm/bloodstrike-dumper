#include "pch.hpp"
#include "offsets/resolvers/python_resolvers.hpp"

#include "core/strings.hpp"

namespace dumper::offsets {

	void PyTypeResolver::resolve(ResolveContext& context) const {
		const auto* rdata = context.image.section(".rdata");
		if (!rdata) return fail(context, "no .rdata section");

		// "\0IEntity\0": the leading NUL keeps us from matching the tail of a longer name
		std::vector<std::uint8_t> needle(type_name_.size() + 2, 0);
		std::ranges::copy(type_name_, needle.begin() + 1);

		const auto strings = context.image.find_bytes(needle, *rdata);
		if (strings.empty()) return fail(context, std::format("\"{}\" string not found", type_name_));

		for (const auto match : strings) {
			const Rva string = match + 1;
			for (const auto reference : context.image.find_rip_references(string)) {
				const auto type = type_from_registration(context, reference);
				if (!type) continue;

				record(context, offset_name_, *type, std::format("{}@{}", name(), core::to_hex(reference)));

				// once the game has started, tp_name points at the string
				const auto tp_name = context.image.read<std::uint64_t>(*type + kTpNameOffset);
				if (tp_name != context.image.base() + string)
					warn(context, std::format("tp_name of {} isn't set (expected when reading a file instead of the game)", core::to_hex(*type)));
			}
		}

		if (context.offsets.hits(offset_name_).empty())
			fail(context, std::format("no registration of \"{}\" found", type_name_));
	}

	std::optional<Rva> PyTypeResolver::type_from_registration(const ResolveContext& context, Rva reference) const {
		const auto code = context.disassembler.sweep(reference, 12, analysis::SweepEnd::Never);
		if (code.empty()) return std::nullopt;
		const auto name_register = code.front().register_operand(0);

		const auto call = std::ranges::find_if(code, &analysis::Instruction::is_call);
		if (call == code.end() || std::next(call) == code.end()) return std::nullopt;

		// mov [rax+18h], name_register
		const auto& store = *std::next(call);
		const auto destination = store.memory();
		const bool sets_tp_name = store.is(ZYDIS_MNEMONIC_MOV) && destination && !store.register_operand(0) &&
			destination->base == ZYDIS_REGISTER_RAX && destination->displacement == kTpNameOffset &&
			store.register_operand(1) == name_register;
		if (!sets_tp_name) return std::nullopt;

		const auto getter = call->branch_target();
		return getter ? type_from_getter(context, *getter) : std::nullopt;
	}

	std::optional<Rva> PyTypeResolver::type_from_getter(const ResolveContext& context, Rva getter) const {
		for (const auto& instruction : context.disassembler.sweep(getter, 24))
			if (instruction.is(ZYDIS_MNEMONIC_LEA) && instruction.register_operand(0) == ZYDIS_REGISTER_RAX)
				if (const auto target = instruction.rip_target()) return target;
		return std::nullopt;
	}

} // namespace dumper::offsets
