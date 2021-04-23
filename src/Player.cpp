#include "Player.hpp"

Ressources add(const Ressources& a, const Ressources& b) noexcept {
	Ressources res;

	// >TODO(Tackwin): do something about that... like have a true reflection system.

	for (size_t i = 0; i < sizeof(res); i += sizeof(size_t)) {
		#define X(x) *reinterpret_cast<size_t*>(i + reinterpret_cast<char*>(&x))
		#define Y(x) *reinterpret_cast<const size_t*>(i + reinterpret_cast<const char*>(&x))
		X(res) = Y(a) + Y(b);
		#undef X
		#undef Y
	}

	return res;
}