#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	class CameraRenderResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "camera_render"; }
		void resolve(ResolveContext& context) const override;

	private:
		[[nodiscard]] static std::optional<std::int64_t> viewport_from_getter(const ResolveContext& context, Rva getter);

		const core::Pattern pattern_{ "48 8B 98 ? ? ? ? 48 85 DB 0F 84 ? ? ? ? 33 D2 48 8D 8D ? ? ? ? 48 8B 85 ? ? ? ? 38 93" };
	};

	class ViewportGetterResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "viewport_getter"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "0F B7 81 ? ? ? ? 66 89 02 0F B7 81 ? ? ? ? 66 89 42 02 48 8B C2 C3" };
	};

	class CameraWorldPositionResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "camera_world_position"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "48 83 EC 18 45 33 C9 81 B9 ? ? ? ? FF FF FF 7F 45 8B D1" };
	};

} // namespace dumper::offsets
