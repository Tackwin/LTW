#include "Render.hpp"
#ifdef ES
#include <GLES3/gl3.h>
#else
#include "GL/gl3w.h"
#endif

#include "Math/Ray.hpp"
#include "Math/Matrix.hpp"

#include "Managers/AssetsManager.hpp"
#include "global.hpp"

#include "xstd.hpp"

render::Order render::current_camera;

void render::Camera3D::look_at(Vector3f target) noexcept {
	dir = target - pos;
	dir = dir.normalize();
}

Matrix4f render::Camera3D::get_view(Vector3f up) const noexcept {
	auto x = cross(dir, up).normalize();
	auto y = cross(x, dir).normalize();
	auto z = -1 * dir.normed();

	float comp[] {
		x.x, y.x, z.x, 0.f,
		x.y, y.y, z.y, 0.f,
		x.z, y.z, z.z, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	Matrix4f view(comp);
	view = view * Matrix4f::translation(-1 * pos);
	return view;
}

Matrix4f render::Camera3D::get_VP(Vector3f up) const noexcept {
	return get_projection() * get_view(up);
}

Matrix4f render::Camera3D::get_projection() const noexcept {
	return perspective(3.1415926 * fov / 180, ratio, far_, near_);
	return orthographic({-1, -1, 1, 1}, 500, 0.01f);
}

Vector2f normalize_mouse(Vector2f mouse_pos) {
	Rectanglef viewport = {0, 0, 1, 1};
	return {
		(2.f * (mouse_pos.x - viewport.x)) / viewport.w - 1.f,
		(2.f * (mouse_pos.y - viewport.y)) / viewport.h - 1.f
	};
}

Vector2f render::Camera3D::project(Vector2f mouse) noexcept {
	Vector2f p = normalize_mouse(mouse);

	Vector3f ray = {
		p.x, // (2.f * IM::getMouseScreenPos().x) / Window_Info.size.x - 1.f,
		p.y, // 1.f - (2.f * IM::getMouseScreenPos().y) / Window_Info.size.y,
		1
	};
	Vector4f ray_nds = { ray.x, ray.y, 1, 0 };
	Vector4f ray_clip = { ray_nds.x, ray_nds.y, 1, 0 };
	auto ray_eye = (*get_projection().invert()) * ray_clip;
	ray_eye.z = -1;
	ray_eye.w = 0;
	auto ray_world = (*get_view().invert()) * ray_eye;
	ray = Vector3f{ ray_world.x, ray_world.y, -ray_world.z }.normalize();

	return Rayf{pos, ray}.intersect_plane();
}



void render::Orders::push(render::Order o) noexcept {
	push(o, commands.size());
}

void render::Orders::push(render::Order o, float z) noexcept {
	o->z = z;
	commands.push_back(std::move(o));
}


void render::immediate(std::span<Rectangle> x) noexcept {
	if (current_camera.kind == render::Order::Camera3D_Kind) immediate3d(x);
	else immediate2d(x);
}
void render::immediate(std::span<Circle> x) noexcept {
	if (current_camera.kind == render::Order::Camera3D_Kind) immediate3d(x);
	else immediate2d(x);
}
void render::immediate(std::span<Arrow> x) noexcept {
	if (current_camera.kind == render::Order::Camera3D_Kind) immediate3d(x);
	else immediate2d(x);
}

void vertex_attrib_matrix(size_t off_id, size_t off_ptr, size_t stride) noexcept {
	glEnableVertexAttribArray(off_id); // Model Matrix
	glVertexAttribPointer(off_id, 4, GL_FLOAT, GL_FALSE, stride, (void*)off_ptr);
	glVertexAttribDivisor(off_id, 1); // tell OpenGL this is an instanced vertex attribute.
	off_id++;
	off_ptr += 16;

	glEnableVertexAttribArray(off_id); // Model Matrix
	glVertexAttribPointer(off_id, 4, GL_FLOAT, GL_FALSE, stride, (void*)off_ptr);
	glVertexAttribDivisor(off_id, 1); // tell OpenGL this is an instanced vertex attribute.
	off_id++;
	off_ptr += 16;

	glEnableVertexAttribArray(off_id); // Model Matrix
	glVertexAttribPointer(off_id, 4, GL_FLOAT, GL_FALSE, stride, (void*)off_ptr);
	glVertexAttribDivisor(off_id, 1); // tell OpenGL this is an instanced vertex attribute.
	off_id++;
	off_ptr += 16;

	glEnableVertexAttribArray(off_id); // Model Matrix
	glVertexAttribPointer(off_id, 4, GL_FLOAT, GL_FALSE, stride, (void*)off_ptr);
	glVertexAttribDivisor(off_id, 1); // tell OpenGL this is an instanced vertex attribute.
}

void render::immediate3d(std::span<Rectangle> rectangle) noexcept {
	constexpr size_t GPU_Rectangle_Size = 16 * 4 + 16;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint quad_vao = 0;
	thread_local GLuint quad_vbo = 0;

	auto& cam = current_camera.Camera3D_;

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
			0.0f, 1.f, 0.0f,
			0.0f, 0.f, 0.0f,
			1.0f, 1.f, 0.0f,
			1.0f, 0.f, 0.0f
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
		auto M =
			Matrix4f::translation(Vector3f(x.pos, x.z)) *
			Matrix4f::scale(Vector3f(x.size, 1));

		#define X(a) memcpy(instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(x.color); // color
		X(M);
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, GPU_Rectangle_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	vertex_attrib_matrix(2, 16, GPU_Rectangle_Size);

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Primitive_3D_Batched);
	shader.use();
	shader.set_uniform("VP", cam.get_VP());

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate3d(std::span<Circle> circles) noexcept {
	constexpr size_t GPU_Instance_Size = 16 * 4 + 16;
	constexpr size_t Half_Prec = 30;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_vao = 0;
	thread_local GLuint vbo = 0;

	auto& cam = current_camera.Camera3D_;

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

		GLfloat vertices[3 * Half_Prec * 2];
		unsigned coord_idx = 0;

		vertices[coord_idx++] = 1.0f;
		vertices[coord_idx++] = 0.0f;
		vertices[coord_idx++] = 0.0f;

		float xc = 1.0f;
		float yc = 0.0f;
		for (unsigned i = 1; i < Half_Prec; ++i) {
			float xc_new = cos_inc * xc - sin_inc * yc;
			yc = sin_inc * xc + cos_inc * yc;
			xc = xc_new;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = yc;
			vertices[coord_idx++] = 0.0f;

			vertices[coord_idx++] = xc;
			vertices[coord_idx++] = -yc;
			vertices[coord_idx++] = 0.0f;
		}

		vertices[coord_idx++] = -1.0f;
		vertices[coord_idx++] = 0.0f;
		vertices[coord_idx++] = 0.0f;

		glGenVertexArrays(1, &instance_vao);
		glBindVertexArray(instance_vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(
			GL_ARRAY_BUFFER, 3 * Half_Prec * 2 * sizeof(GLfloat), vertices, GL_STATIC_DRAW
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
	Matrix4f VP = cam.get_VP();
	for (auto& x : circles) {
		Vector3f p = Vector3f(x.pos, x.z);
		Vector3f s = {x.r, x.r, 1.f};
		Vector4f c = x.color;

		Matrix4f MVP = VP * Matrix4f::translation(p) * Matrix4f::scale(s);

		#define X(a) memcpy(instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(c); // color
		X(MVP);
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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	
	glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
	glEnableVertexAttribArray(1); // Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	vertex_attrib_matrix(2, 16, GPU_Instance_Size);

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Primitive_3D_Batched);
	shader.use();

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 2 * Half_Prec, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate3d(std::span<Arrow> arrows) noexcept {
	constexpr size_t GPU_Instance_Size = 16 * 4 + 16;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_head_vao = 0;
	thread_local GLuint instance_quad_vao = 0;
	thread_local GLuint head_vbo = 0;
	thread_local GLuint quad_vbo = 0;

	auto& cam = current_camera.Camera3D_;

	if (instance_vbo_size < arrows.size()) {
		size_t new_size = (instance_vbo_size * 5) / 3;
		instance_vbo_size = std::max(new_size, arrows.size());

		if (instance_vbo) glDeleteBuffers(1, &instance_vbo);
		glGenBuffers(1, &instance_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, instance_vbo);
		glBufferData(GL_ARRAY_BUFFER, instance_vbo_size * GPU_Instance_Size, NULL, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "arrow_batch_instance_buffer";
		glObjectLabel(GL_BUFFER, instance_vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	if (!instance_head_vao) {
		static float head_vertices[] = {
			+0.0f, +0.0f, 0.f,
			-1.0f, +1.0f, 0.f,
			-1.0f, -1.0f, 0.f
		};

		// setup plane VAO
		glGenVertexArrays(1, &instance_head_vao);
		glBindVertexArray(instance_head_vao);
		glGenBuffers(1, &head_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, head_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(head_vertices), &head_vertices, GL_STATIC_DRAW);

		static float quad_vertices[] = {
			// positions    
			0.0f, +0.5f, 0.f,
			0.0f, -0.5f, 0.f,
			1.0f, +0.5f, 0.f,
			1.0f, -0.5f, 0.f
		};

		// setup plane VAO
		glGenVertexArrays(1, &instance_quad_vao);
		glBindVertexArray(instance_quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	}

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Primitive_3D_Batched);
	shader.use();

	instance_data.clear();
	instance_data.resize(arrows.size() * GPU_Instance_Size);
	size_t off = 0;
	Matrix4f VP = cam.get_VP();
	for (auto& x : arrows) {
		Vector4f c = x.color;
		Matrix4f MVP = VP * (
			Matrix4f::translation(Vector3f(x.b, x.z)) *
			Matrix4f::scale(Vector3f(V2F((x.a - x.b).length() / 10.f), 1.f))
			* Matrix4f::rotationz((float)(x.b - x.a).angleX())
		);

		#define X(a) memcpy(instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(c); // color
		X(MVP);
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
	glEnableVertexAttribArray(1); // Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);

	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	vertex_attrib_matrix(2, 16, GPU_Instance_Size);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 3, (GLsizei)instance_vbo_size);

	instance_data.clear();
	instance_data.resize(arrows.size() * GPU_Instance_Size);
	off = 0;
	for (auto& x : arrows) {
		Vector4f c = x.color;
		Matrix4f MVP = (
			Matrix4f::translation(Vector3f(x.a, x.z)) *
			Matrix4f::scale(Vector3f(V2F((x.a - x.b).length() / 10.f), 1.f))
			* Matrix4f::rotationz((float)(x.b - x.a).angleX())
		) * VP;

		#define X(a) memcpy(instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(c); // color
		X(MVP);
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
	glEnableVertexAttribArray(1); // Color
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(1, 1); // tell OpenGL this is an instanced vertex attribute.
	vertex_attrib_matrix(2, 16, GPU_Instance_Size);

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
	
	auto& cam = current_camera.Camera_;
	Rectanglef view;
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

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

	auto& cam = current_camera.Camera_;
	Rectanglef view;
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

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


	auto& cam = current_camera.Camera_;
	Rectanglef view;
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;


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

		//immediate(rec);
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
		asset::Store.get_albedo(info.texture).bind(5);
		auto normal = asset::Store.get_normal(info.texture); if (normal) normal->bind(6);
	}

	auto& cam = current_camera.Camera_;
	Rectanglef view;
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

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
	shader.set_uniform("normal_texture", 6);
	shader.set_texture(5);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
}

struct Gpu_Vector {

	void upload(size_t size, void* data) noexcept {
		glBindBuffer(target, buffer);
		glBufferData(target, size, data, GL_DYNAMIC_DRAW);
	}

	void fit(size_t new_size) noexcept {
		if (capacity < new_size) {
			size_t new_capacity = (capacity * 5) / 3;
			capacity = std::max(new_capacity, new_size);

			if (buffer) glDeleteBuffers(1, &buffer);
			glGenBuffers(1, &buffer);
			glBindBuffer(target, buffer);
			glBufferData(target, capacity, NULL, GL_DYNAMIC_DRAW);
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

	auto& cam = current_camera.Camera3D_;

	auto scale = Vector3f(info.scale, info.scale, info.scale);
	auto model = Matrix4f::scale(Vector3f(1, 1, 1));
	model *= Matrix4f::translation(info.pos);
	model *= Matrix4f::scale(Vector3f(info.scale, info.scale, info.scale));

	if (!info.shader_id) info.shader_id = asset::Shader_Id::Default_3D;
	auto& shader = asset::Store.get_shader(info.shader_id);
	texture.bind(5);
	shader.use();
	shader.set_texture(5);
	shader.set_uniform("MVP", cam.get_VP() * model);

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

	for (size_t i = 0; i < 3; ++i) glDisableVertexAttribArray(i);
}


void render::immediate2d(std::span<Rectangle> rectangle) noexcept {
	constexpr size_t GPU_Rectangle_Size = 8 + 8 + 4 + 4 + 16;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint quad_vao = 0;
	thread_local GLuint quad_vbo = 0;

	auto& cam = current_camera.Camera_;

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
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default_Batched);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);

	glBindVertexArray(quad_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate2d(std::span<Circle> circles) noexcept {
	constexpr size_t GPU_Instance_Size = 8 + 8 + 4 + 4 + 16;
	constexpr size_t Half_Prec = 30;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_vao = 0;
	thread_local GLuint vbo = 0;

	auto& cam = current_camera.Camera_;

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
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Default_Batched);
	shader.use();
	shader.set_window_size(Environment.window_size);
	shader.set_view(view);

	glBindVertexArray(instance_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 2 * Half_Prec, (GLsizei)instance_vbo_size);

	for (size_t i = 0; i < 6; ++i) glDisableVertexAttribArray(i);
}
void render::immediate2d(std::span<Arrow> arrows) noexcept {
	constexpr size_t GPU_Instance_Size = 8 + 8 + 4 + 4 + 16;
	constexpr size_t Half_Prec = 30;

	thread_local GLuint instance_vbo = 0;
	thread_local size_t instance_vbo_size = 0;
	thread_local xstd::vector<std::uint8_t> instance_data;

	thread_local GLuint instance_head_vao = 0;
	thread_local GLuint instance_quad_vao = 0;
	thread_local GLuint head_vbo = 0;
	thread_local GLuint quad_vbo = 0;

	auto& cam = current_camera.Camera_;

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
	view.pos = cam.pos - cam.frame_size / 2;
	view.size = cam.frame_size;

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
void render::immediate(
	std::span<Model> models, const Batch& batch, const Camera3D& cam
) noexcept {
	constexpr size_t GPU_Instance_Size = 16 * 4 * 2 + 4 + 4 * 3;

	thread_local xstd::vector<std::uint8_t> host_instance_data;
	thread_local Gpu_Vector                device_instance_data;

	thread_local GLuint vao = 0;
	thread_local Gpu_Vector vertices;
	thread_local Gpu_Vector indices;

	if (!vao) glGenVertexArrays(1, &vao);
	if (models.empty()) return;

	size_t shader_id = models.front().shader_id;
	if (!shader_id) shader_id = asset::Shader_Id::Default_3D_Batched;

	auto& texture = asset::Store.get_albedo(batch.texture_id);
	auto& object  = asset::Store.get_object(batch.object_id);
	auto& shader  = asset::Store.get_shader(batch.shader_id);

	vertices.debug_name = "Vertex buffer object";
	vertices.target = GL_ARRAY_BUFFER;
	indices.debug_name  = "Index buffer object";
	indices.target = GL_ELEMENT_ARRAY_BUFFER;
	device_instance_data.debug_name = "Instance data";
	device_instance_data.target = GL_ARRAY_BUFFER;

	device_instance_data.fit(models.size() * GPU_Instance_Size);
	vertices.fit(object.vertices.size() * sizeof(Object::Vertex));
	indices.fit(object.faces.size() * sizeof(size_t));

	vertices.upload(object.vertices.size() * sizeof(Object::Vertex), object.vertices.data());
	indices.upload(object.faces.size() * sizeof(unsigned short), object.faces.data());


	glBindVertexArray(vao);
	glBindBuffer(vertices.target, vertices.buffer);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)12);

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Object::Vertex), (void*)24);

	glBindBuffer(GL_ARRAY_BUFFER, device_instance_data.buffer);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 1, GL_UNSIGNED_INT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(3, 1); // tell OpenGL this is an instanced vertex attribute.

	vertex_attrib_matrix(4, 4, GPU_Instance_Size);
	vertex_attrib_matrix(8, 16 * 4 + 4, GPU_Instance_Size);

	glEnableVertexAttribArray(12);
	glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, GPU_Instance_Size, (void*)(16 * 4 * 2 + 4));
	glVertexAttribDivisor(12, 1); // tell OpenGL this is an instanced vertex attribute.

	texture.bind(5);
	shader.use();
	shader.set_texture(5);
	shader.set_uniform("VP", cam.get_VP());
	if (models.front().object_blur) {
		auto t = cam;
		t.pos = cam.last_pos;
		shader.set_uniform("last_VP", t.get_VP());
	} else {
		shader.set_uniform("last_VP", cam.get_VP());
	}

	constexpr size_t Batch_Size = 10'000;

	for (size_t i = 0; i < models.size(); i += Batch_Size) {

	auto batch = std::span<Model>(models.begin() + i, std::min(models.size() - i, Batch_Size));

	Matrix4f VP = cam.get_VP();

	auto offset = [&](Vector3f origin) {
		return object.size.hadamard(origin) * -1.f;
	};

	host_instance_data.clear();
	host_instance_data.resize(batch.size() * GPU_Instance_Size);
	size_t off = 0;
	for (auto& x : batch) {
		uint32_t tag = x.object_id;
		auto M =
			Matrix4f::translation(x.pos) *
			Matrix4f::scale({x.scale, x.scale, x.scale}) *
			Matrix4f::rotate({1, 0, 0}, x.dir) *
			Matrix4f::translation(offset(x.origin));
		Matrix4f last_M = M;
		if (x.object_blur){
			last_M = Matrix4f::translation(x.last_pos) *
				Matrix4f::scale({x.last_scale, x.last_scale, x.last_scale}) *
				Matrix4f::rotate({1, 0, 0}, x.last_dir) *
				Matrix4f::translation(offset(x.origin));
		}

		#define X(a)\
			memcpy(host_instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(tag);
		X(M);
		X(last_M);
		X(x.color);
		#undef X
	}

	device_instance_data.upload(host_instance_data.size(), host_instance_data.data());

	glBindBuffer(indices.target, indices.buffer);
	glDrawElementsInstanced(
		GL_TRIANGLES, object.faces.size(), GL_UNSIGNED_SHORT, (void*)0, batch.size()
	);

	}
	for (size_t i = 0; i < 13; ++i) glDisableVertexAttribArray(i);
}

void render::immediate(Ring ring, const Camera3D& cam) noexcept {
	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			-1.0f, +1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			+1.0f, +1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			+1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)12);
	}
	auto M =
		Matrix4f::translation(ring.pos) * Matrix4f::scale({ring.radius, ring.radius, ring.radius});

	auto& shader = asset::Store.get_shader(asset::Shader_Id::Ring);
	shader.use();
	shader.set_uniform("M", M);
	shader.set_uniform("VP", cam.get_VP());
	shader.set_uniform("color", ring.color);
	shader.set_uniform("glow_intensity", ring.glowing_intensity);

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render::immediate(Particle particle, const Camera3D& cam, bool velocity) noexcept {
	if (velocity) if (!particle.velocity) return;
	if (!velocity) if (!particle.bloom)   return;

	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			-1.0f, +1.0f, 0.0f, -1.0f, +1.0f,
			-1.0f, -1.0f, 0.0f, -1.0f, -1.0f,
			+1.0f, +1.0f, 0.0f, +1.0f, +1.0f,
			+1.0f, -1.0f, 0.0f, +1.0f, -1.0f
		};

		// setup plane VAO
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "Particle rectangle VBO";
		glObjectLabel(GL_BUFFER, quad_vbo, (GLsizei)strlen(label) - 1, label);
#endif
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)12);
	}
	Matrix4f M;
	if (particle.face_camera) {
		M =
			Matrix4f::translation(particle.pos) *
			Matrix4f::scale(particle.scale) *
			Matrix4f::rotate({0, 0, 1}, (cam.pos - particle.pos).normed());
	} else {
		M =
			Matrix4f::translation(particle.pos) *
			Matrix4f::scale(particle.scale) *
			Matrix4f::rotate({0, 0, 1}, particle.dir);
	}

	if (velocity) {
		auto& shader = asset::Store.get_shader(asset::Shader_Id::Particle_Deferred);
		shader.use();
		shader.set_uniform("M", M);
		shader.set_uniform("VP", cam.get_VP());
		shader.set_uniform("intensity", particle.intensity);
		shader.set_uniform("u_velocity", *particle.velocity);
		shader.set_uniform("radial_velocity", particle.radial_velocity);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glColorMaski(3, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	} else {
		auto& shader = asset::Store.get_shader(asset::Shader_Id::Particle_Bloom);
		shader.use();
		shader.set_uniform("M", M);
		shader.set_uniform("VP", cam.get_VP());
		shader.set_uniform("intensity", particle.intensity);
		shader.set_uniform("color", *particle.bloom);
	}

	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (velocity) {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}
}

void render::immediate(
	xstd::span<World_Sprite> world_sprite, const Batch& batch, const Camera3D& cam
) noexcept {
	static GLuint quad_vao{ 0 };
	static GLuint quad_vbo{ 0 };

	constexpr size_t GPU_Instance_Size = 16 * 4 + 4;
	thread_local xstd::vector<std::uint8_t> host_instance_data;
	thread_local Gpu_Vector                device_instance_data;

	if (!quad_vao) {
		static float quad_vertices[] = {
			// positions    
			-0.5f, +0.5f, 0.0f, -0.0f, +1.0f,
			-0.5f, -0.5f, 0.0f, -0.0f, -0.0f,
			+0.5f, +0.5f, 0.0f, +1.0f, +1.0f,
			+0.5f, -0.5f, 0.0f, +1.0f, -0.0f
		};

		// setup plane VAO
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
#ifdef GL_DEBUG
		auto label = "World Sprite rectangle VBO";
		glObjectLabel(GL_BUFFER, quad_vbo, (GLsizei)strlen(label) - 1, label);
#endif
	}

	auto& texture = asset::Store.get_albedo(batch.texture_id);
	auto& shader  = asset::Store.get_shader(batch.shader_id);

	device_instance_data.debug_name = "Instance data";
	device_instance_data.target = GL_ARRAY_BUFFER;
	device_instance_data.fit(world_sprite.size * GPU_Instance_Size);

	glBindVertexArray(quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);

	glBindBuffer(GL_ARRAY_BUFFER, device_instance_data.buffer);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, GPU_Instance_Size, (void*)0);
	glVertexAttribDivisor(2, 1); // tell OpenGL this is an instanced vertex attribute.

	vertex_attrib_matrix(3, 4, GPU_Instance_Size);

	texture.bind(5);
	shader.use();
	shader.set_texture(5);
	shader.set_uniform("VP", cam.get_VP());

	auto offset = [&](Vector3f origin) { return origin * -1.f; };

	host_instance_data.clear();
	host_instance_data.resize(world_sprite.size * GPU_Instance_Size);
	size_t off = 0;
	auto color = V3F(1);
	for (auto& x : world_sprite) {
		uint32_t tag = 0;
		auto M =
			Matrix4f::translation(x.pos) *
			Matrix4f::scale(V3F(x.size)) *
			Matrix4f::rotate({0, 0, 1}, cam.dir * -1.f);
		#define X(a)\
			memcpy(host_instance_data.data() + off, (uint8_t*)&a, sizeof(a)); off += sizeof(a);
		X(tag);
		X(M);
		#undef X
	}

	device_instance_data.upload(host_instance_data.size(), host_instance_data.data());

	glBindVertexArray(quad_vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, world_sprite.size);

	for (size_t i = 0; i < 13; ++i) glDisableVertexAttribArray(i);
}
