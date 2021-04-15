#pragma once

#include <array>
#include <vector>
#include <optional>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"

#include "Managers/InputsManager.hpp"

#include "Interface.hpp"
#include "Board.hpp"

struct Player {
	size_t gold = 0;
	size_t income = 25;
};

struct Controller {
	size_t board_id  = 0;
	size_t player_id = 0;
	Tower placing = nullptr;

	std::optional<Vector2f> start_drag_selection;
	Vector2f end_drag_selection;
	bool ended_drag_selection = false;

	std::vector<size_t> tower_selected;
};

struct Game_Request {
	bool confine_cursor = false;
};

struct Game {
	struct Gui {
		bool board_open = true;
		bool game_debug_open = true;
		bool debug_depth_buffer = false;

		size_t edge_blur = 1;
	} gui;

	std::array<Board,  1>  boards;
	std::array<Player, 1> players;

	size_t board_per_line = 1;
	Vector2f board_pos_offset = { 15, 30 };

	Controller controller;

	Input_Record input_record;
	Interface interface;

	render::Camera3D camera3d;
	float camera_speed = 1.f;

	size_t wave = 0;
	float wave_timer = 25.f;
	float wave_time = 25.f;

	Vector2f zqsd_vector;

	float time_to_income = 0.f;
	float income_interval = 15.f;

	Game() noexcept {
		camera3d.pos = {0, -12, 30};
		camera3d.look_at({});
	}

	void input(Input_Info in) noexcept;
	Game_Request update(double dt) noexcept;

	void next_wave() noexcept;

	Interface::Ressource_Info construct_interface_ressource_info() noexcept;
};


#ifdef BUILD_DLL
#define EXTERN extern __declspec(dllexport)
#else
#define EXTERN extern
#endif	

EXTERN Game_Request game_update(Game& game, double dt) noexcept;
EXTERN void game_render(Game& game, render::Orders& orders) noexcept;
EXTERN void game_startup(Game& game) noexcept;
EXTERN void game_shutdown(Game& game) noexcept;