#pragma once

#include <vector>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"

#include "Managers/InputsManager.hpp"

#include "Interface.hpp"
#include "Tower.hpp"

inline static size_t Id_Increment = 1;
struct Common_Tile {
	Vector4f color = {.1f, .1f, .1f, 1};
};
struct Empty : Common_Tile {
	Empty() noexcept { color = {.1f, .1f, .1f, 1}; }
};
#define TILE_LIST(X) X(Empty)
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

	Vector2f pos = {0, 0};
	Vector2u size = { 20, 50 };
	float tile_size = 0.49f;
	float tile_padding = 0.01f;

	size_t start_zone_height = 2;
	size_t cease_zone_height = 2;

	Pool<Tile> tiles;
	Pool<Unit> units;
	Pool<Tower> towers;
	Pool<Projectile> projectiles;

	std::vector<size_t> next_tile;

	size_t gold_gained = 0;

	void input(const Input_Info& in, Vector2f mouse_world_pos) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	Tile& tile(Vector2u pos) noexcept;
	Rectanglef tile_box(Vector2u pos) noexcept;
	Rectanglef tile_box(size_t idx) noexcept { return tile_box({ idx % size.x, idx / size.x }); }

	void soft_compute_paths() noexcept;
	Tile* get_tile_at(Vector2f x) noexcept;
};

struct Player {
	size_t gold = 0;

	Tower placing = nullptr;
};

struct Game {
	struct Gui {
		bool board_open = true;
		bool game_debug_open = true;
	} gui;

	Board board;
	Player player;
	Input_Record input_record;
	Interface interface;

	render::Camera camera;
	float camera_speed = 1.f;
	
	Vector2f zqsd_vector;

	size_t golds = 0;

	Game() noexcept {
		camera.pos = {0, 0};
		camera.frame_size = { 16, 9 };
	}

	Interface::Ressource_Info construct_interface_ressource_info() noexcept;
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