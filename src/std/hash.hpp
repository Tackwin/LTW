#pragma once

#include <type_traits>

namespace xstd {

	template<typename T>
	struct hash {
		template<typename U = T>
		std::enable_if_t<std::is_enum_v<U>, unsigned long long> operator()(U x) noexcept {
			return hash<unsigned long long>()((unsigned long long)x);
		}
	};

	template<>
	struct hash<unsigned long long> {
		unsigned long long operator()(unsigned long long x) noexcept {
			x = (x ^ (x >> 31) ^ (x >> 62)) * 0x319642b2d24d8ec3ull;
			x = (x ^ (x >> 27) ^ (x >> 54)) * 0x96de1b173f119089ull;
			x = x ^ (x >> 30) ^ (x >> 60);
			return x;
		}
	};

	constexpr inline size_t hash_op(size_t x) noexcept {
		x = (x ^ (x >> 31) ^ (x >> 62)) * 0x319642b2d24d8ec3ull;
		x = (x ^ (x >> 27) ^ (x >> 54)) * 0x96de1b173f119089ull;
		x = x ^ (x >> 30) ^ (x >> 60);
		return x;
	}
	
	constexpr inline size_t hash_combine(size_t a, size_t b) noexcept {
#pragma warning(push)
#pragma warning(disable: 4307)
		return a + 0x9e3779b9u + (b << 6) + (b >> 2);
#pragma warning(pop)
	}

}