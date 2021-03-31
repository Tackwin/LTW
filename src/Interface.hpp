#pragma once

#include <array>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

#include "Managers/InputsManager.hpp"

struct Button {
	Vector4f target_color = { 0.2f, 0.05f, 0.05f, 1.f };
	Vector4f actual_color = { 0.2f, 0.05f, 0.05f, 1.f };

	bool hovered = false;
	bool pressed = false;
};

struct Interface {
	render::Camera ui_camera;

	float button_content = 0.18f / 4;
	float button_padding = button_content / 20;
	float button_bounds  = button_content + button_padding;
	std::array<Button, 4*4> left_down_buttons;

	bool input(const Input_Info& info) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& order) noexcept;

	Rectanglef get_left_down_buttons_zone() noexcept;

	Interface() noexcept {
		ui_camera.frame_size = {1.6f, 0.9f};
		ui_camera.pos = ui_camera.frame_size / 2;
	}
	
	
};