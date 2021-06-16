#pragma once

#include "xstd.hpp"
#include "std/vector.hpp"



struct Effect_Base {
	float cooldown = 0.f;
};


struct Slow_AS {
	float debuff = 0.5f;
};


#define EFFECT_LIST(X) X(Slow_AS)

struct Effect {
	sum_type(Effect, EFFECT_LIST);
	sum_type_base(Effect_Base);
};

struct Tower;
extern void apply_effects(Tower& tower, xstd::span<Effect> effects) noexcept;
extern size_t get_effect_icon(Effect::Kind e) noexcept;