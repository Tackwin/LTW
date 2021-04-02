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
	struct Text {
		size_t font_id = 0;

		Vector2f origin;
		Vector2f pos;
		Vector4f color;

		float height = 0;

		char* text = nullptr;
		size_t text_length = 0;

		enum Style {
			Normal = 0,
			Underline = 1,
			Count = 2
		};

		std::uint32_t style_mask = 0;
	};
	struct Sprite {
		union {
			Rectanglef rec;
			struct {
				Vector2f pos;
				Vector2f size;
			};
		};
		Vector2f origin;
		float rotation = 0.f;
		Vector4f color = V4F(1);
		Rectanglef texture_rect;
		size_t texture = 0;
		size_t shader = 0;

		Sprite() {
			texture_rect.pos  = V2F(0);
			texture_rect.size = V2F(1);
		};
	};

	#define ORDER_LIST(X)\
	X(Rectangle) X(Circle) X(Camera) X(Pop_Camera) X(Arrow) X(Text) X(Sprite)

	struct Order { sum_type(Order, ORDER_LIST); };

	using Orders = std::vector<Order>;

	extern Camera current_camera;
	void immediate(Rectangle rec) noexcept;
	void immediate(Circle circle) noexcept;
	void immediate(Sprite sprite) noexcept;
	void immediate(Arrow arrow) noexcept;
	void immediate(Text text) noexcept;
};