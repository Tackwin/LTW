#pragma once

#include <type_traits>
#include "int.hpp"

namespace xstd {

#ifndef __EMSCRIPTEN__
	constexpr inline size_t hash_op(size_t x) noexcept {
		x = (x ^ (x >> 31) ^ (x >> 62)) * 0x319642b2d24d8ec3ull;
		x = (x ^ (x >> 27) ^ (x >> 54)) * 0x96de1b173f119089ull;
		x = x ^ (x >> 30) ^ (x >> 60);
		return x;
	}
#endif
	constexpr inline uint32_t hash_op(uint32_t x) noexcept {
		return x;
	}


	template<typename T>
	struct hash {
		template<typename U = T>
		std::enable_if_t<std::is_enum_v<U>, size_t> operator()(U x) noexcept {
			return (size_t)x;
		}
	};

	#ifndef __EMSCRIPTEN__
	template<>
	struct hash<uint64_t> {
		uint64_t operator()(uint64_t x) noexcept {
			return hash_op(x);
		}
	};
	#endif
	template<>
	struct hash<uint32_t> {
		uint32_t operator()(uint32_t x) noexcept {
			return x;
		}
	};
	template<>
	struct hash<unsigned long> {
		unsigned long operator()(unsigned long x) noexcept {
			return x;
		}
	};

	
	constexpr inline size_t hash_combine(size_t a, size_t b) noexcept {
#pragma warning(push)
#pragma warning(disable: 4307)
		return a + 0x9e3779b9u + (b << 6) + (b >> 2);
#pragma warning(pop)
	}

}