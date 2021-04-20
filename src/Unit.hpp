#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Managers/AssetsManager.hpp"

struct Unit_Base {
	size_t object_id = 0;

	float life_time = 0;

	size_t current_tile = 0;
	size_t target_tile  = SIZE_MAX;
	Vector2f pos;
	Vector2f last_pos;
	float speed = 0.5f;

	float health = 1.f;

	size_t income = 1;
	size_t cost   = 5;
	size_t batch  = 5;

	Vector3f color = {1, 1, 1};

	bool to_die = false;
};

struct Methane : Unit_Base {
	Methane() noexcept {
		health = 1.f;
		life_time += xstd::random();
		speed = 0.5f;
		object_id = asset::Object_Id::Methane;
	}
};
struct Ethane : Unit_Base {
	Ethane() noexcept {
		health = 3.f;
		life_time += xstd::random();
		speed = 0.4f;
		object_id = asset::Object_Id::Ethane;
		color /= 2;
	}
};
struct Propane : Unit_Base {
	Propane() noexcept {
		health = 4.5f;
		life_time += xstd::random();
		speed = 0.3f;
		object_id = asset::Object_Id::Propane;
		color /= 3;
	}
};
struct Butane : Unit_Base {
	Butane() noexcept {
		health = 5.5f;
		life_time += xstd::random();
		speed = 0.15f;
		object_id = asset::Object_Id::Butane;
		color /= 5;
	}
};
struct Water   : Unit_Base {
	Water()   noexcept {
		life_time += xstd::random();
		object_id = asset::Object_Id::Water;
		health = 0.5f;
		speed = 2;
	}
};
struct Oxygen  : Unit_Base {
	Oxygen() noexcept {
		life_time += xstd::random();
		object_id = asset::Object_Id::Oxygen;
		health = 1.f;
		speed  = 1.f;
	}
};

#define LIST_UNIT(X) X(Methane) X(Ethane) X(Propane) X(Butane) X(Water) X(Oxygen)

struct Unit {
	sum_type(Unit, LIST_UNIT);
	sum_type_base(Unit_Base);

	size_t id = 0;
	bool to_remove = false;
};

struct Board;
extern void on_death(Board& board, Unit& unit) noexcept;