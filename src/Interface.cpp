#include "Interface.hpp"

Rectanglef Interface::get_left_down_buttons_zone() noexcept {
	Rectanglef left_down_buttons_zone;
	left_down_buttons_zone.pos = {1.6f - 4 * button_bounds, 0.f};
	left_down_buttons_zone.size = {4 * button_bounds, 4 * button_bounds};
	return left_down_buttons_zone;
}

bool Interface::input(const Input_Info& info) noexcept {
	auto left_down_buttons_zone = get_left_down_buttons_zone();

	auto mouse_pos = project_mouse_pos(info.mouse_pos, ui_camera);

	bool mouse_pressed = info.mouse_infos[Mouse::Left].just_pressed;
	bool return_value = false;

	for (auto& x : left_down_buttons) x.pressed = false;
	for (auto& x : left_down_buttons) x.hovered = false;

	for2(x, y, Vector2u(4, 4)) {
		Rectanglef rec;
		rec.pos.x = left_down_buttons_zone.x + button_bounds * x + button_padding / 2;
		rec.pos.y = left_down_buttons_zone.y + button_bounds * y + button_padding / 2;
		rec.size = V2F(button_content);

		if (rec.in(mouse_pos)) {
			left_down_buttons[x + y * 4].hovered = true;
			left_down_buttons[x + y * 4].pressed = mouse_pressed;
			return_value |= true;
		}
	}
	return return_value;
}

void Interface::update(double dt) noexcept {
	for (auto& x : left_down_buttons) {
		x.actual_color += (x.target_color - x.actual_color) * dt;
		if (x.hovered) x.actual_color = std::max(x.target_color * 1.1f, x.actual_color);
		if (x.pressed) x.actual_color = std::max(x.target_color * 1.6f, x.actual_color);
	}
}

void Interface::render(render::Orders& orders) noexcept {
	orders.push_back(ui_camera);
	defer { orders.push_back(render::Pop_Camera{}); };
	
	auto left_down_buttons_zone = get_left_down_buttons_zone();

	render::Rectangle rec;
	rec.rec = left_down_buttons_zone;
	rec.color = { 0.1f, 0.1f, 0.1f, 1.0f };
	orders.push_back(rec);

	for2(x, y, Vector2u(4, 4)) {
		auto& it = left_down_buttons[x + y * 4];
		rec.pos.x = left_down_buttons_zone.x + button_bounds * x + button_padding / 2;
		rec.pos.y = left_down_buttons_zone.y + button_bounds * y + button_padding / 2;
		rec.size = V2F(button_content);

		rec.color = it.actual_color;

		orders.push_back(rec);
	}
}