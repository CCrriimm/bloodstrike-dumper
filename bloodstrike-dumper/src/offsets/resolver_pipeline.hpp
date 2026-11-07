#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	// Ordered list of resolvers.
	class ResolverPipeline {
	public:
		[[nodiscard]] static ResolverPipeline standard();

		template <std::derived_from<Resolver> T, typename... Args>
		ResolverPipeline& add(Args&&... args) {
			resolvers_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
			return *this;
		}

		void run(const core::Image& image, OffsetTable& offsets) const;

	private:
		std::vector<std::unique_ptr<Resolver>> resolvers_;
	};

} // namespace dumper::offsets
