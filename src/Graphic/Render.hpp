#pragma once

#include <span>

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"
#include "Math/Matrix.hpp"
#include "xstd.hpp"
#include "std/vector.hpp"

namespace render {
	struct Order_Base {
		float z = 1.f;
	};

	struct Depth_Test : Order_Base {
		bool active = true;
		enum Func {
			Equal,
			Greater,
			Less,
			Count
		};
		Func func;
	};
	struct Color_Mask : Order_Base {
		Vector4b mask;
	};

	struct Clear_Depth : Order_Base {};
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

	struct Particle : Order_Base {
		Vector3f pos;
		Vector3f dir = {1, 0, 0};
		Vector3f scale = {1, 1, 1};

		float intensity = 1;

		bool face_camera = true;

		float radial_velocity = 0;

		std::optional<Vector3f> velocity;
		std::optional<Vector4f> bloom;
	};

	struct Batch : Order_Base {
		size_t object_id = 0;
		size_t shader_id = 0;
		size_t texture_id = 0;
	};
	struct Pop_Batch : Order_Base {};

	struct Push_Ui : Order_Base {};
	struct Pop_Ui : Order_Base {};

	struct Camera3D : Order_Base {
		float fov = 70;
		float ratio = 16/9.f;
		float far_ = 500.f;
		float near_ = 0.01f;
		Vector3f pos;
		Vector3f last_pos;
		Vector3f dir;

		void look_at(Vector3f target) noexcept;
		Matrix4f get_projection() const noexcept;
		Matrix4f get_view(Vector3f up = {0, 0, 1}) const noexcept;
		Matrix4f get_VP(Vector3f up = {0, 0, 1}) const noexcept;

		Vector2f project(Vector2f mouse) noexcept;
	};
	struct Pop_Camera3D : Order_Base {};

	
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

	struct Ring : Order_Base {
		Vector3f pos = {};
		Vector3f normal = {0, 0, 1};
		
		Vector3f color;
		float glowing_intensity = 0.f;

		float radius = 1.f;
		float thickness = 0.f;
	};

	struct Model : Order_Base {
		size_t object_id = 0;
		size_t shader_id = 0;
		size_t texture_id = 0;

		Vector3f color = {1, 1, 1};

		Vector3f origin = {.5f, .5f, .5f};

		Vector3f pos;
		Vector3f dir = {1, 0, 0};
		float    scale = 1.f;

		bool object_blur = false;
		Vector3f last_pos;
		Vector3f last_dir = {1, 0, 0};
		float    last_scale = 1;

		bool ignore_depth_test = false;

		uint32_t bitmask = 0;
		enum {
			Edge_Glow = 1,
		};
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
		Vector4f color = {1, 1, 1, 1};

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
		Vector2f origin = {};
		float rotation = 0.f;
		Vector4f color = V4F(1);
		Rectanglef texture_rect = {V2F(0), V2F(1)};
		size_t texture = 0;
		size_t shader = 0;

		// Need to shut up deleted implictly default constructor.
		Sprite() { rec = {}; }
	};

	struct World_Sprite : Order_Base {
		Vector3f pos = {};
		float size = 1.f;

		size_t texture_id = 0;
	};

	#define ORDER_LIST(X)\
	X(Rectangle) X(Circle) X(Camera) X(Pop_Camera) X(Arrow) X(Text) X(Sprite) X(Model) X(Camera3D)\
	X(Pop_Camera3D) X(Clear_Depth) X(Batch) X(Pop_Batch) X(Push_Ui) X(Pop_Ui) X(Depth_Test)\
	X(Color_Mask) X(Ring) X(Particle) X(World_Sprite)

	struct Order {
		sum_type(Order, ORDER_LIST);
		sum_type_base(Order_Base);
	};


	struct Orders {
		xstd::vector<Order> commands;

		size_t frame_ptr = 0;
		xstd::vector<std::uint8_t> frame_data;

		Orders() noexcept {
			frame_data.resize(100'000, 0);
		}

		void push(Order o) noexcept;
		void push(Order o, float z) noexcept;

		void reserve(size_t n) noexcept { commands.reserve(n); }
		void clear() noexcept {
			commands.clear();
			memset(frame_data.data(), 0, frame_data.size());
			frame_ptr = 0;
		}

		auto begin() noexcept -> auto {
			return std::begin(commands);
		}
		auto end() noexcept -> auto {
			return std::end(commands);
		}

		char* string(const char* str) noexcept {
			char* ptr = (char*)frame_data.data() + frame_ptr;
			while (*str && frame_ptr < frame_data.size()) {
				frame_data[frame_ptr++] = *(str++);
			}
			frame_data[frame_ptr++] = 0;
			return ptr;
		}

		size_t size() const noexcept { return commands.size(); }

		Order& operator[](size_t idx) noexcept {
			return commands[idx];
		}
	};

	extern Order current_camera;

	extern void immediate(Rectangle rec) noexcept;
	extern void immediate(Circle circle) noexcept;
	extern void immediate(Sprite sprite) noexcept;
	extern void immediate(Arrow arrow) noexcept;
	extern void immediate(Model model) noexcept;
	extern void immediate(Text text) noexcept;

	extern void immediate(std::span<Rectangle> rectangles) noexcept;
	extern void immediate(std::span<Circle> circles) noexcept;
	extern void immediate(std::span<Arrow> arrows) noexcept;

	extern void immediate2d(std::span<Rectangle> rectangles) noexcept;
	extern void immediate2d(std::span<Circle> circles) noexcept;
	extern void immediate2d(std::span<Arrow> arrows) noexcept;

	extern void immediate3d(std::span<Rectangle> rectangles) noexcept;
	extern void immediate3d(std::span<Circle> circles) noexcept;
	extern void immediate3d(std::span<Arrow> arrows) noexcept;

	extern void immediate(
		xstd::span<World_Sprite> world_sprite, const Batch& batch, const Camera3D& camera
	) noexcept;
	extern void immediate(Ring ring, const Camera3D& camera) noexcept;
	extern void immediate(Particle particle, const Camera3D& camera, bool velocity) noexcept;
	extern void immediate(
		std::span<Model> models, const Batch& batch, const Camera3D& camera
	) noexcept;

	struct Render_Param {
		float gamma = 0.7f;
		float exposure = 1.0f;
		size_t n_samples = 4;

		float ssao_radius = 1.f;
		float ssao_bias = 1.f;
		float edge_blur = 3.f;

		float motion_scale = 0.1f;
		size_t target_fps = 240;
		size_t current_fps = 60;

		Vector3f cam_pos;

		bool render = true;
	};

	extern void render_orders(render::Orders& orders, Render_Param param) noexcept;
};