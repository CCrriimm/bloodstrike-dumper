#pragma once

namespace dumper::core {

	class Pattern {
	public:
		explicit Pattern(std::string_view signature);

		[[nodiscard]] std::string_view signature() const noexcept { return signature_; }
		[[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }
		[[nodiscard]] std::vector<std::size_t> scan(std::span<const std::uint8_t> data) const;

	private:
		[[nodiscard]] bool matches_at(std::span<const std::uint8_t> data, std::size_t offset) const noexcept;

		std::string signature_;
		std::vector<std::optional<std::uint8_t>> bytes_;
		std::size_t anchor_ = 0; // index of the first concrete byte, used to memchr through the data
	};

} // namespace dumper::core
