#include "Font.hpp"

#include "xstd.hpp"
#include "dyn_struct.hpp"

std::optional<Font::Font_Info::Char> Font::Font_Info::map(std::uint8_t code) const noexcept {
	for (auto& x : chars) if (x.code == code) return x;
	return std::nullopt;
}

Vector2f Font::compute_size(std::string_view str, float size) noexcept {
	Vector2f result = { 0, 0 };

//			Rectangle_Info rec;
//		rec.color = {0, 0, 0, 1};
//		rec.pos = pos;
//		rec.size = sprite.size;
//		rec.pos.x -= sprite.origin.x * rec.size.x;
//		rec.pos.y -= rec.size.y + (sprite.origin.y - 1) * rec.size.y;
//		rec.pos.y += font.info.height * info.height / font.info.char_size;
//		sprite.origin.x        = -1.f * glyph.offset.x / glyph.rect.w;
//		sprite.origin.y        = 1 + 1.f * glyph.offset.y / glyph.rect.h;
//			sprite.pos             = pos;
//		sprite.pos.y          += font.info.height * 1.f * info.height / font.info.char_size;
// 		sprite.size            = (Vector2f)glyph.rect.size;
// 		sprite.size *= 1.f * info.height / font.info.char_size;

	Vector2f pos = {0.f, 0.f};

	for (auto& x : str) {
		auto opt_c = info.map(x);
		if (!opt_c) continue;

		Rectanglef rec;
		rec.pos = pos;
		rec.size = (Vector2f)opt_c->rect.size;
		rec.size *= size / info.char_size;
		rec.pos.y += info.height * size / info.char_size;
		rec.pos.x += (0.f + 1.f * opt_c->offset.x / opt_c->rect.w) * rec.size.x;
		rec.pos.y -= (1.f + 1.f * opt_c->offset.y / opt_c->rect.h) * rec.size.y;
		//rec.pos.y += info.height * size / info.char_size;

		result.y = xstd::max(result.y, rec.pos.y + rec.size.y);
		result.x = xstd::max(result.x, rec.pos.x + rec.size.x);

		pos.x += opt_c->width * size / info.char_size;
	}

	return result;
}

void from_dyn_struct(const dyn_struct& str, Font::Font_Info& font) noexcept {
	font.char_size = (size_t)str["size"];
	font.texture_path = (std::string)str["texture"];
	font.family = (std::string)str["family"]; 
	font.height = (size_t)str["height"];
	
	if (auto e = (std::string)str["style"]; e == "Regular"){
		font.style = Font::Font_Info::Style::Regular;
	} else {
		assert(false);
	}

	for (auto& x : iterate_array(str["Char"])) {
		auto str = (std::string)x["code"];
		
		Font::Font_Info::Char c;
		c.width = (size_t)x["width"];
		c.code = (std::uint8_t)str[0];
		c.rect = (Rectangleu)x["rect"];
		c.offset = (Vector2i)x["offset"];
		font.chars.push_back(c);
	}
}

void to_dyn_struct(dyn_struct& str, const Font::Font_Info& font) noexcept {
	assert(false);
}