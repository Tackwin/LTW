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
	
};