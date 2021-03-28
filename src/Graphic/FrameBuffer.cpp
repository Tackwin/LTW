#include "FrameBuffer.hpp"

#include <cassert>
#include <stdio.h>

#ifdef ES
#include <GLES3/gl3.h>
#else
#include <GL/glew.h>
#endif

G_Buffer::G_Buffer(Vector2u size) noexcept : size(size) {
	glGenFramebuffers(1, &g_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
#ifdef GL_DEBUG
	auto label = "geometry_buffer";
	glObjectLabel(GL_FRAMEBUFFER, g_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - color + specular color buffer
	glGenTextures(1, &albedo_buffer);
	glBindTexture(GL_TEXTURE_2D, albedo_buffer);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_buffer, 0);
#ifdef GL_DEBUG
	label = "geometry_color + specular";
	glObjectLabel(GL_TEXTURE, albedo_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - normal color buffer
	glGenTextures(1, &normal_buffer);
	glBindTexture(GL_TEXTURE_2D, normal_buffer);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA16F,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGBA,
		GL_FLOAT,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_buffer, 0);
#ifdef GL_DEBUG
	label = "geometry_normal_color";
	glObjectLabel(GL_TEXTURE, normal_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - position color buffer
	glGenTextures(1, &pos_buffer);
	glBindTexture(GL_TEXTURE_2D, pos_buffer);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA16F,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGBA,
		GL_FLOAT,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, pos_buffer, 0);
#ifdef GL_DEBUG
	label = "geometry_pos_buffer";
	glObjectLabel(GL_TEXTURE, pos_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);

	glGenRenderbuffers(1, &depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, (GLsizei)size.x, (GLsizei)size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		assert("Framebuffer not complete!");
	}
#ifdef GL_DEBUG
	label = "geometry_depth";
	glObjectLabel(GL_RENDERBUFFER, depth_rbo, (GLsizei)strlen(label) - 1, label);
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	float quad_vertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quad_VAO);
	glGenBuffers(1, &quad_VBO);
	glBindVertexArray(quad_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}
G_Buffer::~G_Buffer() noexcept {
	glDeleteFramebuffers(1, &g_buffer);
	glDeleteRenderbuffers(1, &depth_rbo);
	glDeleteTextures(1, &pos_buffer);
	glDeleteTextures(1, &normal_buffer);
	glDeleteTextures(1, &albedo_buffer);
	glDeleteVertexArrays(1, &quad_VAO);
	glDeleteBuffers(1, &quad_VBO);
}

void G_Buffer::clear(Vector4d color) noexcept {
	Vector4f f = (Vector4f)color;
#ifdef ES
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);
	glClear(GL_COLOR_BUFFER_BIT);
#else
	glClearTexImage(albedo_buffer, 0, GL_RGBA, GL_UNSIGNED_BYTE, &f.x);
	glClearTexImage(normal_buffer, 0, GL_RGBA, GL_FLOAT, nullptr);
	glClearTexImage(pos_buffer, 0, GL_RGBA, GL_FLOAT, nullptr);
#endif
}

void G_Buffer::set_active() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);
}

void G_Buffer::set_active_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo_buffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal_buffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pos_buffer);
}

void G_Buffer::set_disable_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void G_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void G_Buffer::copy_depth_to(uint32_t id) noexcept {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); // write to default framebuffer
	// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
	// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
	// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
	glBlitFramebuffer(
		0,
		0,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		0,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}

void G_Buffer::copy_depth_to(uint32_t id, Rectanglef v) noexcept {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); // write to default framebuffer
	// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
	// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
	// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
	glBlitFramebuffer(
		0,
		0,
		(GLsizei)size.x,
		(GLsizei)size.y,
		(GLsizei)(v.x * size.x),
		(GLsizei)(v.y * size.y),
		(GLsizei)((v.x + v.w) * size.x),
		(GLsizei)((v.y + v.h) * size.y),
		GL_DEPTH_BUFFER_BIT,
		GL_NEAREST
	);
	glBindFramebuffer(GL_FRAMEBUFFER, id);
}


HDR_Buffer::HDR_Buffer(Vector2u size) noexcept : size(size) {
	glGenFramebuffers(1, &hdr_buffer);

	glGenTextures(1, &color_buffer);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA16F,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGBA,
		GL_FLOAT,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifdef GL_DEBUG
	auto label = "hdr_color_buffer";
	glObjectLabel(GL_TEXTURE, color_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, hdr_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);
#ifdef GL_DEBUG
	label = "hdr_frame_buffer";
	glObjectLabel(GL_FRAMEBUFFER, hdr_buffer, (GLsizei)strlen(label) - 1, label);
#endif
	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);

	glGenRenderbuffers(1, &rbo_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, (GLsizei)size.x, (GLsizei)size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		assert("Framebuffer not complete!");
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	float quad_vertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quad_VAO);
	glGenBuffers(1, &quad_VBO);
	glBindVertexArray(quad_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

}

HDR_Buffer::~HDR_Buffer() noexcept {
	glDeleteFramebuffers(1, &hdr_buffer);
	glDeleteTextures(1, &color_buffer);
}

Vector2u HDR_Buffer::get_size() const noexcept {
	return size;
}

void HDR_Buffer::set_active() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, hdr_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_buffer);
	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);
}
void HDR_Buffer::set_active_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
}
void HDR_Buffer::set_disable_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void HDR_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

uint32_t HDR_Buffer::get_depth_id() const noexcept {
	return rbo_buffer;
}

Texture_Buffer::Texture_Buffer(Vector2u size) noexcept {
	glGenFramebuffers(1, &frame_buffer);
	auto label = "texture_target_frame_buffer";

	texture.create_rgb_null(size);
#ifdef GL_DEBUG
	label = "texture_target";
	glObjectLabel(GL_TEXTURE, texture.get_texture_id(), (GLsizei)strlen(label) - 1, label);
#endif
	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get_texture_id(), 0
	);

	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);

	glGenRenderbuffers(1, &rbo_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, (GLsizei)size.x, (GLsizei)size.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		assert("Framebuffer not complete!");
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	float quad_vertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quad_VAO);
	glGenBuffers(1, &quad_VBO);
	glBindVertexArray(quad_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

Texture_Buffer::~Texture_Buffer() noexcept {
	glDeleteFramebuffers(1, &frame_buffer);
}

const Texture& Texture_Buffer::get_texture() const noexcept {
	return texture;
}

void Texture_Buffer::set_active() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_buffer);
	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);
}

uint32_t Texture_Buffer::get_frame_buffer_id() const noexcept {
	return frame_buffer;
}

void Texture_Buffer::clear(Vector4f color) noexcept {
	glClearColor(COLOR_UNROLL(color));
	glClear(GL_COLOR_BUFFER_BIT);
}

void Texture_Buffer::set_active_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.get_texture_id());
}

void Texture_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}


SSAO_Buffer::SSAO_Buffer(Vector2u size) noexcept {
	glGenFramebuffers(1, &ssao_buffer);
	glGenFramebuffers(1, &ssao_blur_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_buffer);

	glGenTextures(1, &color_buffer);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGB,
		GL_FLOAT,
		NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_buffer);
	glGenTextures(1, &blur_buffer);
	glBindTexture(GL_TEXTURE_2D, blur_buffer);
	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RED, (GLsizei)size.x, (GLsizei)size.y, 0, GL_RGB, GL_FLOAT, NULL
	);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_buffer, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	float quad_vertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	glGenVertexArrays(1, &quad_VAO);
	glGenBuffers(1, &quad_VBO);
	glBindVertexArray(quad_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, quad_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

SSAO_Buffer::~SSAO_Buffer() noexcept {
	glDeleteFramebuffers(1, &ssao_buffer);
	glDeleteFramebuffers(1, &ssao_blur_buffer);
	glDeleteTextures(1, &color_buffer);
	glDeleteTextures(1, &blur_buffer);
}

void SSAO_Buffer::set_active_ssao() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_buffer);
	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);
}
void SSAO_Buffer::set_active_blur() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_buffer);
	unsigned int attachments[1] = {
		GL_COLOR_ATTACHMENT0
	};
	glDrawBuffers(1, attachments);
}

void SSAO_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void SSAO_Buffer::set_active_texture_for_blur() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
}

void SSAO_Buffer::set_active_texture(size_t n) noexcept {
	glActiveTexture((GLenum)(GL_TEXTURE0 + n));
	glBindTexture(GL_TEXTURE_2D, blur_buffer);
}
