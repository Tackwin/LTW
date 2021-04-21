#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Graphic/Render.hpp"

#include "Managers/AssetsManager.hpp"

struct Tower_Base {
	// >TOWER_TARGET_MODE:
	enum Target_Mode {
		First = 0,
		Closest,
		Farthest,
		Random,
		Count
	};

	union {
		Rectangleu tile_rec;
		struct {
			Vector2u tile_pos;
			Vector2u tile_size = {1, 1};
		};
	};

	Target_Mode target_mode;

	size_t object_id = 0;
	size_t gold_cost = 5;
	size_t target_id = 0;
	size_t texture_icon_id = 0;

	size_t kill_count = 0;

	Tower_Base() noexcept {}
};

struct Archer : public Tower_Base {
	float range        = 3.5f;
	float damage       = 1.0f;
	float attack_speed = 1.0f;
	float attack_cd    = 0.0f;

	Archer() noexcept {
		tile_size = {3, 3};
		object_id = asset::Object_Id::Range1;
		target_mode = Target_Mode::First;
		texture_icon_id = asset::Texture_Id::Range1_Icon;
	}
};

struct Sharp : public Tower_Base {
	float range  = 1.0f;
	float damage = 0.1f;

	float rot = 0.f;
	float last_rot = 0.f;

	Sharp() noexcept {
		tile_size = {4, 4};
		object_id = asset::Object_Id::Sharp;

		texture_icon_id = asset::Texture_Id::Sharp_Icon;
	}
};

struct Volter : public Tower_Base {
	bool to_surge = false;
	bool always_surge = false;
	float surge_timer = 0.f;
	float surge_time  = 30.f;

	Volter() noexcept {
		tile_size = {6, 6};
		object_id = asset::Object_Id::Volter;
		texture_icon_id = asset::Texture_Id::Volter_Icon;
	}
};

#define TOWER_LIST(X) X(Archer) X(Sharp) X(Volter)

struct Tower {
	sum_type(Tower, TOWER_LIST);
	sum_type_base(Tower_Base);

	size_t id = 0;
	bool to_remove = false;
};

