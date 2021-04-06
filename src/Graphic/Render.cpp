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

void render::Orders::push(render::Order o) noexcept {
	push(o, commands.size());
}

void render::Orders::push(render::Order o, float z) noexcept {
	o->z = z;
	commands.push_back(std::move(o));
}

void render::immediate(std::span<Rectangle> rectangle) noexcept {
	constexpr size_t GPU_Rectangle_Size = 8 + 8 + 4 + 4 + 16;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local std::vector<std::uint8_t> instance_data;

	thread_local GLuint quad_vao = 0;
	thread_local GLuint quad_vbo = 0;


	if (instance_vbo_size < rectangle.size()) {
		size_t new_size = (instance_vbo_size * 5) / 3;
		instance_vbo_size = std::max(new_size, rectangle.size());

		if (instance_vbo) glDeleteBuffers(1, &instance_vbo);
		glGenBuffers(1, &instance_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, instance_vbo_size * GPU_Rectangle_Size, NULL, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "rectangle_batch_instance_buffer";
		glObjectLabel(GL_BUFFER, instance_vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			0.0f, 1.0f,
			0.0f, 0.0f,
			1.0f, 1.0f,
			1.0f, 0.0f
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
	}

	auto& i = instance_data;
	instance_data.clear();
	instance_data.resize(rectangle.size() * GPU_Rectangle_Size);
	size_t off = 0;
	for (auto& x : rectangle) {
		Vector2f p = x.pos;
		Vector2f s = x.size;
		float z = x.z;
		Vector4f c = x.color;
		float r = 0;

		#define X(a, b) memcpy(instance_data.data() + off, (uint8_t*)a, b); off += b;
		X(&p, 8);  // world pos
		X(&s, 8);  // world size
		X(&r, 4);  // world rotation
		X(&z, 4);  // world z
		X(&c, 16); // color
		#undef X
	}

	glBindVertexArray(quad_vao);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		instance_data.size(),
		instance_data.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glEnableVertexAttribArray(0); // Model position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // World position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	
	glEnableVertexAttribArray(2); // World size
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)(8));
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(3); // World R
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)(16));
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(4); // World Z
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)(20));
	glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(5); // Color
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)(24));
	glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default_Batched);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);

	glBindVertexArray(quad_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate(std::span<Circle> circles) noexcept {
	constexpr size_t GPU_Instance_Size = 8 + 8 + 4 + 4 + 16;
	constexpr size_t Half_Prec = 30;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local std::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_vao = 0;
	thread_local GLuint vbo = 0;


	if (instance_vbo_size < circles.size()) {
		size_t new_size = (instance_vbo_size * 5) / 3;
		instance_vbo_size = std::max(new_size, circles.size());

		if (instance_vbo) glDeleteBuffers(1, &instance_vbo);
		glGenBuffers(1, &instance_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, instance_vbo_size * GPU_Instance_Size, NULL, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "circle_batch_instance_buffer";
		glObjectLabel(GL_BUFFER, instance_vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	if (!instance_vao) {
		const float ang_inc = 3.1415926f / Half_Prec;
		const float cos_inc = cos(ang_inc);
		const float sin_inc = sin(ang_inc);

		GLfloat vertices[2 * Half_Prec * 2];
		unsigned coord_idx = 0;

		vertices[coord_idx++] = 1.0f;
		vertices[coord_idx++] = 0.0f;

		float xc = 1.0f;
		float yc = 0.0f;
		for (unsigned i = 1; i < Half_Prec; ++i) {
			float xc_new = cos_inc * xc - sin_inc * yc;
			yc = sin_inc * xc + cos_inc * yc;
			xc = xc_new;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = yc;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = -yc;
		}

		vertices[coord_idx++] = -1.0f;
		vertices[coord_idx++] = 0.0f;

		glGenVertexArrays(1, &instance_vao);
		glBindVertexArray(instance_vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER, 2 * Half_Prec * 2 * sizeof(GLfloat), vertices, GL_STATIC_DRAW
		);
#ifdef GL_DEBUG
		auto label = "circle_info_vbo";
		glObjectLabel(GL_BUFFER, vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	auto& i = instance_data;
	instance_data.clear();
	instance_data.resize(circles.size() * GPU_Instance_Size);
	size_t off = 0;
	for (auto& x : circles) {
		Vector2f p = x.pos;
		Vector2f s = V2F(x.r);
		float z = x.z;
		Vector4f c = x.color;
		float r = 0;

		#define X(a, b) memcpy(instance_data.data() + off, (uint8_t*)a, b); off += b;
		X(&p, 8);  // world pos
		X(&s, 8);  // world size
		X(&r, 4);  // world r
		X(&z, 4);  // world z
		X(&c, 16); // color
		#undef X
	}

	glBindVertexArray(instance_vao);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		instance_data.size(),
		instance_data.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0); // Model position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // World position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	
	glEnableVertexAttribArray(2); // World size
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(8));
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(3); // World R
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(16));
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(4); // World Z
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(20));
	glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(5); // Color
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(24));
	glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.

	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default_Batched);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);

	glBindVertexArray(instance_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 2 * Half_Prec, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate(std::span<Arrow> arrows) noexcept {
	constexpr size_t GPU_Instance_Size = 8 + 8 + 4 + 4 + 16;
	constexpr size_t Half_Prec = 30;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local std::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_head_vao = 0;
	thread_local GLuint instance_quad_vao = 0;
	thread_local GLuint head_vbo = 0;
	thread_local GLuint quad_vbo = 0;


	if (instance_vbo_size < arrows.size()) {
		size_t new_size = (instance_vbo_size * 5) / 3;
		instance_vbo_size = std::max(new_size, arrows.size());

		if (instance_vbo) glDeleteBuffers(1, &instance_vbo);
		glGenBuffers(1, &instance_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, instance_vbo_size * GPU_Instance_Size, NULL, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "circle_batch_instance_buffer";
		glObjectLabel(GL_BUFFER, instance_vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	if (!instance_head_vao) {
		static float head_vertices[] = {
			+0.0f, +0.0f,
			-1.0f, +1.0f,
			-1.0f, -1.0f,
		};

		// setup plane VAO
		glGenVertexArrays(1, &instance_head_vao);
		glBindVertexArray(instance_head_vao);
		glGenBuffers(1, &head_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, head_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(head_vertices), &head_vertices, GL_STATIC_DRAW);

		static float quad_vertices[] = {
			// positions    
			0.0f, +0.5f,
			0.0f, -0.5f,
			1.0f, +0.5f,
			1.0f, -0.5f
		};

		// setup plane VAO
		glGenVertexArrays(1, &instance_quad_vao);
		glBindVertexArray(instance_quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	}

	Rectanglef view;
	view.pos = current_camera.pos - current_camera.frame_size / 2;
	view.size = current_camera.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default_Batched);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);

	instance_data.clear();
	instance_data.resize(arrows.size() * GPU_Instance_Size);
	size_t off = 0;
	for (auto& x : arrows) {
		Vector2f p = x.b;
		Vector2f s = V2F((x.a - x.b).length() / 10.f);
		float z = x.z;
		float r = (float)(x.b - x.a).angleX();
		Vector4f c = x.color;

		#define X(a, b) memcpy(instance_data.data() + off, (uint8_t*)a, b); off += b;
		X(&p, 8);  // world pos
		X(&s, 8);  // world size
		X(&r, 4);  // world r
		X(&z, 4);  // world z
		X(&c, 16); // color
		#undef X
	}

	glBindVertexArray(instance_head_vao);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		instance_data.size(),
		instance_data.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, head_vbo);
	glEnableVertexAttribArray(0); // Model position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // World position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	
	glEnableVertexAttribArray(2); // World size
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(8));
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(3); // World R
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(16));
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(4); // World Z
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(20));
	glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(5); // Color
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(24));
	glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.


	glBindVertexArray(instance_head_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 3, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < arrows.size(); ++i) {
		size_t off = i * GPU_Instance_Size;
		auto& x = arrows[i];
		Vector2f size = V2F((x.a - x.b).length() / 10.f);;
		size = { (x.a - x.b).length() - size.x, size.y };
		Vector2f pos = x.a;

		#define X(a, b) memcpy(instance_data.data() + off, (uint8_t*)a, b); off += b;
		X(&pos, 8);
		X(&size, 8);
		#undef X
	}

	glBindVertexArray(instance_quad_vao);

	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glBufferData(
		GL_ARRAY_BUFFER,
		instance_data.size(),
		instance_data.data(),
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glEnableVertexAttribArray(0); // Model position
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // World position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	
	glEnableVertexAttribArray(2); // World size
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(8));
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(3); // World R
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(16));
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(4); // World Z
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(20));
	glVertexAttribDivisor(4, 1); // tell OpenGL this is an instanced vertex attribute.

	glEnableVertexAttribArray(5); // Color
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(24));
	glVertexAttribDivisor(5, 1); // tell OpenGL this is an instanced vertex attribute.


	glBindVertexArray(instance_quad_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}

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
	shader.set_uniform("world_z", rec.z);
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
	shader.set_uniform("world_z", info.z);
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
		underline.z = info.z;
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
		sprite.z = info.z;

		Rectangle rec;
		rec.color = {0, 0, 0, 1};
		rec.pos = pos;
		rec.size = sprite.size;
		rec.pos.x -= sprite.origin.x * rec.size.x;
		rec.pos.y -= rec.size.y + (sprite.origin.y - 1) * rec.size.y;
		rec.pos.y += font.info.height * info.height / font.info.char_size;
		rec.z = info.z;

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
			0.0f, 1.0f, -1.0f,
			0.0f, 0.0f, -1.0f,
			1.0f, 1.0f, -1.0f,
			1.0f, 0.0f, -1.0f
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
	shader.set_uniform("world_z", info.z);
	shader.set_uniform("texture_rect", info.texture_rect);
	shader.set_uniform("use_normal_texture", asset::Store.get_normal(info.texture) ? 1 : 0);
	shader.set_uniform("texture_rect", info.texture_rect);
	shader.set_uniform("normal_texture", 5);
	shader.set_texture(4);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
}

struct Gpu_Vector {

	void upload(size_t size, void* data) noexcept {
		glBindBuffer(target, buffer);
		glBufferData(target, size, data, GL_STATIC_DRAW);
	}

	void fit(size_t new_size) noexcept {
		if (capacity < new_size) {
			size_t new_capacity = (capacity * 5) / 3;
			capacity = std::max(new_capacity, new_size);

			if (buffer) glDeleteBuffers(1, &buffer);
			glGenBuffers(1, &buffer);
			glBindBuffer(target, buffer);
			glBufferData(target, capacity, NULL, GL_STATIC_DRAW);
	#ifdef GL_DEBUG
			glObjectLabel(GL_BUFFER, buffer, debug_name.size(), debug_name.data());
	#endif
		}
	}

	std::string debug_name;
	GLuint buffer = 0;
	size_t capacity = 0;
	size_t target = GL_ARRAY_BUFFER;
};

void render::immediate(Model info) noexcept {
	thread_local GLuint vao = 0;
	thread_local Gpu_Vector vertices;
	thread_local Gpu_Vector indices;

	if (!vao) glGenVertexArrays(1, &vao);

	auto& object = asset::Store.get_object(info.object_id);
	auto& texture = asset::Store.get_albedo(info.texture_id);

	vertices.debug_name = "Vertex buffer object";
	vertices.target = GL_ARRAY_BUFFER;
	indices.debug_name  = "Index buffer object";
	indices.target = GL_ELEMENT_ARRAY_BUFFER;

	vertices.fit(object.vertices.size() * sizeof(Object::Vertex));
	indices.fit(object.faces.size() * sizeof(size_t));

	vertices.upload(object.vertices.size() * sizeof(Object::Vertex), object.vertices.data());
	indices.upload(object.faces.size() * sizeof(unsigned short), object.faces.data());

	if (!info.shader_id) info.shader_id = asset::Shader_Id::Default_3D;
	auto& shader = asset::Store.get_shader(info.shader_id);
	texture.bind(4);
	shader.use();
	shader.set_uniform("world_position", info.pos);
	shader.set_texture(4);

	glBindVertexArray(vao);
	glBindBuffer(vertices.target, vertices.buffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)12);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)24);

	glBindBuffer(indices.target, indices.buffer);
	glDrawElements(GL_TRIANGLES, object.vertices.size() * 3, GL_UNSIGNED_SHORT, (void*)0);

	for (size_t i = 0; i < 2; ++i) glDisableVertexAttribArray(i);
}