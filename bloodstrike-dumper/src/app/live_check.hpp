#pragma once
#include "core/process.hpp"
#include "offsets/offset_table.hpp"

namespace dumper::app {

	// Follows ClientEngine -> IGameplay -> ClientPlayer -> Camera in the running game and reads the fov and viewport, so the result can be sanity checked at a glance.
	class LiveCheck {
	public:
		LiveCheck(const core::Process& process, const offsets::OffsetTable& offsets) noexcept
			: process_(process), offsets_(offsets) {}

		[[nodiscard]] std::optional<std::string> run() const;

	private:
		[[nodiscard]] std::optional<std::uint64_t> pointer(std::uint64_t address) const;

		const core::Process& process_;
		const offsets::OffsetTable& offsets_;
	};

} // namespace dumper::app
