#pragma once
#include "core/pattern.hpp"
#include "core/types.hpp"

namespace dumper::core {

	struct Section {
		std::string name;
		Rva rva = 0;
		std::uint32_t size = 0;
		std::uint32_t characteristics = 0;

		[[nodiscard]] Rva end() const noexcept { return rva + size; }
		[[nodiscard]] bool executable() const noexcept { return (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0; }
		[[nodiscard]] bool contains(Rva address) const noexcept { return address >= rva && address < end(); }
	};

	class Image {
	public:
		Image(std::vector<std::uint8_t> bytes, std::uint64_t base);

		[[nodiscard]] static Image from_file(const std::filesystem::path& path);
		[[nodiscard]] std::uint64_t base() const noexcept { return base_; }
		[[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }
		[[nodiscard]] std::uint32_t timestamp() const noexcept { return timestamp_; }
		[[nodiscard]] std::span<const Section> sections() const noexcept { return sections_; }
		[[nodiscard]] const Section* section(std::string_view name) const noexcept;
		[[nodiscard]] const Section& code() const noexcept { return sections_[code_index_]; }

		[[nodiscard]] bool contains(Rva rva, std::size_t length = 1) const noexcept {
			return static_cast<std::size_t>(rva) + length <= bytes_.size();
		}

		[[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }
		[[nodiscard]] std::span<const std::uint8_t> bytes(const Section& section) const noexcept {
			return std::span(bytes_).subspan(section.rva, section.size);
		}

		template <typename T> requires std::is_trivially_copyable_v<T>
		[[nodiscard]] std::optional<T> read(Rva rva) const noexcept {
			if (!contains(rva, sizeof(T))) return std::nullopt;
			T value;
			std::memcpy(&value, bytes_.data() + rva, sizeof(T));
			return value;
		}

		[[nodiscard]] std::vector<Rva> find(const Pattern& pattern) const { return find(pattern, code()); }
		[[nodiscard]] std::vector<Rva> find(const Pattern& pattern, const Section& section) const;
		[[nodiscard]] std::vector<Rva> find_bytes(std::span<const std::uint8_t> needle, const Section& section) const;
		[[nodiscard]] std::vector<Rva> find_rip_references(Rva target) const;
		[[nodiscard]] std::vector<Rva> find_pointers(Rva target, const Section& section) const;

		[[nodiscard]] std::optional<Rva> to_rva(std::uint64_t address) const noexcept {
			if (address < base_ || address - base_ >= bytes_.size()) return std::nullopt;
			return static_cast<Rva>(address - base_);
		}

	private:
		void parse_headers();

		std::vector<std::uint8_t> bytes_;
		std::uint64_t base_ = 0;
		std::uint32_t timestamp_ = 0;
		std::vector<Section> sections_;
		std::size_t code_index_ = 0;
	};

} // namespace dumper::core
