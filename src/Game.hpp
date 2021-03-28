#pragma once

#include "Graphic/Order.hpp"

struct Game {
	void update(double dt) noexcept;
	void render(render::Orders& orders) noexcept;

	void startup() noexcept;
	void shutdown() noexcept;

	void post_char(size_t arg) noexcept;
};