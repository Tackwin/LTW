#include "Effect.hpp"

#include "Tower.hpp"

void apply_effects(Tower& tower, xstd::span<Effect> effects) noexcept {
	for (auto& e : effects) {
		tower->attack_speed_factor = 1;
		switch (e.kind) {
			case Effect::Slow_AS_Kind : {
				tower->attack_speed_factor *= e.Slow_AS_.debuff;
			} break;
			case Effect::None_Kind:
			case Effect::Count: break;
		}
	}
}


