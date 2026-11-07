#include "pch.hpp"
#include "core/image.hpp"

namespace dumper::core {

	Image::Image(std::vector<std::uint8_t> bytes, std::uint64_t base) : bytes_(std::move(bytes)), base_(base) {
		parse_headers();
	}

	Image Image::from_file(const std::filesystem::path& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error(std::format("can't open {}", path.string()));
		const std::vector<std::uint8_t> raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		if (raw.size() < sizeof(IMAGE_DOS_HEADER)) throw std::runtime_error("file is too small to be a PE");
		const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(raw.data());
		if (dos->e_magic != IMAGE_DOS_SIGNATURE || static_cast<std::size_t>(dos->e_lfanew) + sizeof(IMAGE_NT_HEADERS64) > raw.size())
			throw std::runtime_error("file is not a PE");
		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(raw.data() + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE) throw std::runtime_error("file is not a PE");

		std::vector<std::uint8_t> mapped(nt->OptionalHeader.SizeOfImage, 0);
		std::memcpy(mapped.data(), raw.data(), std::min<std::size_t>({ nt->OptionalHeader.SizeOfHeaders, raw.size(), mapped.size() }));

		for (const auto& section : std::span(IMAGE_FIRST_SECTION(nt), nt->FileHeader.NumberOfSections)) {
			if (section.SizeOfRawData == 0 || section.PointerToRawData >= raw.size() || section.VirtualAddress >= mapped.size())
				continue;
			const std::size_t length = std::min<std::size_t>({
				section.SizeOfRawData,
				raw.size() - section.PointerToRawData,
				mapped.size() - section.VirtualAddress,
			});
			std::memcpy(mapped.data() + section.VirtualAddress, raw.data() + section.PointerToRawData, length);
		}

		return Image(std::move(mapped), nt->OptionalHeader.ImageBase);
	}

	void Image::parse_headers() {
		const auto dos = read<IMAGE_DOS_HEADER>(0);
		if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE || !contains(static_cast<Rva>(dos->e_lfanew), sizeof(IMAGE_NT_HEADERS64)))
			throw std::runtime_error("image has no valid DOS header");

		const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(bytes_.data() + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
			throw std::runtime_error("image is not a PE64");

		timestamp_ = nt->FileHeader.TimeDateStamp;

		for (const auto& header : std::span(IMAGE_FIRST_SECTION(nt), nt->FileHeader.NumberOfSections)) {
			const auto* name = reinterpret_cast<const char*>(header.Name);
			Section section{
				.name = std::string(name, strnlen(name, IMAGE_SIZEOF_SHORT_NAME)),
				.rva = header.VirtualAddress,
				.size = header.Misc.VirtualSize,
				.characteristics = header.Characteristics,
			};
			if (section.rva >= bytes_.size()) continue;
			section.size = std::min<std::uint32_t>(section.size, static_cast<std::uint32_t>(bytes_.size() - section.rva));
			sections_.push_back(std::move(section));
		}

		auto code = std::ranges::find(sections_, ".text", &Section::name);
		if (code == sections_.end()) code = std::ranges::find_if(sections_, &Section::executable);
		if (code == sections_.end()) throw std::runtime_error("image has no code section");
		code_index_ = static_cast<std::size_t>(std::distance(sections_.begin(), code));
	}

	const Section* Image::section(std::string_view name) const noexcept {
		const auto it = std::ranges::find(sections_, name, &Section::name);
		return it != sections_.end() ? &*it : nullptr;
	}

	std::vector<Rva> Image::find(const Pattern& pattern, const Section& section) const {
		std::vector<Rva> matches;
		for (const auto offset : pattern.scan(bytes(section)))
			matches.push_back(static_cast<Rva>(section.rva + offset));
		return matches;
	}

	std::vector<Rva> Image::find_bytes(std::span<const std::uint8_t> needle, const Section& section) const {
		std::vector<Rva> matches;
		if (needle.empty()) return matches;

		const auto haystack = bytes(section);
		const std::boyer_moore_horspool_searcher searcher(needle.begin(), needle.end());
		for (auto it = haystack.begin();;) {
			const auto [first, last] = searcher(it, haystack.end());
			if (first == haystack.end()) break;
			matches.push_back(static_cast<Rva>(section.rva + std::distance(haystack.begin(), first)));
			it = std::next(first);
		}
		return matches;
	}

	std::vector<Rva> Image::find_rip_references(Rva target) const {
		// lea r64, [rip+rel32]: REX.W (48 / 4C), 8D, ModRM with mod=00 rm=101, then rel32
		constexpr std::size_t kLength = 7;
		std::vector<Rva> references;
		const auto& text = code();
		if (text.size < kLength) return references;

		for (Rva rva = text.rva; rva <= text.end() - kLength; ++rva) {
			const auto* p = bytes_.data() + rva;
			if ((p[0] & 0xFB) != 0x48 || p[1] != 0x8D || (p[2] & 0xC7) != 0x05) continue;

			std::int32_t displacement;
			std::memcpy(&displacement, p + 3, sizeof(displacement));
			if (static_cast<std::int64_t>(rva) + kLength + displacement == target) references.push_back(rva);
		}
		return references;
	}

	std::vector<Rva> Image::find_pointers(Rva target, const Section& section) const {
		const auto address = std::bit_cast<std::array<std::uint8_t, sizeof(std::uint64_t)>>(base_ + target);

		// pointers in tables are 8-byte aligned
		auto slots = find_bytes(address, section);
		std::erase_if(slots, [](Rva slot) { return slot % sizeof(std::uint64_t) != 0; });
		return slots;
	}

} // namespace dumper::core
