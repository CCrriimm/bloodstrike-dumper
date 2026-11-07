#pragma once
#include "core/types.hpp"

namespace dumper::analysis {

	using Register = ZydisRegister;
	using Mnemonic = ZydisMnemonic;

	struct MemoryOperand {
		Register base = ZYDIS_REGISTER_NONE;  // widened to 64-bit
		Register index = ZYDIS_REGISTER_NONE;
		std::int64_t displacement = 0;
		std::uint16_t size = 0;               // access size in bits

		[[nodiscard]] bool rip_relative() const noexcept { return base == ZYDIS_REGISTER_RIP; }
	};

	// One decoded x86-64 instruction with helpers for the questions resolvers ask.
	// Register results are always widened to their 64-bit form (eax -> rax, r8b -> r8).
	class Instruction {
	public:
		Instruction(Rva rva, const ZydisDecodedInstruction& info, std::span<const ZydisDecodedOperand> operands) noexcept;

		[[nodiscard]] Rva rva() const noexcept { return rva_; }
		[[nodiscard]] Rva next() const noexcept { return rva_ + info_.length; }
		[[nodiscard]] Mnemonic mnemonic() const noexcept { return info_.mnemonic; }
		[[nodiscard]] bool is(Mnemonic mnemonic) const noexcept { return info_.mnemonic == mnemonic; }
		[[nodiscard]] bool is_call() const noexcept { return is(ZYDIS_MNEMONIC_CALL); }
		[[nodiscard]] bool is_return() const noexcept { return is(ZYDIS_MNEMONIC_RET); }
		[[nodiscard]] bool is_padding() const noexcept { return is(ZYDIS_MNEMONIC_INT3); }
		[[nodiscard]] std::optional<Register> register_operand(std::size_t index) const noexcept;
		[[nodiscard]] std::optional<std::int64_t> immediate(std::size_t index) const noexcept;
		[[nodiscard]] std::optional<MemoryOperand> memory() const noexcept;
		[[nodiscard]] std::optional<Register> memory_base() const noexcept;
		[[nodiscard]] std::optional<std::int64_t> displacement() const noexcept;
		[[nodiscard]] bool is_rip_relative() const noexcept;
		[[nodiscard]] std::optional<Rva> rip_target() const noexcept;
		[[nodiscard]] std::optional<Rva> branch_target() const noexcept;
		[[nodiscard]] bool is_pointer_load() const noexcept;
		[[nodiscard]] bool is_float_load() const noexcept;
		[[nodiscard]] bool accesses(Register base) const noexcept { return memory_base() == base; }
		[[nodiscard]] bool writes(Register reg) const noexcept;
		[[nodiscard]] static Register widen(Register reg) noexcept;

	private:
		[[nodiscard]] const ZydisDecodedOperand* operand(std::size_t index) const noexcept;

		Rva rva_;
		ZydisDecodedInstruction info_;
		std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands_{};
	};

} // namespace dumper::analysis
