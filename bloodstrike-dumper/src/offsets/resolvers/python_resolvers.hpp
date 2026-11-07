#pragma once
#include "offsets/resolver.hpp"

namespace dumper::offsets {

	class PyTypeResolver final : public Resolver {
	public:
		PyTypeResolver(std::string type_name, std::string offset_name)
			: type_name_(std::move(type_name)), offset_name_(std::move(offset_name)) {}

		[[nodiscard]] std::string_view name() const noexcept override { return "py_type"; }
		void resolve(ResolveContext& context) const override;

	private:
		static constexpr std::int64_t kTpNameOffset = 0x18; // PyTypeObject::tp_name

		// The type object if the instructions at `reference` are a registration of the type.
		[[nodiscard]] std::optional<Rva> type_from_registration(const ResolveContext& context, Rva reference) const;
		[[nodiscard]] std::optional<Rva> type_from_getter(const ResolveContext& context, Rva getter) const;

		std::string type_name_;
		std::string offset_name_;
	};

} // namespace dumper::offsets
