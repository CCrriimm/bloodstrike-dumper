#include "pch.hpp"
#include "offsets/resolver_pipeline.hpp"

#include "offsets/resolvers/camera_resolvers.hpp"
#include "offsets/resolvers/engine_resolvers.hpp"
#include "offsets/resolvers/entity_resolvers.hpp"
#include "offsets/resolvers/pose_resolvers.hpp"
#include "offsets/resolvers/python_resolvers.hpp"

namespace dumper::offsets {

	ResolverPipeline ResolverPipeline::standard() {
		ResolverPipeline pipeline;
		pipeline
			// camera first: LocalPositionResolver needs Camera to tell its two ClientPlayer loads apart
			.add<CameraRenderResolver>()
			.add<ViewportGetterResolver>()
			.add<CameraWorldPositionResolver>()
			.add<LocalPositionResolver>()
			.add<ClientEngineResolver>()
			.add<ClientPlayerResolver>()
			.add<EntityListResolver>()
			.add<ComponentEntityResolver>()
			.add<ActorChainResolver>()
			.add<InWorldResolver>()
			.add<EntityAreaResolver>()
			.add<EntityMaskResolver>()
			.add<PoseBonesResolver>()
			.add<BoneMatrixResolver>()
			.add<SkeletonWorldResolver>()
			.add<PyTypeResolver>("IEntity", "IEntityType");
		return pipeline;
	}

	void ResolverPipeline::run(const core::Image& image, OffsetTable& offsets) const {
		const analysis::Disassembler disassembler(image);
		ResolveContext context{ .image = image, .disassembler = disassembler, .offsets = offsets };
		for (const auto& resolver : resolvers_) resolver->resolve(context);
	}

} // namespace dumper::offsets
