#include "Graphic/Texture.hpp"
#include "OS/OpenGL.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

#include "xstd.hpp"

Texture::Texture() noexcept {
	GLuint temp;
	glGenTextures(1, &temp);
	info.id = temp;
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
Texture::~Texture() noexcept {
	GLuint temp = info.id;
	glDeleteTextures(1, &temp);
}

Texture::Texture(Texture&& that) noexcept {
	if (&that == this) return;

	info = that.info;
	that.info = {};
}
Texture& Texture::operator=(Texture&& that) noexcept {
	if (&that == this) return *this;

	this->~Texture();
	info = that.info;
	that.info = {};
	return *this;
}

bool Texture::load_file(std::filesystem::path path) {
	auto comp = 0;

	stbi_set_flip_vertically_on_load(true);

	int w = (int)info.width;
	int h = (int)info.height;
	std::uint8_t* data = stbi_load(
		path.string().c_str(), &w, &h, &comp, STBI_rgb_alpha
	);
	defer{ stbi_image_free(data); };
	info.width = w;
	info.height = h;

	if (!data) return false;

	bind();

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		info.width,
		info.height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);
	glGenerateMipmap(GL_TEXTURE_2D);

	return true;
}
void Texture::create_rgb_null(Vector2u size) const {
	bind();
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		NULL
	);
	glGenerateMipmap(GL_TEXTURE_2D);
}
void Texture::create_depth_null(Vector2u size) const {
	bind();
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH24_STENCIL8,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_DEPTH_STENCIL,
		GL_UNSIGNED_BYTE,
		NULL
	);
	glGenerateMipmap(GL_TEXTURE_2D);
}
void Texture::create_mask_null(Vector2u size) const {
	return;
	bind();
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_R32UI,
		(GLsizei)size.x,
		(GLsizei)size.y,
		0,
		GL_RED_INTEGER,
		GL_UNSIGNED_INT,
		NULL
	);
}

void Texture::set_parameteri(int parameter, int value) const {
	bind();
	glTexParameteri(GL_TEXTURE_2D, parameter, value);
}
void Texture::set_parameterfv(int parameter, float* value) const {
	bind();
	glTexParameterfv(GL_TEXTURE_2D, parameter, value);
}

void Texture::bind(size_t unit) const {
	glActiveTexture((GLenum)(GL_TEXTURE0 + unit));
	glBindTexture(GL_TEXTURE_2D, info.id);
}

size_t Texture::get_texture_id() const {
	return info.id;
}

Vector2u Texture::get_size() noexcept {
	return { info.width, info.height };
}

void Texture::set_resize_filter(Texture::Filter filter) noexcept{
	switch (filter)
	{
		case Filter::Linear:
			set_parameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			set_parameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
		case Filter::Nearest:
			set_parameteri(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			set_parameteri(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		break;
		default: assert(false);
	}
}

