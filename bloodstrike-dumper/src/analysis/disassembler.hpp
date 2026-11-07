#pragma once
#include "analysis/instruction.hpp"
#include "core/image.hpp"

namespace dumper::analysis {

	enum class SweepEnd {
		Return,  // stop after the first ret
		Padding, // stop at int3 padding
		Never,   // only the instruction limit stops the sweep
	};

	class Disassembler {
	public:
		explicit Disassembler(const core::Image& image) noexcept;

		[[nodiscard]] std::optional<Instruction> decode(Rva rva) const noexcept;
		[[nodiscard]] std::vector<Instruction> sweep(Rva start, std::size_t max_instructions, SweepEnd end = SweepEnd::Return) const;

	private:
		const core::Image& image_;
		ZydisDecoder decoder_{};
	};

} // namespace dumper::analysis
