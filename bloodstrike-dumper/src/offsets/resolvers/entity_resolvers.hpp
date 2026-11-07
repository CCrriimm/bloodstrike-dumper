#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	class EntityListResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "entity_list"; }
		void resolve(ResolveContext& context) const override;

	private:
		static constexpr std::uint64_t kListHead = 0x8;   // std::unordered_map -> std::list _Myhead
		static constexpr std::uint64_t kNodeValue = 0x18; // std::_List_node -> _Myval (after _Next, _Prev, key)

		const core::Pattern pattern_{ "48 8B 1D ? ? ? ? 48 8B 4B ? 48 8B 43" };
	};

	class ComponentEntityResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "component_entity"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "4C 8B 51 ? 45 33 DB 4C 89 5C 24 ? 48 8B F9 41 8B DB 4D 85 D2 74 ? 41 81 BA" };
	};

	class ActorChainResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "actor_chain"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "48 8B 43 ? 48 8B 88 ? ? ? ? 48 8B 41 ? 48 8B 58 ? 48 85 DB 74 ? 48 89 74 24 ? 48 8D 4B" };
	};

	class InWorldResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "in_world"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "48 8B 81 ? ? ? ? BE FF FF FF FF 33 ED 48 85 C0 74 ? 40 38 A8" };
	};

	class EntityAreaResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "entity_area"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{
			"48 89 5C 24 ? 57 48 83 EC 20 48 8B F9 48 8B DA 48 8B 89 ? ? ? ? 48 3B CA 74 ? 48 85 C9 74 ? 48 8B D7 E8 ? ? ? ? 48 85 DB 74 ? 48 8B D7 48 8B CB E8"
		};
	};

	class EntityMaskResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "entity_mask"; }
		void resolve(ResolveContext& context) const override;

	private:
		static constexpr std::int64_t kMapSizeOffset = 0x8; // std::map: _Myhead, _Mysize

		const core::Pattern pattern_{ "49 8B AF ? ? ? ? 4C 8B 4D 08 4C 8B C5 41 80 79 19 00 75" };
	};

} // namespace dumper::offsets
