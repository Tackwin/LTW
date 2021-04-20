#pragma once

#include <unordered_map>
#include <filesystem>
#include <functional>
#include <optional>
#include <memory>

#include "Graphic/Font.hpp"
#include "Graphic/Object.hpp"
#include "Graphic/Shader.hpp"
#include "Graphic/Texture.hpp"
#include "OS/DLL.hpp"

#include "miniaudio.h"

namespace asset {
	struct DLL_Id {
		inline static size_t Game = 1;
	};
	struct Texture_Id {
		inline static size_t Gold_Icon  = 1;
		inline static size_t Build_Icon = 2;
		inline static size_t Back_Icon  = 3;
		inline static size_t Null_Icon  = 4;
		inline static size_t Up_Icon    = 5;
		inline static size_t Left_Icon  = 6;
		inline static size_t Down_Icon  = 7;
		inline static size_t Right_Icon = 8;
		inline static size_t Archer_Build_Icon = 9;
		inline static size_t Splash_Build_Icon = 10;
		inline static size_t Cancel_Icon   = 11;
		inline static size_t Send_Icon     = 12;
		inline static size_t Dummy         = 13;
		inline static size_t Palette       = 14;
		inline static size_t Methane_Icon  = 15;
		inline static size_t Dice_Icon     = 16;
		inline static size_t First_Icon    = 17;
		inline static size_t Target_Icon   = 18;
		inline static size_t Closest_Icon  = 19;
		inline static size_t Farthest_Icon = 20;
		inline static size_t Range1_Icon   = 21;
		inline static size_t Sharp_Icon    = 22;
		inline static size_t Volter_Icon   = 23;
	};
	struct Shader_Id {
		inline static size_t Default = 1;
		inline static size_t Light   = 2;
		inline static size_t HDR     = 3;
		inline static size_t Line    = 4;
		inline static size_t Default_Batched = 5;
		inline static size_t Default_3D = 6;
		inline static size_t Default_3D_Batched = 7;
		inline static size_t Primitive_3D_Batched = 8;
		inline static size_t Edge_Glow = 9;
		inline static size_t Blur = 10;
		inline static size_t SSAO = 11;
		inline static size_t Motion = 12;
	};
	struct Font_Id {
		inline static size_t Consolas{1};
	};
	struct Object_Id {
		inline static size_t Methane = 1;
		inline static size_t Empty_Tile = 2;
		inline static size_t Range1 = 2;
		inline static size_t Projectile = 3;
		inline static size_t Sharp = 4;
		inline static size_t Water = 5;
		inline static size_t Base = 6;
		inline static size_t Photon = 7;
		inline static size_t Oxygen = 8;
		inline static size_t Volter = 9;
		inline static size_t Electron = 10;
		inline static size_t Ethane = 11;
		inline static size_t Propane = 12;
	};
	struct Sound_Id {
		inline static size_t Ui_Action = 1;
		inline static size_t Range1_Shoot = 2;
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

	struct Asset_Object {
		Object asset;
		std::filesystem::path path;
	};

	struct Asset_Sound {
		ma_decoder asset;
		std::filesystem::path path;
	};

	struct Store_t {
		bool stop{ false };
		std::atomic<bool> ready = false;

		std::unordered_map<std::string, std::uint64_t> textures_loaded;

		std::unordered_map<std::uint64_t, Asset_DLL> dlls;
		std::unordered_map<std::uint64_t, Asset_Font> fonts;
		std::unordered_map<std::uint64_t, Asset_Sound> sounds;
		std::unordered_map<std::uint64_t, Asset_Object> objects;
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

		[[nodiscard]] Object& get_object(size_t k) noexcept;
		[[nodiscard]] bool load_object(size_t k, std::filesystem::path path) noexcept;
		[[nodiscard]] std::optional<size_t> load_object(std::filesystem::path path) noexcept;

		[[nodiscard]] ma_decoder& get_sound(size_t k) noexcept;
		[[nodiscard]] bool load_sound(size_t k, std::filesystem::path path) noexcept;
		[[nodiscard]] std::optional<size_t> load_sound(std::filesystem::path path) noexcept;

		void monitor_path(std::filesystem::path dir) noexcept;

		void load_known_textures() noexcept;
		void load_known_shaders() noexcept;
		void load_known_objects() noexcept;
		void load_known_sounds() noexcept;
		void load_known_fonts() noexcept;
		void load_from_config(std::filesystem::path config_path) noexcept;
	};
	extern Store_t Store;

}

