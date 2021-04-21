#pragma once

#include "xstd.hpp"

#include "Unit.hpp"

struct Board;

struct Wave {
	struct Bunch {
		std::vector<Unit> units;
		std::vector<size_t> to_spawn;
		std::vector<size_t> spawned;
		float duration = 1.f;

		void add_unit(Unit x, size_t n) noexcept;
	};

	std::vector<Bunch> bunches;
	std::vector<float> spaces;

	float cursor = 0.f;
	void add_bunch(Bunch b, float wait) noexcept;

	bool spawn(double dt, Board& board) noexcept;
};

extern Wave gen_wave(size_t n) noexcept;