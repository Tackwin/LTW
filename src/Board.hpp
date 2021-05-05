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
	float speed = 5.f;

	size_t object_id = 0;
	size_t from = 0;

	float life_time = FLT_MAX;

	bool hit = false;
};

struct Seek_Projectile : Base_Projectile {
	size_t to = 0;

	float damage = 0.5f;

	size_t unit_hit = 0;

	Seek_Projectile() noexcept {
		object_id = asset::Object_Id::Photon;
	}
};
struct Straight_Projectile : Base_Projectile {
	float damage = 0.5f;

	size_t power = 10;
	size_t unit_hit = 0;

	Straight_Projectile() noexcept {
		speed = 2.f;
		object_id = asset::Object_Id::Electron;
	}
};

struct Split_Projectile : Base_Projectile {
	float split_chance = 0.6f;
	size_t n_split = 2;
	size_t max_split = 30;

	size_t unit_hit = 0;

	float damage = 1.f;

	Split_Projectile() noexcept {
		speed = 1.f;
		object_id = asset::Object_Id::Neutron;
	}
};

struct Splash_Projectile : Base_Projectile {
	float aoe_radius = 0;
	Vector2f target;

	Splash_Projectile() {
		speed = 3.f;
		object_id = asset::Object_Id::Heat_Surge;
	}
};

// travel behavior
#define PROJ_SEEK_LIST Seek_Projectile
#define PROJ_TARGET_LIST Splash_Projectile
#define PROJ_STRAIGHT_LIST Straight_Projectile, Split_Projectile

// hit behavior
#define PROJ_SPLASH_LIST Splash_Projectile
#define PROJ_SPLIT_LIST Split_Projectile
#define PROJ_SIMPLE_HIT_LIST Seek_Projectile, Split_Projectile

#define PROJ_LIST(X)\
	X(Seek_Projectile) X(Straight_Projectile) X(Splash_Projectile) X(Split_Projectile)

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
	Vector2u size = { 60, 20 };
	float tile_size = 0.39f;
	float tile_padding = 0.01f;

	double seconds_elapsed = 0.0;

	size_t start_zone_width = 2;
	size_t cease_zone_width = 2;

	xstd::Pool<Tile> tiles;
	xstd::Pool<Unit> units;
	xstd::Pool<Tower> towers;
	xstd::Pool<Projectile> projectiles;

	std::vector<Unit> unit_to_add;
	std::vector<Projectile> proj_to_add;

	struct Effect {
		Vector3f pos;
		Vector4f color = {1, 1, 1, 1};
		static constexpr float Lifetime = 0.1f;
		float age = Lifetime;
	};
	std::vector<Effect> effects;

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

	Rectanglef tile_box(Rectangleu rec) noexcept { return tile_box(rec.pos, rec.size); }
	Rectanglef tile_box(Vector2u pos, Vector2u size = {1, 1}) noexcept;
	Rectanglef tile_box(size_t idx) noexcept { return tile_box(idx_to_vec(idx)); }
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
	void spawn_unit_at(Unit u, size_t idx) noexcept {
		spawn_unit_at(std::move(u), {idx / size.y, idx % size.y});
	}
	void spawn_unit_at(Unit u, Vector2u tile) noexcept;

	Tile& at(size_t idx) noexcept { return at(idx_to_vec(idx)); }
	Tile& at(Vector2u p) noexcept;

	size_t vec_to_idx(Vector2u x) noexcept { return x.y + x.x * size.y; }
	Vector2u idx_to_vec(size_t x) noexcept { return {x / size.y, x % size.y}; }

	float bounding_tile_size() noexcept { return tile_size + tile_padding; };
	bool can_place_at(Rectangleu zone) noexcept;

	void hit_event_at(Vector3f pos, const Projectile& proj) noexcept;
	void die_event_at(Unit& u) noexcept;

	void pick_new_target(Tower& tower) noexcept;
	bool is_valid_target(const Tower& t, const Unit& u) noexcept;

	Projectile get_projectile(Tower& from, Unit& target) noexcept;
};