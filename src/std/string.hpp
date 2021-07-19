#pragma once

#include "std/int.hpp"
#include "std/vector.hpp"

namespace xstd {
	struct string : vector<char> {
		string(const char* str) noexcept {
			size_t len = 0;
			while(*str) {
				str++;
				len++;
			}

			resize(len);
			for (size_t i = 0; i < len; ++i) data_[i] = str[i];
			return *this;
		}
		string& operator=(const char* str) noexcept {
			size_t len = 0;
			while(*str) {
				str++;
				len++;
			}

			resize(len);
			for (size_t i = 0; i < len; ++i) data_[i] = str[i];
			return *this;
		}

		const char* c_str() noexcept {
			return data_;
		}
	};
}