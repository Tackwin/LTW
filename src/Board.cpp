#include "Board.hpp"


#include "imgui/imgui.h"

void Board::input(const Input_Info& in, Vector2f mouse_world_pos) noexcept {
	for2(i, j, size) if (tile_box({i, j}).in(mouse_world_pos)) {
		if (in.mouse_infos[Mouse::Middle].just_released) remove_tower({i, j});
	}
}

void Board::update(double dt) noexcept {
	tiles.resize(size.x * size.y, Empty{});

	if (path_construction.soft_dirty) soft_compute_paths();
	else if (path_construction.dirty) compute_paths();

	gold_gained = 0;

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

	for (size_t i = 0; i < towers.size(); ++i) {
		auto world_pos = Vector2f{
			(towers[i]->tile_pos.x - size.x / 2.f) * (tile_size + tile_padding),
			(towers[i]->tile_pos.y - size.y / 2.f) * (tile_size + tile_padding)
		};
		if (towers[i].typecheck(Tower::Archer_Kind)) {
			auto& x = towers[i].Archer_;
			x.attack_cd -= dt;
			if (x.attack_cd > 0) continue;

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
		} else if (towers[i].typecheck(Tower::Sharp_Kind)) {
			auto& x = towers[i].Sharp_;

			for (auto& y : units) if ((world_pos - y.pos).length2() < x.range * x.range) {
				y.health -= x.damage * dt;
			}
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
	for (size_t i = towers.size() - 1; i + 1 > 0; --i) if (towers[i].to_remove)
		towers.remove_at(i);
}

void Board::render(render::Orders& order) noexcept {
	render::Rectangle rec;
	render::Circle circle;
	render::Arrow arrow;

	Rectanglef tile_zone;
	float tile_bound = tile_size + tile_padding;
	tile_zone.pos  = -tile_bound * (Vector2f)size / 2;
	tile_zone.size = +tile_bound * (Vector2f)size;

	for2(i, j, size) {
		rec.pos = tile_box({i, j}).pos + pos;
		rec.size = tile_box({i, j}).size;
		rec.color = tiles[i + j * size.x]->color;
		order.push(rec, 0);
	}

	Rectanglef start_zone;
	start_zone.x = tile_zone.x;
	start_zone.y = tile_zone.y + tile_zone.h - start_zone_height * tile_bound;
	start_zone.w = tile_zone.w;
	start_zone.h = start_zone_height * tile_bound;

	rec.pos   = start_zone.pos + pos;
	rec.size  = start_zone.size;
	rec.color = {.8f, .8f, .8f, 1.f};
	order.push(rec, 1);

	Rectanglef cease_zone;
	cease_zone.x = tile_zone.x;
	cease_zone.y = tile_zone.y;
	cease_zone.w = tile_zone.w;
	cease_zone.h = cease_zone_height * tile_bound;

	rec.pos   = cease_zone.pos + pos;
	rec.size  = cease_zone.size;
	rec.color = {.8f, .8f, .8f, 1.f};
	order.push(rec, 1);

	if (gui.render_path) {
		for (size_t i = 0; i < next_tile.size(); ++i)
			if (tiles[i].typecheck(Tile::Empty_Kind))
		{
			size_t j = next_tile[i];
			if (j == SIZE_MAX) continue;

			arrow.a = tile_box({ i % size.x, i / size.x }).center() + pos;
			arrow.b = tile_box({ j % size.x, j / size.x }).center() + pos;
			arrow.color = V4F(0.5f);

			order.push(arrow);
		}

		auto& path = path_construction;
		for (size_t i = path.open_idx; i < path.open.size(); ++i) {
			auto& x = path.open[i];
			circle.r = 0.1f;
			circle.color = {0, 0, 1, 1};
			circle.pos = tile_box(x).center() + pos;
			order.push(circle);
		}
		for (size_t i = 0; i < path.closed.size(); ++i) if (path.closed[i]) {
			circle.r = 0.05f;
			circle.color = {0, 1, 0, 1};
			circle.pos = tile_box(i).center() + pos;
			order.push(circle);
		}
	}

	circle.r = 0.2f;
	circle.color = { 0.6f, 0.5f, 0.1f, 1.f };
	for (auto& x : units) {
		circle.pos = x.pos + pos;
		order.push(circle);
	}

	for (auto& x : projectiles) {
		circle.r = x.r;
		circle.pos = x.pos + pos;
		circle.color = x.color;

		order.push(circle);
	}

	for (auto& x : towers) {
		auto s = (tile_size + tile_padding);
		circle.r     = 0.2f;
		circle.pos   = tile_box(x->tile_pos).pos + s * (Vector2f)x->tile_size / 2 + pos;
		circle.color = x->color;

		order.push(circle);
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
	auto& path = path_construction;
	path.open.clear();
	path.closed.clear();
	path.closed.resize(tiles.size(), false);

	next_tile.clear();
	next_tile.resize(tiles.size(), SIZE_MAX);

	for (size_t i = 0; i < size.x; ++i) {
		path.open.push_back(i);
		path.closed[i] = true;;
	}

	auto neighbors_list = std::initializer_list<Vector2i>{ {0, +1}, {0, -1}, {-1, 0}, {+1, 0} };

	path.open_idx = 0;
	while (path.open_idx < path.open.size()) {
		auto it = path.open[path.open_idx++];
		auto x = it % size.x;
		auto y = it / size.x;

		for (auto off : neighbors_list) if (
			0 <= x + off.x && x + off.x < size.x && 0 <= y + off.y && y + off.y < size.y
		) {
			size_t t = (x + off.x) + (y + off.y) * size.x;
			if (!tiles[t]->passthrough) continue;
			if (path.closed[t]) continue;

			next_tile[t] = it;
			path.open.push_back(t);
			path.closed[t] = true;
		}
	}

	path.dirty = false;
	return;
}

void Board::soft_compute_paths() noexcept {
	auto& path = path_construction;
	next_tile.resize(tiles.size(), SIZE_MAX);

	auto neighbors_list = std::initializer_list<Vector2i>{ {0, +1}, {0, -1}, {-1, 0}, {+1, 0} };

	auto it = path.open[path.open_idx++];
	auto x = it % size.x;
	auto y = it / size.x;

	for (auto off : neighbors_list) if (
		0 <= x + off.x && x + off.x < size.x && 0 <= y + off.y && y + off.y < size.y
	) {
		size_t t = (x + off.x) + (y + off.y) * size.x;
		if (!tiles[t]->passthrough) continue;
		if (path.closed[t]) continue;

		next_tile[t] = it;
		path.open.push_back(t);
		path.closed[t] = true;
	}

	path.soft_dirty = path.open_idx < path.open.size();
	return;
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

void Board::insert_tower(Tower t) noexcept {
	auto pos = t->tile_pos;
	if (pos.y < cease_zone_height) return;
	if (size.y - pos.y <= start_zone_height) return;

	towers.push_back(std::move(t));
	tiles[pos.x + pos.y * size.x] = Block{};
	invalidate_paths();
}

void Board::remove_tower(Vector2u p) noexcept {
	tiles[p.x + p.y * size.x] = Empty{};
	for (auto& x : towers) {
		if (x->tile_pos.x <= p.x && p.x <= x->tile_pos.x + x->tile_size.x)
		if (x->tile_pos.y <= p.y && p.y <= x->tile_pos.y + x->tile_size.y) x.to_remove = true;
	}
	invalidate_paths();
}

void Board::invalidate_paths() noexcept {
	auto& path = path_construction;
	path.soft_dirty = false;
	path.dirty = true;
	path.open_idx = 0;
	path.open.clear();
	path.closed.clear();
	path.closed.resize(size.x * size.y, false);
	next_tile.clear();

	for (size_t i = 0; i < size.x; ++i) {
		path.open.push_back(i);
		path.closed[i] = true;;
	}
}