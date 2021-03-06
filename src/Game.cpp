#include "Game.hpp"

#include <stdio.h>
#include <set>

#include "std/unordered_map.hpp"
#include "OS/OpenGL.hpp"

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
	PROFILER_SEQ("load_sounds");
	asset::Store.load_known_sounds();
	PROFILER_SEQ("other");
	game.user_interface.init_buttons();

	for (size_t i = 0; i < game.boards.size(); ++i) {
		game.boards[i].pos.x = game.board_pos_offset.x * (i % game.board_per_line);
		game.boards[i].pos.y = game.board_pos_offset.y * (i / game.board_per_line);

		game.boards[i].pos.x -= (game.board_pos_offset.x * game.board_per_line) / 2;
		game.boards[i].pos.y -=
			(game.board_pos_offset.y * game.boards.size() / game.board_per_line) / 2;

		game.boards[i].pos += game.board_pos_offset / 2;
	}

	auto& b = game.boards[game.controller.board_id];
	game.camera3d.pos = Vector3f(b.pos, 10);
	game.camera3d.pos.y -= b.size.y * 5 * b.bounding_tile_size() / 9;
	PROFILER_END_SEQ();
	asset::Store.ready = true;
}

void game_shutdown(Game&) noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

void Game::input(Input_Info in) noexcept {
	if (!in.focused) return;
	if (in.mouse_pos.x < 0 || in.mouse_pos.x > 1 || in.mouse_pos.y < 0 || in.mouse_pos.y > 1)
		return;
	controller.ended_drag_selection = false;

	auto& board  = boards[controller.board_id];
	auto& player = players[controller.player_id];

	Vector2i mouse_screen_pos;
	mouse_screen_pos.x = (int)(in.mouse_pos.x * Environment.window_size.x);
	mouse_screen_pos.y = (int)(in.mouse_pos.y * Environment.window_size.y);

	if (!user_interface.input(in)) {

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

		if (in.mouse_infos[Mouse::Left].pressed) {
			if (controller.placing.kind != Tower::None_Kind) {

				if (
					player.ressources.gold >= controller.placing->gold_cost &&
					board.can_place_at(controller.placing->tile_rec)
				) {
					board.insert_tower(controller.placing);
					player.ressources.gold -= controller.placing->gold_cost;
				}

				if (!in.key_infos[Keyboard::LSHIFT].pressed) controller.placing = nullptr;
			} else if (!controller.start_drag_selection) {
				controller.start_drag_selection = in.mouse_pos;
			}
		}

		if (in.mouse_infos[Mouse::Middle].pressed) {
			auto a = camera3d.project(in.mouse_pos - in.mouse_delta);
			auto b = camera3d.project(in.mouse_pos);

			camera3d.pos.x -= (b.x - a.x);
			camera3d.pos.y -= (b.y - a.y);
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

	if (user_interface.action.state_button[Ui_State::Up].pressed)    zqsd_vector.y += 1;
	if (user_interface.action.state_button[Ui_State::Left].pressed)  zqsd_vector.x -= 1;
	if (user_interface.action.state_button[Ui_State::Down].pressed)  zqsd_vector.y -= 1;
	if (user_interface.action.state_button[Ui_State::Right].pressed) zqsd_vector.x += 1;
	zqsd_vector = zqsd_vector.normed();

	if (user_interface.action.state_button[Ui_State::Send_First].just_pressed) {
		size_t next = (controller.board_id + 1) % boards.size();
		auto to_spawn = Methane{};

		if (player.ressources.gold >= to_spawn.cost) {
			player.ressources.gold -= to_spawn.cost;
			players[controller.player_id].income += to_spawn.income;

			for (size_t i = 0; i < to_spawn.batch; ++i) boards[next].spawn_unit(to_spawn);
		}
	}

	if (user_interface.action.state_button[Ui_State::Cancel].just_pressed) {
		controller.placing = nullptr;
		controller.tower_selected.clear();
	}

	if (!controller.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = board.bounding_tile_size();
		auto mouse = camera3d.project(in.mouse_pos);
		mouse -= board.pos;
		mouse += Vector2f{tile_size * (board.size.x - 1), tile_size * (board.size.y - 1)} / 2;

		auto place_size = controller.placing->tile_size;
		mouse -= Vector2f{tile_size * (place_size.x - 1), tile_size * (place_size.y - 1)} / 2;

		mouse /= tile_size;
		mouse  = xstd::max(mouse, V2F(0));
		auto mouse_u = (Vector2u)mouse;
		mouse_u = xstd::min(mouse_u, board.size - place_size);
		controller.placing->tile_pos = mouse_u;
	}

// >ADD_TOWER(Tackwin):
	if (user_interface.action.state_button[Ui_State::Mirror_Build].just_pressed) {
		controller.placing = Mirror{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Splash_Build].just_pressed) {
		controller.placing = Sharp{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Volter_Build].just_pressed) {
		controller.placing = Volter{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Heat_Build].just_pressed) {
		controller.placing = Heat{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Radiation_Build].just_pressed) {
		controller.placing = Radiation{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Circuit_Build].just_pressed) {
		controller.placing = Circuit{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Block_Build].just_pressed) {
		controller.placing = Block_Tower{};
		user_interface.action.back_to_main();
	}
	if (user_interface.action.state_button[Ui_State::Surge_Spell].just_pressed) {
		for (auto& i : controller.tower_selected) if (
			board.towers.id(i).kind == Tower::Volter_Kind
		) {
			board.towers.id(i).Volter_.to_surge = true;
		}
	}
	if (user_interface.action.state_button[Ui_State::Sell].just_pressed) {
		assert(!controller.tower_selected.empty());

		for (auto& x : controller.tower_selected) {
			auto& tower = board.towers.id(x);
			player.ressources.gold += tower->gold_cost / 2;
			board.remove_tower(tower);
		}

		user_interface.action.back_to_main();
		controller.tower_selected.clear();
	}
	if (user_interface.action.state_button[Ui_State::Next_Wave].just_pressed) {
		next_wave();
		wave_timer = wave_time;
	}
	if (user_interface.action.state_button[Ui_State::Upgrade].just_pressed) {
		for (auto& id : controller.tower_selected) if (board.towers.exist(id)) {
			auto& x = board.towers.id(id);

			auto up = Tower(x.get_upgrade());
			if (up.kind != Tower::None_Kind && player.ressources.gold >= up->gold_cost) {
				up->tile_pos = x->tile_pos;
				board.remove_tower(x);
				board.insert_tower(up);
			}
		}
	}

	if (!controller.tower_selected.empty()) {
		// >TOWER_TARGET_MODE:
		xstd::unordered_map<Ui_State, Tower_Target::Target_Mode> button_to_target_mode = {
			{Ui_State::Target_First, Tower_Target::First},
			{Ui_State::Target_Farthest, Tower_Target::Farthest},
			{Ui_State::Target_Closest, Tower_Target::Closest},
			{Ui_State::Target_Random, Tower_Target::Random}
		};

		for (auto& [b, t] : button_to_target_mode) {
			if (user_interface.action.state_button[b].just_pressed) {
				for (auto& x : controller.tower_selected) {
					auto& tower = board.towers.id(x);
					tower.on_one_off_<TOWER_TARGET_LIST>() =
						[t = t] (auto& y) { y.target_mode = t; };
				}
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

Game_Request Game::update(audio::Orders& audio_orders, double dt) noexcept {
	TIMED_FUNCTION;

	running_ms = dt;

	input_record.push_back(get_new_frame_input(input_record, dt));
	auto in = input_record.back();
	input(in);

	camera3d.last_pos = camera3d.pos;

	auto& board  = boards[controller.board_id];
	auto& player = players[controller.player_id];

	Game_Request response;
	// response.confine_cursor = true;
	time_to_income -= dt;
	if (time_to_income < 0) {
		time_to_income += income_interval;
		for (auto& x : players) x.ressources.gold += x.income;
	}

	wave_timer -= dt;

	if (wave_timer <= 0) {
		next_wave();

		wave_timer += wave_time;
	}


	user_interface.tower_selection.pool = &board.towers;
	user_interface.tower_selection.selection = controller.tower_selected;
	user_interface.current_player = &player;
	user_interface.current_wave = wave;
	user_interface.seconds_to_wave = wave_timer;
	user_interface.update(dt);

	board.input(in, camera3d.project(in.mouse_pos));
	for (size_t i = 0; i < boards.size(); ++i) {
		boards[i].update(audio_orders, dt);
		players[i].ressources = add(players[i].ressources, boards[i].ressources_gained);
		boards[i].ressources_gained = {};
	}

	camera3d.pos += camera_speed * Vector3f(zqsd_vector, 0) * dt * camera3d.pos.z;

	return response;
}

void Game::next_wave() noexcept {
	for (auto& x : boards) x.current_wave = gen_wave(wave);
	wave++;
}

Game_Request game_update(Game& game, audio::Orders& audio_orders, double dt) noexcept {
	auto res = game.update(audio_orders, dt);

	frame_sample_log_frame_idx++;
	frame_sample_log_frame_idx %= Sample_Log::MAX_FRAME_RECORD;
	frame_sample_log[frame_sample_log_frame_idx].sample_count = 0;

	return res;
}

void game_render(Game& game, render::Orders& order) noexcept {
	
	if (game.gui.game_debug_open) {
		int temp = 0;

#ifndef WEB
		ImGui::Begin("Game Debug", &game.gui.game_debug_open);
		if (ImGui::CollapsingHeader("SSAO")) {
			ImGui::SliderSize("radius", &game.gui.edge_blur, 0, 10);
		}
		ImGui::Checkbox("Render Profiler", &game.gui.render_profiler);
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
		ImGui::Text(
			"Ups % 10d, us: % 10.2lf",
			(int)(1 / game.running_ms),
			1'000'000 * game.running_ms
		);
		size_t units = 0;
		size_t projectiles = 0;

		for (auto& x : game.boards) units += x.units.size();
		for (auto& x : game.boards) projectiles += x.projectiles.size();
		ImGui::Text("Units: %zu", units);
		ImGui::Text("Projectiles: %zu", projectiles);
		ImGui::End();
#endif
	}


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
		if (game.controller.board_id == i) {
			for (auto& id : game.controller.tower_selected) if (board.towers.exist(id)) {
				auto& x = board.towers.id(id);
				x.on_one_off(TOWER_TARGET_LIST) (auto& y) {
					render::Ring ring;
					ring.color = {0, 1, 0};
					ring.glowing_intensity = 2;
					ring.pos = Vector3f(board.tower_box(x).center(), 0.1f);
					ring.radius = y.range;
					order.push(ring);
				};
			}
		}

	}

	order.push(render::Pop_Camera3D{});

	game.user_interface.dragging = false;
	if (game.controller.start_drag_selection) {
		game.user_interface.dragging = true;
		game.user_interface.drag_selection.pos  = *game.controller.start_drag_selection;
		game.user_interface.drag_selection.size =
			game.controller.end_drag_selection - *game.controller.start_drag_selection;
		game.user_interface.drag_selection = game.user_interface.drag_selection.canonical();

		game.user_interface.drag_selection.x *= 1.6f;
		game.user_interface.drag_selection.w *= 1.6f;
		game.user_interface.drag_selection.y *= 0.9f;
		game.user_interface.drag_selection.h *= 0.9f;
	}

	game.user_interface.render(order);

	if (game.gui.render_profiler) {
		render::Camera cam;
		cam.rec = {{0, 0}, {1920, 1080}};
		cam.rec.pos += cam.rec.size / 2;
		order.push(render::Push_Ui{});
		order.push(cam);

		defer {
			order.push(render::Pop_Camera{});
			order.push(render::Pop_Ui{});
		};

		thread_local std::set<const char*> function_names;
		function_names.clear();
		for (size_t i = 0; i < Sample_Log::MAX_FRAME_RECORD; ++i)
			for (size_t j = 0; j < frame_sample_log[i].sample_count; ++j) {
				function_names.insert(frame_sample_log[i].samples[j].function_name);
			}

		size_t max_time = 0;
		for (size_t i = 0; i < Sample_Log::MAX_FRAME_RECORD; ++i) {
			for (size_t j = 0; j < frame_sample_log[i].sample_count; ++j) {
				auto& it = frame_sample_log[i].samples[j];
				max_time = xstd::max(max_time, (size_t)(it.time_end - it.time_start));
			}
		}
		max_time = xstd::max(max_time, (size_t)2'200'000 /*ns*/);

		Vector2f cursor = {10, 10};
		for (size_t i = 0; i < Sample_Log::MAX_FRAME_RECORD; ++i) {
			float cursor_y = cursor.y;
			auto& it = frame_sample_log[i];

			for (auto& f : function_names) {
				size_t sum = 0;
				for (size_t j = 0; j < it.sample_count; ++j) {
					auto& sample = it.samples[j];
					if (sample.function_name != f) continue;

					sum += sample.time_end - sample.time_start;
				}

				render::Rectangle r;
				r.pos = cursor;
				r.size = {5.f, 50.f * sum / max_time};
				     if (sum < 1'000'000) r.color = {0, 0, 1, 1};
				else if (sum < 2'000'000) r.color = {1, 1, 0, 1};
				else                      r.color = {1, 0, 0, 1};

				order.push(r);

				cursor.y += 100;
			}

			cursor.x += 5;
			cursor.y = cursor_y;
		}
		cursor = {10, 10};
		for (auto& f : function_names) {
			size_t anchor = 1'000'000;

			render::Rectangle r;
			r.pos = cursor;
			r.pos.y += 50.f * anchor / max_time;
			r.size = {5 * Sample_Log::MAX_FRAME_RECORD, 2};
			r.color = {0.5f, 0.5f, 0.5f, 0.5f};

			order.push(r);
			anchor = 2'000'000;

			r.pos = cursor;
			r.pos.y += 50.f * anchor / max_time;
			r.size = {5 * Sample_Log::MAX_FRAME_RECORD, 2};
			r.color = {1.f, 1.f, 1.f, 1.f};

			order.push(r);
			cursor.y += 100;
		}

		cursor.x = 50 + 5 * Sample_Log::MAX_FRAME_RECORD;
		cursor.y = 10;
		for (auto& f : function_names) {
			render::Text text;
			text.color = {1, 1, 1, 1};
			text.font_id = asset::Font_Id::Consolas;
			text.height = 20;
			text.origin = {0, 0};
			text.pos = cursor;
			text.text = order.string(f);
			text.text_length = strlen(f);

			order.push(text);
			cursor.y += 100;
		}
	}

}

