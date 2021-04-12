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

Game_Request game_update(Game& game, double dt) noexcept {
	Game_Request response;
	// response.confine_cursor = true;

	game.controller.ended_drag_selection = false;

	game.input_record.push_back(get_new_frame_input(game.input_record, dt));
	auto in = game.input_record.back();
	auto& board  = game.boards[game.controller.board_id];
	auto& player = game.players[game.controller.player_id];

	Vector2i mouse_screen_pos;
	mouse_screen_pos.x = (int)(in.mouse_pos.x * Environment.window_size.x);
	mouse_screen_pos.y = (int)(in.mouse_pos.y * Environment.window_size.y);

	if (!game.interface.input(in)) {

		if (in.key_infos[Keyboard::LCTRL].pressed && in.key_infos[Keyboard::Num1].just_pressed) {
			game.gui.board_open = true;
		}

		if (in.key_infos[Keyboard::PageUp].just_pressed) game.gui.game_debug_open = true;

		if (in.scroll) {
			float mult = 1/5.f;
			if (in.key_infos[Keyboard::LSHIFT].pressed) mult = 1 / 20.f;
			game.camera3d.pos.z *= std::powf(2, in.scroll * mult);
			if (game.camera3d.pos.z < 1) game.camera3d.pos.z = 1;
			if (game.camera3d.pos.z > 50) game.camera3d.pos.z = 50;
		}

		if (in.key_infos[Keyboard::O].pressed) {
			for (size_t i = 0; i < 500; ++i) game.boards[game.controller.board_id].spawn_unit(Unit());
		}

		if (in.mouse_infos[Mouse::Left].just_pressed) {
			if (!game.controller.placing.typecheck(Tower::None_Kind)) {

				if (
					player.gold >= game.controller.placing->gold_cost &&
					board.can_place_at(game.controller.placing->tile_rec)
				) {
					board.insert_tower(game.controller.placing);
					player.gold -= game.controller.placing->gold_cost;
				}

				if (!in.key_infos[Keyboard::LSHIFT].pressed) game.controller.placing = nullptr;
			} else if (!game.controller.start_drag_selection) {
				game.controller.start_drag_selection = in.mouse_pos;
			}
		}

		if (in.mouse_infos[Mouse::Left].pressed && game.controller.start_drag_selection) {
			game.controller.end_drag_selection = in.mouse_pos;
		}

		if (game.controller.start_drag_selection && !in.mouse_infos[Mouse::Left].pressed)
			game.controller.start_drag_selection.reset();

		if (in.mouse_infos[Mouse::Left].just_released) {
			game.controller.ended_drag_selection = true;
		}
	}

	if (game.gui.game_debug_open) {
		int temp = 0;

		ImGui::Begin("Game Debug", &game.gui.game_debug_open);
		if (ImGui::CollapsingHeader("SSAO")) {
			ImGui::SliderSize("radius", &game.gui.edge_blur, 0, 10);
		}
		ImGui::Checkbox("render path", &board.gui.render_path);
		ImGui::Checkbox("depth buffer", &game.gui.debug_depth_buffer);
		ImGui::SliderFloat("Cam speed", &game.camera_speed, 0, 10);
		temp = game.controller.board_id;
		if (ImGui::InputInt("Player Board", &temp)) {
			temp = temp + game.boards.size();
			temp %= game.boards.size();
			game.controller.board_id  = temp;
			game.controller.player_id = temp;
		}
		ImGui::SliderFloat("Fov", &game.camera3d.fov, 0, 180);
		ImGui::Text("Fps %zu", (size_t)(1 / dt));
		size_t units = 0;
		for (auto& x : game.boards) units += x.units.size();
		ImGui::Text("Units: %zu", units);
		ImGui::End();
	}


	game.zqsd_vector = {};
	// if ((int)Environment.window_size.y - mouse_screen_pos.y < 5) game.zqsd_vector.y += 1;
	// if (mouse_screen_pos.x < 5)                                  game.zqsd_vector.x -= 1;
	// if (mouse_screen_pos.y < 5)                                  game.zqsd_vector.y -= 1;
	// if ((int)Environment.window_size.x - mouse_screen_pos.x < 5) game.zqsd_vector.x += 1;

	if (game.interface.action.state_button[Ui_State::Up].pressed)    game.zqsd_vector.y += 1;
	if (game.interface.action.state_button[Ui_State::Left].pressed)  game.zqsd_vector.x -= 1;
	if (game.interface.action.state_button[Ui_State::Down].pressed)  game.zqsd_vector.y -= 1;
	if (game.interface.action.state_button[Ui_State::Right].pressed) game.zqsd_vector.x += 1;
	game.zqsd_vector = game.zqsd_vector.normed();

	if (game.interface.action.state_button[Ui_State::Send_First].just_pressed) {
		size_t next = (game.controller.board_id + 1) % game.boards.size();
		auto to_spawn = Unit();

		if (player.gold >= to_spawn.cost) {
			player.gold -= to_spawn.cost;
			game.players[game.controller.player_id].income += to_spawn.income;

			for (size_t i = 0; i < to_spawn.batch; ++i) game.boards[next].spawn_unit(to_spawn);
		}
	}

	if (game.interface.action.state_button[Ui_State::Cancel].just_pressed)
		game.controller.placing = nullptr;

	if (!game.controller.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = board.tile_size + board.tile_padding;
		auto mouse = game.camera3d.project(in.mouse_pos);
		mouse -= board.pos;
		mouse += Vector2f{tile_size * board.size.x, tile_size * board.size.y} / 2;
		mouse /= board.tile_size + board.tile_padding;
		mouse  = std::max(mouse, V2F(0));
		auto mouse_u = (Vector2u)mouse;
		mouse_u = std::min(mouse_u, board.size - Vector2u{1, 1});
		game.controller.placing->tile_pos = mouse_u;
	}

	if (game.interface.action.state_button[Ui_State::Archer_Build].just_pressed) {
		game.controller.placing = Archer{};
		game.interface.action.back_to_main();
	}
	if (game.interface.action.state_button[Ui_State::Splash_Build].just_pressed) {
		game.controller.placing = Sharp{};
		game.interface.action.back_to_main();
	}
	if (game.interface.action.state_button[Ui_State::Sell].just_pressed) {
		assert(game.controller.tower_selected);

		auto& tower = board.towers.id(game.controller.tower_selected);
		player.gold += tower->gold_cost / 2;
		board.remove_tower(tower);
		game.interface.action.back_to_main();

		game.controller.tower_selected = 0;
	}

	if (game.controller.ended_drag_selection) {
		Rectanglef world_selection;
		world_selection.pos  = game.camera3d.project(*game.controller.start_drag_selection);
		world_selection.size =
			game.camera3d.project(game.controller.end_drag_selection) - world_selection.pos;
		world_selection = world_selection.canonical();

		game.controller.tower_selected = 0;
		for (auto& x : board.towers) if (world_selection.intersect(board.tower_box(x))) {
			game.controller.tower_selected = x.id;
			break;
		}
	}

	game.time_to_income -= dt;
	if (game.time_to_income < 0) {
		game.time_to_income += game.income_interval;

		for (auto& x : game.players) x.gold += x.income;
	}

	game.interface.tower_selected = nullptr;
	if (game.controller.tower_selected)
		game.interface.tower_selected = board.towers.id(game.controller.tower_selected);
	game.interface.ressources = game.construct_interface_ressource_info();
	game.interface.update(dt);

	board.input(in, game.camera3d.project(in.mouse_pos));
	for (size_t i = 0; i < game.boards.size(); ++i) {
		game.boards[i].update(dt);
		game.players[i].gold += game.boards[i].gold_gained;
		game.boards[i].gold_gained = 0;
	}

	game.camera3d.pos += game.camera_speed * V3F(game.zqsd_vector, 0) * dt * game.camera3d.pos.z;

	return response;
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
			rec.size  = (Vector2f)game.controller.placing->tile_size * tile_size;
			rec.pos   = (Vector2f)game.controller.placing->tile_pos * tile_size;
			rec.pos  -= Vector2f{ board.size.x * tile_size, board.size.y * tile_size } / 2;
			rec.pos  += board.pos;
			order.push(rec, 0.02f);
		}

	}
	order.push(render::Pop_Camera3D{});
	order.push(render::Clear_Depth{});

	game.interface.dragging = false;
	if (game.controller.start_drag_selection) {
		game.interface.dragging = true;
		game.interface.drag_selection.pos  = *game.controller.start_drag_selection;
		game.interface.drag_selection.size =
			game.controller.end_drag_selection - *game.controller.start_drag_selection;

		game.interface.drag_selection.x *= 1.6f;
		game.interface.drag_selection.w *= 1.6f;
		game.interface.drag_selection.y *= 0.9f;
		game.interface.drag_selection.h *= 0.9f;
	}

	game.interface.render(order);
}
