#include "pch.hpp"
#include "offsets/resolvers/camera_resolvers.hpp"

namespace dumper::offsets {

	void CameraRenderResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 80);
		if (code.empty() || !code.front().is_pointer_load()) return fail(context, "anchor is not a pointer load");

		record(context, "Camera", *code.front().displacement());
		const auto camera = *code.front().register_operand(0);

		std::optional<std::int64_t> fov, view_matrix, viewport;
		for (const auto& instruction : code | std::views::drop(1)) {
			if (!fov && instruction.is(ZYDIS_MNEMONIC_MOVSS) && instruction.accesses(camera))
				fov = instruction.displacement();
			if (!view_matrix && instruction.is(ZYDIS_MNEMONIC_LEA) && instruction.accesses(camera))
				view_matrix = instruction.displacement();
			if (!viewport && instruction.is_call())
				if (const auto target = instruction.branch_target()) viewport = viewport_from_getter(context, *target);
			if (instruction.writes(camera)) break;
		}

		if (fov) record(context, "CamFov", *fov);
		else fail(context, "no fov load");

		if (view_matrix) record(context, "CamViewMatrix", *view_matrix);
		else fail(context, "no view matrix lea");

		if (viewport) record(context, "CamViewport", *viewport);
		else fail(context, "GetViewport call not found");
	}

	std::optional<std::int64_t> CameraRenderResolver::viewport_from_getter(const ResolveContext& context, Rva getter) {
		const auto first = context.disassembler.decode(getter);
		if (!first || !first->is(ZYDIS_MNEMONIC_MOVZX) || !first->accesses(ZYDIS_REGISTER_RCX)) return std::nullopt;
		if (first->memory()->size != 16) return std::nullopt;
		return first->displacement();
	}

	void ViewportGetterResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 8);
		if (code.size() < 3) return fail(context, "decode failed");

		const auto width = *code[0].displacement();
		const auto height = *code[2].displacement();
		if (height != width + 2) warn(context, std::format("height (0x{:X}) does not follow width (0x{:X})", height, width));
		record(context, "CamViewport", width);
	}

	void CameraWorldPositionResolver::resolve(ResolveContext& context) const {
		const auto anchor = find_unique(context, pattern_);
		if (!anchor) return;

		const auto code = context.disassembler.sweep(*anchor, 64);

		if (const auto shift = origin_shift(context, code, ZYDIS_REGISTER_RCX)) record(context, "CamOriginShift", *shift);
		else fail(context, "no origin shift checks");

		// the translation is the lowest of the three float loads (the shift itself is read with movsd)
		auto translation = code
			| std::views::filter([](const auto& i) { return i.is(ZYDIS_MNEMONIC_MOVSS) && i.accesses(ZYDIS_REGISTER_RCX); })
			| std::views::transform([](const auto& i) { return *i.displacement(); });
		if (translation.empty()) return fail(context, "no translation loads");
		record(context, "CamPos", std::ranges::min(translation));
	}

} // namespace dumper::offsets
