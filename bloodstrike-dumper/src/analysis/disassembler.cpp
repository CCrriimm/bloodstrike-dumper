#include "pch.hpp"
#include "analysis/disassembler.hpp"

namespace dumper::analysis {

	Disassembler::Disassembler(const core::Image& image) noexcept : image_(image) {
		ZydisDecoderInit(&decoder_, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
	}

	std::optional<Instruction> Disassembler::decode(Rva rva) const noexcept {
		if (!image_.contains(rva)) return std::nullopt;

		const auto bytes = image_.bytes().subspan(rva);
		const auto length = std::min<std::size_t>(bytes.size(), ZYDIS_MAX_INSTRUCTION_LENGTH);

		ZydisDecodedInstruction info{};
		std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
		if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder_, bytes.data(), length, &info, operands.data())))
			return std::nullopt;

		return Instruction(rva, info, operands);
	}

	std::vector<Instruction> Disassembler::sweep(Rva start, std::size_t max_instructions, SweepEnd end) const {
		std::vector<Instruction> instructions;
		instructions.reserve(max_instructions);

		for (Rva rva = start; instructions.size() < max_instructions;) {
			auto instruction = decode(rva);
			if (!instruction) break;
			if (end == SweepEnd::Padding && instruction->is_padding()) break;

			rva = instruction->next();
			const bool is_return = instruction->is_return();
			instructions.push_back(std::move(*instruction));
			if (end == SweepEnd::Return && is_return) break;
		}
		return instructions;
	}

} // namespace dumper::analysis
