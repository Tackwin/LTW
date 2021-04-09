#pragma once
#include "Vector.hpp"
#include "Rectangle.hpp"
#include <cmath>

template<size_t R, size_t C, typename T = float>
struct Matrix {
	Vector<C, T> rows[R];

	template<size_t N = R>
	static constexpr std::enable_if_t<N == C, Matrix<N, N, T>>
		translation(Vector<N - 1, T> vec)
	{
		Matrix<N, N, T> matrix;
		for (size_t i = 0u; i < N - 1; ++i) {
			matrix[i][N - 1] = vec[i];
			matrix[i][i] = (T)1;
		}
		matrix[N - 1][N - 1] = (T)1;
		return matrix;
	}
	template<size_t N = R>
	static constexpr std::enable_if_t<N == C, Matrix<N, N, T>> scale(Vector<N - 1, T> vec) {
		Matrix<N, N, T> matrix;
		for (size_t i = 0u; i < N - 1; ++i) matrix[i][i] = vec[i];
		matrix[N - 1][N - 1] = 1;
		return matrix;
	}

	template<size_t N = R>
	static constexpr std::enable_if_t<N == C && C == 4, Matrix<N, N, T>>
		rotation(Vector<3, T> a, T θ)
	{
		auto c = cosf(θ);
		auto s = sinf(θ);

		auto x = a.x;
		auto y = a.y;
		auto z = a.z;

		return {
			x * x * (1 - c) + c    , x * y * (1 - c) - z * s, x * z * (1 - c) + y * s, 0,
			y * x * (1 - c) + z * s, y * y * (1 - c) + c    , y * z * (1 - c) - x * s, 0,
			z * x * (1 - c) - y * s, z * x * (1 - c) + x * s, c + z * z * (1 - c)    , 0,
			0					   , 0						, 0						 , 1
		};
	}

	template<size_t N = R>
	static constexpr std::enable_if_t<N == C && C == 4, Matrix<N, N, T>>
		rotationz(T θ)
	{
		auto c = cosf(θ);
		auto s = sinf(θ);

		return {
			+c, -s, 0, 0,
			+s, +c, 0, 0,
			0,   0, 1, 0,
			0,   0, 0, 1
		};
	}


	Matrix() {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] = (T)0;
			}
		}
	}

	Matrix(const T ele[R * C]) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] = ele[i + j * R];
			}
		}
	}

	Matrix(const Vector<C, T> rows_[R]) {
		for (size_t i = 0u; i < R; ++i) {
			rows[i] = rows_[i];
		}
	}

	Matrix(const std::initializer_list<T> ele) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] = *(ele.begin() + (i + j * R));
			}
		}
	}

	Matrix(const std::initializer_list<Vector<C, T>> rows_) {
		for (size_t i = 0u; i < R; ++i) {
			rows[i] = *(rows_.begin() + (i));
		}
	}

	void print() {
		for (size_t i = 0u; i < R; ++i) {
			rows[i].print();
		}
	}

	template<typename U>
	Matrix<R, C, T> operator+(U scalar) const {
		Matrix<R, C, T> result;

		for (size_t i = 0u; i < R; ++i) {
			auto vec = result[i];
			for (size_t j = 0u; j < C; ++j) {
				vec[j] += (T)scalar;
			}
			result[i] = vec;
		}
		return result;
	}
	template<typename U>
	Matrix<R, C, T> operator+(const Matrix<R, C, U>& other) const {
		Matrix<R, C, T> result;

		for (size_t i = 0u; i < R; ++i) {
			auto vec = result[i];
			for (size_t j = 0u; j < C; ++j) {
				vec[j] += (T)other[i][j];
			}
			result[i] = vec;
		}
		return result;
	}
	template<typename U>
	Matrix<R, C, T>& operator+=(U scalar) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] += (T)scalar;
			}
		}
		return *this;
	}
	template<typename U>
	Matrix<R, C, T> operator+=(const Matrix<R, C, U>& other) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] += (T)other[i][j];
			}
		}
		return *this;
	}

	Matrix<R, C, T> operator-() const {
		Matrix<R, C, T> m;

		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				m[i][j] = -rows[i][j];
			}
		}
		return *this;
	}

	template<typename U>
	Matrix<R, C, T> operator-(U scalar) const {
		return *this + (-scalar);
	}
	template<typename U>
	Matrix<R, C, T> operator-(const Matrix<R, C, U>& other) const {
		return *this + (-other);
	}
	template<typename U>
	Matrix<R, C, T>& operator-=(U scalar) {
		return *this += (-scalar);
	}
	template<typename U>
	Matrix<R, C, T> operator-=(const Matrix<R, C, U>& other) {
		return *this += (-other);
	}

	template<typename U>
	Matrix<R, C, T> operator*(std::enable_if_t<std::is_scalar_v<U>, U> scalar) const {
		Matrix<R, C, T> result;

		for (size_t i = 0u; i < R; ++i) {
			auto vec = result[i];
			for (size_t j = 0u; j < C; ++j) {
				vec[j] *= (T)scalar;
			}
			result[i] = vec;
		}
		return result;
	}
	template<typename U>
	Matrix<R, C, T>& operator*=(std::enable_if_t<std::is_scalar_v<U>, U> scalar) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] *= (T)scalar;
			}
		}
		return *this;
	}

	Vector<R, T> operator*(Vector<R, T> vec) const {
		Vector<R, T> result;

		for (size_t i = 0u; i < R; ++i) {
			T sum = (T)0;
			for (size_t j = 0u; j < C; ++j) {
				sum += vec[j] * rows[i][j];
			}
			result[i] = sum;
		}
		return result;
	}

	template<size_t P, typename U>
	Matrix<R, P, T> operator*(const Matrix<C, P, U>& other) const {
		Matrix<R, P, T> matrix;

		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < P; ++j) {
				T sum = (T)0;

				for (size_t n = 0u; n < C; ++n) {
					sum += rows[i][n] * other.rows[n][j];
				}

				matrix[i][j] = sum;
			}
		}

		return matrix;
	}
	template<typename U>
	Matrix<R, C, T>& operator*=(const Matrix<C, C, U>& other) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				T sum = (T)0;

				for (size_t n = 0u; n < C; ++n) {
					sum += rows[i][n] * other.rows[n][j];
				}

				rows[i][j] = sum;
			}
		}

		return *this;
	}

	template<typename U>
	Matrix<R, C, T> operator/(U scalar) const {
		return *this * (1 / scalar);
	}
	template<typename U>
	Matrix<R, C, T>& operator/=(U scalar) {
		return *this *= (1 / scalar);
	}

	Vector<C, T> operator[](size_t i) const {
		return rows[i];
	}
	Vector<C, T>& operator[](size_t i) {
		return rows[i];
	}

	T operator[](const Vector<2, unsigned int>& idx) const {
		return rows[idx.x][idx.y];
	}
	T& operator[](const Vector<2, unsigned int>& idx) {
		return rows[idx.x][idx.y];
	}

	template<typename X_T = std::enable_if_t<R == 4 && C == 4>>
	std::optional<Matrix<4, 4, T>> invert() const noexcept {
		Matrix<4, 4, T> result;
		T inv[16], det;
		int i;

		T* result_ptr = &result[0][0];
		const T* m = &rows[0].x;

		inv[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		inv[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		inv[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		inv[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		inv[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		inv[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		inv[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		inv[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		inv[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		inv[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		inv[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		inv[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +
			m[12] * m[2] * m[5];

		inv[3] = -m[1] * m[6] * m[11] +
			m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -
			m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +
			m[9] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] -
			m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +
			m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -
			m[8] * m[3] * m[6];

		inv[11] = -m[0] * m[5] * m[11] +
			m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -
			m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +
			m[8] * m[3] * m[5];

		inv[15] = m[0] * m[5] * m[10] -
			m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +
			m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -
			m[8] * m[2] * m[5];

		det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

		if (det == 0)
			return std::nullopt;

		det = 1.f / det;

		for (i = 0; i < 16; i++)
			result_ptr[i] = inv[i] * det;

		return result;
	}
};

template<typename T>
using Matrix4 = Matrix<4, 4, T>;
using Matrix4f = Matrix4<float>;

inline Matrix4f perspective(float fov, float ratio, float f, float n) noexcept {
	float uw = 1.f / std::tanf(fov / 2);
	float uh = uw;

	Matrix4f matrix;
	matrix[0] = Vector4f{ uw, 0, 0, 0 };
	matrix[1] = Vector4f{ 0, uh, 0, 0 };
	matrix[2] = Vector4f{ 0, 0, (f + n) / (n - f), 2 * (f * n) / (n - f) };
	matrix[3] = Vector4f{ 0, 0, -1, 0};

	return matrix;
}
inline Matrix4f orthographic(Rectanglef rec, float f, float n) noexcept {
	Matrix4f matrix;
	matrix[0] = Vector4f{ 2 / rec.w, 0, 0,  - (2 * rec.x + rec.w) / rec.w };
	matrix[1] = Vector4f{ 0, 2 / rec.h, 0, - (2 * rec.y + rec.h) / rec.h };
	matrix[2] = Vector4f{ 0, 0, -2 * f / (f - n), - (f + n) / (f- n) };
	matrix[3] = Vector4f{0, 0, 0, 1};
	return matrix;
}