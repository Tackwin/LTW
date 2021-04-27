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
	seconds_elapsed += dt;
	if (tiles.size() != size.x * size.y) tiles.resize(size.x * size.y, Empty{});

	if (path_construction.soft_dirty) soft_compute_paths();
	else if (path_construction.dirty) compute_paths();

	unit_spatial_partition();

	ressources_gained = {};
	current_wave.spawn(dt, *this);

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

		if (x->pos.y + size.y * bounding_tile_size() / 2 < cease_zone_height) x.to_remove = true;
	}
	for (auto& x : units) if (!x.to_remove) {
		if (x->health <= 0) {
			if (!x.to_remove) x->to_die = true;
			x.to_remove = true;
		}
		if (x.to_remove) {
			ressources_gained = add(ressources_gained, x->get_drop());
		}
	}

	for (auto& x : die_effects) {
		x.age -= dt;
	}

	for (size_t i = 0; i < towers.size(); ++i) {
		Vector2f tower_pos = tile_box(towers[i]->tile_pos, towers[i]->tile_size).center();

		if (towers[i].kind == Tower::Mirror_Kind) {
			auto& x = towers[i].Mirror_;
			x.attack_cd -= dt;
			if (units.exist(x.target_id))
				x.charging_anim = 1.f - (std::max(x.attack_cd, 0.f) / x.attack_speed);
			else
				x.charging_anim = 0.f;
			if (x.attack_cd > 0) continue;

			auto r2 = x.range * x.range;

			if (!units.exist(x.target_id) || tower_pos.dist_to2(units.id(x.target_id)->pos) > r2) {
				pick_new_target(x);
			}

			if (units.exist(x.target_id)) {
				auto& y = units.id(x.target_id);
				if ((tower_pos - y->pos).length2() < r2) {
					Seek_Projectile new_projectile;
					new_projectile.from = tiles[i].id;
					new_projectile.to = y.id;

					new_projectile.pos = tower_pos;
					projectiles.push_back(new_projectile);
					x.attack_cd = x.attack_speed;
				}
			}
		} else if (towers[i].kind == Tower::Mirror2_Kind) {
			auto& x = towers[i].Mirror2_;
			x.attack_cd -= dt;
			if (units.exist(x.target_id))
				x.charging_anim = 1.f - (std::max(x.attack_cd, 0.f) / x.attack_speed);
			else
				x.charging_anim = 0.f;
			if (x.attack_cd > 0) continue;

			auto r2 = x.range * x.range;

			if (!units.exist(x.target_id) || tower_pos.dist_to2(units.id(x.target_id)->pos) > r2) {
				pick_new_target(x);
			}

			if (units.exist(x.target_id)) {
				auto& y = units.id(x.target_id);
				if ((tower_pos - y->pos).length2() < r2) {
					Seek_Projectile new_projectile;
					new_projectile.from = tiles[i].id;
					new_projectile.to = y.id;

					new_projectile.pos = tower_pos;
					projectiles.push_back(new_projectile);
					x.attack_cd = x.attack_speed;
				}
			}
		} else if (towers[i].typecheck(Tower::Sharp_Kind)) {
			auto& x = towers[i].Sharp_;
			x.rot += dt * 5;

			for (auto& y : units) if ((tower_pos - y->pos).length2() < x.range * x.range) {
				y->health -= x.damage * dt;
			}

		} else if (towers[i].kind == Tower::Volter_Kind) {
			auto& x = towers[i].Volter_;
			x.surge_timer -= dt;

			if (x.surge_timer < 0 && x.to_surge) {
				size_t n_projectiles = 200;
				for (size_t j = 0; j < n_projectiles; ++j) {
					Straight_Projectile p;
					p.dir = Vector2f::createUnitVector(2 * 3.1415926 * j / (n_projectiles - 1));
					p.pos = tower_pos;
					p.r   = 0.2f;
					p.from = towers[i].id;
					p.life_time = 10;
					p.damage = 1.f;
					p.power = 20;
					projectiles.push_back(p);
				}
				x.surge_timer = x.surge_time;
			}
			x.to_surge = x.always_surge;
		}
	}

	for (auto& y : projectiles) {
		y->life_time -= dt;
		if (y->life_time < 0) y.to_remove = true;
		if (y.to_remove) continue;

		if (y.kind == Projectile::Seek_Projectile_Kind) {
			
			auto& x = y.Seek_Projectile_;
			if (!units.exist(x.to)) { y.to_remove = true; continue; }
			auto& to = units.id(x.to);
			if (to.to_remove) { y.to_remove = true; continue; }

			x.pos += (to->pos - x.pos).normed() * x.speed * dt;
			if ((to->pos - x.pos).length2() < 0.1f) {
				y.to_remove = true;
				to->health -= x.damage;
			}

			auto target = units.id(x.to)->pos;
			x.dir = (target - x.pos).normalize();
		} else if (y.kind == Projectile::Straight_Projectile_Kind) {
			auto& x = y.Straight_Projectile_;

			x.pos += x.dir * x.speed * dt;

			for (auto& u : units) if (!u.to_remove) {
				auto d = (x.pos - u->pos).length2();

				if (d < x.r * x.r) {
					u->health -= x.damage;
					x.power--;
					if (x.power == 0) y.to_remove = true;
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < units.size(); ++i) {
		auto& x = units[i];
		if (x->to_die) {
			on_death(*this, x);
			die_event_at(Vector3f(x->pos, 0.5f));
			x->to_die = false;
		}
	}

	xstd::remove_all(
		die_effects, [](auto& x) { return x.age < 0; }
	);
	for (size_t i = projectiles.size() - 1; i + 1 > 0; --i) if (projectiles[i].to_remove)
		projectiles.remove_at(i);
	for (size_t i = units.size() - 1; i + 1 > 0; --i) if (units[i].to_remove)
		units.remove_at(i);
	for (size_t i = towers.size() - 1; i + 1 > 0; --i) if (towers[i].to_remove)
		towers.remove_at(i);
}

void Board::render(render::Orders& order) noexcept {
	render::Particle particle;
	render::Circle circle;
	render::Arrow arrow;
	render::Model m;
	render::Batch b;
	render::Depth_Test depth;

	b.object_id = asset::Object_Id::Empty_Tile;
	b.shader_id = asset::Shader_Id::Default_3D_Batched;
	b.texture_id = asset::Texture_Id::Palette;

	order.push(b);
	m.origin = {0.5f, 0.5f, 0.5f};
	for2(i, j, size) if (tiles[i + j * size.x].typecheck(Tile::Empty_Kind)) {
		m.pos = Vector3f(tile_box({i, j}).center() + pos, 0);
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

			order.push(arrow, 0.5f);
		}

		auto& path = path_construction;
		for (size_t i = path.open_idx; i < path.open.size(); ++i) {
			auto& x = path.open[i];
			circle.r = 0.1f;
			circle.color = {0, 0, 1, 1};
			circle.pos = tile_box(x).center() + pos;
			order.push(circle, 0.5f);
		}
		for (size_t i = 0; i < path.closed.size(); ++i) if (path.closed[i]) {
			circle.r = 0.05f;
			circle.color = {0, 1, 0, 1};
			circle.pos = tile_box(i).center() + pos;
			order.push(circle, 0.5f);
		}
	}

	circle.r = 0.2f;
	circle.color = { 0.6f, 0.5f, 0.1f, 1.f };

	m.object_blur = false;
	m.dir = {1, 0, 0};

	thread_local std::unordered_map<size_t, std::vector<render::Model>> models_by_object;
	for (auto& [_, x] : models_by_object) x.clear();

	for (auto& x : units) {
		m.object_blur = true;
		m.object_id = x->object_id;
		m.pos.x = x->pos.x + pos.x;
		m.pos.y = x->pos.y + pos.y;
		m.pos.z = std::sinf(x->life_time) * 0.1f + 0.3f;
		m.last_pos.x = x->last_pos.x + pos.x;
		m.last_pos.y = x->last_pos.y + pos.y;
		m.last_pos.z = std::sinf(x->life_time) * 0.1f + 0.3f;
		m.scale = 1;
		m.last_scale = m.scale;
		m.last_dir = m.dir;
		m.color = x->color;
		models_by_object[m.object_id].push_back(m);

		m.object_blur = false;

		x->last_pos = x->pos;
	}
	m.color = {1, 1, 1};

	for (auto& x : towers) if (x.kind == Tower::Mirror_Kind) if (x.Mirror_.charging_anim > 0.f) {
		auto plane_pos = tile_box(x->tile_pos, x->tile_size).center() + pos;
		particle.pos = Vector3f(plane_pos, bounding_tile_size() * 0.3f * x->tile_size.x);
		particle.scale = {0.4f, 0.4f, 0.4f};
		particle.bloom = {1, 1, 0, 0.2f};
		particle.intensity = x.Mirror_.charging_anim;
		particle.radial_velocity = true;
		order.push(particle);
	}
	for (auto& x : die_effects) {
		auto ease = [](auto x) { return x * x * x - x * x * x * x * x * x; };
		particle.pos = x.pos + Vector3f(pos, 0);
		particle.intensity = 4 * ease(x.age / x.Lifetime);
		particle.scale = V3F(particle.intensity / 4);
		particle.bloom = {1, 1, 1, 1};
		particle.dir = {1, 0, 0};
		particle.radial_velocity = true;
		order.push(particle);
	}

	m.origin = {0.5f, 0.5f, 0.0f};
	for (auto& x : towers) {
		auto plane_pos = tile_box(x->tile_pos, x->tile_size).center() + pos;

		m.object_id = x->object_id;
		m.shader_id = asset::Shader_Id::Default_3D_Batched;
		m.texture_id = asset::Texture_Id::Palette;
		m.scale = x->tile_size.x * tile_size;
		m.pos = Vector3f(plane_pos, bounding_tile_size() * 0.1f * x->tile_size.x);
		m.bitmask = 0;

		m.dir = {1, 0, 0};
		m.object_blur = false;
		if (x.kind == Tower::Sharp_Kind) {
			m.object_blur = true;

			m.dir      = Vector3f(Vector2f::createUnitVector(x.Sharp_.rot), 0);
			m.last_dir = Vector3f(Vector2f::createUnitVector(x.Sharp_.last_rot), 0);
			m.last_pos = m.pos;
			m.last_scale = m.scale;

			x.Sharp_.last_rot = x.Sharp_.rot;
		}
		if (x.kind == Tower::Mirror_Kind || x.kind == Tower::Mirror2_Kind) {
			if (units.exist(x->target_id)) {
				auto dt = units.id(x->target_id)->pos - plane_pos;
				m.dir = Vector3f(dt.normed(), 0);
			} else {
				m.dir = {1, 0, 0};
			}
		}

		models_by_object[m.object_id].push_back(m);
		m.object_blur = false;

		m.dir = {1, 0, 0};
		m.object_id = asset::Object_Id::Base;
		m.scale = x->tile_size.x * bounding_tile_size();
		m.pos.z = 0;
		models_by_object[m.object_id].push_back(m);
	}
	
	m.texture_id = asset::Texture_Id::Palette;
	m.bitmask |= render::Model::Edge_Glow;

	m.object_blur = true;
	m.origin = {0.5f, 0.5f, 0.5f};
	for (auto& x : projectiles) {
		m.scale = x->r;
		m.last_scale = x->r;
		m.pos = Vector3f(x->pos + pos, 0.5f);
		m.last_pos = Vector3f(x->last_pos + pos, 0.5f);

		m.dir = Vector3f(x->dir, 0);
		m.last_dir = Vector3f(x->last_dir, 0);

		m.object_id = x->object_id;

		models_by_object[m.object_id].push_back(m);

		x->last_pos = x->pos;
		x->last_dir = x->dir;
	}

	render::Color_Mask color_mask;
	color_mask.mask = {false, false, false, false};
	order.push(color_mask);
	for (auto& [object_id, x] : models_by_object) if (object_id != 0) {
		b.object_id = object_id;
		b.shader_id = asset::Shader_Id::Depth_Prepass;
		order.push(b);
		for (auto& y : x) order.push(y);
		order.push(render::Pop_Batch());
	}
	color_mask.mask = {true, true, true, true};
	order.push(color_mask);

	depth.func = depth.Equal;
	order.push(depth);
	for (auto& [object_id, x] : models_by_object) if (object_id != 0) {
		b.object_id = object_id;
		b.shader_id = asset::Shader_Id::Default_3D_Batched;
		order.push(b);
		for (auto& y : x) order.push(y);
		order.push(render::Pop_Batch());
	}
	depth.func = depth.Less;
	order.push(depth);
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

	dist_tile.clear();
	dist_tile.resize(tiles.size(), SIZE_MAX);

	for (size_t i = 0; i < size.x; ++i) {
		path.open.push_back(i);
		path.closed[i] = true;;
		dist_tile[i] = 0;
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
			dist_tile[t] = dist_tile[it] + 1;
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
	dist_tile.resize(tiles.size(), SIZE_MAX);

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
		dist_tile[t] = dist_tile[it] + 1;
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
	dist_tile.clear();
	dist_tile.resize(tiles.size(), SIZE_MAX);

	for (size_t i = 0; i < size.x; ++i) {
		path.open.push_back(i);
		dist_tile[i] = 0;
		path.closed[i] = true;
	}
}

void Board::spawn_unit(Unit u) noexcept {
	size_t first = (size.y - start_zone_height) * size.x;
	size_t final = size.x * size.y;

	size_t t = (size_t)(first + xstd::random() * (final - first));
	spawn_unit_at(u, { t % size.x, t / size.x });
}
void Board::spawn_unit_at(Unit u, Vector2u tile) noexcept {
	u->current_tile = tile.x + tile.y * size.x;

	auto rec = tile_box(tile);

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
	rec.x -= (size.x - 1) * bounding_tile_size() / 2.f;
	rec.y -= (size.y - 1) * bounding_tile_size() / 2.f;
	rec.x += tower->tile_rec.x * bounding_tile_size();
	rec.y += tower->tile_rec.y * bounding_tile_size();
	rec.w = tower->tile_rec.w * bounding_tile_size();
	rec.h = tower->tile_rec.h * bounding_tile_size();
	return rec;
}

void Board::pick_new_target(Mirror& tower) noexcept {
	auto tower_pos = tile_box(tower.tile_rec).center();

	auto r = tower.range * tower.range;

	tower.target_id = 0;
	switch (tower.target_mode) {
		case Tower_Base::Target_Mode::First: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (x->pos.dist_to2(tower_pos) > r) continue;
				if (!candidate) candidate = &x;

				auto d_candidate = dist_tile[(*candidate)->current_tile];
				auto d_current   = dist_tile[x->current_tile];
				if (d_current < r && d_current < d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Closest: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (!candidate) candidate = &x;

				auto d_candidate = (*candidate)->pos.dist_to2(tower_pos);
				auto d_current   = x->pos.dist_to2(tower_pos);
				if (d_current < r && d_current < d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Farthest: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (!candidate) candidate = &x;

				auto d_candidate = (*candidate)->pos.dist_to2(tower_pos);
				auto d_current   = x->pos.dist_to2(tower_pos);
				if (d_current < r && d_current > d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Random: {
			Unit* candidate = nullptr;
			for (size_t i = 1; auto& x : units) {
				if (x->pos.dist_to2(tower_pos) <= r && xstd::random() < 1.f / (i++)) {
					candidate = &x;
				}
			}

			if (candidate) tower.target_id = candidate->id;
			break;
		}
		default: break;
	}
}
void Board::pick_new_target(Mirror2& tower) noexcept {
	auto tower_pos = tile_box(tower.tile_rec).center();

	auto r = tower.range * tower.range;

	tower.target_id = 0;
	switch (tower.target_mode) {
		case Tower_Base::Target_Mode::First: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (x->pos.dist_to2(tower_pos) > r) continue;
				if (!candidate) candidate = &x;

				auto d_candidate = dist_tile[(*candidate)->current_tile];
				auto d_current   = dist_tile[x->current_tile];
				if (d_current < r && d_current < d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Closest: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (!candidate) candidate = &x;

				auto d_candidate = (*candidate)->pos.dist_to2(tower_pos);
				auto d_current   = x->pos.dist_to2(tower_pos);
				if (d_current < r && d_current < d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Farthest: {
			Unit* candidate = nullptr;
			for (auto& x : units) {
				if (!candidate) candidate = &x;

				auto d_candidate = (*candidate)->pos.dist_to2(tower_pos);
				auto d_current   = x->pos.dist_to2(tower_pos);
				if (d_current < r && d_current > d_candidate) candidate = &x;
			}

			if (candidate && (*candidate)->pos.dist_to2(tower_pos) <= r)
				tower.target_id = candidate->id;
			break;
		}
		case Tower_Base::Target_Mode::Random: {
			Unit* candidate = nullptr;
			for (size_t i = 1; auto& x : units) {
				if (x->pos.dist_to2(tower_pos) <= r && xstd::random() < 1.f / (i++)) {
					candidate = &x;
				}
			}

			if (candidate) tower.target_id = candidate->id;
			break;
		}
		default: break;
	}
}

void Board::unit_spatial_partition() noexcept {
	// >TODO(Tackwin): this crash.
	return;
	unit_id_by_tile.resize(size.x * size.y);
	for (auto& x : unit_id_by_tile) x.clear();

	for (auto& x : units) {
		Vector2f p = x->pos + Vector2f{size.x - 1.f, size.y - 1.f} * tile_size / 2;
		p /= tile_size;
		Vector2u p_u = (Vector2u)p;

		unit_id_by_tile[p_u.x + p_u.y * size.x].push_back(x.id);
	}
}

void Board::die_event_at(Vector3f pos) noexcept {
	Die_Effect d;
	d.pos = pos;
	die_effects.push_back(d);
}