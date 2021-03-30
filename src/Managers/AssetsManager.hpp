#pragma once

#include <unordered_map>
#include <filesystem>
#include <functional>
#include <optional>
#include <memory>

#include "Graphic/Font.hpp"
#include "Graphic/Shader.hpp"
#include "Graphic/Texture.hpp"
#include "OS/DLL.hpp"

namespace asset {
	struct DLL_Id {
		inline static size_t Game = 1;
	};
	struct Texture_Id {
	};
	struct Shader_Id {
		inline static size_t Default = 1;
		inline static size_t Light   = 2;
		inline static size_t HDR     = 3;
		inline static size_t Line    = 4;
	};
	struct Font_Id {
	};


	struct Asset_DLL {
		DLL asset;
		std::filesystem::path path;
	};

	struct Asset_Font {
		Font asset;
		std::filesystem::path path;
	};

	struct Asset_Shader {
		Shader asset;
		std::filesystem::path vertex;
		std::filesystem::path fragment;
		std::filesystem::path geometry;
	};

	struct Asset_Texture {
		Texture albedo;
		std::filesystem::path path;
		std::unique_ptr<Texture> normal;
	};

	struct Store_t {
		bool stop{ false };

		std::unordered_map<std::string, std::uint64_t> textures_loaded;

		std::unordered_map<std::uint64_t, Asset_DLL> dlls;
		std::unordered_map<std::uint64_t, Asset_Font> fonts;
		std::unordered_map<std::uint64_t, Asset_Shader> shaders;
		std::unordered_map<std::uint64_t, Asset_Texture> textures;

		std::unordered_map<std::string, std::uint64_t> texture_string_map;

		[[nodiscard]] Texture* get_normal(size_t k) const noexcept;
		[[nodiscard]] Texture& get_albedo(size_t k) noexcept;
		[[nodiscard]] size_t make_texture() noexcept;
		[[nodiscard]] std::optional<size_t> load_texture(std::filesystem::path path) noexcept;
		[[nodiscard]] bool load_texture(size_t k, std::filesystem::path path) noexcept;
		
		[[nodiscard]] Shader& get_shader(size_t k) noexcept;
		[[nodiscard]] std::optional<size_t> load_shader(
			std::filesystem::path vertex, std::filesystem::path fragment
		) noexcept;
		[[nodiscard]] std::optional<size_t> load_shader(
			std::filesystem::path vertex,
			std::filesystem::path fragment,
			std::filesystem::path geometry
		) noexcept;
		[[nodiscard]] bool load_shader(
			size_t k, std::filesystem::path vertex, std::filesystem::path fragment
		) noexcept;
		[[nodiscard]] bool load_shader(
			size_t k,
			std::filesystem::path vertex,
			std::filesystem::path fragment,
			std::filesystem::path geometry
		) noexcept;

		[[nodiscard]] Font& get_font(size_t k) noexcept;
		[[nodiscard]] bool load_font(size_t k, std::filesystem::path path) noexcept;
		[[nodiscard]] std::optional<size_t> load_font(std::filesystem::path path) noexcept;

		[[nodiscard]] DLL& get_dll(size_t k) noexcept;
		[[nodiscard]] bool load_dll(size_t k, std::filesystem::path path) noexcept;
		[[nodiscard]] std::optional<size_t> load_dll(std::filesystem::path path) noexcept;

		void monitor_path(std::filesystem::path dir) noexcept;

		void load_known_textures() noexcept;
		void load_known_shaders() noexcept;
		void load_known_fonts() noexcept;
		void load_from_config(std::filesystem::path config_path) noexcept;
	};
	extern Store_t Store;

}

