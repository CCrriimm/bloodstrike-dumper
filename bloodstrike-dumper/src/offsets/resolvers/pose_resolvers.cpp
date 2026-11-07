#include "pch.hpp"
#include "offsets/resolvers/pose_resolvers.hpp"

#include "core/strings.hpp"

namespace dumper::offsets {

	void PoseBonesResolver::resolve(ResolveContext& context) const {
		const auto matches = context.image.find(pattern_);
		if (matches.empty()) return fail(context, "signature not found");

		for (const auto match : matches) resolve_match(context, match);

		if (context.offsets.hits("PoseBoneSize").empty()) fail(context, "pose bone stride not found in any match");
	}

	void PoseBonesResolver::resolve_match(ResolveContext& context, Rva match) const {
		const auto source = std::format("{}@{}", name(), core::to_hex(match));
		const auto code = context.disassembler.sweep(match, 96);
		if (code.size() < 6) return;

		using analysis::Register;
		const auto actor = *code.front().register_operand(0);
		std::optional<Register> pose, iterator;
		std::optional<std::int64_t> begin;
		bool found_size = false, found_definition = false;

		for (std::size_t n = 1; n < code.size() && !(found_size && found_definition); ++n) {
			const auto& instruction = code[n];
			const auto* following = n + 1 < code.size() ? &code[n + 1] : nullptr;

			// mov pose, [actor+Pose]
			if (!pose) {
				if (instruction.is_pointer_load() && instruction.accesses(actor)) {
					record(context, "Pose", *instruction.displacement(), source);
					pose = instruction.register_operand(0);
				}
				continue;
			}

			// mov r, [pose+End] ; sub r, [pose+Begin]
			if (!begin) {
				if (instruction.is_pointer_load() && instruction.accesses(*pose) && following &&
					following->is(ZYDIS_MNEMONIC_SUB) && following->register_operand(0) == instruction.register_operand(0) &&
					following->accesses(*pose)) {
					record(context, "PoseBonesEnd", *instruction.displacement(), source);
					record(context, "PoseBones", *following->displacement(), source);
					begin = following->displacement();
				}
				continue;
			}

			// iterator = mov r, [pose+Begin]
			if (!iterator) {
				if (instruction.is_pointer_load() && instruction.accesses(*pose) && instruction.displacement() == begin)
					iterator = instruction.register_operand(0);
				continue;
			}

			// mov r, [iterator+BoneDef] ; add r, BoneDefName
			if (!found_definition && instruction.is_pointer_load() && instruction.accesses(*iterator) && following &&
				following->is(ZYDIS_MNEMONIC_ADD) && following->register_operand(0) == instruction.register_operand(0) &&
				following->immediate(1)) {
				record(context, "PoseBoneDef", *instruction.displacement(), source);
				record(context, "BoneDefName", *following->immediate(1), source);
				found_definition = true;
			}

			// add iterator, PoseBoneSize
			if (!found_size && instruction.is(ZYDIS_MNEMONIC_ADD) && instruction.register_operand(0) == iterator &&
				instruction.immediate(1)) {
				record(context, "PoseBoneSize", *instruction.immediate(1), source);
				found_size = true;
			}
		}

		if (!pose || !begin) warn(context, std::format("{}: pose layout not found", source));
	}

	void BoneMatrixResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 160);

		// the bone pointer is whatever the result of the lookup call is moved into
		const auto call = std::ranges::find_if(code, &analysis::Instruction::is_call);
		const auto move = std::ranges::find_if(call, code.end(), [](const analysis::Instruction& i) {
			return i.is(ZYDIS_MNEMONIC_MOV) && i.register_operand(1) == ZYDIS_REGISTER_RAX;
		});
		if (move == code.end()) return fail(context, "bone lookup result not found");
		const auto bone = *move->register_operand(0);

		std::optional<std::int64_t> lowest;
		for (const auto& instruction : std::ranges::subrange(std::next(move), code.end())) {
			if (instruction.accesses(bone)) lowest = std::min(lowest.value_or(INT64_MAX), *instruction.displacement());
			if (instruction.writes(bone)) break;
		}
		if (!lowest) return fail(context, "no reads through the bone pointer");
		record(context, "BoneMatrix", *lowest);
	}

	void SkeletonWorldResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, on_enter_world_);
		if (!anchor) return;

		// mov byte [rcx+InWorld], 1  (a second source for InWorld)
		const auto code = context.disassembler.sweep(*anchor, 8);
		const auto flag = std::ranges::find_if(code, [](const analysis::Instruction& i) {
			return i.is(ZYDIS_MNEMONIC_MOV) && i.accesses(ZYDIS_REGISTER_RCX) && i.memory()->size == 8 && i.immediate(1) == 1;
		});
		if (flag != code.end()) record(context, "InWorld", *flag->displacement());

		const auto* rdata = context.image.section(".rdata");
		if (!rdata) return fail(context, "no .rdata section");

		const auto slots = context.image.find_pointers(*anchor, *rdata);
		if (slots.empty()) return fail(context, "OnEnterWorld is not referenced from any table");

		for (const auto slot : slots) {
			// nearest neighbouring slot that holds a matrix setter
			for (std::size_t distance = 1; distance <= kSlotRadius; ++distance) {
				const auto found = std::ranges::any_of(std::array{ -1, 1 }, [&](int direction) {
					const auto entry = static_cast<Rva>(slot + direction * static_cast<std::int64_t>(distance * sizeof(std::uint64_t)));
					const auto pointer = context.image.read<std::uint64_t>(entry);
					const auto function = pointer ? context.image.to_rva(*pointer) : std::nullopt;
					if (!function || !context.image.code().contains(*function)) return false;

					const auto matrix = matrix_setter(context, *function);
					if (matrix) record(context, "SkeletonWorld", *matrix, std::format("{}@{}", name(), core::to_hex(entry)));
					return matrix.has_value();
				});
				if (found) break;
			}
		}

		if (context.offsets.hits("SkeletonWorld").empty()) fail(context, "no matrix setter next to OnEnterWorld");
	}

	std::optional<std::int64_t> SkeletonWorldResolver::matrix_setter(const ResolveContext& context, Rva function) {
		constexpr std::size_t kRows = 3;
		constexpr std::int64_t kRowSize = 0x10;

		const auto code = context.disassembler.sweep(function, kRows * 2 + 1);
		if (code.size() != kRows * 2 + 1 || !code.back().is_return()) return std::nullopt;

		std::optional<std::int64_t> first_row;
		for (std::size_t row = 0; row < kRows; ++row) {
			const auto& load = code[row * 2];
			const auto& store = code[row * 2 + 1];
			const auto row_offset = static_cast<std::int64_t>(row) * kRowSize;

			const bool copies_row =
				load.is(ZYDIS_MNEMONIC_MOVUPS) && load.accesses(ZYDIS_REGISTER_RDX) && load.displacement() == row_offset &&
				store.is(ZYDIS_MNEMONIC_MOVUPS) && store.accesses(ZYDIS_REGISTER_RCX) &&
				store.register_operand(1) == load.register_operand(0);
			if (!copies_row) return std::nullopt;

			if (!first_row) first_row = store.displacement();
			else if (store.displacement() != *first_row + row_offset) return std::nullopt;
		}
		return first_row;
	}

} // namespace dumper::offsets
