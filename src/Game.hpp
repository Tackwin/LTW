#pragma once

#include <vector>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"

#include "Managers/InputsManager.hpp"

inline static size_t Id_Increment = 1;
struct Common_Tile {
	Vector4f color = {.1f, .1f, .1f, 1};
};
struct Empty : Common_Tile {
	Empty() noexcept { color = {.1f, .1f, .1f, 1}; }
};
struct Archer : Common_Tile {
	float range = 5.f;
	float attack_speed = 1.f;
	float attack_cd = 0;
	Archer() noexcept { color = {1, 0, 0, 1}; }
};
struct Sharp : Common_Tile {
	Sharp()  noexcept { color = {0, 1, 0, 1}; }
};

#define TILE_LIST(X)\
	X(Archer) X(Sharp) X(Empty)
struct Tile {
	sum_type(Tile, TILE_LIST);
	sum_type_base(Common_Tile);
	size_t id = Id_Increment++;
};

struct Projectile {
	size_t id = Id_Increment++;

	size_t from = 0;
	size_t to = 0;

	Vector2f pos;
	Vector4f color;
	float r = 0.05f;
	float speed = 20.f;
	float damage = 0.1f;

	bool to_remove = false;
};

struct Unit {
	size_t id = Id_Increment++;

	size_t current_tile = 0;
	Vector2f pos;
	float speed = 5.f;

	float health = 1.f;

	bool to_remove = false;
};

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

struct Board {
	struct Gui {
		bool render_path = false;
	} gui;

	Vector2u size = { 10, 30 };
	float tile_size = 0.99f;
	float tile_padding = 0.01f;

	size_t start_zone_height = 2;
	size_t cease_zone_height = 2;
	Pool<Tile> tiles;

	Pool<Unit> units;
	Pool<Projectile> projectiles;

	std::vector<size_t> next_tile;

	void input(const Input_Info& in, Vector2f mouse_world_pos) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	Tile& tile(Vector2u pos) noexcept;
	Rectanglef tile_box(Vector2u pos) noexcept;
	Rectanglef tile_box(size_t idx) noexcept { return tile_box({ idx % size.x, idx / size.x }); }

	void compute_paths() noexcept;
};


struct Game {
	struct Gui {
		bool board_open = true;
		bool game_debug_open = true;
	} gui;
	
	Board board;
	Input_Record input_record;

	render::Camera camera{ .pos = {0, 0}, .frame_size = { 16, 9 }};
	float camera_speed = 1.f;
	
	Vector2f zqsd_vector;
};


#ifdef BUILD_DLL
#define EXTERN extern __declspec(dllexport)
#else
#define EXTERN extern
#endif

EXTERN void game_update(Game& game, double dt) noexcept;
EXTERN void game_render(Game& game, render::Orders& orders) noexcept;
EXTERN void game_startup(Game& game) noexcept;
EXTERN void game_shutdown(Game& game) noexcept;