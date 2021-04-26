#pragma once

#include <array>

#include "Game.hpp"


struct Game_Input_State {
	static constexpr size_t Cell_State_Size =
		(size_t)Unit::Kind::Count + (size_t)Tile::Kind::Count + (size_t)Tower::Kind::Count;

	using Cell_State = std::array<float, Cell_State_Size>;
	
	std::array<Cell_State, 40 * 120> board_state;
	float golds = 0;
};

struct Game_Output_State {
	Tower::Kind to_place;
	float place_x;
	float place_y;
};

extern Game_Input_State extract_input(const Game& game, size_t player, size_t board) noexcept;
extern void apply_output(Game& game, Game_Output_State out, size_t player, size_t board) noexcept;