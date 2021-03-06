#include "FrameBuffer.hpp"

#include <cassert>
#include <stdio.h>
#include "OS/OpenGL.hpp"

Frame_Buffer::Frame_Buffer(Vector2u size, size_t n_samples, std::string label) noexcept {
	this->size = size;
	this->n_samples = n_samples;

	GLuint x = buffer; glGenFramebuffers(1, &x); buffer = x;
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
#ifdef GL_DEBUG
	glObjectLabel(GL_FRAMEBUFFER, buffer, label.size(), label.data());
#endif

	x = depth_attachment; glGenRenderbuffers(1, &x); depth_attachment = x;
	glBindRenderbuffer(GL_RENDERBUFFER, depth_attachment);
	glRenderbufferStorageMultisample(
		GL_RENDERBUFFER, n_samples, GL_DEPTH_COMPONENT32F, (GLsizei)size.x, (GLsizei)size.y
	);
	glFramebufferRenderbuffer(
		GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_attachment
	);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		assert("Framebuffer not complete!");
	}
#ifdef GL_DEBUG
	std::string l = label + " Depth";
	glObjectLabel(GL_RENDERBUFFER, depth_attachment, l.size(), l.data());
#endif

	float quad_vertices[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	// setup plane VAO
	x = quad_vao; glGenVertexArrays(1, &x); quad_vao = x;
	x = quad_vbo; glGenBuffers(1, &x); quad_vbo = x;
	glBindVertexArray(quad_vao);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void Frame_Buffer::clear() noexcept {
	unsigned int attachments[16];
	for (size_t i = 0; i < color_attachment.size(); ++i) attachments[i] = GL_COLOR_ATTACHMENT0 + i;

	glDrawBuffers(color_attachment.size(), attachments);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Frame_Buffer::new_color_attach(size_t format, std::string label) noexcept {
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
	glTexStorage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		n_samples,
		format,
		size.x,
		size.y,
		GL_TRUE
	);
#ifdef GL_DEBUG
	glObjectLabel(GL_TEXTURE, id, label.size(), label.data());
#endif

	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_COLOR_ATTACHMENT0 + color_attachment.size(),
		GL_TEXTURE_2D_MULTISAMPLE,
		id,
		0
	);

	color_attachment.push_back(id);
}

void Frame_Buffer::set_active() noexcept {
	unsigned int attachments[16];
	for (size_t i = 0; i < color_attachment.size(); ++i) attachments[i] = GL_COLOR_ATTACHMENT0 + i;

	glDrawBuffers(color_attachment.size(), attachments);
	for (size_t i = 0; i < color_attachment.size(); ++i) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color_attachment[i]);
	}
}

void Frame_Buffer::set_active_texture(size_t texture, size_t id) noexcept {
	glActiveTexture(GL_TEXTURE0 + texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color_attachment[id]);
}
void Frame_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

G_Buffer::G_Buffer(Vector2u size, size_t n_samples) noexcept {
	this->size = size;
	this->n_samples = n_samples;

	glGenFramebuffers(1, &g_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
#ifdef GL_DEBUG
	auto label = "geometry_buffer";
	glObjectLabel(GL_FRAMEBUFFER, g_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - color + specular color buffer
	glGenTextures(1, &albedo_buffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, albedo_buffer);
	glTexStorage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		n_samples,
		GL_RGB8,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_TRUE
	);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, albedo_buffer, 0
	);
#ifdef GL_DEBUG
	label = "geometry_color + specular";
	glObjectLabel(GL_TEXTURE, albedo_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - normal color buffer
	glGenTextures(1, &normal_buffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, normal_buffer);
	glTexStorage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		n_samples,
		GL_RGB8_SNORM,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_TRUE
	);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, normal_buffer, 0
	);
#ifdef GL_DEBUG
	label = "geometry_normal_color";
	glObjectLabel(GL_TEXTURE, normal_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - position color buffer
	glGenTextures(1, &pos_buffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, pos_buffer);
	glTexStorage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		n_samples,
		GL_RGB16F,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_TRUE
	);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D_MULTISAMPLE, pos_buffer, 0
	);
#ifdef GL_DEBUG
	label = "geometry_pos_buffer";
	glObjectLabel(GL_TEXTURE, pos_buffer, (GLsizei)strlen(label) - 1, label);
#endif


	// - velocity color buffer
	glGenTextures(1, &velocity_buffer);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, velocity_buffer);
	glTexStorage2DMultisample(
		GL_TEXTURE_2D_MULTISAMPLE,
		n_samples,
		GL_RG16F,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_TRUE

	);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D_MULTISAMPLE, velocity_buffer, 0
	);
#ifdef GL_DEBUG
	label = "Geometry Velocity Buffer";
	glObjectLabel(GL_TEXTURE, velocity_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	// - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2,
		GL_COLOR_ATTACHMENT3,
		GL_COLOR_ATTACHMENT4
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);

	glGenRenderbuffers(1, &depth_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
	glRenderbufferStorageMultisample(
		GL_RENDERBUFFER, n_samples, GL_DEPTH_COMPONENT32F, (GLsizei)size.x, (GLsizei)size.y
	);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		assert("Framebuffer not complete!");
	}
#ifdef GL_DEBUG
	label = "geometry_depth";
	glObjectLabel(GL_RENDERBUFFER, depth_rbo, (GLsizei)strlen(label) - 1, label);
#endif


	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
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
	glDeleteTextures(1, &velocity_buffer);
	glDeleteVertexArrays(1, &quad_VAO);
	glDeleteBuffers(1, &quad_VBO);
}

void G_Buffer::clear(Vector4d color) noexcept {
	Vector4f f = (Vector4f)color;
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);
	glClear(GL_COLOR_BUFFER_BIT);
}

void G_Buffer::set_active() noexcept {
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
	unsigned int attachments[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);
}

void G_Buffer::set_active_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, albedo_buffer);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, normal_buffer);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, pos_buffer);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, velocity_buffer);
}

void G_Buffer::set_disable_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void G_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void G_Buffer::copy_depth_to(uint32_t id) noexcept {
	static GLenum b = GL_BACK;

	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); // write to default framebuffer
	// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
	// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
	// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
	glDrawBuffers(1, &b);
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

void G_Buffer::copy_draw_to(uint32_t from_id, uint32_t dest_id, uint32_t color) noexcept {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, from_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest_id);

	glReadBuffer(GL_COLOR_ATTACHMENT0 + color);
	glBlitFramebuffer(
		0,
		0,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		0,
		(GLsizei)size.x,
		(GLsizei)size.y,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR
	);
}

void G_Buffer::copy_depth_to(uint32_t id, Rectanglef v) noexcept {
	static GLenum b = GL_BACK;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id); // write to default framebuffer
	// blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
	// the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
	// depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
	glDrawBuffers(1, &b);
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


HDR_Buffer::HDR_Buffer(
	Vector2u size, size_t n_color, std::string label
) noexcept : size(size), n_color(n_color) {
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_DEBUG
	auto l = label + " Color 0";
	glObjectLabel(GL_TEXTURE, color_buffer, l.size(), l.data());
#endif
	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, hdr_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffer, 0);

	if (n_color > 1) {
		glGenTextures(1, &color2_buffer);
		glBindTexture(GL_TEXTURE_2D, color2_buffer);
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	#ifdef GL_DEBUG
		l = label + " Color 1";
		glObjectLabel(GL_TEXTURE, color2_buffer, l.size(), l.data());
	#endif
	}

	// attach buffers
	glBindFramebuffer(GL_FRAMEBUFFER, hdr_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, color2_buffer, 0);
#ifdef GL_DEBUG
	glObjectLabel(GL_FRAMEBUFFER, hdr_buffer, label.size(), label.data());
#endif
	unsigned int attachments[2] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
	};
	glDrawBuffers(n_color, attachments);

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
	unsigned int attachments[2] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1
	};
	glDrawBuffers(sizeof(attachments) / sizeof(unsigned int), attachments);
}
void HDR_Buffer::set_active_texture() noexcept {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
	if (n_color > 1) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, color2_buffer);
	}
}
void HDR_Buffer::set_active_texture(size_t n) noexcept {
	glActiveTexture(GL_TEXTURE0 + n);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
	if (n_color > 1) {
		glActiveTexture(GL_TEXTURE0 + n + 1);
		glBindTexture(GL_TEXTURE_2D, color2_buffer);
	}
}

void HDR_Buffer::render_quad() noexcept {
	glBindVertexArray(quad_VAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

uint32_t HDR_Buffer::get_depth_id() const noexcept {
	return rbo_buffer;
}

Texture_Buffer::Texture_Buffer(Vector2u size, std::string label) noexcept {
	glGenFramebuffers(1, &frame_buffer);

	texture.create_rgb_null(size);
	texture.set_parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	texture.set_parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_DEBUG
	glObjectLabel(GL_TEXTURE, texture.get_texture_id(), label.size(), label.data());
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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32F, (GLsizei)size.x, (GLsizei)size.y);
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

void Texture_Buffer::set_active_texture(size_t n) noexcept {
	glActiveTexture(GL_TEXTURE0 + n);
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
#ifdef GL_DEBUG
	auto label = "SSAO Buffer";
	glObjectLabel(GL_FRAMEBUFFER, ssao_buffer, (GLsizei)strlen(label) - 1, label);
#endif

	glGenTextures(1, &color_buffer);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
#ifdef GL_DEBUG
	label = "SSAO Texture";
	glObjectLabel(GL_TEXTURE, color_buffer, (GLsizei)strlen(label) - 1, label);
#endif
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

void SSAO_Buffer::set_active_texture_for_blur(size_t n) noexcept {
	glActiveTexture(GL_TEXTURE0 + n);
	glBindTexture(GL_TEXTURE_2D, color_buffer);
}

void SSAO_Buffer::set_active_texture(size_t n) noexcept {
	glActiveTexture((GLenum)(GL_TEXTURE0 + n));
	glBindTexture(GL_TEXTURE_2D, blur_buffer);
}
