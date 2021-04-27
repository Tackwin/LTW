#include "Tower.hpp"

Tower::Kind Tower::get_upgrade() noexcept {
	switch (kind) {
	case Mirror_Kind: return Mirror2_Kind;
	default: return None_Kind;
	}
}
