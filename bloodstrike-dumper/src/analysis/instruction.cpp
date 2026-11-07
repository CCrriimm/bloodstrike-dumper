#include "pch.hpp"
#include "analysis/instruction.hpp"

namespace dumper::analysis {

	Instruction::Instruction(Rva rva, const ZydisDecodedInstruction& info, std::span<const ZydisDecodedOperand> operands) noexcept
		: rva_(rva), info_(info) {
		std::ranges::copy(operands.first(std::min(operands.size(), operands_.size())), operands_.begin());
	}

	const ZydisDecodedOperand* Instruction::operand(std::size_t index) const noexcept {
		return index < info_.operand_count_visible ? &operands_[index] : nullptr;
	}

	std::optional<Register> Instruction::register_operand(std::size_t index) const noexcept {
		const auto* op = operand(index);
		if (!op || op->type != ZYDIS_OPERAND_TYPE_REGISTER) return std::nullopt;
		return widen(op->reg.value);
	}

	std::optional<std::int64_t> Instruction::immediate(std::size_t index) const noexcept {
		const auto* op = operand(index);
		if (!op || op->type != ZYDIS_OPERAND_TYPE_IMMEDIATE) return std::nullopt;
		return op->imm.is_signed ? op->imm.value.s : static_cast<std::int64_t>(op->imm.value.u);
	}

	std::optional<MemoryOperand> Instruction::memory() const noexcept {
		for (std::size_t i = 0; i < info_.operand_count_visible; ++i) {
			const auto& op = operands_[i];
			if (op.type != ZYDIS_OPERAND_TYPE_MEMORY) continue;
			return MemoryOperand{
				.base = widen(op.mem.base),
				.index = widen(op.mem.index),
				.displacement = op.mem.disp.has_displacement ? op.mem.disp.value : 0,
				.size = op.size,
			};
		}
		return std::nullopt;
	}

	std::optional<Register> Instruction::memory_base() const noexcept {
		const auto mem = memory();
		if (!mem || mem->index != ZYDIS_REGISTER_NONE) return std::nullopt;
		return mem->base;
	}

	std::optional<std::int64_t> Instruction::displacement() const noexcept {
		const auto mem = memory();
		return mem ? std::optional(mem->displacement) : std::nullopt;
	}

	bool Instruction::is_rip_relative() const noexcept {
		const auto mem = memory();
		return mem && mem->rip_relative();
	}

	std::optional<Rva> Instruction::rip_target() const noexcept {
		const auto mem = memory();
		if (!mem || !mem->rip_relative()) return std::nullopt;
		return static_cast<Rva>(static_cast<std::int64_t>(next()) + mem->displacement);
	}

	std::optional<Rva> Instruction::branch_target() const noexcept {
		if (!(info_.attributes & ZYDIS_ATTRIB_IS_RELATIVE)) return std::nullopt;
		const auto* op = operand(0);
		if (!op || op->type != ZYDIS_OPERAND_TYPE_IMMEDIATE) return std::nullopt;
		return static_cast<Rva>(static_cast<std::int64_t>(next()) + op->imm.value.s);
	}

	bool Instruction::is_pointer_load() const noexcept {
		const auto* destination = operand(0);
		const auto* source = operand(1);
		return is(ZYDIS_MNEMONIC_MOV) && destination && source &&
			destination->type == ZYDIS_OPERAND_TYPE_REGISTER && destination->size == 64 &&
			source->type == ZYDIS_OPERAND_TYPE_MEMORY && !is_rip_relative();
	}

	bool Instruction::is_float_load() const noexcept {
		switch (mnemonic()) {
		case ZYDIS_MNEMONIC_MOVSS:
		case ZYDIS_MNEMONIC_MOVSD:
		case ZYDIS_MNEMONIC_MOVUPS:
		case ZYDIS_MNEMONIC_MOVAPS:
		case ZYDIS_MNEMONIC_MOVQ:
		case ZYDIS_MNEMONIC_CVTSS2SD: {
			const auto* source = operand(1);
			return source && source->type == ZYDIS_OPERAND_TYPE_MEMORY;
		}
		default:
			return false;
		}
	}

	bool Instruction::writes(Register reg) const noexcept {
		const auto* destination = operand(0);
		return destination && destination->type == ZYDIS_OPERAND_TYPE_REGISTER &&
			widen(destination->reg.value) == reg && (destination->actions & ZYDIS_OPERAND_ACTION_MASK_WRITE);
	}

	Register Instruction::widen(Register reg) noexcept {
		if (reg == ZYDIS_REGISTER_NONE) return reg;
		const auto widest = ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, reg);
		return widest == ZYDIS_REGISTER_NONE ? reg : widest;
	}

} // namespace dumper::analysis
