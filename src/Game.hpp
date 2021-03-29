#pragma once

#include <vector>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"

#include "Managers/InputsManager.hpp"

struct Common_Tile {
	Vector4f color = {.1f, .1f, .1f, 1};
};
struct Empty : Common_Tile {
	Empty() noexcept { color = {.1f, .1f, .1f, 1}; }
};
struct Archer : Common_Tile {
	Archer() noexcept { color = {1, 0, 0, 1}; }
};
struct Sharp : Common_Tile {
	Sharp()  noexcept { color = {0, 1, 0, 1}; }
};

#define TILE_LIST(X)\
	X(Archer) X(Sharp) X(Empty)
struct Tile {
	sum_type(Tile, TILE_LIST);
	sum_type_base(Common_Tile);
};

struct Board {
	Vector2u size;
	float tile_size = 10;
	float tile_padding = 1;

	std::vector<Tile> tiles;

	void input(const Input_Info& in, Vector2f mouse_world_pos) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	Tile& tile(Vector2u pos) noexcept;
	Rectanglef tile_box(Vector2u pos) noexcept;
};

struct Game {
	Board board;
	Input_Record input_record;

	Vector2f pointer;
	render::Camera camera;

	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	void startup() noexcept;
	void shutdown() noexcept;

	void post_char(size_t arg) noexcept;
};