#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	class LocalPositionResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "local_position"; }
		void resolve(ResolveContext& context) const override;

	private:
		enum class Kind { Engine, Gameplay, Player, PlayerField };

		struct Tracked {
			Kind kind;
			std::int64_t field = 0; // displacement this register was loaded from, for PlayerField
		};

		const core::Pattern pattern_{ "48 8B 05 ? ? ? ? 48 85 C0 74 ? 48 8B 50 ? 48 85 D2 74 ? 48 8B 42 ? 48 8B 52 ? 48 8B 80" };
	};

	class ClientEngineResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "client_engine"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "48 83 79 68 00 74 ? 48 8B 05 ? ? ? ? 48 8B 48 58" };
	};

	class ClientPlayerResolver final : public Resolver {
	public:
		[[nodiscard]] std::string_view name() const noexcept override { return "client_player"; }
		void resolve(ResolveContext& context) const override;

	private:
		const core::Pattern pattern_{ "33 FF 48 8B 48 58 48 85 C9 74 ? 48 8B 49 58 EB" };
	};

} // namespace dumper::offsets
