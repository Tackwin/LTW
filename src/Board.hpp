#pragma once 
#include <unordered_set>

#include "xstd.hpp"
#include "Managers/InputsManager.hpp"
#include "Math/Vector.hpp"
#include "Tower.hpp"

struct Common_Tile {
	Vector4f color = {.1f, .1f, .1f, 1};
	bool passthrough = true;
};
struct Empty : Common_Tile {
	Empty() noexcept { color = {.1f, .1f, .1f, 1}; }
};
struct Block : Common_Tile {
	Block() noexcept {
		color = {.4f, .4f, .4f, 1};
		passthrough = false;
	}
};
#define TILE_LIST(X) X(Empty) X(Block)
struct Tile {
	inline static size_t Id_Increment = 1;
	sum_type(Tile, TILE_LIST);
	sum_type_base(Common_Tile);
	size_t id = Id_Increment++;
};

struct Projectile {
	inline static size_t Id_Increment = 0;
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
	inline static size_t Id_Increment = 0;
	size_t id = Id_Increment++;

	size_t current_tile = 0;
	Vector2f pos;
	float speed = 5.f;

	float health = 1.f;

	bool to_remove = false;
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

	xstd::Pool<Tile> tiles;
	xstd::Pool<Unit> units;
	xstd::Pool<Tower> towers;
	xstd::Pool<Projectile> projectiles;

	std::vector<size_t> next_tile;
	struct Path_Construction {
		std::vector<bool> closed;
		std::vector<size_t> open;
		size_t open_idx = 0;
		bool dirty = true;
		bool soft_dirty = false;
	} path_construction;

	size_t gold_gained = 0;

	void input(const Input_Info& in, Vector2f mouse_world_pos) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	Tile& tile(Vector2u pos) noexcept;
	Rectanglef tile_box(Vector2u pos) noexcept;
	Rectanglef tile_box(size_t idx) noexcept { return tile_box({ idx % size.x, idx / size.x }); }

	void soft_compute_paths() noexcept;
	void compute_paths() noexcept;
	void invalidate_paths() noexcept;


	Tile* get_tile_at(Vector2f x) noexcept;

	void insert_tower(Tower t) noexcept;
	void remove_tower(Vector2u p) noexcept;
};