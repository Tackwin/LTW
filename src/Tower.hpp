#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Graphic/Render.hpp"

#include "Managers/AssetsManager.hpp"

struct Tower_Base {
	enum Target_Mode {
		First = 0,
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
	Vector4f color = {1, 0, 0, 1};

	Target_Mode target_mode;

	size_t object_id = 0;
	size_t gold_cost = 5;

	Tower_Base() noexcept {}
};

struct Archer : public Tower_Base {
	float range        = 5.0f;
	float damage       = 1.0f;
	float attack_speed = 1.0f;
	float attack_cd    = 0.0f;

	Archer() noexcept {
		color = {1, 0, 0, 1};
		tile_size = {1, 1};
		object_id = asset::Object_Id::Range1;
	}
};

struct Sharp : public Tower_Base {
	float range  = 1.0f;
	float damage = 0.1f;

	Sharp() noexcept {
		color = {1, 1, 0, 1};
		tile_size = {2, 2};
		object_id = asset::Object_Id::Sharp;
	}
};

#define TOWER_LIST(X) X(Archer) X(Sharp)

struct Tower {
	sum_type(Tower, TOWER_LIST);
	sum_type_base(Tower_Base);

	size_t id = 0;
	bool to_remove = false;
};

