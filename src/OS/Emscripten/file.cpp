#ifdef WEB

#include <OS/file.hpp>

#include <stdio.h>
#include <string>
#include <optional>

#include "xstd.hpp"
#include "std/vector.hpp"

std::optional<std::string>
file::read_whole_text(const std::filesystem::path& path) noexcept {
	FILE* file = fopen(path.string().c_str(), "r");
	defer{ fclose(file); };

	if (!file) {
		return std::nullopt;
	}

	std::string bytes;

	while (!feof(file)) if (char c = fgetc(file); c != EOF) bytes.append(1, c);

	return bytes;
}

std::optional<xstd::vector<std::uint8_t>>
file::read_whole_file(const std::filesystem::path& path) noexcept {
	FILE* file = fopen(path.string().c_str(), "rb");
	defer{ fclose(file); };

	if (!file) {
		return std::nullopt;
	}

	xstd::vector<std::uint8_t> bytes;

	while (!feof(file)) if (char c = fgetc(file); c != EOF) bytes.push_back(c);

	return bytes;
}

bool file::overwrite_file_byte(
	std::filesystem::path path, const xstd::vector<std::uint8_t>& bytes
) noexcept {
	printf("Write operation are not supported in the browser.\n");
	return false;
}

bool file::overwrite_file(const std::filesystem::path& path, std::string_view str) noexcept {
	printf("Write operation are not supported in the browser.\n");
	return false;
}

void file::open_file_async(
	std::function<void(OpenFileResult)>&& callback, OpenFileOpts opts
) noexcept {
	printf("Can't browse file in the browser.\n");
}

void file::open_dir_async(
	std::function<void(std::optional<std::filesystem::path>)>&& callback
) noexcept {
	printf("Can't browse directory in the browser.\n");
}
std::optional<std::filesystem::path> file::open_dir() noexcept {
	printf("Can't browse directory in the browser.\n");
	return std::nullopt;
}
file::OpenFileResult file::open_file(OpenFileOpts opts) noexcept {
	OpenFileResult result;
	result.error_code = 558;
	return result;
}

void file::monitor_file(std::filesystem::path path, std::function<bool()> f) noexcept {
	printf("Can't use monitor operation in the browser.\n");
}
void file::monitor_dir(
	std::filesystem::path dir, std::function<bool(std::filesystem::path)> f
) noexcept {
	printf("Can't use monitor operation in the browser.\n");
}
void file::monitor_dir(
	std::function<void()> init_thread,
	std::filesystem::path dir,
	std::function<bool(std::filesystem::path)> f
) noexcept {
	printf("Can't use monitor operation in the browser.\n");
}
#endif
