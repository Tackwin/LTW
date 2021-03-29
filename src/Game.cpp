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

void Game::startup() noexcept {
	PROFILER_BEGIN_SEQ("monitor");
	asset::Store.monitor_path("assets/");
	PROFILER_SEQ("load_shaders");
	asset::Store.load_known_shaders();
	PROFILER_END_SEQ();
}

void Game::shutdown() noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

void Game::update(double dt) noexcept {
	input_record.push_back(get_new_frame_input(input_record, dt));

	pointer = input_record.back().mouse_pos;
	pointer = project_mouse_pos(pointer, camera);

	board.input(input_record.back(), pointer);

	ImGui::Begin("Board");

	ImGui::SliderFloat("tile_size   ", &board.tile_size, 0, 20);
	ImGui::SliderFloat("tile_padding", &board.tile_padding, 0, 20);
	ImGui::SliderSize("size_x", &board.size.x, 0, 10);
	ImGui::SliderSize("size_y", &board.size.y, 0, 50);

	ImGui::Text("Fps %zu", (size_t)(1 / dt));

	ImGui::End();

	board.update(dt);
}

void Game::render(render::Orders& order) noexcept {
	camera.pos = {0, 0};
	camera.frame_size = {160, 90};
	order.push_back(camera);
	defer { order.push_back(render::Pop_Camera{}); };

	board.render(order);

	render::Circle p;
	p.pos = pointer;
	p.r = 0.5f;
	p.color = {1, 1, 1, 1};
	order.push_back(p);
}

void Board::input(const Input_Info& in, Vector2f mouse_world_pos) noexcept {
	for2(i, j, size) if (tile_box({i, j}).in(mouse_world_pos)) {
		if (in.mouse_infos[Mouse::Left ].just_released) tile({i, j}) = Archer{};
		if (in.mouse_infos[Mouse::Right].just_released) tile({i, j}) = Sharp{};
	}
}

void Board::update(double) noexcept {
	tiles.resize(size.x * size.y, Empty{});
}

void Board::render(render::Orders& order) noexcept {
	for2(i, j, size) {
		render::Rectangle rec;
		rec.color = tile({i, j})->color;
		rec.pos = tile_box({i, j}).pos;
		rec.size = tile_box({i, j}).size;
		order.push_back(rec);
	}
}

Tile& Board::tile(Vector2u pos) noexcept {
	return tiles[pos.x + pos.y * size.x];
}
Rectanglef Board::tile_box(Vector2u pos) noexcept {
	Rectanglef rec;

	float tile_offset = tile_size + tile_padding;
	rec.pos = V2F(-tile_size / 2);
	rec.size = V2F(tile_size);

	rec.pos.x -= size.x * tile_offset / 2;
	rec.pos.y -= size.y * tile_offset / 2;

	rec.pos.x += tile_offset * pos.x;
	rec.pos.y += tile_offset * pos.y;
	return rec;
}