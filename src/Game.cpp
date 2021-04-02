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
	r.golds = player.gold;
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
	PROFILER_END_SEQ();
}

void game_shutdown(Game&) noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

void game_update(Game& game, double dt) noexcept {
	game.input_record.push_back(get_new_frame_input(game.input_record, dt));
	auto in = game.input_record.back();
	if (!game.interface.input(in)) {
		if (in.key_infos[Keyboard::LCTRL].pressed && in.key_infos[Keyboard::Num1].just_pressed) {
			game.gui.board_open = true;
		}

		if (in.scroll) {
			game.camera.frame_size *= std::powf(2, in.scroll / 5.f);
		}

		if (in.key_infos[Keyboard::Space].pressed) {
			Unit new_unit;
			size_t first_spawn_tile =
				(game.board.size.y - game.board.start_zone_height) * game.board.size.x;
			size_t final_spawn_tile = game.board.size.x * game.board.size.y;

			new_unit.current_tile =
				(size_t)(first_spawn_tile + xstd::random() * (final_spawn_tile - first_spawn_tile));

			new_unit.pos = game.board.tile_box({
				new_unit.current_tile % game.board.size.x,
				new_unit.current_tile / game.board.size.x
			}).center();

			new_unit.speed = 5.f;
			game.board.units.push_back(new_unit);
		}

		if (in.mouse_infos[Mouse::Left].just_pressed)
		if (!game.player.placing.typecheck(Tower::None_Kind)) {
			game.board.towers.push_back(game.player.placing);

			if (!in.key_infos[Keyboard::LSHIFT].pressed) game.player.placing = nullptr;
		}
	}

	if (game.gui.board_open) {
		ImGui::Begin("Board", &game.gui.board_open);
		ImGui::SliderFloat("tile_size   ", &game.board.tile_size, 0, 2);
		ImGui::SliderFloat("tile_padding", &game.board.tile_padding, 0, 1);
		ImGui::SliderSize("size_x", &game.board.size.x, 0, 10);
		ImGui::SliderSize("size_y", &game.board.size.y, 0, 50);

		ImGui::Checkbox("render path", &game.board.gui.render_path);
		ImGui::End();
	}

	if (game.gui.game_debug_open) {
		ImGui::Begin("Game Debug", &game.gui.game_debug_open);
		ImGui::SliderFloat("Cam speed", &game.camera_speed, 0, 10);
		ImGui::Text("Fps %zu", (size_t)(1 / dt));
		ImGui::End();
	}


	game.zqsd_vector = {};
	if (game.interface.action.state_button[Action::State::Up].pressed)    game.zqsd_vector.y += 1;
	if (game.interface.action.state_button[Action::State::Left].pressed)  game.zqsd_vector.x -= 1;
	if (game.interface.action.state_button[Action::State::Down].pressed)  game.zqsd_vector.y -= 1;
	if (game.interface.action.state_button[Action::State::Right].pressed) game.zqsd_vector.x += 1;
	game.zqsd_vector = game.zqsd_vector.normed();

	if (game.interface.action.state_button[Action::State::Cancel].just_pressed)
		game.player.placing = nullptr;

	if (!game.player.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = game.board.tile_size + game.board.tile_padding;
		auto mouse = project_mouse_pos(in.mouse_pos, game.camera);
		mouse -= game.board.pos;
		mouse += Vector2f{tile_size * game.board.size.x, tile_size * game.board.size.y} / 2;
		mouse /= game.board.tile_size + game.board.tile_padding;
		mouse  = std::max(mouse, V2F(0));
		auto mouse_u = (Vector2u)mouse;
		mouse_u = std::min(mouse_u, game.board.size - Vector2u{1, 1});
		game.player.placing->tile_pos = mouse_u;
	}

	if (game.interface.action.state_button[Action::State::Archer_Build].just_pressed) {
		game.player.placing = Archer{};
		game.interface.action.back_to_main();
	}
	if (game.interface.action.state_button[Action::State::Splash_Build].just_pressed) {
		game.player.placing = Sharp{};
		game.interface.action.back_to_main();
	}

	game.interface.ressources = game.construct_interface_ressource_info();
	game.interface.update(dt);

	game.board.input(in, project_mouse_pos(in.mouse_pos, game.camera));
	game.board.update(dt);

	game.player.gold += game.board.gold_gained;

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

	if (!game.player.placing.typecheck(Tower::None_Kind)) {
		auto tile_size = (game.board.tile_padding + game.board.tile_size);

		render::Rectangle rec;
		rec.color = {0, 1, 0, 0.3f};
		rec.size  = (Vector2f)game.player.placing->tile_size * tile_size;
		rec.pos   = (Vector2f)game.player.placing->tile_pos * tile_size;
		rec.pos  -= Vector2f{ game.board.size.x * tile_size, game.board.size.y * tile_size } / 2;
		order.push_back(rec);
	}

	game.interface.render(order);
}

void Board::input(const Input_Info& in, Vector2f mouse_world_pos) noexcept {
	for2(i, j, size) if (tile_box({i, j}).in(mouse_world_pos)) {
		if (in.mouse_infos[Mouse::Middle].just_released) tile({i, j}) = Empty{};
	}
}

void Board::update(double dt) noexcept {
	tiles.resize(size.x * size.y, Empty{});
	gold_gained = 0;

	soft_compute_paths();

	for (auto& x : units) if (!x.to_remove) {
		auto next = next_tile[x.current_tile];
		if (next == SIZE_MAX) continue;

		auto last_pos = tile_box(x.current_tile).center();
		auto next_pos = tile_box(next).center();

		auto dt_vec = next_pos - x.pos;

		if (dt_vec.length2() < (last_pos - x.pos).length2()) x.current_tile = next;

		dt_vec = dt_vec.normed();
		x.pos += dt_vec * x.speed * dt;
	}
	for (auto& x : units) if (!x.to_remove) {
		x.to_remove = x.health < 0;
		if (x.to_remove) gold_gained += 1;
	}

	for (size_t i = 0; i < towers.size(); ++i) if (towers[i].typecheck(Tower::Archer_Kind)) {
		auto& x = towers[i].Archer_;
		x.attack_cd -= dt;
		if (x.attack_cd > 0) continue;

		auto world_pos = Vector2f{
			(x.tile_pos.x - size.x / 2.f) * (tile_size + tile_padding),
			(x.tile_pos.y - size.y / 2.f) * (tile_size + tile_padding)
		};

		for (auto& y : units) if ((world_pos - y.pos).length2() < x.range * x.range) {
			Projectile new_projectile;
			new_projectile.from = tiles[i].id;
			new_projectile.to = y.id;

			new_projectile.pos = world_pos;
			new_projectile.color = x.color * 0.5f;
			projectiles.push_back(new_projectile);
			x.attack_cd = x.attack_speed;
			break;
		}
	}

	for (auto& x : projectiles) if (!x.to_remove) {
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

	rec.pos   = start_zone.pos + pos;
	rec.size  = start_zone.size;
	rec.color = {.8f, .8f, .8f, 1.f};
	order.push_back(rec);

	Rectanglef cease_zone;
	cease_zone.x = tile_zone.x;
	cease_zone.y = tile_zone.y;
	cease_zone.w = tile_zone.w;
	cease_zone.h = cease_zone_height * tile_bound;

	rec.pos   = cease_zone.pos + pos;
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

	for (auto& x : towers) {
		auto s = (tile_size + tile_padding);
		circle.r     = 0.2f;
		circle.pos   = tile_box(x->tile_pos).pos + s * (Vector2f)x->tile_size / 2;
		circle.color = x->color;

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

void Board::soft_compute_paths() noexcept {
	thread_local std::vector<size_t> dist;
	if (tiles.size() != dist.size()) {
		dist.clear();
		dist.resize(tiles.size());
		for (size_t i = size.x; i < dist.size(); ++i) dist[i] = SIZE_MAX - 1;
	}

	if (tiles.size() != next_tile.size()) {
		next_tile.clear();
		next_tile.resize(tiles.size(), SIZE_MAX);
	}

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

Tile* Board::get_tile_at(Vector2f x) noexcept {
	auto s = tile_size + tile_padding;
	if (pos.x < x.x && x.x < pos.x + size.x * s)
	if (pos.y < x.y && x.y < pos.y + size.y * s) {
		Vector2u t;
		t.x = (x.x - pos.x) / s;
		t.y = (x.y - pos.y) / s;
		return &tile(t);
	}
	return nullptr;
}
