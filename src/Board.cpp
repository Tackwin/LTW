#include "Board.hpp"

#include "Managers/AssetsManager.hpp"

#include "imgui/imgui.h"

void Board::input(const Input_Info& in, Vector2f mouse_world_pos) noexcept {
	if (in.mouse_infos[Mouse::Right].just_released) {
		auto tile = get_tile_at(mouse_world_pos);
		if (tile) remove_tower(*tile);
	}
}

void Board::update(double dt) noexcept {
	if (tiles.size() != size.x * size.y) tiles.resize(size.x * size.y, Empty{});

	if (path_construction.soft_dirty) soft_compute_paths();
	else if (path_construction.dirty) compute_paths();

	gold_gained = 0;

	for (auto& x : units) x->life_time += dt;

	for (auto& x : units) if (!x.to_remove) {
		auto next = next_tile[x->current_tile];
		if (next == SIZE_MAX && x->target_tile == SIZE_MAX) continue;
		else if (next != SIZE_MAX) x->target_tile = next;

		auto last_pos = tile_box(x->current_tile).center();
		auto next_pos = tile_box(x->target_tile).center();

		auto dt_vec = next_pos - x->pos;

		if (dt_vec.length2() < (last_pos - x->pos).length2()) x->current_tile = x->target_tile;

		dt_vec = dt_vec.normed();
		x->pos += dt_vec * x->speed * dt;
	}
	for (auto& x : units) if (!x.to_remove) {
		x.to_remove = x->health < 0;
		if (x.to_remove) gold_gained += 1;
	}

	for (size_t i = 0; i < towers.size(); ++i) {
		auto world_pos = Vector2f{
			(towers[i]->tile_pos.x - size.x / 2.f) * bounding_tile_size(),
			(towers[i]->tile_pos.y - size.y / 2.f) * bounding_tile_size()
		};

		if (towers[i].typecheck(Tower::Archer_Kind)) {
			auto& x = towers[i].Archer_;
			x.attack_cd -= dt;
			if (x.attack_cd > 0) continue;

			for (auto& y : units) if ((world_pos - y->pos).length2() < x.range * x.range) {
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

			for (auto& y : units) if ((world_pos - y->pos).length2() < x.range * x.range) {
				y->health -= x.damage * dt;
			}
		}
	}

	for (auto& x : projectiles) if (!x.to_remove) {
		if (!units.exist(x.to)) { x.to_remove = true; continue; }
		auto& to = units.id(x.to);

		x.pos += (to->pos - x.pos).normed() * x.speed * dt;
		if ((to->pos - x.pos).length2() < 0.1f) {
			x.to_remove = true;
			to->health -= x.damage;
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
	render::Circle circle;
	render::Arrow arrow;
	render::Model m;

	m.object_id = asset::Object_Id::Empty_Tile;
	m.shader_id = asset::Shader_Id::Default_3D_Batched;
	m.texture_id = asset::Texture_Id::Palette;
	order.push(render::Push_Batch());
	for2(i, j, size) if (tiles[i + j * size.x].typecheck(Tile::Empty_Kind)) {
		m.pos = V3F(tile_box({i, j}).center() + pos, 0);
		m.scale = tile_box({i, j}).size.x;
		order.push(m);
	}
	order.push(render::Pop_Batch());

	if (gui.render_path) {
		for (size_t i = 0; i < next_tile.size(); ++i)
			if (tiles[i].typecheck(Tile::Empty_Kind))
		{
			size_t j = next_tile[i];
			if (j == SIZE_MAX) continue;

			arrow.a = tile_box({ i % size.x, i / size.x }).center() + pos;
			arrow.b = tile_box({ j % size.x, j / size.x }).center() + pos;
			arrow.color = V4F(0.5f);
			arrow.color.a = 1.f;

			order.push(arrow, 0.03f);
		}

		auto& path = path_construction;
		for (size_t i = path.open_idx; i < path.open.size(); ++i) {
			auto& x = path.open[i];
			circle.r = 0.1f;
			circle.color = {0, 0, 1, 1};
			circle.pos = tile_box(x).center() + pos;
			order.push(circle, 0.02f);
		}
		for (size_t i = 0; i < path.closed.size(); ++i) if (path.closed[i]) {
			circle.r = 0.05f;
			circle.color = {0, 1, 0, 1};
			circle.pos = tile_box(i).center() + pos;
			order.push(circle, 0.02f);
		}
	}

	circle.r = 0.2f;
	circle.color = { 0.6f, 0.5f, 0.1f, 1.f };

	m.object_id = asset::Object_Id::Projectile;
	order.push(render::Push_Batch());
	for (auto& x : projectiles) {
		m.scale = x.r;
		m.pos = V3F(x.pos + pos, 1);
		order.push(m);
	}
	order.push(render::Pop_Batch());

	thread_local std::unordered_map<size_t, std::vector<render::Model>> models_by_object;
	for (auto& [_, x] : models_by_object) x.clear();

	for (auto& x : towers) {
		m.object_id = x->object_id;
		m.shader_id = asset::Shader_Id::Default_3D_Batched;
		m.texture_id = asset::Texture_Id::Palette;
		m.scale = x->tile_size.x * tile_size;
		m.pos = Vector3f(tile_box(x->tile_pos, x->tile_size).center() + pos, 0.3f);
		m.bitmask = 0;
		models_by_object[m.object_id].push_back(m);

		m.object_id = asset::Object_Id::Base;
		m.scale = x->tile_size.x * tile_size;
		m.pos.z = 0;
		models_by_object[m.object_id].push_back(m);
	}
	
	m.texture_id = asset::Texture_Id::Palette;
	m.scale = 0.5f;
	m.bitmask |= render::Model::Edge_Glow;
	for (auto& x : units) {
		m.object_id = x->object_id;
		m.pos.x = x->pos.x + pos.x;
		m.pos.y = x->pos.y + pos.y;
		m.pos.z = std::sinf(x->life_time) * 0.1f + 0.3f;
		models_by_object[m.object_id].push_back(m);
	}

	for (auto& [object_id, x] : models_by_object) if (object_id != 0) {
		order.push(render::Push_Batch());
		for (auto& y : x) order.push(y);
		order.push(render::Pop_Batch());
	}
}

Tile& Board::tile(Vector2u pos) noexcept {
	return tiles[pos.x + pos.y * size.x];
}
Rectanglef Board::tile_box(Vector2u pos, Vector2u size) noexcept {
	Rectanglef rec;

	rec.size = (Vector2f)size * bounding_tile_size();
	rec.pos = {};

	rec.pos.x -= (this->size.x - 1) * bounding_tile_size() / 2;
	rec.pos.y -= (this->size.y - 1) * bounding_tile_size() / 2;

	rec.pos.x += bounding_tile_size() * pos.x;
	rec.pos.y += bounding_tile_size() * pos.y;
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

std::optional<Vector2u> Board::get_tile_at(Vector2f x) noexcept {
	auto s = tile_size + tile_padding;
	if (pos.x - size.x * s / 2 < x.x && x.x < pos.x + size.x * s / 2)
	if (pos.y - size.y * s / 2 < x.y && x.y < pos.y + size.y * s / 2) {
		Vector2u t;
		t.x = (x.x - (pos.x - size.x * s / 2)) / s;
		t.y = (x.y - (pos.y - size.y * s / 2)) / s;
		return t;
	}
	return std::nullopt;
}

void Board::insert_tower(Tower t) noexcept {
	auto pos = t->tile_pos;

	towers.push_back(std::move(t));
	for2 (i, j, t->tile_size) tiles[(pos.x + i) + (pos.y + j) * size.x] = Block{};
	invalidate_paths();
}

void Board::remove_tower(Vector2u p) noexcept {
	Tower* to_remove = nullptr;
	for (auto& x : towers) {
		if (x->tile_pos.x <= p.x && p.x < x->tile_pos.x + x->tile_size.x)
		if (x->tile_pos.y <= p.y && p.y < x->tile_pos.y + x->tile_size.y) to_remove = &x;
	}

	if (!to_remove) return;
	remove_tower(*to_remove);
}

void Board::remove_tower(Tower& to_remove) noexcept {
	to_remove.to_remove = true;
	auto& t = to_remove;
	for (size_t i = 0; i < t->tile_size.x; ++i)
	for (size_t j = 0; j < t->tile_size.y; ++j) {
		tiles[(t->tile_pos.x + i) + (t->tile_pos.y + j) * size.x] = Empty{};
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
		path.closed[i] = true;
	}
}

void Board::spawn_unit(Unit u) noexcept {
	size_t first = (size.y - start_zone_height) * size.x;
	size_t final = size.x * size.y;

	u->current_tile = (size_t)(first + xstd::random() * (final - first));

	auto rec = tile_box({u->current_tile % size.x, u->current_tile / size.x});

	u->pos.x = rec.x + xstd::random() * rec.w;
	u->pos.y = rec.y + xstd::random() * rec.h;

	units.push_back(u);
}

bool Board::can_place_at(Rectangleu zone) noexcept {
	if (zone.y < cease_zone_height) return false;
	if (zone.y + zone.h > size.y - start_zone_height) return false;

	for (size_t i = zone.x; i < zone.x + zone.w; ++i)
	for (size_t j = zone.y; j < zone.y + zone.h; ++j) {
		if (!tiles[i + j * size.x].typecheck(Tile::Empty_Kind)) return false;
	}

	return true;
}

Rectanglef Board::tower_box(const Tower& tower) noexcept {
	Rectanglef rec;
	rec.pos = pos;
	rec.x -= size.x * bounding_tile_size() / 2.f;
	rec.y -= size.y * bounding_tile_size() / 2.f;
	rec.x += tower->tile_rec.x * bounding_tile_size();
	rec.y += tower->tile_rec.y * bounding_tile_size();
	rec.w = tower->tile_rec.w * bounding_tile_size();
	rec.h = tower->tile_rec.h * bounding_tile_size();
	return rec;
}
