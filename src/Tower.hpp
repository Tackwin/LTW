#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Graphic/Render.hpp"

struct Tower_Base {
	Vector2u tile_pos;
	Vector2u tile_size = {1, 1};
	Vector4f color = {1, 0, 0, 1};
};

struct Archer : public Tower_Base {
	float range        = 5.0f;
	float damage       = 1.0f;
	float attack_speed = 1.0f;
	float attack_cd    = 0.0f;

	Archer() noexcept { color = {1, 0, 0, 1}; }
};

struct Sharp : public Tower_Base {
	float range  = 1.0f;
	float damage = 0.1f;

	Sharp() noexcept { color = {1, 1, 0, 1}; }
};

#define TOWER_LIST(X) X(Archer) X(Sharp)

struct Tower {
	sum_type(Tower, TOWER_LIST);
	sum_type_base(Tower_Base);

	inline static size_t Tower_Id = 0;
	size_t id = Tower_Id++;
	bool to_remove = false;
};
