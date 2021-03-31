#pragma once

#include <vector>

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"
#include "xstd.hpp"

namespace render {

	struct Camera {
		union {
			Rectanglef rec;
			struct {
				Vector2f pos;
				Vector2f frame_size;
			};
		};

		Camera() {};
	};
	struct Pop_Camera {};
	struct Rectangle {
		union {
			Rectanglef rec;
			struct {
				Vector2f pos;
				Vector2f size;
			};
		};
		Vector4f color;

		Rectangle() {};
	};
	struct Circle {
		Vector2f pos;
		Vector4f color;
		float r = 0;
	};
	struct Arrow {
		Vector2f a;
		Vector2f b;
		Vector4f color;
	};

	#define ORDER_LIST(X)\
	X(Rectangle) X(Circle) X(Camera) X(Pop_Camera) X(Arrow)

	struct Order { sum_type(Order, ORDER_LIST); };

	using Orders = std::vector<Order>;

	extern Camera current_camera;
	void immediate(Rectangle rec) noexcept;
	void immediate(Circle circle) noexcept;
	void immediate(Arrow arrow) noexcept;
};