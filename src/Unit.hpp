#pragma once

#include "xstd.hpp"
#include "Math/Vector.hpp"
#include "Managers/AssetsManager.hpp"

// >TODO(Tackwin): Besoin de ce include pour Ressource seulement
#include "Player.hpp"

struct Unit_Base {
	size_t object_id = 0;

	float life_time = 0;

	size_t current_tile = 0;
	size_t target_tile  = SIZE_MAX;
	Vector2f pos;
	Vector2f last_pos;
	float speed = 2.f;

	float health = 1.f;

	size_t income = 1;
	size_t cost   = 5;
	size_t batch  = 5;

	Vector3f color = {1, 1, 1};

	bool to_die = false;
	float invincible = 0.f;

	virtual void hit(float damage) noexcept;

	virtual Ressources get_drop() noexcept { return {}; }
};

struct Methane : Unit_Base {
	Methane() noexcept {
		speed = 2.f;
		object_id = asset::Object_Id::Methane;
	}

	virtual Ressources get_drop() noexcept { return {.gold = 1, .carbons = 1, .hydrogens = 4}; }
};
struct Ethane : Unit_Base {
	using split_to = Methane;
	size_t     split_n  = 2;

	Ethane() noexcept {
		speed = 1.5f;
		object_id = asset::Object_Id::Ethane;
		color /= 2;
	}
	virtual Ressources get_drop() noexcept { return {.gold = 1, .carbons = 2, .hydrogens = 6}; }
};
struct Propane : Unit_Base {
	using split_to = Ethane;
	size_t     split_n  = 2;

	Propane() noexcept {
		speed = 0.75f;
		object_id = asset::Object_Id::Propane;
		color /= 3;
	}
	virtual Ressources get_drop() noexcept { return {.gold = 1, .carbons = 3, .hydrogens = 8}; }
};
struct Butane : Unit_Base {
	using split_to = Propane;
	size_t     split_n  = 2;

	Butane() noexcept {
		speed = 0.25f;
		object_id = asset::Object_Id::Butane;
		color /= 5;
	}
	virtual Ressources get_drop() noexcept { return {.gold = 1, .carbons = 4, .hydrogens = 10}; }
};
struct Water   : Unit_Base {
	Water()   noexcept {
		object_id = asset::Object_Id::Water;
		speed = 2;
	}

	virtual Ressources get_drop() noexcept { return {.gold = 1, .hydrogens = 2, .oxygens = 1}; }
};
struct Oxygen  : Unit_Base {
	Oxygen() noexcept {
		object_id = asset::Object_Id::Oxygen;
		health = 1.f;
		speed  = 1.f;
	}
	virtual Ressources get_drop() noexcept { return {.gold = 1, .oxygens = 2}; }
};
struct Chloroform  : Unit_Base {

	float debuff_range = 1.f;
	float debuff_cd = 1.f;
	float debuff_as = 0.5f;

	Chloroform() noexcept {
		object_id = asset::Object_Id::Chloroform;
		health = 1.f;
		speed  = 0.75f;
	}
	virtual Ressources get_drop() noexcept { return {.gold = 1, .carbons = 1, .hydrogens = 2}; }
};

template<typename T> struct Merge {};
template<typename T> using Merge_t = typename Merge<T>::type;
template<>           struct Merge<Methane> { using type = Ethane; };
template<>           struct Merge<Ethane>  { using type = Propane; };
template<>           struct Merge<Propane> { using type = Butane; };

#define UNIT_MERGE Methane, Ethane, Propane
#define UNIT_SPLIT Ethane, Propane, Butane
#define UNIT_DIE_CATALYST_MERGE Water
#define UNIT_DIE_INVINCIBLE_BUFF Oxygen
#define LIST_UNIT(X) X(Methane) X(Ethane) X(Propane) X(Butane) X(Water) X(Oxygen) X(Chloroform)

struct Unit {
	sum_type(Unit, LIST_UNIT);
	sum_type_base(Unit_Base);

	size_t id = 0;
	bool to_remove = false;
};
