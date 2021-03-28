#pragma once

#include "Vector.hpp"
#include "Rectangle.hpp"

template<typename T>
struct Circle {
	Vector2<T> c;
	T r;
};

using Circlef = Circle<float>;
using Circled = Circle<double>;

template<typename T>
bool is_in(const Rectangle_t<T>& rec, const Circle<T>& c) noexcept {
	Vector2<T> circle_distance = (c.c - rec.center()).applyCW([](auto x) {return std::abs(x); });

	if (circle_distance.x >=  (rec.w / 2 + c.r)) { return false; }
	if (circle_distance.y >=  (rec.h / 2 + c.r)) { return false; }
	if (circle_distance.x < (rec.w / 2))       { return true;  }
	if (circle_distance.y < (rec.h / 2))       { return true;  }

	auto corner_distance_sq =
		(circle_distance.x - rec.w / 2) * (circle_distance.x - rec.w / 2) +
		(circle_distance.y - rec.h / 2) * (circle_distance.y - rec.h / 2);

	return corner_distance_sq < c.r * c.r;
}
template<typename T>
bool is_in(const Vector2<T>& vec, const Circle<T>& c) noexcept {
	return  (c.c - vec).length2() < c.r * c.r;
}

template<typename T>
bool is_fully_in(const Rectangle_t<T>& rec, const Circle<T>& c) noexcept {
	Vector2<T> circle_distance = (c.c - rec.center()).applyCW([](auto x) {return std::abs(x); });

	if (circle_distance.x > (rec.w / 2 - c.r)) { return false; }
	if (circle_distance.y > (rec.h / 2 - c.r)) { return false; }
	return true;
}

