#pragma once
#include <vector>
#include <optional>
#include <filesystem>

#include "Math/Vector.hpp"

struct Object {

	Object() = default;
	Object(Object&) = delete;
	Object& operator=(Object&) = delete;

	Object(Object&& other) noexcept {
		this->~Object();
		memcpy(this, &other, sizeof(other));
		memset(&other, 0, sizeof(other));
	}
	Object& operator=(Object&& other) noexcept {
		this->~Object();
		memcpy(this, &other, sizeof(other));
		memset(&other, 0, sizeof(other));
		return *this;
	}

	static std::optional<Object> load_from_file(
		const std::filesystem::path& path
	) noexcept;

	std::filesystem::path material_file;
	std::string material;

	std::string name;

	struct Vertex {
		Vector3f pos;
		Vector3f normal;
		Vector2f uv;
	};
	std::vector<Vertex> vertices;
	std::vector<unsigned short> faces;
};