#include "Object.hpp"

#include <sstream>
#include <float.h>

#include "OS/file.hpp"

std::optional<Object> Object::load_from_file(
	const std::filesystem::path& path
) noexcept {
	auto opt_str = file::read_whole_text(path);
	if (!opt_str) return std::nullopt;

	auto& file = *opt_str;
	Object obj;

	size_t cursor = 0;

	std::string line;
	std::stringstream file_stream(file);

	size_t n_vertex = 0;
	size_t n_face   = 0;
	while (std::getline(file_stream, line)) {
		cursor += line.size() + 1;
		if (line.starts_with("comment")) continue;
		if (line.starts_with("element vertex")) {
			n_vertex = std::strtoull(line.data() + sizeof("element vertex"), nullptr, 10);
			continue;
		}
		if (line.starts_with("element face")) {
			n_face = std::strtoull(line.data() + sizeof("element face"), nullptr, 10);
			continue;
		}
		if (line == "end_header") break;
	}

	for (size_t i = 0; i < n_vertex; ++i) {
		thread_local Vertex temp;
		memcpy(&temp, file.data() + cursor, sizeof(Vertex));

		obj.vertices.push_back(temp);
		cursor += sizeof(Vertex);
	}

	for (size_t i = 0; i < n_face; ++i) {
		assert(file[cursor] == 3);
		cursor++;

		obj.faces.push_back((std::uint16_t)*reinterpret_cast<std::uint32_t*>(file.data() + cursor));
		cursor += 4;
		obj.faces.push_back((std::uint16_t)*reinterpret_cast<std::uint32_t*>(file.data() + cursor));
		cursor += 4;
		obj.faces.push_back((std::uint16_t)*reinterpret_cast<std::uint32_t*>(file.data() + cursor));
		cursor += 4;
	}

	Vector3f m{ FLT_MAX, FLT_MAX, FLT_MAX };
	for (auto& x : obj.vertices) {
		m.x = std::min(m.x, x.pos.x);
		m.y = std::min(m.y, x.pos.y);
		m.z = std::min(m.z, x.pos.z);
	}
	for (auto& x : obj.vertices) x.pos -= m;

	for (auto& x : obj.vertices) {
		obj.size.x = std::max(obj.size.x, x.pos.x);
		obj.size.y = std::max(obj.size.y, x.pos.y);
		obj.size.z = std::max(obj.size.z, x.pos.z);
	}

	return obj;
}
