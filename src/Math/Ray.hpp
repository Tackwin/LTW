#pragma once
#include <type_traits>

#include "Vector.hpp"

template<typename T>
struct Ray {
	static_assert(std::is_floating_point_v<T>);

	Vector2<T> pos;
	T angle;
};

using Rayf = Ray<float>;
using Rayd = Ray<double>;
