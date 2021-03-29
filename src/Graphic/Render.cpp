#include "Render.hpp"
#ifdef ES
#include <GLES3/gl3.h>
#else
#include "GL/glew.h"
#include "GL/wglew.h"
#endif

#include "Managers/AssetsManager.hpp"
#include "global.hpp"

render::Camera render::current_camera;

void render::immediate(Circle circle) noexcept {
	static GLuint vao{ 0 };
	static GLuint vbo{ 0 };
	constexpr size_t Half_Prec = 10;

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