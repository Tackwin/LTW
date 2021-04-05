#pragma once

#include <vector>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"

#include "Managers/InputsManager.hpp"

#include "Interface.hpp"
#include "Board.hpp"

struct Player {
	size_t gold = 0;
};

struct Controller {
	size_t board_id  = 0;
	size_t player_id = 0;
	Tower placing = nullptr;
};

struct Game {
	struct Gui {
		bool board_open = true;
		bool game_debug_open = true;
	} gui;

	std::array<Board,  2>  boards;
	std::array<Player, 2> players;

	size_t board_per_line = 8;
	Vector2f board_pos_offset = { 15, 30 };

	Controller controller;

	Input_Record input_record;
	Interface interface;

	render::Camera camera;
	float camera_speed = 1.f;
	
	Vector2f zqsd_vector;

	size_t golds = 0;

	Game() noexcept {
		camera.pos = {0, 0};
		camera.frame_size = { 16, 9 };
	}

	Interface::Ressource_Info construct_interface_ressource_info() noexcept;
};


#ifdef BUILD_DLL
#define EXTERN extern __declspec(dllexport)
#else
#define EXTERN extern
#endif

struct Game_Request {
	bool confine_cursor = false;
};

EXTERN Game_Request game_update(Game& game, double dt) noexcept;
EXTERN void game_render(Game& game, render::Orders& orders) noexcept;
EXTERN void game_startup(Game& game) noexcept;
EXTERN void game_shutdown(Game& game) noexcept;