#pragma once
#include <cassert>

#include <vector>
#include <atomic>
#include <unordered_map>

namespace details {
	template<typename Callable>
	struct Defer {
		~Defer() noexcept { todo(); }
		Defer(Callable todo) noexcept : todo(todo) {};
	private:
		Callable todo;
	};
};

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define defer details::Defer CONCAT(defer_, __COUNTER__) = [&]

#define BEG(x) std::begin(x)
#define END(x) std::end(x)
#define BEG_END(x) BEG(x), END(x)
#define V2F(x) Vector2f{(x), (x)}
#define V4F(x) Vector4f{(x), (x), (x), (x)}

#define dbgbool(x) printf(#x " = %s\n", (x) ? "true" : "false");
#define dbgstr(x) printf(#x " = %s\n", x);

#define for2(i, j, vec) for (size_t j = 0; j < vec.y; ++j) for (size_t i = 0; i < vec.x; ++i)

template<bool flag = false> void static_no_match() noexcept {
	static_assert(flag, "No match.");
}

#define sum_type_X_Kind(x) , x##_Kind
#define sum_type_X_Union(x) x x##_;
#define sum_type_X_cst(x) else if constexpr (std::is_same_v<T, x>) {\
	kind = x##_Kind; new (&x##_) x; x##_ = (y);\
}
#define sum_type_X_case_cpy(x) case x##_Kind: new(&x##_) x; x##_ = that.x##_; break;
#define sum_type_X_case_mve(x) case x##_Kind: new(&x##_) x; x##_ = std::move(that.x##_); break;
#define sum_type_X_dst(x) case x##_Kind: x##_ .x::~x (); break;
#define sum_type_X_name(x) case x##_Kind: return #x;
#define sum_type_X_cast(x) if constexpr (std::is_same_v<T, x>) { return x##_; }
#define sum_type_X_one_of(x) std::is_same_v<T, x> ||

#define sum_type(n, list)\
		union { list(sum_type_X_Union) };\
		enum Kind { None_Kind = 0 list(sum_type_X_Kind) } kind;\
		n() noexcept { kind = None_Kind; }\
		template<typename T> n(const T& y) noexcept {\
			if constexpr (false);\
			list(sum_type_X_cst)\
			else static_no_match<list(sum_type_X_one_of) false>();\
		}\
		~n() {\
			switch(kind) {\
				list(sum_type_X_dst)\
				default: break;\
			}\
		}\
		n(std::nullptr_t) noexcept { kind = None_Kind; }\
		n(n&& that) noexcept {\
			kind = that.kind;\
			switch (kind) {\
				list(sum_type_X_case_mve)\
				default: break;\
			}\
		}\
		n(const n& that) noexcept {\
			kind = that.kind;\
			switch (kind) {\
				list(sum_type_X_case_cpy)\
				default: break;\
			}\
		}\
		n& operator=(const n& that) noexcept {\
			kind = that.kind;\
			switch (kind) {\
				list(sum_type_X_case_cpy)\
				default: break;\
			}\
			return *this;\
		}\
		n& operator=(n&& that) noexcept {\
			kind = that.kind;\
			switch (kind) {\
				list(sum_type_X_case_mve)\
				default: break;\
			}\
			return *this;\
		}\
		const char* name() const noexcept {\
			switch (kind) {\
				list(sum_type_X_name)\
				default: break;\
			}\
			return "??";\
		}\
		bool typecheck(n::Kind k) const noexcept { return kind == k; }\
		template<typename T>\
		const T& cast() const noexcept {\
			list(sum_type_X_cast)\
			assert("Yeah no.");\
			return *reinterpret_cast<const T*>(this);\
		}\
		template<typename T>\
		T cast() noexcept {\
			list(sum_type_X_cast)\
			assert("Yeah no.");\
			return *reinterpret_cast<T*>(this);\
		}

#define sum_type_base(base)\
	base* operator->() noexcept { return (base*)this; }\
	const base* operator->() const noexcept { return (base*)this; }

#include <chrono>

namespace xstd {

	[[nodiscard]] inline std::uint64_t nanoseconds() noexcept {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now().time_since_epoch()
		).count();
	}
	[[nodiscard]] inline double seconds() noexcept {
		return nanoseconds() / 1'000'000'000.0;
	}
	[[nodiscard]] inline std::uint64_t uuid() noexcept {
		static std::atomic<std::uint16_t> counter = 0;
		return (nanoseconds() << 16) | (counter++);
	}
	template<typename T>
	constexpr T max(T a, T b) noexcept {
		return a > b ? a : b;
	}
	template<typename T>
	constexpr T min(T a, T b) noexcept {
		return a < b ? a : b;
	}
	
	inline std::vector<std::string> split(std::string_view str, std::string_view delim) noexcept {
		std::vector<std::string> result;

		size_t i = 0;
		for (; i < str.size(); i += delim.size()) {
			auto to = str.find_first_of(delim, i);
			result.push_back((std::string)str.substr(i, to - i));
			i = to;
			if (i == str.npos) break;
		}

		return result;
	}

	constexpr inline size_t hash_combine(size_t a, size_t b) noexcept {
#pragma warning(push)
#pragma warning(disable: 4307)
		return a + 0x9e3779b9u + (b << 6) + (b >> 2);
#pragma warning(pop)
	}

	inline size_t seed() noexcept {
		auto s = time(nullptr);
		srand(s);
		return s;
	}
	inline void seed(size_t s) noexcept {
		srand(s);
	}

	inline double random() noexcept {
		// i know whatever.
		return rand() / (double)RAND_MAX;
	}


	template<typename T>
	struct Pool {
		std::vector<T> pool;
		std::unordered_map<size_t, size_t> pool_ids;

		void resize(size_t n, T v = {}) noexcept {
			pool.resize(n, v);
			pool_ids.clear();
			for (size_t i = 0; i < pool.size(); ++i) pool_ids[pool[i].id] = i;
		}
		void clear() noexcept {
			pool.clear();
			pool_ids.clear();
		}
		void push_back(const T& v) noexcept {
			pool_ids[v.id] = pool.size();
			pool.push_back(v);
		}

		void remove_at(size_t idx) noexcept {
			for (auto& [_, x] : pool_ids) if (x > idx) x--;
			pool_ids.erase(pool[idx].id);
			pool.erase(std::begin(pool) + idx);
		}

		auto begin() noexcept {
			return std::begin(pool);
		}
		auto end() noexcept {
			return std::end(pool);
		}

		T& operator[](size_t idx) noexcept {
			return pool[idx];
		}

		T& id(size_t id) noexcept {
			return pool[pool_ids.at(id)];
		}

		size_t size() const noexcept {
			return pool.size();
		}

		bool exist(size_t id) noexcept { return pool_ids.contains(id); }
	};
}
constexpr size_t operator""_id(const char* user, size_t size) {
	size_t seed = 0;
	for (size_t i = 0; i < size; ++i) seed = xstd::hash_combine(seed, (size_t)user[i]);
	return seed;
}