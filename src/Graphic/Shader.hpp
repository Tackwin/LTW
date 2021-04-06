#pragma once

#include <string>
#include <string>
#include <optional>
#include <filesystem>
#include <string_view>

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

struct ShaderInfo {
	size_t vertexId   = 0;
	size_t programId  = 0;
	size_t geometryId = 0;
	size_t fragmentId = 0;

	bool vertex_compiled = false;
	bool vertex_linked   = false;

	bool fragment_compiled = false;
	bool fragment_linked   = false;
	
	bool geometry_compiled = false;
	bool geometry_linked   = false;
};

class Shader {
public:
	static std::optional<Shader> create_shader(
		std::filesystem::path vertex, std::filesystem::path fragment
	) noexcept;
	static std::optional<Shader> create_shader(
		std::filesystem::path vertex, std::filesystem::path fragment, std::filesystem::path geometry
	) noexcept;

public:
	Shader() noexcept;
	~Shader() noexcept;
	
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	Shader(Shader&& that) noexcept;
	Shader& operator=(Shader&& that) noexcept;

	void use() const noexcept;

	void set_window_size(Vector2u size) noexcept;
	void set_view(Rectanglef view) noexcept;
	void set_primary_color(Vector4f color) noexcept;
	void set_rotation(float rotation) noexcept;
	void set_origin(Vector2f origin) noexcept;
	void set_position(Vector2f pos) noexcept;
	void set_size(Vector2f size) noexcept;
	void set_texture(size_t id) noexcept;

	void set_use_texture(bool x) noexcept;

	bool load_vertex(std::filesystem::path path) noexcept;
	bool load_fragment(std::filesystem::path path) noexcept;
	bool load_geometry(std::filesystem::path path) noexcept;
	bool build_shaders() noexcept;


	void set_uniform(const std::string& name, Rectanglef x) noexcept;
	void set_uniform(const std::string& name, Vector2f x) noexcept;
	void set_uniform(const std::string& name, Vector3f x) noexcept;
	void set_uniform(const std::string& name, Vector4u x) noexcept;
	void set_uniform(const std::string& name, Vector4d x) noexcept;
	void set_uniform(const std::string& name, float x) noexcept;
	void set_uniform(const std::string& name, int x) noexcept;

private:
	std::unordered_map<std::string, size_t> cache_loc;

	std::optional<std::string> check_shader_error(size_t shader) noexcept;
	std::optional<std::string> check_program_error(size_t program) noexcept;

	ShaderInfo info;
};

