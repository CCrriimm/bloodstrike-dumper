#pragma once

namespace dumper::output {

	enum class Color { Default, Red, Green, Yellow, Cyan, Gray };

	class Console {
	public:
		static void initialize() noexcept;

		template <typename... Args>
		static void info(std::format_string<Args...> format, Args&&... args) {
			write(Color::Cyan, "[*] ", std::format(format, std::forward<Args>(args)...));
		}

		template <typename... Args>
		static void success(std::format_string<Args...> format, Args&&... args) {
			write(Color::Green, "[+] ", std::format(format, std::forward<Args>(args)...));
		}

		template <typename... Args>
		static void warning(std::format_string<Args...> format, Args&&... args) {
			write(Color::Yellow, "[!] ", std::format(format, std::forward<Args>(args)...));
		}

		template <typename... Args>
		static void error(std::format_string<Args...> format, Args&&... args) {
			write(Color::Red, "[-] ", std::format(format, std::forward<Args>(args)...));
		}

		static void write(Color color, std::string_view prefix, std::string_view text);
		static void wait_for_enter();

	private:
		[[nodiscard]] static std::string_view escape(Color color) noexcept;
	};

} // namespace dumper::output
