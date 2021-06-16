#include "Effect.hpp"

#include "Tower.hpp"

#include "Managers/AssetsManager.hpp"

void apply_effects(Tower& tower, xstd::span<Effect> effects) noexcept {
	tower->attack_speed_factor = 1;
	for (auto& e : effects) {
		switch (e.kind) {
			case Effect::Slow_AS_Kind : {
				tower->attack_speed_factor *= e.Slow_AS_.debuff;
			} break;
			case Effect::None_Kind:
			case Effect::Count: break;
		}
	}
}

size_t get_effect_icon(Effect::Kind e) noexcept {
	switch (e) {
		case Effect::Slow_AS_Kind: return asset::Texture_Id::Slow_AS_Icon;
		case Effect::None_Kind: return 0;
		case Effect::Count: return 0;
	}
}
