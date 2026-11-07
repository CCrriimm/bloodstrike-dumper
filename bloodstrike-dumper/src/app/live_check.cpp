#include "pch.hpp"
#include "app/live_check.hpp"

namespace dumper::app {

	std::optional<std::string> LiveCheck::run() const {
		const auto client_engine = offsets_.value("ClientEngine");
		const auto gameplay = offsets_.value("IGameplay");
		const auto client_player = offsets_.value("ClientPlayer");
		const auto camera = offsets_.value("Camera");
		const auto fov = offsets_.value("CamFov");
		const auto viewport = offsets_.value("CamViewport");
		if (!client_engine || !gameplay || !client_player || !camera || !fov || !viewport) return std::nullopt;

		const auto engine_ptr = pointer(process_.module().base + *client_engine);
		if (!engine_ptr) return "ClientEngine is null";
		const auto gameplay_ptr = pointer(*engine_ptr + *gameplay);
		if (!gameplay_ptr) return "IGameplay is null";
		const auto player_ptr = pointer(*gameplay_ptr + *client_player);
		if (!player_ptr) return "ClientPlayer is null";
		const auto camera_ptr = pointer(*player_ptr + *camera);
		if (!camera_ptr) return "Camera is null";

		const auto fov_value = process_.read<float>(*camera_ptr + *fov);
		const auto width = process_.read<std::uint16_t>(*camera_ptr + *viewport);
		const auto height = process_.read<std::uint16_t>(*camera_ptr + *viewport + 2);
		if (!fov_value || !width || !height) return "camera read failed";

		return std::format("fov {:.1f}, viewport {}x{}", *fov_value, *width, *height);
	}

	std::optional<std::uint64_t> LiveCheck::pointer(std::uint64_t address) const {
		const auto value = process_.read<std::uint64_t>(address);
		return value && *value ? value : std::nullopt;
	}

} // namespace dumper::app
