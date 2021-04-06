#pragma once

#include <vector>
#include <span>

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"
#include "xstd.hpp"

namespace render {
	struct Order_Base {
		float z = 1.f;
	};

	struct Camera : Order_Base {
		union {
			Rectanglef rec;
			struct {
				Vector2f pos;
				Vector2f frame_size;
			};
		};

		Camera() {};
	};
	struct Pop_Camera : Order_Base {};
	struct Rectangle : Order_Base {
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
	struct Model : Order_Base {
		size_t object_id = 0;
		size_t shader_id = 0;
		size_t texture_id = 0;

		Vector3f pos;
		float scale = 1.f;
	};
	struct Circle : Order_Base {
		Vector2f pos;
		Vector4f color;
		float r = 0;
	};
	struct Arrow : Order_Base {
		Vector2f a;
		Vector2f b;
		Vector4f color;
	};
	struct Text : Order_Base {
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
	struct Sprite : Order_Base {
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
	X(Rectangle) X(Circle) X(Camera) X(Pop_Camera) X(Arrow) X(Text) X(Sprite) X(Model)

	struct Order {
		sum_type(Order, ORDER_LIST);
		sum_type_base(Order_Base);
	};


	struct Orders {
		std::vector<Order> commands;

		void push(Order o) noexcept;
		void push(Order o, float z) noexcept;

		void reserve(size_t n) noexcept { commands.reserve(n); }
		void clear() noexcept { commands.clear(); }

		auto begin() noexcept -> auto {
			return std::begin(commands);
		}
		auto end() noexcept -> auto {
			return std::end(commands);
		}
	};

	extern Camera current_camera;
	void immediate(Rectangle rec) noexcept;
	void immediate(Circle circle) noexcept;
	void immediate(Sprite sprite) noexcept;
	void immediate(Arrow arrow) noexcept;
	void immediate(Model model) noexcept;
	void immediate(Text text) noexcept;
	void immediate(std::span<Rectangle> rectangles) noexcept;
	void immediate(std::span<Circle> circles) noexcept;
	void immediate(std::span<Arrow> arrows) noexcept;
};