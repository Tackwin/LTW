#pragma once

#include <type_traits>
#include <functional>
#include <random>

#include "dyn_struct.hpp"

#pragma warning(push)
#pragma warning(disable: 4201)

#ifndef FLT_EPSILON
#define FLT_EPSILON 0.0001f
#endif
#ifndef DBL_EPSILON
#define DBL_EPSILON 0.000000001
#endif

static constexpr auto Vector2_Type_Tag = "Vector2<T>"_id;
static constexpr auto Vector4_Type_Tag = "Vector4<T>"_id;

#define COLOR_UNROLL(x) (x).r, (x).g, (x).b, (x).a
#define XYZW_UNROLL(v) (v).x, (v).y, (v).z, (v).w

template<size_t D, typename T>
struct Vector;

template<typename T> using Vector2 = Vector<2U, T>;
template<typename T> using Vector3 = Vector<3U, T>;
template<typename T> using Vector4 = Vector<4U, T>;
using Vector2u = Vector2<std::size_t>;
using Vector2i = Vector2<std::int32_t>;
using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
using Vector4i = Vector4<std::int32_t>;
using Vector4u = Vector4<std::size_t>;
using Vector4f = Vector4<float>;
using Vector4d = Vector4<double>;

template<size_t D, typename T>
struct __vec_member {
	T components[D];
};
template<typename T>
struct __vec_member<1, T> {
	union {
		struct {
			T x;
		};
		T components[1];
	};

	constexpr __vec_member() : x(0) {}
	constexpr __vec_member(T x) : x(x) {}
};
template<typename T>
struct __vec_member<2, T> {
	union {
		struct {
			T x;
			T y;
		};
		T components[2];
	};

	constexpr __vec_member() : x(0), y(0) {}
	constexpr __vec_member(T x, T y) : x(x), y(y) {}
};
template<typename T>
struct __vec_member<3, T> {
	union {
		struct {
			T x;
			T y;
			T z;
		};
		struct {
			T h;
			T s;
			T l;
		};
		T components[3];
	};

	constexpr __vec_member() : x(0), y(0), z(0) {}
	constexpr __vec_member(T x, T y, T z) : x(x), y(y), z(z) {}
};
template<typename T>
struct __vec_member<4, T> {
	union {
		struct {
			T x;
			T y;
			T z;
			T w;
		};
		struct {
			union {
				struct {
					T r;
					T g;
					T b;
				};
				struct {
					T h;
					T s;
					T v;
				};
			};
			T a;
		};
		T components[4];
	};

	constexpr __vec_member() : x(0), y(0), z(0), w(0) {}
	constexpr __vec_member(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
};

template<size_t D, typename T = float>
struct Vector : public __vec_member<D, T> {
    
#pragma region STATIC
    
	static Vector<D, T> createUnitVector(float angles[D - 1]) {
		Vector<D, T> result;
		result[0] = cosf(angles[0]);
		for (size_t i = 1u; i < D; ++i) {
            
			result[i] = (i + 1u != D) ?
				cosf(angles[i]) :
			1;
            
			for (size_t j = 0u; j < D - 1u; ++j) {
				result[i] *= sinf(angles[j]);
			}
		}
		return result;
	}
	static Vector<2, T> createUnitVector(float angles) { // i'm not doing all the shit above for 2d
		return { cosf(angles), sinf(angles) };
	}
	static Vector<D, T> createUnitVector(double angles[D - 1]) {
		Vector<D, T> result;
		result[0] = static_cast<T>(cos(angles[0]));
		for (size_t i = 1u; i < D; ++i) {
            
			result[i] = (i + 1u != D) ?
				static_cast<T>(cos(angles[i])) :
			1;
            
			for (size_t j = 0u; j < D - 1u; ++j) {
				result[i] *= static_cast<T>(sin(angles[j]));
			}
		}
		return result;
	}
	static Vector<2, T> createUnitVector(double angles) { // i'm not doing all the shit above for 2d
		return {
			static_cast<T>(cos(angles)),
			static_cast<T>(sin(angles))
		};
	}
	template<typename V>
		static bool equalf(const V& A, const V& B, float eps = FLT_EPSILON) {
		return (A - B).length2() <= eps * eps;
	}
	template<typename V>
		static bool equal(const V& A, const V& B, double eps = DBL_EPSILON) {
		return (A - B).length2() <= eps * eps;
	}
	static Vector<D, T> clamp(const Vector<D, T>& V, const Vector<D, T>& min, const Vector<D, T>& max) {
		Vector<D, T> result;
		for (std::size_t i = 0u; i < D; ++i) {
			result[i] = std::clamp(V[i], min[i], max[i]);
		}
		return result;
	}
	static Vector<D, T> rand_unit(double(*rng)(void)) {
		double r[D - 1];

		for (size_t i = 0u; i < D - 1; ++i) {
			r[i] = 2 * 3.1415926 * rng();
		}

		return createUnitVector(r);
	}
	static Vector<D, T> rand(
		const Vector<D, T>& min,
		const Vector<D, T>& max,
		std::default_random_engine rng = std::default_random_engine()
		) {
		Vector<D, T> r;
        
		if constexpr (std::is_integral_v<T>) {
			for (size_t i = 0u; i < D; ++i) {
				r[i] = std::uniform_int_distribution<T>(min[i], max[i])(rng);
			}
			return r;
		}
        
		for (size_t i = 0u; i < D; ++i) {
			r[i] = std::uniform_real_distribution<T>(min[i], max[i])(rng);
		}
        
		return r;
	}
#pragma endregion
    
	constexpr Vector() {
		for (size_t i = 0u; i < D; ++i) {
			this->components[i] = static_cast<T>(0);
		}
	}
    
	template<size_t Dp = D>
		constexpr Vector(T x, std::enable_if_t<Dp == 2, T> y) :
	__vec_member<2, T>(x, y)
	{}
    
	template<size_t Dp = D>
		constexpr Vector(T x, T y, std::enable_if_t<Dp == 3, T> z) :
	__vec_member<3, T>(x, y, z)
	{}


    
	template<size_t Dp = D>
		constexpr Vector(T x, T y, T z, std::enable_if_t<Dp == 4, T> w) :
	__vec_member<4, T>(x, y, z, w)
	{}


	size_t getDimension() const {
		return D;
	}

	Vector<D, T>& clamp(const Vector<D, T>& min, const Vector<D, T>& max) {
		for (std::size_t i = 0u; i < D; ++i) {
			this->components[i] = std::clamp(this->components[i], min[i], max[i]);
		}
		return *this;
	}
    
	template<typename U>
		bool inRect(const Vector<D, U>& pos, const Vector<D, U>& size) const {
		for (size_t i = 0u; i < D; ++i) {
			if (!(
				static_cast<T>(pos[i]) < this->components[i] &&
				this->components[i] < static_cast<T>(pos[i] + size[i])
				))
			{
				return false;
			}
		}
        
		return true;
	}
    
	T dot(const Vector<D, T>& other) const noexcept {
		T sum = 0;
		for (size_t i = 0; i < D; ++i) {
			sum += this->components[i] * other.components[i];
		}
		return sum;
	}

	T length() const {
		T result = 0;
		for (size_t i = 0u; i < D; ++i) {
			result += this->components[i] * this->components[i];
		}
		return sqrt(result);
	}

	T length2() const {
		T result = 0;
		for (size_t i = 0u; i < D; ++i) {
			result += this->components[i] * this->components[i];
		}
		return result;
	}

	T dist_to2(const Vector<D, T>& x) const noexcept {
		return (*this - x).length2();
	}
	T dist_to(const Vector<D, T>& x) const noexcept {
		return (*this - x).length();
	}

	template<size_t Dp = D>
		std::enable_if_t<Dp == 2, double> angleX() const noexcept {
		return std::atan2(this->y, this->x);
	}

	template<typename U, size_t Dp = D>
		std::enable_if_t<Dp == 2, double> angleTo(const Vector<2U, U>& other) const noexcept {
		return std::atan2(other.y - this->y, other.x - this->x);
	}

	template<typename U, size_t Dp = D>
		std::enable_if_t<Dp == 2, double> angleFrom(const Vector<2U, U>& other) const noexcept {
		return std::atan2(this->y - other.y, this->x - other.x);
	}

	template<size_t Dp = D>
		std::enable_if_t<Dp == 2, double> pseudoAngleX() const noexcept {
		auto dx = this->x;
		auto dy = this->y;
		return std::copysign(1.0 - dx / (std::fabs(dx) + fabs(dy)), dy);
	}

	template<size_t Dp = D>
		std::enable_if_t<Dp == 2, double>
		pseudoAngleTo(const Vector<2U, T>& other) const noexcept {
		return (other - *this).pseudoAngleX();
	}

	template<size_t Dp = D>
		std::enable_if_t<Dp == 2, double>
		pseudoAngleFrom(const Vector<2U, T>& other) const noexcept {
		return (*this - other).pseudoAngleX();
	}

	Vector<D, T>& normalize() {
		const auto& l = length();
		if (l == 0) return *this;
		for (size_t i = 0u; i < D; ++i) {
			this->components[i] /= l;
		}
		return *this;
	}

	constexpr Vector<D, T> normale() const noexcept {
		const auto& l = length();
		auto result = *this;
		
		if (l == 0) return result;
		for (size_t i = 0u; i < D; ++i) {
			result[i] /= l;
		}
		return result;
	}

	constexpr Vector<D, T> projectTo(const Vector<D, T>& other) const noexcept {
		return other * dot(other) / other.length2();
	}

	Vector<D, T> round(T magnitude) {
		Vector<D, T> results;
		for (size_t i = 0; i < D; ++i) {
			results[i] = static_cast<T>(
				std::round(this->components[i] / magnitude) * magnitude
				);
		}
		return results;
	}

	template<size_t Dp = D>
		std::enable_if_t<Dp == 2, Vector<D, T>> fitUpRatio(double ratio) const noexcept {
		if (this->x > this->y) {
			return { this->x, (T)(this->x / ratio) };
		}
		else {
			return { (T)(this->y * ratio), this->y };
		}
	}

	template<size_t Dp = D>
	std::enable_if_t<Dp == 2, Vector<D, T>> fitDownRatio(Vector2<T> ratio) const noexcept {
		auto h_scale = this->x / ratio.x;
		auto w_scale = this->y / ratio.y;

		auto scale = std::min(h_scale, w_scale);

		return ratio * scale;
	}

	template<typename L>
		std::enable_if_t<
		std::is_invocable_r_v<T, L, T>, Vector<D, T>
		> applyCW(L lamb) const noexcept {
		Vector<D, T> res;
		for (size_t i = 0; i < D; ++i) {
			res[i] = lamb(this->components[i]);
		}
		return res;
	}
    
	template<size_t Dp = D>
		constexpr std::enable_if_t<Dp == 2, Vector<D, T>> rotate90() const noexcept {
		return { -this->y, this->x };
	}
    
#pragma region OPERATOR
	T& operator[](size_t i) {
		return this->components[i];
	}
	const T& operator[](size_t i) const {
		return this->components[i];
	}
    
    
	template<typename U>
		auto operator*(const U& scalaire) const noexcept -> Vector<D, decltype(T{} * scalaire)> {
		static_assert(std::is_scalar<U>::value, "need to be a scalar");
		Vector<D, decltype(T{} *scalaire) > result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = this->components[i] * scalaire;
		}
        
		return result;
	}
	template<typename U>
		Vector<D, T> operator/(const U& scalaire) const {
		static_assert(std::is_scalar<U>::value);
		Vector<D, T> result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = static_cast<T>(this->components[i] / scalaire);
		}
        
		return result;
	}
    
	template<typename U>
		Vector<D, T> operator+(const Vector<D, U>& other) const {
		Vector<D, T> result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = static_cast<T>(this->components[i] + other[i]);
		}

		return result;
	}

	Vector<D, T> operator+(const T& other) const {
		Vector<D, T> result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = static_cast<T>(this->components[i] + other);
		}

		return result;
	}

	template<typename U>
	Vector<D, T> operator-(const Vector<D, U>& other) const {
		Vector<D, T> result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = static_cast<T>(this->components[i] - other[i]);
		}

		return result;
	}

	Vector<D, T> operator-(const T& other) const {
		Vector<D, T> result;
		for (size_t i = 0; i < getDimension(); ++i) {
			result[i] = static_cast<T>(this->components[i] - other);
		}

		return result;
	}

	template<typename U>
	Vector<D, T>& operator+=(const Vector<D, U>& other) {
		for (size_t i = 0; i < getDimension(); ++i) {
			this->components[i] += static_cast<T>(other[i]);
		}
		return *this;
	}
	Vector<D, T>& operator+=(const T& other) {
		for (size_t i = 0; i < getDimension(); ++i) {
			this->components[i] += static_cast<T>(other);
		}
		return *this;
	}
	template<typename U>
		Vector<D, T>& operator-=(const U& other) {
		return this->operator+=(static_cast<T>(-1) * other);
	}
	template<typename U>
		Vector<D, T>& operator*=(const U& scalaire) {
		static_assert(std::is_scalar<U>::value);
		for (size_t i = 0; i < getDimension(); ++i) {
			this->components[i] *= static_cast<T>(scalaire);
		}
		return *this;
	}
    
	template<typename U>
		bool operator==(const Vector<D, U>& other) const {
		for (size_t i = 0u; i < D; ++i) {
			if (this->components[i] != other.components[i])
				return false;
		}
		return true;
	}
	template<typename U>
		bool operator!=(const Vector<D, U>& other) const {
		return !(this->operator==(other));
	}
    
	// this got to be the most convulated method signature that i've written.
	template<typename F>
		std::enable_if_t<
		std::is_invocable_r_v<T, F, T>,
	Vector<D, T>
		> apply(F&& f) noexcept(std::is_nothrow_invocable_v<F, T>) {
		Vector<D, T> result;
		for (auto i = 0; i < D; ++i) {
			result[i] = f(this->components[i]);
		}
		return result;
	}
    
    
	template<typename U>
		explicit operator const Vector<D, U>() const {
		Vector<D, U> results;
		for (size_t i = 0u; i < D; ++i) {
			results[i] = static_cast<U>(this->components[i]);
		}
		return results;
	}
    
	explicit operator const std::string() const {
		std::string r = std::to_string(this->components[0]);
		for (size_t i = 1u; i < D; ++i) {
			r += ' ';
			r += std::to_string(this->components[i]);
		}
		return r;
	}
#pragma endregion
};

template<size_t D, typename T, typename U>
auto operator*(U scalar, const Vector<D, T>& vec) noexcept -> Vector<D, decltype(scalar * vec[0])>
{
	return vec * scalar;
}
template<size_t D, typename T, typename U>
Vector<D, T> operator/(U scalar, const Vector<D, T>& vec) noexcept {
	Vector<D, T> result;
	for (auto i = 0; i < D; ++i) {
		result[i] = scalar / vec[i];
	}
	return result;
}


template<typename T>
void to_dyn_struct(dyn_struct& s, const Vector<2, T>& x) noexcept {
	s = { x.x, x.y };
	s.type_tag = Vector2_Type_Tag;
}
template<typename T>
void from_dyn_struct(const dyn_struct& s, Vector<2, T>& x) noexcept {
	x.x = (T)s[0];
	x.y = (T)s[1];
}

template<typename T>
void to_dyn_struct(dyn_struct& s, const Vector<4, T>& x) noexcept {
	s = { x.x, x.y, x.z, x.w };
	s.type_tag = Vector4_Type_Tag;
}
template<typename T>
void from_dyn_struct(const dyn_struct& s, Vector<4, T>& x) noexcept {
	x.x = (T)s[0];
	x.y = (T)s[1];
	x.z = (T)s[2];
	x.w = (T)s[3];
}


inline Vector4d to_rgba(Vector4d in) noexcept {
	double      hh, p, q, t, ff;
	long        i;
	Vector4d    out;

	if (in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h * 360;
	if (hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch (i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}

	out.a = in.a;
	return out;
}

#pragma warning(pop)
