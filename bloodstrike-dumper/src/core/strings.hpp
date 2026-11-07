#pragma once

namespace dumper::core {

	[[nodiscard]] std::string to_utf8(std::wstring_view text);
	[[nodiscard]] std::string to_hex(std::uint64_t value);

} // namespace dumper::core
