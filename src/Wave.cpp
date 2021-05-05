#include "Wave.hpp"
#include "Board.hpp"

bool Wave::spawn(double dt, Board& board) noexcept {
	cursor += dt;
	float running = 0;

	for (size_t i = 0; i < bunches.size(); ++i) {
		auto& bunch = bunches[i];


		// We have still work to do in this bunch
		if (cursor < running + bunch.duration) {
			auto to_do = cursor - (running);
			auto t = to_do / bunch.duration;

			for (size_t j = 0; j < bunch.to_spawn.size(); ++j) {
				size_t to_spawn = (size_t)std::roundf(bunch.to_spawn[j] * t);
				size_t left_to_spawn = to_spawn - bunch.spawned[j];

				for (size_t k = 0; k < left_to_spawn; ++k) board.spawn_unit(bunch.units[j]);
				bunch.spawned[j] += left_to_spawn;
			}

			break;
		}
		running += bunch.duration;


		// We are in the pause for the next bunch
		if (cursor < running + spaces[i]) break;
		running += spaces[i];


		// Otherwise we go to the next bunch
	}
	return false;
}

void Wave::Bunch::add_unit(Unit x, size_t n) noexcept {
	units.push_back(std::move(x));
	to_spawn.push_back(n);
	spawned.push_back(0);
}

void Wave::add_bunch(Bunch b, float wait) noexcept {
	bunches.push_back(std::move(b));
	spaces.push_back(wait);
}

Wave gen_wave(size_t n) noexcept {
	Wave wave;
	Wave::Bunch bunch;

	size_t to_spawn = 5 * (1 + n) * (1 + n / 4);
	if (n > 10) bunch.add_unit(Water(), (9 + to_spawn) / 10);
	if (n > 20) bunch.add_unit(Oxygen(), (9 + to_spawn) / 10);
	if ((n % 4) == 0) bunch.add_unit(Methane(), to_spawn);
	if ((n % 4) == 1) bunch.add_unit(Ethane(), to_spawn);
	if ((n % 4) == 2) bunch.add_unit(Propane(), to_spawn);
	if ((n % 4) == 3) {
		bunch.add_unit(Methane(), to_spawn);
		bunch.duration = 1 * (size_t)sqrt(1 + n);
		wave.add_bunch(bunch, 1);
		bunch = {};
		bunch.duration = 1 * (size_t)sqrt(1 + n);
		bunch.add_unit(Ethane(), to_spawn);
		wave.add_bunch(bunch, 1);
		bunch = {};
		bunch.duration = 1 * (size_t)sqrt(1 + n);
		bunch.add_unit(Propane(), to_spawn);
		wave.add_bunch(bunch, 1);
		bunch = {};
		bunch.add_unit(Butane(), to_spawn);
	}

	bunch.duration = 1 * (size_t)sqrt(1 + n);

	wave.add_bunch(bunch, 0);

	return wave;
}