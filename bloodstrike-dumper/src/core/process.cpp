#include "pch.hpp"
#include "core/process.hpp"

#include "core/strings.hpp"

namespace dumper::core {

	Process Process::attach(std::wstring_view process_name) {
		const auto pid = find_pid(process_name);
		if (!pid) throw std::runtime_error(std::format("{} is not running", to_utf8(process_name)));

		const auto module = find_module(*pid, process_name);
		if (!module) throw std::runtime_error(std::format("can't read the module list of {}", to_utf8(process_name)));

		UniqueHandle handle(OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, *pid));
		if (!handle) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "OpenProcess");

		return Process(std::move(handle), *module);
	}

	bool Process::read(std::uint64_t address, std::span<std::uint8_t> out) const noexcept {
		return read(address, std::as_writable_bytes(out));
	}

	bool Process::read(std::uint64_t address, std::span<std::byte> out) const noexcept {
		SIZE_T read = 0;
		return ReadProcessMemory(handle_.get(), reinterpret_cast<LPCVOID>(address), out.data(), out.size(), &read) &&
			read == out.size();
	}

	Snapshot Process::snapshot() const {
		std::vector<std::uint8_t> bytes(module_.size, 0);
		std::size_t readable = 0;

		const std::uint64_t end = module_.base + module_.size;
		for (std::uint64_t address = module_.base; address < end;) {
			MEMORY_BASIC_INFORMATION region{};
			if (!VirtualQueryEx(handle_.get(), reinterpret_cast<LPCVOID>(address), &region, sizeof(region))) break;

			const std::uint64_t region_end = std::min(reinterpret_cast<std::uint64_t>(region.BaseAddress) + region.RegionSize, end);
			const bool accessible = region.State == MEM_COMMIT && !(region.Protect & (PAGE_NOACCESS | PAGE_GUARD));
			if (accessible) readable += copy_region(address, region_end, bytes);
			address = region_end;
		}

		return Snapshot{ .image = Image(std::move(bytes), module_.base), .readable_bytes = readable };
	}

	std::size_t Process::copy_region(std::uint64_t begin, std::uint64_t end, std::vector<std::uint8_t>& out) const {
		constexpr std::uint64_t kChunk = 0x100000;
		constexpr std::uint64_t kPage = 0x1000;
		std::size_t copied = 0;

		auto destination = [&](std::uint64_t address, std::uint64_t length) {
			return std::span(out).subspan(static_cast<std::size_t>(address - module_.base), static_cast<std::size_t>(length));
		};

		for (std::uint64_t chunk = begin; chunk < end; chunk += kChunk) {
			const std::uint64_t chunk_size = std::min(kChunk, end - chunk);
			if (read(chunk, destination(chunk, chunk_size))) {
				copied += static_cast<std::size_t>(chunk_size);
				continue;
			}
			// a page in this chunk is unreadable, salvage what we can page by page
			for (std::uint64_t page = chunk; page < chunk + chunk_size; page += kPage) {
				const std::uint64_t page_size = std::min(kPage, chunk + chunk_size - page);
				if (read(page, destination(page, page_size))) copied += static_cast<std::size_t>(page_size);
			}
		}
		return copied;
	}

	std::optional<DWORD> Process::find_pid(std::wstring_view process_name) {
		const UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
		if (!snapshot) return std::nullopt;

		PROCESSENTRY32W entry{ .dwSize = sizeof(entry) };
		for (BOOL ok = Process32FirstW(snapshot.get(), &entry); ok; ok = Process32NextW(snapshot.get(), &entry))
			if (_wcsicmp(entry.szExeFile, std::wstring(process_name).c_str()) == 0) return entry.th32ProcessID;
		return std::nullopt;
	}

	std::optional<ModuleInfo> Process::find_module(DWORD pid, std::wstring_view module_name) {
		const UniqueHandle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
		if (!snapshot) return std::nullopt;

		MODULEENTRY32W entry{ .dwSize = sizeof(entry) };
		for (BOOL ok = Module32FirstW(snapshot.get(), &entry); ok; ok = Module32NextW(snapshot.get(), &entry)) {
			if (_wcsicmp(entry.szModule, std::wstring(module_name).c_str()) == 0) {
				return ModuleInfo{
					.pid = pid,
					.base = reinterpret_cast<std::uint64_t>(entry.modBaseAddr),
					.size = entry.modBaseSize,
				};
			}
		}
		return std::nullopt;
	}

} // namespace dumper::core
