#include "pch.hpp"
#include "core/strings.hpp"

namespace dumper::core {

	std::string to_utf8(std::wstring_view text) {
		if (text.empty()) return {};
		const auto length = static_cast<int>(text.size());
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), length, nullptr, 0, nullptr, nullptr);
		std::string result(static_cast<std::size_t>(size), '\0');
		WideCharToMultiByte(CP_UTF8, 0, text.data(), length, result.data(), size, nullptr, nullptr);
		return result;
	}

	std::string to_hex(std::uint64_t value) {
		return std::format("0x{:X}", value);
	}

} // namespace dumper::core
