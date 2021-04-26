#include "State.hpp"

Game_Input_State extract_input(const Game& game, size_t player_id, size_t board_id) noexcept {
	Game_Input_State state;

	auto& board  = game.boards[board_id];
	auto& player = game.players[player_id];

	for (size_t i = 0; i < board.tile_size; ++i) {
		for (auto& x : board.unit_id_by_tile[i]) {
			state.board_state[i][(size_t)board.units.id(x).kind] += 1;
		}
		state.board_state[i][Unit::Kind::Count + (size_t)board.tiles[i].kind] += 1;
	}

	for (const auto& x : board.towers) {
		for (size_t i = x->tile_pos.x; i < x->tile_pos.x + x->tile_size.x; ++i)
		for (size_t j = x->tile_pos.y; j < x->tile_pos.y + x->tile_size.y; ++j) {
			state.board_state[i + j * board.size.x][(size_t)x.kind] += 1;
		}
	}

	state.golds = player.ressources.gold;

	return state;
}

void apply_output(Game& game, Game_Output_State out, size_t player_id, size_t board_id) noexcept {
	auto& board  = game.boards[board_id];
	auto& player = game.players[player_id];

	auto tower = Tower(out.to_place);
	tower->tile_pos.x = (size_t)std::roundf(out.place_x * board.size.x);
	tower->tile_pos.y = (size_t)std::roundf(out.place_y * board.size.y);

	if (player.ressources.gold >= tower->gold_cost && board.can_place_at(tower->tile_rec)) {
		player.ressources.gold -= tower->gold_cost;
		board.insert_tower(tower);
	}
}
