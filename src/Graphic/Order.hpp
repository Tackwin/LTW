#pragma once

#include <vector>

#include "Math/Vector.hpp"
#include "xstd.hpp"

namespace render {
	struct Square {
		Vector2f pos;
		Vector2f size;
		Vector4f color;
	};
	struct Circle {
		Vector2f pos;
		float radius = 0;
	};

	#define ORDER_LIST(X)\
	X(Square) X(Circle)

	struct Order { sum_type(Order, ORDER_LIST); };

	using Orders = std::vector<Order>;
};