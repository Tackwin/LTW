#include "Render.hpp"
#ifdef ES
#include <GLES3/gl3.h>
#else
#include "GL/glew.h"
#include "GL/wglew.h"
#endif

#include "Managers/AssetsManager.hpp"
#include "global.hpp"

#include "xstd.hpp"

render::Camera render::current_camera;
 
void render::immediate(Circle circle) noexcept {
	static GLuint vao{ 0 };
	static GLuint vbo{ 0 };
	constexpr size_t Half_Prec = 30;

	if (!vao) {
		const float ang_inc = 3.1415926f / Half_Prec;
		const float cos_inc = cos(ang_inc);
		const float sin_inc = sin(ang_inc);

		GLfloat vertices[3 * Half_Prec * 2];
		unsigned coord_idx = 0;

		vertices[coord_idx++] = 1.0f;
		vertices[coord_idx++] = 0.0f;
		vertices[coord_idx++] = 1.0f;

		float xc = 1.0f;
		float yc = 0.0f;
		for (unsigned i = 1; i < Half_Prec; ++i) {
			float xc_new = cos_inc * xc - sin_inc * yc;
			yc = sin_inc * xc + cos_inc * yc;
			xc = xc_new;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = yc;
			vertices[coord_idx++] = 1;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = -yc;
			vertices[coord_idx++] = 1;
		}

		vertices[coord_idx++] = -1.0f;
		vertices[coord_idx++] = 0.0f;
		vertices[coord_idx++] = 1.0f;

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER, 2 * Half_Prec * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW
		);
#ifdef GL_DEBUG
		auto label = "circle_info_vbo";
		glObjectLabel(GL_BUFFER, vbo, (GLsizei)strlen(label) - 1, label);
#endif
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}
	
	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);
	shader.set_origin({ 0, 0 });
	shader.set_position(circle.pos);
	shader.set_primary_color(circle.color);
	shader.set_rotation(0);
	shader.set_size({ circle.r, circle.r });
	shader.set_texture(0);
	shader.set_uniform("use_normal_texture", false);
	shader.set_use_texture(false);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 2 * Half_Prec);
}


void render::immediate(Rectangle rec) noexcept {
	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f,
			1.0f, 0.0f, 0.0f
		};

		// setup plane VAO
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "rectangle_info_vbo";
		glObjectLabel(GL_BUFFER, quad_vbo, (GLsizei)strlen(label) - 1, label);
#endif
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}

	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);
	shader.set_origin({ 0, 0 });
	shader.set_position(rec.pos);
	shader.set_primary_color(rec.color);
	shader.set_rotation(0);
	shader.set_size(rec.size);
	shader.set_uniform("use_normal_texture", false);
	shader.set_use_texture(false);
	shader.set_texture(0);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render::immediate(Arrow info) noexcept {
	static GLuint head_vao{ 0 };
	static GLuint head_vbo{ 0 };
	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	auto size = V2F((info.a - info.b).length() / 10.f);

	if (!head_vao) {
		static float head_vertices[] = {
			+0.0f, +0.0f, 1.0f,
			-1.0f, +1.0f, 1.0f,
			-1.0f, -1.0f, 1.0f,
		};

		// setup plane VAO
		glGenVertexArrays(1, &head_vao);
		glBindVertexArray(head_vao);
		glGenBuffers(1, &head_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, head_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(head_vertices), &head_vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		static float quad_vertices[] = {
			// positions    
			0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f,
			1.0f, 1.0f, 0.0f,
			1.0f, 0.0f, 0.0f
		};

		// setup plane VAO
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}


	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);
	shader.set_origin({ 0, 0 });
	shader.set_position(info.b);
	shader.set_primary_color(info.color);
	shader.set_rotation((float)(info.b - info.a).angleX());
	shader.set_size(size);
	shader.set_uniform("use_normal_texture", false);
	shader.set_use_texture(false);
	shader.set_texture(0);

	glBindVertexArray(head_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);

	shader.set_origin({ 0, 0.5 });
	shader.set_position(info.a);
	shader.set_size({ (info.a - info.b).length() - size.x, size.y });

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render::immediate(render::Text info) noexcept {
	auto& font = asset::Store.get_font(info.font_id);
	auto texture_size = asset::Store.get_albedo(font.texture_id).get_size();
	Vector2f pos = info.pos;

	auto size = font.compute_size(std::string_view{info.text, info.text_length}, info.height);
	pos.x -= size.x * info.origin.x;
	pos.y -= size.y * info.origin.y;

	if (info.style_mask & Text::Style::Underline) {
		Rectangle underline;
		underline.size.x = size.x;
		underline.size.y = info.height * 0.1;
		underline.pos = pos;
		underline.color = info.color;
		immediate(underline);
	}

	for (size_t i = 0; i < info.text_length; ++i) {
		auto& c = info.text[i];

		auto opt_glyph = font.info.map(c);
		if (!opt_glyph) continue;
		auto& glyph = *opt_glyph;
		
		Sprite sprite;
		sprite.color           = info.color;
		sprite.origin.x        = -1.f * glyph.offset.x / glyph.rect.w;
		sprite.origin.y        = 1 + 1.f * glyph.offset.y / glyph.rect.h;
		sprite.pos             = pos;
		sprite.pos.y          += font.info.height * 1.f * info.height / font.info.char_size;
		sprite.rotation        = 0;
		sprite.shader          = asset::Shader_Id::Default;
		sprite.size            = (Vector2f)glyph.rect.size * info.height / font.info.char_size;
		sprite.texture         = font.texture_id;
		sprite.texture_rect    = (Rectanglef)glyph.rect;
		
		// transformation from top left to bottom left.
		sprite.texture_rect.y  = texture_size.y - sprite.texture_rect.y;
		sprite.texture_rect.y -= sprite.texture_rect.h;

		sprite.texture_rect.x /= texture_size.x;
		sprite.texture_rect.w /= texture_size.x;
		sprite.texture_rect.y /= texture_size.y;
		sprite.texture_rect.h /= texture_size.y;

		Rectangle rec;
		rec.color = {0, 0, 0, 1};
		rec.pos = pos;
		rec.size = sprite.size;
		rec.pos.x -= sprite.origin.x * rec.size.x;
		rec.pos.y -= rec.size.y + (sprite.origin.y - 1) * rec.size.y;
		rec.pos.y += font.info.height * info.height / font.info.char_size;

		// immediate(rec);
		immediate(sprite);

		pos.x += glyph.width * 1.f * info.height / font.info.char_size;
	}

}

void render::immediate(Sprite info) noexcept {
	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			0.0f, 1.0f, 1.0f,
			0.0f, 0.0f, 1.0f,
			1.0f, 1.0f, 1.0f,
			1.0f, 0.0f, 1.0f
		};

		// setup plane VAO
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "sprite_info_vbo";
		glObjectLabel(GL_BUFFER, quad_vbo, (GLsizei)strlen(label) - 1, label);
#endif
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	}

	if (asset::Store.textures.count(info.texture) > 0) {
		asset::Store.get_albedo(info.texture).bind(4);
		auto normal = asset::Store.get_normal(info.texture); if (normal) normal->bind(5);
	}
	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	if (!info.shader) info.shader = asset::Shader_Id::Default;

	auto& shader = asset::Store.get_shader(info.shader);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);
	shader.set_origin(info.origin);
	shader.set_position(info.pos);
	shader.set_primary_color(info.color);
	shader.set_rotation(info.rotation);
	shader.set_size(info.size);
	shader.set_use_texture((bool)info.texture);
	shader.set_uniform("invert_x", info.texture_rect.w * info.size.x < 0);
	shader.set_uniform("texture_rect", info.texture_rect);
	shader.set_uniform("use_normal_texture", asset::Store.get_normal(info.texture) ? 1 : 0);
	shader.set_uniform("texture_rect", info.texture_rect);
	shader.set_uniform("normal_texture", 5);
	shader.set_texture(4);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
}