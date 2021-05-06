#pragma once

#include "xstd.hpp"

#include "Unit.hpp"

#include "std/vector.hpp"

struct Board;

struct Wave {
	struct Bunch {
		xstd::vector<Unit> units;
		xstd::vector<size_t> to_spawn;
		xstd::vector<size_t> spawned;
		float duration = 1.f;

		void add_unit(Unit x, size_t n) noexcept;
	};

	xstd::vector<Bunch> bunches;
	xstd::vector<float> spaces;

	float cursor = 0.f;
	void add_bunch(Bunch b, float wait) noexcept;

	bool spawn(double dt, Board& board) noexcept;
};

extern Wave gen_wave(size_t n) noexcept;