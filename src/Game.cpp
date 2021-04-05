#include "Game.hpp"

#include <stdio.h>
#include "GL/glew.h"

#include "global.hpp"
#include "imgui/imgui.h"

#include "Profiler/Tracer.hpp"

#include "Managers/AssetsManager.hpp"

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

	game.camera.pos = game.boards[game.controller.board_id].pos;
	PROFILER_END_SEQ();
}

void game_shutdown(Game&) noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

Game_Request game_update(Game& game, double dt) noexcept {
	Game_Request response;
	// response.confine_cursor = true;

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

		if (in.scroll) game.camera.frame_size *= std::powf(2, in.scroll / 5.f);

		if (in.mouse_infos[Mouse::Middle].pressed) {
			game.camera.pos -= project_mouse_delta(in.mouse_delta, game.camera);
		}

		if (in.mouse_infos[Mouse::Left].just_pressed)
		if (!game.controller.placing.typecheck(Tower::None_Kind)) {
			board.insert_tower(game.controller.placing);

			if (!in.key_infos[Keyboard::LSHIFT].pressed) game.controller.placing = nullptr;
		}
	}

	if (game.gui.game_debug_open) {
		int temp = 0;

		ImGui::Begin("Game Debug", &game.gui.game_debug_open);
		ImGui::Checkbox("render path", &board.gui.render_path);
		ImGui::SliderFloat("Cam speed", &game.camera_speed, 0, 10);
		temp = game.controller.board_id;
		if (ImGui::InputInt("Player Board", &temp)) {
			temp = temp + game.boards.size();
			temp %= game.boards.size();
			game.controller.board_id  = temp;
			game.controller.player_id = temp;
		}
		ImGui::Text("Fps %zu", (size_t)(1 / dt));
		ImGui::End();
	}


	game.zqsd_vector = {};
	// if ((int)Environment.window_size.y - mouse_screen_pos.y < 5) game.zqsd_vector.y += 1;
	// if (mouse_screen_pos.x < 5)                                  game.zqsd_vector.x -= 1;
	// if (mouse_screen_pos.y < 5)                                  game.zqsd_vector.y -= 1;
	// if ((int)Environment.window_size.x - mouse_screen_pos.x < 5) game.zqsd_vector.x += 1;

	if (game.interface.action.state_button[Action::State::Up].pressed)    game.zqsd_vector.y += 1;
	if (game.interface.action.state_button[Action::State::Left].pressed)  game.zqsd_vector.x -= 1;
	if (game.interface.action.state_button[Action::State::Down].pressed)  game.zqsd_vector.y -= 1;
	if (game.interface.action.state_button[Action::State::Right].pressed) game.zqsd_vector.x += 1;
	game.zqsd_vector = game.zqsd_vector.normed();

	if (game.interface.action.state_button[Action::State::Send_First].just_pressed) {
		size_t next = (game.controller.board_id + 1) % game.boards.size();

		game.boards[next].spawn_unit(Unit{});
	}

	if (game.interface.action.state_button[Action::State::Cancel].just_pressed)
		game.controller.placing = nullptr;

	if (!game.controller.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = board.tile_size + board.tile_padding;
		auto mouse = project_mouse_pos(in.mouse_pos, game.camera);
		mouse -= board.pos;
		mouse += Vector2f{tile_size * board.size.x, tile_size * board.size.y} / 2;
		mouse /= board.tile_size + board.tile_padding;
		mouse  = std::max(mouse, V2F(0));
		auto mouse_u = (Vector2u)mouse;
		mouse_u = std::min(mouse_u, board.size - Vector2u{1, 1});
		game.controller.placing->tile_pos = mouse_u;
	}

	if (game.interface.action.state_button[Action::State::Archer_Build].just_pressed) {
		game.controller.placing = Archer{};
		game.interface.action.back_to_main();
	}
	if (game.interface.action.state_button[Action::State::Splash_Build].just_pressed) {
		game.controller.placing = Sharp{};
		game.interface.action.back_to_main();
	}

	game.interface.ressources = game.construct_interface_ressource_info();
	game.interface.update(dt);

	board.input(in, project_mouse_pos(in.mouse_pos, game.camera));
	for (size_t i = 0; i < game.boards.size(); ++i) {
		game.boards[i].update(dt);
		game.players[i].gold += game.boards[i].gold_gained;
		game.boards[i].gold_gained = 0;
	}

	float camera_zoom_speed_multiplier = std::max(1.f, game.camera.frame_size.x / 1.5f);
	game.camera.pos += game.camera_speed * game.zqsd_vector * dt * camera_zoom_speed_multiplier;

	return response;
}

void game_render(Game& game, render::Orders& order) noexcept {
	order.push(game.camera);
	defer { order.push(render::Pop_Camera{}); };

	for (size_t i = 0; i < game.boards.size(); ++i) {
		auto& board = game.boards[i];
		board.render(order);

		if (game.controller.board_id == i && !game.controller.placing.typecheck(Tower::None_Kind)) {
			auto tile_size = (board.tile_padding + board.tile_size);

			render::Rectangle rec;
			rec.color = {0, 1, 0, 0.3f};
			rec.size  = (Vector2f)game.controller.placing->tile_size * tile_size;
			rec.pos   = (Vector2f)game.controller.placing->tile_pos * tile_size;
			rec.pos  -= Vector2f{ board.size.x * tile_size, board.size.y * tile_size } / 2;
			rec.pos  += board.pos;
			order.push(rec);
		}

	}
	game.interface.render(order);
}
