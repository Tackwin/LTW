#pragma once

#include "Math/Vector.hpp"

struct Ressources {
	size_t gold = 0;
	size_t carbons = 0;
	size_t hydrogens = 0;
	size_t oxygens = 0;
};

extern Ressources add(const Ressources& a, const Ressources& b) noexcept;

struct Player {
	Ressources ressources;

	size_t income = 25;
};
