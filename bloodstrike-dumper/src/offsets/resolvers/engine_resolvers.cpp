#include "pch.hpp"
#include "offsets/resolvers/engine_resolvers.hpp"

namespace dumper::offsets {

	void LocalPositionResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		// the LocalPlayer path returns early and the Camera path follows that ret, so sweep to the padding
		const auto code = context.disassembler.sweep(*anchor, 48, analysis::SweepEnd::Padding);
		if (code.empty() || !code.front().is_rip_relative()) return fail(context, "anchor is not a rip-relative load");

		record(context, "ClientEngine", *code.front().rip_target());

		std::map<analysis::Register, Tracked> registers{ { *code.front().register_operand(0), { Kind::Engine } } };
		std::vector<std::int64_t> player_fields;          // ClientPlayer displacements, in load order
		std::map<std::int64_t, std::int64_t> field_floats; // ClientPlayer field -> first float load through it

		for (const auto& instruction : code | std::views::drop(1)) {
			const auto base = instruction.memory_base();
			const auto tracked = base ? registers.find(*base) : registers.end();

			if (instruction.is_pointer_load() && tracked != registers.end()) {
				const auto displacement = *instruction.displacement();
				const auto destination = *instruction.register_operand(0);
				switch (tracked->second.kind) {
				case Kind::Engine:
					record(context, "IGameplay", displacement);
					registers[destination] = { Kind::Gameplay };
					continue;
				case Kind::Gameplay:
					record(context, "ClientPlayer", displacement);
					registers[destination] = { Kind::Player };
					continue;
				case Kind::Player:
					if (std::ranges::find(player_fields, displacement) == player_fields.end()) player_fields.push_back(displacement);
					registers[destination] = { Kind::PlayerField, displacement };
					continue;
				case Kind::PlayerField:
					break;
				}
			}

			if (instruction.is_float_load() && tracked != registers.end() && tracked->second.kind == Kind::PlayerField)
				field_floats.try_emplace(tracked->second.field, *instruction.displacement());

			// anything else that overwrites a tracked register ends its tracking
			std::erase_if(registers, [&](const auto& entry) { return instruction.writes(entry.first); });
		}

		if (player_fields.size() != 2)
			return fail(context, std::format("expected 2 loads from ClientPlayer, found {}", player_fields.size()));

		const auto camera = context.offsets.value("Camera");
		if (!camera) return fail(context, "Camera is unknown, can't tell Camera and LocalPlayer apart");

		const auto camera_field = static_cast<std::int64_t>(*camera);
		if (std::ranges::find(player_fields, camera_field) == player_fields.end())
			return fail(context, std::format("neither ClientPlayer load matches Camera (0x{:X})", camera_field));

		const auto local_field = player_fields[0] == camera_field ? player_fields[1] : player_fields[0];
		record(context, "Camera", camera_field);
		record(context, "LocalPlayer", local_field);
		if (const auto it = field_floats.find(local_field); it != field_floats.end()) record(context, "EntityTranslation", it->second);
		if (const auto it = field_floats.find(camera_field); it != field_floats.end()) record(context, "CamPos", it->second);
	}

	void ClientEngineResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 6);
		for (std::size_t i = 0; i + 1 < code.size(); ++i) {
			const auto& global = code[i];
			const auto& load = code[i + 1];
			if (global.is_rip_relative() && load.is_pointer_load() && load.accesses(*global.register_operand(0))) {
				record(context, "ClientEngine", *global.rip_target());
				record(context, "IGameplay", *load.displacement());
				return;
			}
		}
		fail(context, "no ClientEngine load followed by a field load");
	}

	void ClientPlayerResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 6);
		if (code.size() < 5 || !code[1].is_pointer_load() || !code[4].is_pointer_load() ||
			!code[4].accesses(*code[1].register_operand(0)))
			return fail(context, "load chain shape changed");

		record(context, "IGameplay", *code[1].displacement());
		record(context, "ClientPlayer", *code[4].displacement());
	}

} // namespace dumper::offsets
