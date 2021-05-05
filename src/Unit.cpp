#include "Unit.hpp"

void Unit_Base::hit(float damage) noexcept {
	if (invincible > 0) return;
	health -= damage;
}
