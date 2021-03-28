#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

struct Font {
	struct Font_Info {
		size_t char_size;
		size_t height;
		enum Style {
			Regular = 0,
			Count
		} style;
		std::filesystem::path texture_path;
		std::string family;

		struct Char {
			size_t width;
			Vector2i offset;
			Rectangleu rect;
			std::uint8_t code;
		};
		std::vector<Char> chars;

		std::optional<Char> map(uint8_t code) const noexcept;
	} info;

	size_t texture_id;

	Vector2f compute_size(std::string_view str, float size) noexcept;
};

struct dyn_struct;
extern void from_dyn_struct(const dyn_struct& str, Font::Font_Info& font) noexcept;
extern void to_dyn_struct(dyn_struct& str, const Font::Font_Info& font) noexcept;
