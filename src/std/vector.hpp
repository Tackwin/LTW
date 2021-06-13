#pragma once

#include "type_traits.hpp"

using size_t = unsigned long long;

namespace xstd {

	#define move(x) (static_cast<remove_reference_t<decltype(x)>&&>(x))

	static void assert_(bool cond) {
		if (!cond) throw "oops";
	}

	template<typename T>
	struct vector {
		using Stored_t = T;

		T* data_ = nullptr;
		size_t size_ = 0;
		size_t capacity = 0;

		vector() noexcept {}
		~vector() noexcept { delete[] data_; size_ = 0; capacity = 0; };

		vector(const vector<T>& other) noexcept { *this = other; }
		vector& operator=(const vector<T>& other) noexcept {
			reserve(other.size());
			for (size_t i = 0; i < other.size(); ++i) data_[i] = other[i];
			size_ = other.size();
			return *this;
		}

		vector(vector<T>&& other) noexcept { *this = move(other); }
		vector& operator=(vector<T>&& other) noexcept {
			data_ = other.data_;
			size_ = other.size_;
			capacity = other.capacity;
			other.data_ = 0;
			other.size_ = 0;
			other.capacity = 0;
			return *this;
		}

		void push_back(T t) noexcept {
			if (size_ + 1 > capacity) reserve((size_t)(10 + capacity * 1.5));

			data_[size_++] = move(t);
		}

		constexpr void pop_back() noexcept { size_--; }

		void reserve(size_t n) noexcept {
			if (n <= capacity) return;

			auto new_data = new T[n];
			for (size_t i = 0; i < size_; ++i) new_data[i] = move(data_[i]);
			delete[] data_;
			data_ = new_data;

			capacity = n;
		}

		constexpr void clear() noexcept { size_ = 0; }

		constexpr T& operator[](size_t idx) noexcept {
			#ifdef _DEBUG
			assert_(idx < size_);
			#endif
			return data_[idx];
		}
		constexpr const T& operator[](size_t idx) const noexcept {
			#ifdef _DEBUG
			assert_(idx < size_);
			#endif
			return data_[idx];
		}
		constexpr size_t size() const noexcept { return size_; }
		constexpr T* data() noexcept { return data_; }
		constexpr const T* data() const noexcept { return data_; }

		void resize(size_t n, const T& v = {}) noexcept {
			reserve(n);

			for (; size_ < n; ++size_) data_[size_] = v;
			size_ = n;
		}

		template<typename F>
		void erase(const F& f) noexcept {
			for (size_t i = 0; i < size_; ++i) if (f(data_[i]))
				data_[i--] = data_[--size_];
		}

		T& back() noexcept { return data_[size_ - 1]; };
		const T& back() const noexcept { return data_[size_ - 1]; };

		T& front() noexcept { return *data_; };
		const T& front() const noexcept { return *data_; };

		T* begin() noexcept { return data_; }
		T* end  () noexcept { return data_ + size_; }
		const T* begin() const noexcept { return data_; }
		const T* end  () const noexcept { return data_ + size_; }

		constexpr bool empty() const noexcept { return size_ == 0; }
		constexpr bool used() const noexcept { return size_ > 0; }
	};

	template<typename T, size_t D_>
	struct array {
		static constexpr size_t D = D_;
		T data[D_];
	};

	template<typename T, size_t S>
	struct small_vector {
		size_t size = 0;
		size_t capacity = 0;
		union {
			T stack_data[S];
			T* heap_data;
		};

		small_vector() noexcept {
			for (size_t i = 0; i < S; ++i) stack_data[i] = {};
		}
		~small_vector() noexcept { if (capacity > S) delete[] heap_data; size = 0; capacity = 0; };

		small_vector(const small_vector<T, S>& other) noexcept { *this = other; }
		small_vector& operator=(const small_vector<T, S>& other) noexcept {
			reserve(other.size);
			for (size_t i = 0; i < other.size; ++i) data()[i] = other[i];
			size = other.size;
			return *this;
		}

		small_vector(small_vector<T, S>&& other) noexcept { *this = move(other); }
		small_vector& operator=(small_vector<T, S>&& other) noexcept {
			for (size_t i = 0; i < S; ++i) stack_data[i] = other.stack_data[i];
			heap_data = other.heap_data;
			size = other.size;
			capacity = other.capacity;
			other.stack_data = {};
			other.heap_data = 0;
			other.size = 0;
			other.capacity = 0;
			return *this;
		}

		void push_back(T t) noexcept {
			if (size + 1 > capacity) reserve((size_t)(10 + capacity * 1.5));

			data()[size++] = move(t);
		}

		constexpr void pop_back() noexcept { size--; }

		void reserve(size_t n) noexcept {
			if (n <= capacity) return;

			if (n < S) {
				capacity = n;
				return;
			}

			auto new_data = new T[n];
			for (size_t i = 0; i < size; ++i) new_data[i] = move(data()[i]);
			delete[] heap_data;
			heap_data = new_data;

			capacity = n;
		}

		constexpr void clear() noexcept { size = 0; }

		constexpr T& operator[](size_t idx) noexcept {
			#ifdef _DEBUG
			assert_(idx < size);
			#endif
			return data()[idx];
		}
		constexpr const T& operator[](size_t idx) const noexcept {
			#ifdef _DEBUG
			assert_(idx < size);
			#endif
			return data()[idx];
		}
		void resize(size_t n, const T& v = {}) noexcept {
			reserve(n);

			for (; size < n; ++size) data()[size] = v;
			size = n;
		}

		template<typename F>
		void erase(const F& f) noexcept {
			for (size_t i = 0; i < size; ++i) if (f(data()[i]))
				data()[i--] = data()[--size_];
		}

		T* data() noexcept { if (capacity < S) return stack_data; return heap_data; }
		const T* data() const noexcept { if (capacity < S) return stack_data; return heap_data; }

		T& back() noexcept { return data()[size - 1]; };
		const T& back() const noexcept { return data()[size - 1]; };

		T& front() noexcept { return *data(); };
		const T& front() const noexcept { return *data(); };

		T* begin() noexcept { return data(); }
		T* end  () noexcept { return data() + size; }
		const T* begin() const noexcept { return data(); }
		const T* end  () const noexcept { return data() + size; }

		constexpr bool empty() const noexcept { return size == 0; }
		constexpr bool used() const noexcept { return size > 0; }
	};

	template<typename T>
	struct span {
		T* data = nullptr;
		size_t size = 0;

		template<size_t D>
		span(xstd::small_vector<T, D>& small_vec) noexcept {
			data = small_vec.data();
			size = small_vec.size;
		}

		constexpr T& operator[](size_t idx) noexcept {
			#ifdef _DEBUG
			assert_(idx < size);
			#endif
			return data[idx];
		}
		constexpr const T& operator[](size_t idx) const noexcept {
			#ifdef _DEBUG
			assert_(idx < size);
			#endif
			return data[idx];
		}

		T* begin() noexcept { return data; }
		T* end  () noexcept { return data + size; }
		const T* begin() const noexcept { return data; }
		const T* end  () const noexcept { return data + size; }
	};

	template<typename T, typename F>
	void remove_all(xstd::span<T>&& vec, F&& f) noexcept {
		size_t s = vec.size;
		for (size_t i = 0; i < s; ++i) if (f(vec[i])) {
			vec[i] = vec[s - 1];
			--s;
			--i;
		}
	}

	#undef move
};

namespace std {
	template<typename T> T* begin(xstd::vector<T>& v) noexcept { return v.data_; }
	template<typename T> T* end  (xstd::vector<T>& v) noexcept { return v.data_ + v.size_; }
	
	template<typename T> const T* cbegin(const xstd::vector<T>& v) noexcept {
		return v.data_;
	}
	template<typename T> const T* cend  (const xstd::vector<T>& v) noexcept {
		return v.data_ + v.size_;
	}
	template<typename T> T* begin(xstd::span<T>& v) noexcept { return v.data; }
	template<typename T> T* end  (xstd::span<T>& v) noexcept { return v.data + v.size; }
	
	template<typename T> const T* cbegin(const xstd::span<T>& v) noexcept {
		return v.data;
	}
	template<typename T> const T* cend  (const xstd::span<T>& v) noexcept {
		return v.data + v.size;
	}
};