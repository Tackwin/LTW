#pragma once 

#include <optional>
#include <unordered_set>

#include "xstd.hpp"
#include "Managers/InputsManager.hpp"
#include "Math/Vector.hpp"
#include "Tower.hpp"

struct Common_Tile {
	Vector2u tile_pos = {0, 0};
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
	sum_type(Tile, TILE_LIST);
	sum_type_base(Common_Tile);
	size_t id = 0;
};

struct Projectile {
	size_t id = 0;

	size_t from = 0;
	size_t to = 0;

	Vector2f pos;
	Vector4f color;
	float r = 0.1f;
	float speed = 5.f;
	float damage = 0.5f;

	bool to_remove = false;
};

struct Unit {
	size_t id = 0;

	float life_time = 0;

	size_t current_tile = 0;
	Vector2f pos;
	float speed = 0.5f;

	float health = 1.f;

	size_t income = 1;
	size_t cost   = 5;
	size_t batch  = 5;

	bool to_remove = false;
};

struct Board {
	struct Gui {
		bool render_path = false;
	} gui;

	Vector2f pos = {0, 0};
	Vector2u size = { 16, 50 };
	float tile_size = 0.49f;
	float tile_padding = 0.01f;

	size_t start_zone_height = 2;
	size_t cease_zone_height = 2;

	xstd::Pool<Tile> tiles;
	xstd::Pool<Unit> units;
	xstd::Pool<Tower> towers;
	xstd::Pool<Projectile> projectiles;

	size_t tower_selected = 0;

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
	Rectanglef tower_box(const Tower& tower) noexcept;

	void soft_compute_paths() noexcept;
	void compute_paths() noexcept;
	void invalidate_paths() noexcept;

	std::optional<Vector2u> get_tile_at(Vector2f x) noexcept;

	void insert_tower(Tower t) noexcept;
	void remove_tower(Vector2u p) noexcept;
	void remove_tower(Tower& p) noexcept;

	void spawn_unit(Unit u) noexcept;

	float bounding_tile_size() noexcept { return tile_size + tile_padding; };
	bool can_place_at(Rectangleu zone) noexcept;
};