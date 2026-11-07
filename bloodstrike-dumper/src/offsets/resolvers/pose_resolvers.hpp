#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	class PoseBonesResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "pose_bones"; }
		void resolve(ResolveContext& context) const override;

	private:
		void resolve_match(ResolveContext& context, Rva match) const;

		const core::Pattern pattern_{
			"48 8B B9 ? ? ? ? 48 85 FF 0F 84 ? ? ? ? 48 8B BF ? ? ? ? 48 8B 4F ? 48 2B 4F ? 48 B8 67 66 66 66 66 66 66 66"
		};
	};

	class BoneMatrixResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "bone_matrix"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "41 B0 01 48 8B D0 48 8B CB E8 ? ? ? ? 48 8D 8D ? ? ? ? 48 8B D8 E8" };
	};

	class SkeletonWorldResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "skeleton_world"; }
		void resolve(ResolveContext& context) const override;

	private:
		static constexpr std::size_t kSlotRadius = 8;

		// movups x,[rdx] ; movups [rcx+X],x  (x3, rows 0x10 apart) ; ret  ->  X
		[[nodiscard]] static std::optional<std::int64_t> matrix_setter(const ResolveContext& context, Rva function);

		const core::Pattern on_enter_world_{ "48 83 EC 28 C6 81 ? ? ? ? 01 66 C7 81 ? ? ? ? 00 00 4C 8B 81" };
	};

} // namespace dumper::offsets
