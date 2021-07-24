#pragma once

#include <thread>
#include <optional>
#include <functional>
#include <filesystem>

#include "std/unordered_map.hpp"
#include "std/vector.hpp"

namespace file {
	[[nodiscard]] extern std::optional<std::string>
		read_whole_text(const std::filesystem::path& path) noexcept;
	[[nodiscard]] extern std::optional<xstd::vector<std::uint8_t>>
		read_whole_file(const std::filesystem::path& path) noexcept;
	[[nodiscard]] extern size_t get_file_size(const std::filesystem::path& path) noexcept;

	[[nodiscard]] extern bool overwrite_file_byte(
		std::filesystem::path path, const xstd::vector<std::uint8_t>& bytes
	) noexcept;

	[[nodiscard]] extern bool overwrite_file(
		const std::filesystem::path& path, std::string_view str
	) noexcept;

	struct OpenFileOpts {
		void* owner{ nullptr };

		xstd::unordered_map<std::wstring, std::vector<std::wstring>> ext_filters;
		std::filesystem::path filepath;
		std::filesystem::path filename;
		std::wstring dialog_title{ NULL };
		std::wstring default_ext{ NULL };

		bool allow_multiple{ false };
		bool prompt_for_create{ false };
		bool allow_redirect_link{ false };
	};
	struct OpenFileResult {
		bool succeded{ false };

		unsigned long error_code{ 0 };

		std::filesystem::path filepath;
		std::filesystem::path filename;
	};

	extern void open_file_async(
		std::function<void(OpenFileResult)>&& callback, OpenFileOpts opts = OpenFileOpts{}
	) noexcept;
	extern void open_dir_async(
		std::function<void(std::optional<std::filesystem::path>)>&& callback
	) noexcept;
	extern std::optional<std::filesystem::path> open_dir() noexcept;
	extern OpenFileResult open_file(OpenFileOpts opts = OpenFileOpts{}) noexcept;


	extern void monitor_file(std::filesystem::path path, std::function<bool()> f) noexcept;
	extern void monitor_dir(
		std::filesystem::path dir, std::function<bool(std::filesystem::path)> f
	) noexcept;
	extern void monitor_dir(
		std::function<void()> init_thread,
		std::filesystem::path dir,
		std::function<bool(std::filesystem::path)> f
	) noexcept;
}
