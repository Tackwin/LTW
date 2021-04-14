#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Managers/AssetsManager.hpp"

struct Unit_Base {
	size_t object_id = 0;

	float life_time = 0;

	size_t current_tile = 0;
	Vector2f pos;
	float speed = 0.5f;

	float health = 1.f;

	size_t income = 1;
	size_t cost   = 5;
	size_t batch  = 5;

};

struct Methane : Unit_Base {
	Methane() noexcept { object_id = asset::Object_Id::Methane; }
};
struct Water   : Unit_Base {
	Water()   noexcept { object_id = asset::Object_Id::Water; }
};

#define LIST_UNIT(X) X(Methane) X(Water)

struct Unit {
	sum_type(Unit, LIST_UNIT);
	sum_type_base(Unit_Base);

	size_t id = 0;
	bool to_remove = false;
};