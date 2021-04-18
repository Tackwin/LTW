#include "Game.hpp"

#include <stdio.h>
#include "GL/glew.h"

#include "global.hpp"
#include "imgui/imgui.h"

#include "Profiler/Tracer.hpp"

#include "Managers/AssetsManager.hpp"

#include "Math/Ray.hpp"
#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

Environment_t Environment;
float wheel_scroll = 0;
std::mutex Main_Mutex;

void* platform::handle_window = nullptr;
void* platform::handle_dc_window = nullptr;
void* platform::main_opengl_context = nullptr;
void* platform::asset_opengl_context = nullptr;

Interface::Ressource_Info Game::construct_interface_ressource_info() noexcept {
	Interface::Ressource_Info r;
	r.golds = players[controller.player_id].gold;
	return r;
}

void game_startup(Game& game) noexcept {
	xstd::seed(0);
	PROFILER_BEGIN_SEQ("monitor");
	asset::Store.monitor_path("assets/");
	PROFILER_SEQ("load_shaders");
	asset::Store.load_known_shaders();
	PROFILER_SEQ("load_fonts");
	asset::Store.load_known_fonts();
	PROFILER_SEQ("load_textures");
	asset::Store.load_known_textures();
	PROFILER_SEQ("load_models");
	asset::Store.load_known_objects();
	PROFILER_SEQ("other");
	game.interface.init_buttons();

	for (size_t i = 0; i < game.boards.size(); ++i) {
		game.boards[i].pos.x = game.board_pos_offset.x * (i % game.board_per_line);
		game.boards[i].pos.y = game.board_pos_offset.y * (i / game.board_per_line);

		game.boards[i].pos.x -= (game.board_pos_offset.x * game.board_per_line) / 2;
		game.boards[i].pos.y -=
			(game.board_pos_offset.y * game.boards.size() / game.board_per_line) / 2;

		game.boards[i].pos += game.board_pos_offset / 2;
	}

	game.camera3d.pos = V3F(game.boards[game.controller.board_id].pos, 5);
	PROFILER_END_SEQ();
}

void game_shutdown(Game&) noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}


void Game::input(Input_Info in) noexcept {
	if (!in.focused) return;
	controller.ended_drag_selection = false;

	auto& board  = boards[controller.board_id];
	auto& player = players[controller.player_id];

	Vector2i mouse_screen_pos;
	mouse_screen_pos.x = (int)(in.mouse_pos.x * Environment.window_size.x);
	mouse_screen_pos.y = (int)(in.mouse_pos.y * Environment.window_size.y);

	if (!interface.input(in)) {

		if (in.key_infos[Keyboard::LCTRL].pressed && in.key_infos[Keyboard::Num1].just_pressed) {
			gui.board_open = true;
		}

		if (in.key_infos[Keyboard::PageUp].just_pressed) gui.game_debug_open = true;

		if (in.scroll) {
			float mult = 1/5.f;
			if (in.key_infos[Keyboard::LSHIFT].pressed) mult = 1 / 20.f;
			camera3d.pos.z *= std::powf(2, in.scroll * mult);
			if (camera3d.pos.z < 1) camera3d.pos.z = 1;
			if (camera3d.pos.z > 50) camera3d.pos.z = 50;
		}

		if (in.key_infos[Keyboard::O].pressed) {
			for (size_t i = 0; i < 0; ++i)
				boards[controller.board_id].spawn_unit(Methane());
		}

		if (in.mouse_infos[Mouse::Left].just_pressed) {
			if (!controller.placing.typecheck(Tower::None_Kind)) {

				if (
					player.gold >= controller.placing->gold_cost &&
					board.can_place_at(controller.placing->tile_rec)
				) {
					board.insert_tower(controller.placing);
					player.gold -= controller.placing->gold_cost;
				}

				if (!in.key_infos[Keyboard::LSHIFT].pressed) controller.placing = nullptr;
			} else if (!controller.start_drag_selection) {
				controller.start_drag_selection = in.mouse_pos;
			}
		}

		if (in.mouse_infos[Mouse::Left].pressed && controller.start_drag_selection) {
			controller.end_drag_selection = in.mouse_pos;
		}

		if (controller.start_drag_selection && in.mouse_infos[Mouse::Left].just_released) {

			auto d =
				controller.start_drag_selection->dist_to2(controller.end_drag_selection);
			if (d > 0.001f) {
				controller.ended_drag_selection = true;
			} else {
				controller.tower_selected.clear();
			}
		}

		if (in.mouse_infos[Mouse::Right].just_pressed && !controller.tower_selected.empty()) {
			controller.tower_selected.clear();
		}

		if (controller.start_drag_selection && !in.mouse_infos[Mouse::Left].pressed)
			controller.start_drag_selection.reset();
	}

	zqsd_vector = {};
	// if ((int)Environment.window_size.y - mouse_screen_pos.y < 5) zqsd_vector.y += 1;
	// if (mouse_screen_pos.x < 5)                                  zqsd_vector.x -= 1;
	// if (mouse_screen_pos.y < 5)                                  zqsd_vector.y -= 1;
	// if ((int)Environment.window_size.x - mouse_screen_pos.x < 5) zqsd_vector.x += 1;

	if (interface.action.state_button[Ui_State::Up].pressed)    zqsd_vector.y += 1;
	if (interface.action.state_button[Ui_State::Left].pressed)  zqsd_vector.x -= 1;
	if (interface.action.state_button[Ui_State::Down].pressed)  zqsd_vector.y -= 1;
	if (interface.action.state_button[Ui_State::Right].pressed) zqsd_vector.x += 1;
	zqsd_vector = zqsd_vector.normed();

	if (interface.action.state_button[Ui_State::Send_First].just_pressed) {
		size_t next = (controller.board_id + 1) % boards.size();
		auto to_spawn = Methane{};

		if (player.gold >= to_spawn.cost) {
			player.gold -= to_spawn.cost;
			players[controller.player_id].income += to_spawn.income;

			for (size_t i = 0; i < to_spawn.batch; ++i) boards[next].spawn_unit(to_spawn);
		}
	}

	if (interface.action.state_button[Ui_State::Cancel].just_pressed) {
		controller.placing = nullptr;
		controller.tower_selected.clear();
	}

	if (!controller.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = board.bounding_tile_size();
		auto mouse = camera3d.project(in.mouse_pos);
		mouse -= board.pos;
		mouse += Vector2f{tile_size * (board.size.x - 1), tile_size * (board.size.y - 1)} / 2;
		mouse /= tile_size;
		mouse  = std::max(mouse, V2F(0));
		auto mouse_u = (Vector2u)mouse;
		mouse_u = std::min(mouse_u, board.size - Vector2u{1, 1});
		controller.placing->tile_pos = mouse_u;
	}

	if (interface.action.state_button[Ui_State::Archer_Build].just_pressed) {
		controller.placing = Archer{};
		interface.action.back_to_main();
	}
	if (interface.action.state_button[Ui_State::Splash_Build].just_pressed) {
		controller.placing = Sharp{};
		interface.action.back_to_main();
	}
	if (interface.action.state_button[Ui_State::Sell].just_pressed) {
		assert(!controller.tower_selected.empty());

		for (auto& x : controller.tower_selected) {
			auto& tower = board.towers.id(x);
			player.gold += tower->gold_cost / 2;
			board.remove_tower(tower);
		}

		interface.action.back_to_main();
		controller.tower_selected.clear();
	}
	
	if (interface.action.state_button[Ui_State::Next_Wave].just_pressed) {
		next_wave();
		wave_timer = wave_time;
	}

	if (!controller.tower_selected.empty()) {
		// >TOWER_TARGET_MODE:
		std::unordered_map<Ui_State, Tower_Base::Target_Mode> button_to_target_mode = {
			{Ui_State::Target_First, Tower_Base::First},
			{Ui_State::Target_Farthest, Tower_Base::Farthest},
			{Ui_State::Target_Closest, Tower_Base::Closest},
			{Ui_State::Target_Random, Tower_Base::Random}
		};

		for (auto& [b, t] : button_to_target_mode) {
			if (interface.action.state_button[b].just_pressed) {
				for (auto& x : controller.tower_selected) board.towers.id(x)->target_mode = t;
				break;
			}
		}
	}

	if (controller.ended_drag_selection) {
		Rectanglef world_selection;
		world_selection.pos  = camera3d.project(*controller.start_drag_selection);
		world_selection.size =
			camera3d.project(controller.end_drag_selection) - world_selection.pos;
		world_selection = world_selection.canonical();

		controller.tower_selected.clear();
		for (auto& x : board.towers) if (world_selection.intersect(board.tower_box(x))) {
			controller.tower_selected.push_back(x.id);
		}
		controller.start_drag_selection.reset();
	}
}

Game_Request Game::update(double dt) noexcept {
	input_record.push_back(get_new_frame_input(input_record, dt));
	auto in = input_record.back();
	input(in);

	camera3d.last_pos = camera3d.pos;

	auto& board  = boards[controller.board_id];
	auto& player = players[controller.player_id];

	Game_Request response;
	// response.confine_cursor = true;

	if (gui.game_debug_open) {
		int temp = 0;

		ImGui::Begin("Game Debug", &gui.game_debug_open);
		if (ImGui::CollapsingHeader("SSAO")) {
			ImGui::SliderSize("radius", &gui.edge_blur, 0, 10);
		}
		ImGui::Checkbox("render path", &board.gui.render_path);
		ImGui::Checkbox("depth buffer", &gui.debug_depth_buffer);
		ImGui::SliderFloat("Cam speed", &camera_speed, 0, 10);
		temp = controller.board_id;
		if (ImGui::InputInt("Player Board", &temp)) {
			temp = temp + boards.size();
			temp %= boards.size();
			controller.board_id  = temp;
			controller.player_id = temp;
		}
		ImGui::SliderFloat("Fov", &camera3d.fov, 0, 180);
		ImGui::Text("Fps % 4d, ms: % 5.2lf", (int)(1 / dt), 1000 * dt);
		size_t units = 0;
		for (auto& x : boards) units += x.units.size();
		ImGui::Text("Units: %zu", units);
		ImGui::End();
	}

	time_to_income -= dt;
	if (time_to_income < 0) {
		time_to_income += income_interval;

		for (auto& x : players) x.gold += x.income;
	}

	wave_timer -= dt;
	if (wave_timer <= 0) {
		next_wave();

		wave_timer += wave_time;
	}


	interface.tower_selection.pool = &board.towers;
	interface.tower_selection.selection = controller.tower_selected;
	interface.ressources = construct_interface_ressource_info();
	interface.current_wave = wave;
	interface.seconds_to_wave = wave_timer;
	interface.update(dt);

	board.input(in, camera3d.project(in.mouse_pos));
	for (size_t i = 0; i < boards.size(); ++i) {
		boards[i].update(dt);
		players[i].gold += boards[i].gold_gained;
		boards[i].gold_gained = 0;
	}

	camera3d.pos += camera_speed * V3F(zqsd_vector, 0) * dt * camera3d.pos.z;

	return response;
}

void Game::next_wave() noexcept {
	for (auto& x : boards) {
		for (size_t i = 0; i < 10 * (wave + 1); ++i) {
			if ((wave % 3) == 0) {
				x.spawn_unit(Methane{});
			} else if ((wave % 3) == 1) {
				x.spawn_unit(Water{});
			} else {
				x.spawn_unit(Oxygen{});
			}
		}
	}
	wave++;
}

Game_Request game_update(Game& game, double dt) noexcept {
	return game.update(dt);
}

void game_render(Game& game, render::Orders& order) noexcept {
	order.push(game.camera3d);

	for (size_t i = 0; i < game.boards.size(); ++i) {
		auto& board = game.boards[i];
		board.render(order);

		if (game.controller.board_id == i && !game.controller.placing.typecheck(Tower::None_Kind)) {
			auto tile_size = (board.tile_padding + board.tile_size);

			render::Rectangle rec;
			rec.color = {1, 0, 0, 0.3f};
			if (board.can_place_at(game.controller.placing->tile_rec)) rec.color = {0, 1, 0, .3f};
			rec.rec = board.tile_box(
				game.controller.placing->tile_pos, game.controller.placing->tile_size
			);
			rec.pos  += board.pos;
			order.push(rec, 0.02f);
		}

	}
	order.push(render::Pop_Camera3D{});

	game.interface.dragging = false;
	if (game.controller.start_drag_selection) {
		game.interface.dragging = true;
		game.interface.drag_selection.pos  = *game.controller.start_drag_selection;
		game.interface.drag_selection.size =
			game.controller.end_drag_selection - *game.controller.start_drag_selection;
		game.interface.drag_selection = game.interface.drag_selection.canonical();

		game.interface.drag_selection.x *= 1.6f;
		game.interface.drag_selection.w *= 1.6f;
		game.interface.drag_selection.y *= 0.9f;
		game.interface.drag_selection.h *= 0.9f;
	}

	game.interface.render(order);
}
