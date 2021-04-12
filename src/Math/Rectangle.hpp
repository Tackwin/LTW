#pragma once
#include "Vector.hpp"

#include <algorithm>
#include <array>
#include <tuple>

template<typename T>
struct Rectangle_t {
	using my_type_t = Rectangle_t<T>;
	
	enum class Side {
		Left = 0,
		Right,
		Top,
		Bot,
		None,
		Size
	};


	constexpr static Vector2<T> get_normal(Side s) noexcept {
		switch (s) {
		case Side::Left: return { -1, 0 };
		case Side::Top: return { 0, 1 };
		case Side::Right: return { 1, 0 };
		case Side::Bot: return { 0, -1 };
		default: return { 0, 0 };
		}
	}

	constexpr static Vector2<T> get_tangent(Side s) noexcept {
		switch (s) {
		case Side::Left: return { 1, 0 };
		case Side::Top: return { 0, 1 };
		case Side::Right: return { 1, 0 };
		case Side::Bot: return { 0, 1 };
		default: return { 0, 0 };
		}
	}

	constexpr static Rectangle_t<T> centered(Vector2<T> p, Vector2<T> size) noexcept {
		Rectangle_t<T> result;
		result.size = size;
		result.setCenter(p);
		return result;
	}

	union {
		struct {
			Vector<2, T> pos;
			Vector<2, T> size;
		};
		struct {
			T x;
			T y;
			T w;
			T h;
		};
	};

	constexpr Rectangle_t() {}
	constexpr Rectangle_t(T x, T y, T w, T h) noexcept : x(x), y(y), w(w), h(h) {
		if constexpr (!std::is_unsigned_v<T>) {
			if (this->size.x < 0) {
				this->pos.x += this->size.x;
				this->size.x = -this->size.x;
			}
			if (this->size.y < 0) {
				this->pos.y += this->size.y;
				this->size.y = -this->size.y;
			}
		}
	}
	constexpr Rectangle_t(const Vector<2, T>& pos, const Vector<2, T>& size) :
		pos(pos),
		size(size)
	{
		if constexpr (!std::is_unsigned_v<T>) {
			if (this->size.x < 0) {
				this->pos.x += this->size.x;
				this->size.x = -this->size.x;
			}
			if (this->size.y < 0) {
				this->pos.y += this->size.y;
				this->size.y = -this->size.y;
			}
		}
	}

	Rectangle_t(const Rectangle_t&) = default;
	Rectangle_t& operator=(const Rectangle_t&) = default;
	Rectangle_t(Rectangle_t&&) = default;
	Rectangle_t& operator=(Rectangle_t&&) = default;

	constexpr Rectangle_t<T> canonical() const noexcept {
		auto ret = *this;
		ret.x = std::min(x, x + w);
		ret.y = std::min(y, y + h);
		ret.w = std::abs(w);
		ret.h = std::abs(h);
		return ret;
	}

	constexpr Vector2f get_normal_to(Rectangle_t<T> r) noexcept {
		std::array<std::tuple<float, Vector2f>, 4> arr;
		arr[0] = {std::abs(x + w - r.x),     { +1,  0 }};
		arr[1] = {std::abs(x - (r.x + r.w)), { -1,  0 }};
		arr[2] = {std::abs(y + h - r.y),     {  0, +1 }};
		arr[3] = {std::abs(y - (r.y + r.h)), {  0, -1 }};

		std::sort(BEG_END(arr), [](auto a, auto b) { return std::get<0>(a) < std::get<0>(b); });

		return std::get<1>(arr[0]);
	}

	constexpr Vector2f get_normal_to(Vector2<T> p) noexcept {
		if (p.y < y + 0) return {0, -1};
		if (p.y > y + h) return {0, +1};
		if (p.x < x + 0) return {-1, 0};
		if (p.x > x + w) return {+1, 0};
		
		return {};
		
//		if (p.y < y + 0 && x < p.x && p.x < x + w) return { 0, -1 };
//		if (p.y > y + h && x < p.x && p.x < x + w) return { 0, +1 };
//		if (p.x < x + 0 && y < p.y && p.y < y + h) return { -1, 0 };
//		if (p.x > x + w && y < p.y && p.y < y + h) return { +1, 0 };
//
//		if (p.x < x + 0 && p.y < y + 0) return Vector2f{ -1, -1 }.normalize();
//		if (p.x > x + w && p.y < y + 0) return Vector2f{ +1, -1 }.normalize();
//		if (p.x < x + 0 && p.y > y + h) return Vector2f{ -1, +1 }.normalize();
//		if (p.x > x + w && p.y > y + h) return Vector2f{ +1, +1 }.normalize();
//
//		return {};
	}

	template<typename U> explicit operator Rectangle_t<U>() const noexcept {
		return {
			Vector2<U>{(U)x, (U)y},
			Vector2<U>{(U)w, (U)h}
		};
	}

	bool intersect(const Rectangle_t<T>& other) const {
		return !(
				pos.x + size.x < other.pos.x || pos.x > other.pos.x + other.size.x ||
				pos.y + size.y < other.pos.y || pos.y > other.pos.y + other.size.y
			);
	}

	constexpr my_type_t exclude(const my_type_t& base) const noexcept {
		my_type_t result;
		
		result.x = std::max(this->x, base.x);
		result.y = std::max(this->y, base.y);
		result.w = std::max((T)0, this->x + this->w - (base.x + base.w));
		result.h = std::max((T)0, this->y + this->h - (base.y + base.h));
		return result;
	}

	constexpr bool is_fully_inside(const std::vector<my_type_t>& vec) const noexcept {
		my_type_t result = *this;
		for (auto& x : vec) result = result.exclude(x);
		return result.area() == 0;
	}

	constexpr Side intersect_side(const Rectangle_t<T>& other) const noexcept {
		if (!intersect(other)) return Side::None;

		auto left = x - (other.x + other.w); left *= left;
		auto right = (x + w) - other.x; right *= right;
		auto top = (y + h) - other.y; top *= top;
		auto bot = y - (other.y + other.h); bot *= bot;

		if (left == std::min<T>({ left, right, top, bot })) return Side::Left;
		else if (right == std::min<T>({ left, right, top, bot })) return Side::Right;
		else if (top == std::min<T>({ left, right, top, bot })) return Side::Top;
		else  return Side::Bot;
	}

	// works only for top down y
	bool isOnTopOf(Rectangle_t<T> other) const {
		T distEdgeToEdge = std::max(other.x + other.w - x, x + w - other.x);
		T sumOfWidth = w + other.w;

		return y < other.y && distEdgeToEdge < sumOfWidth;
	}
	bool isFullyOnTopOf(Rectangle_t<T> other, T tolerance = FLT_EPSILON) const noexcept {
		return y + h < other.y + tolerance;
	}

	// works only for top down y
	bool isOnBotOf(Rectangle_t<T> other) const {
		T distEdgeToEdge = std::max(other.x + other.w - x, x + w - other.x);
		T sumOfWidth = w + other.w;

		return y > other.y && distEdgeToEdge < sumOfWidth;
	}

	Rectangle_t<T> flip_x() noexcept {
		auto copy = *this;
		copy.x += copy.w;
		copy.w *= -1;
		return copy;
	}

	Rectangle_t<T> zoom(float factor) noexcept {
		return {
			center() - size * factor * 0.5,
			size * factor
		};
	}

	Vector<2, T> center() const {
		return pos + size / 2;
	}

	void setCenter(Vector2<T> vec) noexcept {
		pos = vec - size / 2.0;
	}

	T bot() const {
		return pos.y + size.y;
	}

	std::tuple<
		Rectangle_t<T>, Rectangle_t<T>, Rectangle_t<T>, Rectangle_t<T>
	> divide() const noexcept {
		return {
			Rectangle_t<T>{pos, size / 2},
			Rectangle_t<T>{ {pos.x + size.x / 2, pos.y}, size / 2 },
			Rectangle_t<T>{ {pos.x, pos.y + size.y / 2}, size / 2 },
			Rectangle_t<T>{pos + size / 2, size / 2},
		};
	}

	// >TODO: implement this, for now we treat this as if it were the support
	// of a circle of radius max(w, h) / 2
	Vector2<T> support(T a, T d) const noexcept {
		return Vector2<T>::createUnitVector((double)a) * (d + std::max({ w, h }) / 2);
	}

	Vector2<T> topLeft() const noexcept {
		return { x, y + h };
	}
	Vector2<T> topRight() const noexcept {
		return pos + size;
	}
	Vector2<T> botLeft() const noexcept {
		return pos;
	}
	Vector2<T> botRight() const noexcept {
		return { x + w, y };
	}

	Rectangle_t<T> fitUpRatio(double ratio) const noexcept {
		if (w > h) {
			return { pos,{ w, (T)(w / ratio) } };
		}
		else {
			return { pos,{ (T)(h * ratio), h } };
		}
	}
	Rectangle_t<T> fitDownRatio(Vector2<T> ratio) const noexcept {
		auto h_scale = h / ratio.y;
		auto w_scale = w / ratio.x;

		auto scale = std::min(h_scale, w_scale);

		return {pos, ratio * scale};
	}

	Rectangle_t<T> restrict_in(Rectangle_t<T> area) const noexcept {
		Rectangle_t<T> result = *this;

		if (!area.intersect(*this)) return *this;

		if (w > area.w) {
			result.x = area.center().x - result.size.x / 2;
		}
		else {
			// there is a reason it's made like that
			// if for instance area is a narrow (in height) space, then *this can jump up
			// and down if instead of if (result.x ...) it were if (x ...)
			if (x + w > area.x + area.w) {
				result.x = area.x + area.w - w;
			}
			if (result.x < area.x) {
				result.x = area.x;
			}
		}

		if (h > area.h) {
			result.y = area.center().y - result.size.y / 2;
		} else {
			if (y + h > area.y + area.h) {
				result.y = area.y + area.h - h;
			}
			if (result.y < area.y) {
				result.y = area.y;
			}
		}
		return result;
	}

	T area() const noexcept {
		return w * h;
	}

	template<typename U>
	bool in(const Vector<2, U>& p) const {
		return p.inRect(pos, size);
	}

	static Rectangle_t<T> hull(std::vector<Rectangle_t<T>> recs) noexcept {
		Rectangle_t<T> hull = recs[0];

		for (auto rec : recs) {
			hull.x = std::min(rec.x, hull.x);
			hull.y = std::min(rec.y, hull.y);
		}
		for (auto rec : recs) {
			hull.w = std::max(rec.x + rec.w, hull.w + hull.x) - hull.x;
			hull.h = std::max(rec.y + rec.h, hull.h + hull.y) - hull.y;
		}
		return hull;
	}

	T get_intersection_area(const Rectangle_t<T>& other) const noexcept {
		Rectangle_t<T> big;
		big.x = std::min(this->x, other.x);
		big.y = std::min(this->y, other.y);
		big.w = std::max(this->w + this->x, other.x + other.w) - big.x;
		big.h = std::max(this->y + this->h, other.y + other.h) - big.y;
		return std::max(this->area() + other.area() - big.area(), static_cast<T>(0));
	}
};

using Rectanglef = Rectangle_t<float>;
using Rectanglei = Rectangle_t<int>;
using Rectangleu = Rectangle_t<size_t>;

struct dyn_struct;
template<typename T>
void from_dyn_struct(const dyn_struct& str, Rectangle_t<T>& x) noexcept {
	x.x = (T)str[0];
	x.y = (T)str[1];
	x.w = (T)str[2];
	x.h = (T)str[3];
}
template<typename T>
void to_dyn_struct(dyn_struct& str, const Rectangle_t<T>& x) noexcept {
	str = dyn_struct::array_t{};
	str.push_back(x.x);
	str.push_back(x.y);
	str.push_back(x.w);
	str.push_back(x.h);
}

template<typename T>
double dist_to2(const Vector<2, T>& vec, const Rectangle_t<T> & rec) noexcept {
	if (
		(vec.x < rec.x || vec.x > rec.x + rec.w) &&
		(rec.y < vec.y && vec.y < rec.y + rec.h)
	) {
		return (rec.x - vec.x) * (rec.x - vec.x);
	}
	if (
		(vec.y < rec.y || vec.y > rec.y + rec.h) &&
		(rec.x < vec.x && vec.x < rec.x + rec.w)
	) {
		return (rec.y - vec.y) * (rec.y - vec.y);
	}

	Vector<2, T> dt;

	if (vec.x < rec.x) {
		if (vec.y < rec.y)   dt = (vec - Vector<2, T>{rec.x, rec.y});
		else                 dt = (vec - Vector<2, T>{rec.x, rec.y + rec.h});
	}
	else {
		if (vec.y < rec.y)   dt = (vec - Vector<2, T>{rec.x + rec.w, rec.y});
		else                 dt = (vec - Vector<2, T>{rec.x + rec.w, rec.y + rec.h});
	}

	return dt.length2();
}

inline void printf(const Rectanglef& rec) noexcept {
	printf("%3.3f, %3.3f, %3.3f, %3.3f\n", rec.x, rec.y, rec.w, rec.h);
}

template<typename T> bool check_not_fully_inside(
	const Rectangle_t<T>& to_check, std::vector<Rectangle_t<T>> vec
) noexcept {
	if (vec.empty()) return false;

	auto area = 0;

	for (auto& x : vec) area += x.get_intersection_area(to_check);

	return to_check.area() >= 0;
}
