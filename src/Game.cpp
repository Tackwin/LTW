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

void game_startup(Game&) noexcept {
	PROFILER_BEGIN_SEQ("monitor");
	asset::Store.monitor_path("assets/");
	PROFILER_SEQ("load_shaders");
	asset::Store.load_known_shaders();
	PROFILER_END_SEQ();
}

void game_shutdown(Game&) noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

void game_update(Game& game, double dt) noexcept {
	game.input_record.push_back(get_new_frame_input(game.input_record, dt));
	auto in = game.input_record.back();

	if (in.key_infos[Keyboard::LCTRL].pressed && in.key_infos[Keyboard::Num1].just_pressed) {
		game.gui.board_open = true;
	}

	game.zqsd_vector = {};
	if (in.key_infos[Keyboard::Z].pressed) game.zqsd_vector.y += 1;
	if (in.key_infos[Keyboard::Q].pressed) game.zqsd_vector.x -= 1;
	if (in.key_infos[Keyboard::S].pressed) game.zqsd_vector.y -= 1;
	if (in.key_infos[Keyboard::D].pressed) game.zqsd_vector.x += 1;
	game.zqsd_vector = game.zqsd_vector.normed();

	if (in.scroll) {
		game.camera.frame_size *= std::powf(2, in.scroll / 5.f);
	}

	if (in.key_infos[Keyboard::Space].pressed) {
		Unit new_unit;
		new_unit.current_tile = game.board.size.x / 2 + (game.board.size.y - 1) * game.board.size.x;
		new_unit.pos = game.board.tile_box({
			new_unit.current_tile % game.board.size.x,
			new_unit.current_tile / game.board.size.x
		}).center();
		new_unit.speed = 5.f;
		game.board.units.push_back(new_unit);
	}

	ImGui::Begin("Board", &game.gui.board_open);
	if (game.gui.board_open) {
		ImGui::SliderFloat("tile_size   ", &game.board.tile_size, 0, 2);
		ImGui::SliderFloat("tile_padding", &game.board.tile_padding, 0, 1);
		ImGui::SliderSize("size_x", &game.board.size.x, 0, 10);
		ImGui::SliderSize("size_y", &game.board.size.y, 0, 50);

		ImGui::Checkbox("render path", &game.board.gui.render_path);
	}
	ImGui::End();

	ImGui::Begin("Game Debug", &game.gui.game_debug_open);
	if (game.gui.game_debug_open) {
		ImGui::SliderFloat("Cam speed", &game.camera_speed, 0, 10);
		ImGui::Text("Fps %zu", (size_t)(1 / dt));
	}
	ImGui::End();

	game.board.input(in, project_mouse_pos(in.mouse_pos, game.camera));
	game.board.update(dt);

	float camera_zoom_speed_multiplier = std::max(
		1.f,
		game.camera.frame_size.x / 1.5f
	);
	game.camera.pos += game.camera_speed * game.zqsd_vector * dt * camera_zoom_speed_multiplier;
}

void game_render(Game& game, render::Orders& order) noexcept {
	order.push_back(game.camera);
	defer { order.push_back(render::Pop_Camera{}); };

	game.board.render(order);
}

void Board::input(const Input_Info& in, Vector2f mouse_world_pos) noexcept {
	for2(i, j, size) if (tile_box({i, j}).in(mouse_world_pos)) {
		if (in.mouse_infos[Mouse::Left  ].just_released) tile({i, j}) = Archer{};
		if (in.mouse_infos[Mouse::Right ].just_released) tile({i, j}) = Sharp{};
		if (in.mouse_infos[Mouse::Middle].just_released) tile({i, j}) = Empty{};
	}
}

void Board::update(double dt) noexcept {
	tiles.resize(size.x * size.y, Empty{});

	compute_paths();

	for (auto& x : units) {
		auto next = next_tile[x.current_tile];
		if (next == SIZE_MAX) continue;

		auto last_pos = tile_box(x.current_tile).center();
		auto next_pos = tile_box(next).center();

		auto dt_vec = next_pos - x.pos;

		if (dt_vec.length2() < (last_pos - x.pos).length2()) x.current_tile = next;

		dt_vec = dt_vec.normed();
		x.pos += dt_vec * x.speed * dt;
	}
	for (auto& x : units) {
		x.to_remove = x.health < 0;
	}

	for (size_t i = 0; i < tiles.size(); ++i) if (tiles[i].typecheck(Tile::Archer_Kind)) {
		auto& x = tiles[i].Archer_;
		x.attack_cd -= dt;
		if (x.attack_cd > 0) continue;

		for (auto& y : units) if ((tile_box(i).center() - y.pos).length2() < x.range * x.range) {
			Projectile new_projectile;
			new_projectile.from = tiles[i].id;
			new_projectile.to = y.id;

			new_projectile.pos = tile_box(i).center();
			new_projectile.color = x.color * 0.5f;
			projectiles.push_back(new_projectile);
			x.attack_cd = x.attack_speed;
			break;
		}
	}

	for (auto& x : projectiles) {
		if (!units.exist(x.to)) { x.to_remove = true; continue; }
		auto& to = units.id(x.to);

		x.pos += (to.pos - x.pos).normed() * x.speed * dt;
		if ((to.pos - x.pos).length2() < 0.1f) {
			x.to_remove = true;
			to.health -= x.damage;
		}
	}

	for (size_t i = projectiles.size() - 1; i + 1 > 0; --i) if (projectiles[i].to_remove)
		projectiles.remove_at(i);
	for (size_t i = units.size() - 1; i + 1 > 0; --i) if (units[i].to_remove)
		units.remove_at(i);
}

void Board::render(render::Orders& order) noexcept {
	render::Rectangle rec;
	render::Circle circle;
	render::Arrow arrow;

	Rectanglef tile_zone;
	float tile_bound = tile_size + tile_padding;
	tile_zone.pos  = -tile_bound * (Vector2f)size / 2;
	tile_zone.size = +tile_bound * (Vector2f)size;

	rec.color = Empty().color;
	for2(i, j, size) {
		rec.pos = tile_box({i, j}).pos;
		rec.size = tile_box({i, j}).size;
		order.push_back(rec);
	}

	for2(i, j, size) if (!tiles[i + j * size.x].typecheck(Tile::Empty_Kind)) {
		circle.color = tile({i, j})->color;
		circle.pos = tile_box({i, j}).center();
		circle.r   = 0.4f;
		order.push_back(circle);
	}

	Rectanglef start_zone;
	start_zone.x = tile_zone.x;
	start_zone.y = tile_zone.y + tile_zone.h - start_zone_height * tile_bound;
	start_zone.w = tile_zone.w;
	start_zone.h = start_zone_height * tile_bound;

	rec.pos   = start_zone.pos;
	rec.size  = start_zone.size;
	rec.color = {.8f, .8f, .8f, 1.f};
	order.push_back(rec);

	Rectanglef cease_zone;
	cease_zone.x = tile_zone.x;
	cease_zone.y = tile_zone.y;
	cease_zone.w = tile_zone.w;
	cease_zone.h = cease_zone_height * tile_bound;

	rec.pos   = cease_zone.pos;
	rec.size  = cease_zone.size;
	rec.color = {.8f, .8f, .8f, 1.f};
	order.push_back(rec);

	if (gui.render_path) for (size_t i = 0; i < next_tile.size(); ++i)
		if (tiles[i].typecheck(Tile::Empty_Kind))
	{
		size_t j = next_tile[i];
		if (j == SIZE_MAX) continue;

		arrow.a = tile_box({ i % size.x, i / size.x }).center();
		arrow.b = tile_box({ j % size.x, j / size.x }).center();
		arrow.color = V4F(0.5f);

		order.push_back(arrow);
	}

	circle.r = 0.2f;
	circle.color = { 0.6f, 0.5f, 0.1f, 1.f };
	for (auto& x : units) {
		circle.pos = x.pos;
		order.push_back(circle);
	}

	for (auto& x : projectiles) {
		circle.r = x.r;
		circle.pos = x.pos;
		circle.color = x.color;

		order.push_back(circle);
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

	rec.pos.x -= (size.x - 1) * tile_offset / 2;
	rec.pos.y -= (size.y - 1) * tile_offset / 2;

	rec.pos.x += tile_offset * pos.x;
	rec.pos.y += tile_offset * pos.y;
	return rec;
}

void Board::compute_paths() noexcept {
	thread_local std::vector<size_t> dist;
	dist.clear();
	dist.resize(tiles.size());

	next_tile.clear();
	next_tile.resize(tiles.size(), SIZE_MAX);

	for (size_t i = size.x; i < dist.size(); ++i) dist[i] = SIZE_MAX - 1;

	// the first size.x nodes are the goals.
	for (size_t i = size.x; i < dist.size(); ++i) {
		for2 (x, y, size) if (tiles[x + y * size.x].typecheck(Tile::Empty_Kind)) {
			size_t u = x + y * size.x;

			for (auto off : std::initializer_list<Vector2i>{ {-1, 0}, {+1, 0}, {0, -1}, {0, +1} })
				if (0 <= x + off.x && x + off.x < size.x && 0 <= y + off.y && y + off.y < size.y)
			{
				size_t t = (x + off.x) + (y + off.y) * size.x;
				if (!tiles[t].typecheck(Tile::Empty_Kind)) continue;

				if (dist[u] + 1 < dist[t]) {
					dist[t] = dist[u] + 1;
					next_tile[t] = u;
				}
			}
		}
	}
}
