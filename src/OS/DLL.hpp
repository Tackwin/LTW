#pragma once

#include <optional>
#include <filesystem>
#include <string_view>

struct DLL {
	void* ptr = nullptr;

	DLL() = default;
	DLL(DLL&) = delete;
	DLL& operator=(DLL&) = delete;
	DLL(DLL&& other) noexcept;
	DLL& operator=(DLL&& other) noexcept;
	~DLL() noexcept;

	void* get_symbol_(std::string_view name) noexcept;
	template<typename T> T get_symbol(std::string_view name) noexcept {
		return (T)get_symbol_(name);
	}
};

extern std::optional<DLL> load_dll(std::filesystem::path path) noexcept;