#include "Unit.hpp"
#include "Board.hpp"

void on_death(Board& board, Unit& unit) noexcept {
	switch (unit.kind) {
		case Unit::Kind::Oxygen_Kind: {
			Water w;
			board.spawn_unit_at(
				w, { unit->current_tile % board.size.x, unit->current_tile / board.size.x }
			);
			board.spawn_unit_at(
				w, { unit->current_tile % board.size.x, unit->current_tile / board.size.x }
			);
			break;
		};

		default: break;
	}
}

