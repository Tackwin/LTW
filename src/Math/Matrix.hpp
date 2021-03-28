#pragma once
#include "Vector.hpp"

template<size_t R, size_t C, typename T = float>
struct Matrix {
	Vector<C, T> rows[R];

	template<size_t N = R>
	static constexpr std::enable_if_t<N == C, Matrix<N, N, T>> identity() {
		Matrix<N, N, T> m;
		for (size_t i = 0u; i < N; ++i) {
			m[i][i] = (T)1;
		}
		return m;
	}

	template<size_t Rp = R, size_t Cp = C>
	static constexpr std::enable_if_t<Rp == Cp, Matrix<Rp, Cp, T>> diagonal(T scalar) {
		Matrix<Rp, Cp, T> matrix;
		for (size_t i = 0u; i < Rp; ++i) {
			matrix[i][i] = scalar;
		}
		return matrix;
	}

	template<size_t N = R, typename T = float>
	static constexpr std::enable_if_t<N == C, Matrix<N, N, T>>
		scalar(Vector<N, T> scalar)
	{
		Matrix<N, N, T> matrix;
		for (size_t i = 0u; i < N; ++i) {
			matrix[i][i] = scalar[i];
		}
		return matrix;
	}

	template<size_t N = R, typename T = float>
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
				rows[i][j] = ele[i * R + j];
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
				rows[i][j] = *(ele.begin() + (i * R + j));
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
	Matrix<R, C, T> operator*(U scalar) const {
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
	Matrix<R, C, T>& operator*=(U scalar) {
		for (size_t i = 0u; i < R; ++i) {
			for (size_t j = 0u; j < C; ++j) {
				rows[i][j] *= (T)scalar;
			}
		}
		return *this;
	}

	template<typename U>
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
};

template<typename T>
using Matrix4 = Matrix<4, 4, T>;
using Matrix4f = Matrix4<float>;