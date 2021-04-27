#pragma once 

#include <optional>
#include <unordered_set>

#include "xstd.hpp"
#include "Managers/InputsManager.hpp"
#include "Math/Vector.hpp"
#include "Tower.hpp"
#include "Unit.hpp"

#include "Wave.hpp"

struct Common_Tile {
	Vector2u tile_pos = {0, 0};
	Vector4f color = {.1f, .1f, .1f, 1};
	bool passthrough = true;
};
struct Empty : Common_Tile {
	bool end = false;

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

struct Base_Projectile {
	Vector2f pos;
	Vector2f dir;

	Vector2f last_pos;
	Vector2f last_dir;
	float r = 0.1f;

	size_t object_id = 0;

	float life_time = FLT_MAX;
};

struct Seek_Projectile : Base_Projectile {
	size_t from = 0;
	size_t to = 0;

	Vector4f color;

	float speed = 5.f;
	float damage = 0.5f;

	Seek_Projectile() noexcept {
		object_id = asset::Object_Id::Photon;
	}
};
struct Straight_Projectile : Base_Projectile {
	size_t from = 0;

	float speed = 2.f;
	float damage = 0.5f;

	size_t power = 10;

	Straight_Projectile() noexcept {
		object_id = asset::Object_Id::Electron;
	}
};

#define PROJ_LIST(X) X(Seek_Projectile) X(Straight_Projectile)

struct Projectile {
	sum_type(Projectile, PROJ_LIST);
	sum_type_base(Base_Projectile);
	size_t id = 0;
	bool to_remove = false;
};

struct Board {
	struct Gui {
		bool render_path = false;
	} gui;

	Vector2f pos = {0, 0};
	Vector2u size = { 40, 120 };
	float tile_size = 0.19f;
	float tile_padding = 0.01f;

	double seconds_elapsed = 0.0;

	size_t start_zone_height = 2;
	size_t cease_zone_height = 2;

	xstd::Pool<Tile> tiles;
	xstd::Pool<Unit> units;
	xstd::Pool<Tower> towers;
	xstd::Pool<Projectile> projectiles;

	struct Die_Effect {
		Vector3f pos;
		static constexpr float Lifetime = 0.1f;
		float age = Lifetime;
	};
	std::vector<Die_Effect> die_effects;

	std::vector<std::vector<size_t>> unit_id_by_tile;

	std::vector<size_t> next_tile;
	std::vector<size_t> dist_tile;
	struct Path_Construction {
		std::vector<bool> closed;
		std::vector<size_t> open;
		size_t open_idx = 0;
		bool dirty = true;
		bool soft_dirty = false;
	} path_construction;

	Ressources ressources_gained;

	Wave current_wave;

	void input(const Input_Info& in, Vector2f mouse_world_pos) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	Tile& tile(Vector2u pos) noexcept;
	Rectanglef tile_box(Rectangleu rec) noexcept { return tile_box(rec.pos, rec.size); }
	Rectanglef tile_box(Vector2u pos, Vector2u size = {1, 1}) noexcept;
	Rectanglef tile_box(size_t idx) noexcept { return tile_box({ idx % size.x, idx / size.x }); }
	Rectanglef tower_box(const Tower& tower) noexcept;

	void unit_spatial_partition() noexcept;

	void soft_compute_paths() noexcept;
	void compute_paths() noexcept;
	void invalidate_paths() noexcept;

	std::optional<Vector2u> get_tile_at(Vector2f x) noexcept;

	void insert_tower(Tower t) noexcept;
	void remove_tower(Vector2u p) noexcept;
	void remove_tower(Tower& p) noexcept;

	void spawn_unit(Unit u) noexcept;
	void spawn_unit_at(Unit u, Vector2u tile) noexcept;

	void pick_new_target(Mirror& tower) noexcept;
	void pick_new_target(Mirror2& tower) noexcept;

	float bounding_tile_size() noexcept { return tile_size + tile_padding; };
	bool can_place_at(Rectangleu zone) noexcept;

	void die_event_at(Vector3f pos) noexcept;
};