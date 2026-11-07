#pragma once

namespace dumper::offsets {

	struct OffsetDefinition {
		std::string_view group;
		std::string_view name;
		std::string_view description;
	};

	inline constexpr std::array kOffsetDefinitions{
		OffsetDefinition{ "globals", "ClientEngine",      "RVA of ClientEngine*" },
		OffsetDefinition{ "globals", "EntityList",        "RVA of the actor manager*" },

		OffsetDefinition{ "engine",  "IGameplay",         "ClientEngine -> IGameplay" },
		OffsetDefinition{ "engine",  "ClientPlayer",      "IGameplay -> ClientPlayer" },
		OffsetDefinition{ "engine",  "Camera",            "ClientPlayer -> ICamera" },
		OffsetDefinition{ "engine",  "LocalPlayer",       "ClientPlayer -> local IEntity" },

		OffsetDefinition{ "camera",  "CamPos",            "ICamera -> translation (3 floats)" },
		OffsetDefinition{ "camera",  "CamOriginShift",    "ICamera -> origin shift (3x int32)" },
		OffsetDefinition{ "camera",  "CamViewport",       "ICamera -> uint16 width, uint16 height" },
		OffsetDefinition{ "camera",  "CamViewMatrix",     "ICamera -> 3x4 view matrix" },
		OffsetDefinition{ "camera",  "CamFov",            "ICamera -> fov (float, degrees)" },

		OffsetDefinition{ "entity",  "ManagerSentinel",   "manager -> list head (std::unordered_map)" },
		OffsetDefinition{ "entity",  "ActorInstance",     "list node -> value (std::unordered_map)" },
		OffsetDefinition{ "entity",  "InWorld",           "ActorInstance -> in-world flag (byte)" },
		OffsetDefinition{ "entity",  "ActorProps",        "ActorInstance -> ActorProps" },
		OffsetDefinition{ "entity",  "ActorComponent",    "ActorProps -> ActorComponent" },
		OffsetDefinition{ "entity",  "IEntity",           "ActorComponent -> IEntity" },
		OffsetDefinition{ "entity",  "Transform",         "IEntity -> 3x4 transform" },
		OffsetDefinition{ "entity",  "EntityTranslation", "IEntity -> translation (3 floats)" },
		OffsetDefinition{ "entity",  "EntityMask",        "IEntity -> entity type mask" },
		OffsetDefinition{ "entity",  "OriginShift",       "IEntity -> origin shift (3x int32)" },
		OffsetDefinition{ "entity",  "IArea",             "IEntity -> IArea" },

		OffsetDefinition{ "pose",    "Pose",              "Actor -> AnimationCore::Pose*" },
		OffsetDefinition{ "pose",    "PoseBones",         "Pose -> bone array begin" },
		OffsetDefinition{ "pose",    "PoseBonesEnd",      "Pose -> bone array end" },
		OffsetDefinition{ "pose",    "PoseBoneSize",      "sizeof(pose bone)" },
		OffsetDefinition{ "pose",    "BoneMatrix",        "pose bone -> 3x4 matrix" },
		OffsetDefinition{ "pose",    "SkeletonWorld",     "Actor -> 3x4 world matrix" },
		OffsetDefinition{ "pose",    "PoseBoneDef",       "pose bone -> bone definition" },
		OffsetDefinition{ "pose",    "BoneDefName",       "bone definition -> Messiah::Name" },

		OffsetDefinition{ "py",      "IEntityType",       "RVA of the IEntity PyTypeObject" },
	};

	struct OffsetHit {
		std::uint64_t value = 0;
		std::string source;
	};

	// Collects what resolvers find. An offset can be found by several resolvers; it only has a value when all of them agree, otherwise it's reported as a conflict.
	class OffsetTable {
	public:
		void record(std::string_view name, std::uint64_t value, std::string_view source);
		void add_error(std::string message) { errors_.push_back(std::move(message)); }
		void add_warning(std::string message) { warnings_.push_back(std::move(message)); }

		[[nodiscard]] std::optional<std::uint64_t> value(std::string_view name) const;
		[[nodiscard]] bool has_conflict(std::string_view name) const;
		[[nodiscard]] std::span<const OffsetHit> hits(std::string_view name) const;

		[[nodiscard]] std::size_t resolved_count() const;
		[[nodiscard]] std::vector<std::string_view> unresolved() const;
		[[nodiscard]] bool complete() const { return resolved_count() == kOffsetDefinitions.size(); }

		[[nodiscard]] std::span<const std::string> errors() const noexcept { return errors_; }
		[[nodiscard]] std::span<const std::string> warnings() const noexcept { return warnings_; }

	private:
		std::map<std::string, std::vector<OffsetHit>, std::less<>> hits_;
		std::vector<std::string> errors_;
		std::vector<std::string> warnings_;
	};

} // namespace dumper::offsets
