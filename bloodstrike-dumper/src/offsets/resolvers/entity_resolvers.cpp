#include "pch.hpp"
#include "offsets/resolvers/entity_resolvers.hpp"

namespace dumper::offsets {

	void EntityListResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto load = context.disassembler.decode(*anchor);
		if (!load || !load->rip_target()) return fail(context, "anchor is not a rip-relative load");

		record(context, "EntityList", *load->rip_target());
		record(context, "ManagerSentinel", kListHead, "std::unordered_map layout");
		record(context, "ActorInstance", kNodeValue, "std::unordered_map layout");
	}

	void ComponentEntityResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 40);
		if (code.empty() || !code.front().is_pointer_load()) return fail(context, "anchor is not a pointer load");

		record(context, "IEntity", *code.front().displacement());

		if (const auto shift = origin_shift(context, code, *code.front().register_operand(0))) record(context, "OriginShift", *shift);
		else fail(context, "no origin shift checks");
	}

	void ActorChainResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 16);
		if (code.size() < 8) return fail(context, "decode failed");

		const auto& props = code[1];
		const auto& component = code[2];
		const auto& entity = code[3];
		const bool chained = props.is_pointer_load() && component.is_pointer_load() && entity.is_pointer_load() &&
			component.accesses(*props.register_operand(0)) && entity.accesses(*component.register_operand(0));
		if (!chained) return fail(context, "load chain shape changed");

		record(context, "ActorProps", *props.displacement());
		record(context, "ActorComponent", *component.displacement());
		record(context, "IEntity", *entity.displacement());

		const auto entity_register = *entity.register_operand(0);
		const auto transform = std::ranges::find_if(code | std::views::drop(4), [&](const analysis::Instruction& i) {
			return i.is(ZYDIS_MNEMONIC_LEA) && i.accesses(entity_register);
		});
		if (transform == code.end()) return fail(context, "no transform lea");
		record(context, "Transform", *transform->displacement());
	}

	void InWorldResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 12);
		if (code.empty()) return fail(context, "decode failed");

		const auto actor = *code.front().register_operand(0);
		const auto check = std::ranges::find_if(code, [&](const analysis::Instruction& i) {
			return i.is(ZYDIS_MNEMONIC_CMP) && i.accesses(actor) && i.memory()->size == 8;
		});
		if (check == code.end()) return fail(context, "no in-world byte compare");
		record(context, "InWorld", *check->displacement());
	}

	void EntityAreaResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		// the current area, loaded from `this` and compared against the new one (2nd argument)
		const auto code = context.disassembler.sweep(*anchor, 16);
		const auto pair = std::ranges::adjacent_find(code, [](const analysis::Instruction& load, const analysis::Instruction& compare) {
			return load.is_pointer_load() && load.accesses(ZYDIS_REGISTER_RCX) &&
				compare.is(ZYDIS_MNEMONIC_CMP) && compare.register_operand(0) == load.register_operand(0) &&
				compare.register_operand(1) == ZYDIS_REGISTER_RDX;
		});
		if (pair == code.end()) return fail(context, "no load of the current area");
		record(context, "IArea", *pair->displacement());
	}

	void EntityMaskResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 64);
		if (code.empty() || !code.front().is_pointer_load()) return fail(context, "anchor is not a pointer load");

		const auto entity = *code.front().memory_base();
		const auto map_head = *code.front().displacement();

		// mov reg, max_size ; cmp [entity+size], reg
		const auto check = std::ranges::adjacent_find(code, [&](const analysis::Instruction& limit, const analysis::Instruction& compare) {
			const auto max_size = limit.immediate(1);
			return limit.is(ZYDIS_MNEMONIC_MOV) && max_size && *max_size > 0xFFFFFFFF &&
				compare.is(ZYDIS_MNEMONIC_CMP) && compare.accesses(entity) &&
				compare.register_operand(1) == limit.register_operand(0);
		});
		if (check == code.end()) return fail(context, "no map size check");

		const auto map_size = *std::next(check)->displacement();
		if (map_size != map_head + kMapSizeOffset)
			warn(context, std::format("map size (0x{:X}) does not follow its head (0x{:X})", map_size, map_head));
		record(context, "EntityMask", map_size);
	}

} // namespace dumper::offsets
