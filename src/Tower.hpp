#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Graphic/Render.hpp"

#include "Managers/AssetsManager.hpp"

struct Tower_Base {
	union {
		Rectangleu tile_rec;
		struct {
			Vector2u tile_pos;
			Vector2u tile_size = {1, 1};
		};
	};

	size_t object_id = 0;
	size_t gold_cost = 5;
	size_t texture_icon_id = 0;

	size_t kill_count = 0;

	float attack_speed = 1;
	float attack_cd = 0;

	float attack_slowdown_cd = 0;
	float attack_slowdown    = 1;

	Tower_Base() noexcept {}
	virtual ~Tower_Base() noexcept {};
};

struct Unit;
struct Tower_Target {
	enum Target_Mode {
		First = 0,
		Closest,
		Farthest,
		Random,
		Count
	};

	size_t target_id;
	Target_Mode target_mode;
};

struct Tower_Seek_Projectile_Shoot {
	float damage       = 1;
	float range        = 1;
};

struct Mirror : public Tower_Base, Tower_Target, Tower_Seek_Projectile_Shoot {
	Mirror() noexcept {
		range        = 3.5f;
		damage       = 1.0f;
		attack_speed = 1.0f;
		attack_cd    = 0.0f;
		tile_size = {2, 2};
		object_id = asset::Object_Id::Mirror;
		target_mode = Target_Mode::First;
		texture_icon_id = asset::Texture_Id::Mirror_Icon;
	}
};

struct Mirror2 : public Mirror {
	Mirror2() noexcept {
		range = 5.f;
		attack_speed = 4.f;
		gold_cost = 25;
		tile_size = {2, 2};
		object_id = asset::Object_Id::Mirror2;
		target_mode = Target_Mode::First;
		texture_icon_id = asset::Texture_Id::Mirror_Icon;
	}
};

struct Heat : public Tower_Base {
	float range        = 2;
	size_t target_id;
	Tower_Target::Target_Mode target_mode;

	Heat() noexcept {
		tile_size = {2, 2};
		object_id = asset::Object_Id::Heat;
		target_mode = Tower_Target::Target_Mode::Random;
		texture_icon_id = asset::Texture_Id::Heat_Icon;
	}
};

struct Sharp : public Tower_Base {
	float range  = 1.0f;
	float damage = 0.1f;

	float rot = 0.f;
	float last_rot = 0.f;

	Sharp() noexcept {
		tile_size = {3, 3};
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
		tile_size = {4, 4};
		object_id = asset::Object_Id::Volter;
		texture_icon_id = asset::Texture_Id::Volter_Icon;
	}
};

struct Radiation : public Tower_Base {
	float range        = 4;

	size_t target_id;
	Tower_Target::Target_Mode target_mode;

	Radiation() noexcept {
		tile_size = {2, 2};
		object_id = asset::Object_Id::Radiation;
		texture_icon_id = asset::Texture_Id::Radiation_Icon;
	}
};

struct Circuit : public Tower_Base {
	float range         = 4;

	size_t target_id = 0;
	Tower_Target::Target_Mode target_mode;

	Circuit() noexcept {
		tile_size = {2, 2};
		object_id = asset::Object_Id::Circuit;
		texture_icon_id = asset::Texture_Id::Circuit_Icon;
	}
};

struct Block_Tower : public Tower_Base {
	Block_Tower() noexcept {
		tile_size = {1, 1};
		object_id = asset::Object_Id::Block;
		texture_icon_id = asset::Texture_Id::Dummy;
	}
};

// >ADD_TOWER(Tackwin):
#define TOWER_TARGET_LIST Mirror, Mirror2, Heat, Radiation, Circuit
#define TOWER_SHOOT_LIST Heat, Radiation, Circuit
#define TOWER_SEEK_PROJECTILE Mirror, Mirror2
#define TOWER_LIST(X) X(Mirror) X(Mirror2) X(Sharp) X(Volter) X(Heat) X(Radiation) X(Circuit)\
	X(Block_Tower)

struct Tower {
	sum_type(Tower, TOWER_LIST);
	sum_type_base(Tower_Base);

	size_t id = 0;
	bool to_remove = false;

	Kind get_upgrade() noexcept;
};

