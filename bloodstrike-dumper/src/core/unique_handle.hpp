#pragma once

namespace dumper::core {

	class UniqueHandle {
	public:
		UniqueHandle() noexcept = default;
		explicit UniqueHandle(HANDLE handle) noexcept : handle_(handle == INVALID_HANDLE_VALUE ? nullptr : handle) {}
		~UniqueHandle() { reset(); }

		UniqueHandle(UniqueHandle&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}
		UniqueHandle& operator=(UniqueHandle&& other) noexcept {
			if (this != &other) {
				reset();
				handle_ = std::exchange(other.handle_, nullptr);
			}
			return *this;
		}

		UniqueHandle(const UniqueHandle&) = delete;
		UniqueHandle& operator=(const UniqueHandle&) = delete;

		[[nodiscard]] HANDLE get() const noexcept { return handle_; }
		[[nodiscard]] explicit operator bool() const noexcept { return handle_ != nullptr; }

		void reset() noexcept {
			if (handle_) {
				CloseHandle(handle_);
				handle_ = nullptr;
			}
		}

	private:
		HANDLE handle_ = nullptr;
	};

} // namespace dumper::core
