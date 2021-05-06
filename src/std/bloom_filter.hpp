#pragma once

#include "std/hash.hpp"

namespace xstd {
	
	template<size_t M, size_t K>
	struct bloom_filter_idx {
		bool T[M];

		constexpr bool test(size_t k) noexcept {
			auto h1 = xstd::hash_op(k);
			auto h2 = xstd::hash_op(xstd::hash_combine(k, h1));

			for (size_t i = 0; i < K; ++i) {
				auto h = h1 + i * h2;

				if (!T[h % M]) return false;
			}

			return true;
		}

		constexpr void insert(size_t k) noexcept {
			auto h1 = xstd::hash_op(k);
			auto h2 = xstd::hash_op(xstd::hash_combine(k, h1));

			for (size_t i = 0; i < K; ++i) {
				auto h = h1 + i * h2;
				T[h % M] = true;
			}
		}
	};

}