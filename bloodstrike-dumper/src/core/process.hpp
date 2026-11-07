#pragma once
#include "core/image.hpp"
#include "core/unique_handle.hpp"

namespace dumper::core {

	struct ModuleInfo {
		DWORD pid = 0;
		std::uint64_t base = 0;
		std::uint32_t size = 0;
	};

	struct Snapshot {
		Image image;
		std::size_t readable_bytes = 0;
	};

	class Process {
	public:
		[[nodiscard]] static Process attach(std::wstring_view process_name);
		[[nodiscard]] const ModuleInfo& module() const noexcept { return module_; }

		bool read(std::uint64_t address, std::span<std::uint8_t> out) const noexcept;

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		[[nodiscard]] std::optional<T> read(std::uint64_t address) const noexcept {
			T value{};
			if (!read(address, std::as_writable_bytes(std::span(&value, 1)))) return std::nullopt;
			return value;
		}

		[[nodiscard]] Snapshot snapshot() const;

	private:
		Process(UniqueHandle handle, ModuleInfo module) noexcept : handle_(std::move(handle)), module_(module) {}

		bool read(std::uint64_t address, std::span<std::byte> out) const noexcept;
		[[nodiscard]] std::size_t copy_region(std::uint64_t begin, std::uint64_t end, std::vector<std::uint8_t>& out) const;

		[[nodiscard]] static std::optional<DWORD> find_pid(std::wstring_view process_name);
		[[nodiscard]] static std::optional<ModuleInfo> find_module(DWORD pid, std::wstring_view module_name);

		UniqueHandle handle_;
		ModuleInfo module_;
	};

} // namespace dumper::core
