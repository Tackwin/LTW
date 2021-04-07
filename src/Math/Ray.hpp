#pragma once
#include <type_traits>

#include "Vector.hpp"

template<typename T>
struct Ray {
	static_assert(std::is_floating_point_v<T>);

	Vector3<T> pos;
	Vector3<T> dir;

	Vector2<T> intersect_plane() noexcept {
		Vector2<T> i;
		i.x = pos.x + dir.x * pos.z/dir.z;
		i.y = pos.y + dir.y * pos.z/dir.z;
		return i;
	}
};

using Rayf = Ray<float>;
using Rayd = Ray<double>;

static Vector2f project_plane(Vector3f from, Vector3f to) {
	return Rayf{ from, (to - from).normalize() }.intersect_plane();
}
