#include "pch.hpp"
#include "core/pattern.hpp"

namespace dumper::core {

	Pattern::Pattern(std::string_view signature) : signature_(signature) {
		for (const auto token : signature | std::views::split(' ')) {
			const std::string_view text(token.begin(), token.end());
			if (text.empty()) continue;

			if (text.front() == '?') {
				bytes_.emplace_back(std::nullopt);
				continue;
			}

			std::uint8_t byte = 0;
			const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), byte, 16);
			if (error != std::errc{} || end != text.data() + text.size())
				throw std::invalid_argument(std::format("invalid byte '{}' in pattern \"{}\"", text, signature));
			bytes_.emplace_back(byte);
		}

		const auto concrete = std::ranges::find_if(bytes_, [](const auto& byte) { return byte.has_value(); });
		if (concrete == bytes_.end())
			throw std::invalid_argument(std::format("pattern \"{}\" has no concrete bytes", signature));
		anchor_ = static_cast<std::size_t>(std::distance(bytes_.begin(), concrete));
	}

	std::vector<std::size_t> Pattern::scan(std::span<const std::uint8_t> data) const {
		std::vector<std::size_t> matches;
		if (data.size() < bytes_.size()) return matches;

		const std::size_t last = data.size() - bytes_.size();
		const std::uint8_t anchor_byte = *bytes_[anchor_];

		for (std::size_t offset = 0; offset <= last; ++offset) {
			const auto* hit = static_cast<const std::uint8_t*>(
				std::memchr(data.data() + offset + anchor_, anchor_byte, last - offset + 1));
			if (!hit) break;

			offset = static_cast<std::size_t>(hit - data.data()) - anchor_;
			if (matches_at(data, offset)) matches.push_back(offset);
		}
		return matches;
	}

	bool Pattern::matches_at(std::span<const std::uint8_t> data, std::size_t offset) const noexcept {
		for (std::size_t i = 0; i < bytes_.size(); ++i)
			if (bytes_[i] && data[offset + i] != *bytes_[i]) return false;
		return true;
	}

} // namespace dumper::core
