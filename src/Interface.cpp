#include "Interface.hpp"
#include "Managers/AssetsManager.hpp"

Rectanglef Action::get_zone() noexcept {
	Rectanglef zone;
	zone.pos = {0.f, 0.f};
	zone.size = N * Vector2f{button_bounds, button_bounds};
	return zone;
}

void Action::back_to_main() noexcept {
	current_state = Ui_State::Main;
}


bool Interface::input(const Input_Info& info) noexcept {
	auto action_zone = action.get_zone();

	auto mouse_pos = project_mouse_pos(info.mouse_pos, ui_camera);

	bool mouse_pressed = info.mouse_infos[Mouse::Left].pressed;
	bool mouse_just_pressed = info.mouse_infos[Mouse::Left].just_pressed;
	bool return_value = false;

	auto table = action.Button_Nav_Map[action.current_state];

	for (auto& [_, x] : action.state_button) x.hovered = false;
	for (auto& [_, x] : action.state_button) x.pressed = false;
	for (auto& [_, x] : action.state_button) x.just_pressed = false;
	action.any_just_pressed = false;

	constexpr std::array<Keyboard::Key, Action::N * Action::N> key_map = TABLE(
		Keyboard::Num1, Keyboard::Num2, Keyboard::Num3, Keyboard::Num4,
		Keyboard::A,    Keyboard::Z,    Keyboard::E,    Keyboard::R,
		Keyboard::Q,    Keyboard::S,    Keyboard::D,    Keyboard::F,
		Keyboard::W,    Keyboard::X,    Keyboard::C,    Keyboard::V
	);

	for (auto& x : key_map) {
		action.any_just_pressed |= info.key_infos[x].just_pressed;
	}

	if (tower_selected.kind != Tower::None_Kind) table = tower_interface.get_table(tower_selected);

	for (size_t i = 0; i < Action::N * Action::N; ++i) {
		auto& it = action.state_button[table[i]];
		if (table[i] == Ui_State::Null) continue;
		it.pressed |= info.key_infos[key_map[i]].pressed;
		it.just_pressed |= info.key_infos[key_map[i]].just_pressed;
	}

	for2(x, y, V2F(Action::N)) {
		auto i = x + y * Action::N;
		if (table[i] == Ui_State::Null) continue;

		Rectanglef rec;
		rec.pos.x = action.button_bounds * x + action.button_padding / 2;
		rec.pos.y = action.button_bounds * y + action.button_padding / 2;
		rec.pos += action_zone.pos;
		rec.size = V2F(action.button_content);

		if (rec.in(mouse_pos)) {
			action.state_button[table[i]].hovered = true;
			action.state_button[table[i]].pressed |= mouse_pressed;
			action.state_button[table[i]].just_pressed |= mouse_just_pressed;
			return_value |= true;
		}
	}
	return return_value;
}

void Interface::update(double dt) noexcept {
	if (tower_selected.kind != Tower::None_Kind) {
		auto table = tower_interface.get_table(tower_selected);

		if (action.state_button[Ui_State::Pick_Target_Mode].just_pressed) {
			tower_interface.picking_target_mode = !tower_interface.picking_target_mode;
		} else {
			if (tower_interface.picking_target_mode) {
				if (action.any_just_pressed) {
					tower_interface.picking_target_mode = false;
				}
			}
		}
	}

	for (auto& [s, x] : action.state_button) if (s != Ui_State::Null) {
		x.actual_color += (x.target_color - x.actual_color) * dt;
		if (x.hovered) x.actual_color = std::max(x.target_color * 1.3f, x.actual_color);
		if (x.pressed) x.actual_color = std::max(x.target_color * 1.8f, x.actual_color);
	}

	if (tower_selected.kind == Tower::None_Kind) {
		auto& table = action.Button_Nav_Map[action.current_state];
		for (size_t i = 0; i < Action::N * Action::N; ++i) {
			auto& it = action.state_button[table[i]];
			if (!action.Button_Nav_Map.contains(table[i])) continue;
			if (it.just_pressed) {
				action.current_state = table[i];
				break;
			}
		}
	}
}

void Interface::render(render::Orders& orders) noexcept {
	orders.push(ui_camera);
	defer { orders.push(render::Pop_Camera{}); };
	
	auto action_zone = action.get_zone();

	render::Rectangle rec;
	render::Sprite sprite;
	render::Text text;

	rec.rec = action_zone;
	rec.color = { 0.1f, 0.1f, 0.1f, 1.0f };
	orders.push(rec, 2);

	// left down actions buttons
	auto table = action.Button_Nav_Map[action.current_state];
	if (tower_selected.kind != Tower::None_Kind) table = tower_interface.get_table(tower_selected);
	for2(x, y, V2F(Action::N)) {
		auto& it  = action.state_button[table[x + y * Action::N]];
		rec.pos.x = action.button_bounds * x + action.button_padding / 2;
		rec.pos.y = action.button_bounds * y + action.button_padding / 2;
		rec.pos  += action_zone.pos,
		rec.size  =
		 V2F(action.button_content);
		rec.color = it.actual_color;

		orders.push(rec, 3);

		if (it.texture_id) {
			sprite.rec = rec.rec;
			sprite.texture = it.texture_id;
			orders.push(sprite, 4);
		}
	}

	// rectangle selection
	if (dragging) {
		rec.color = {0, 1, 0, 0.1f};
		rec.rec = drag_selection;
		orders.push(rec, 2);
	}

	// top bar info
	rec.pos.x = 0;
	rec.pos.y = ui_camera.frame_size.y - info_bar_height;
	rec.size.x = ui_camera.frame_size.x;
	rec.size.y = info_bar_height;
	rec.color = { 0.1f, 0.1f, 0.1f, 1.0f };
	orders.push(rec, 2);

	thread_local std::string temp_string;
	temp_string.clear();
	temp_string = std::to_string(ressources.golds);

	// top bar gold icon
	sprite.color = V4F(1);
	sprite.origin = V2F(0.5f);
	sprite.pos = { ui_camera.frame_size.x / 2, ui_camera.frame_size.y - info_bar_height / 2 };
	sprite.size = V2F(info_bar_height) / 1.5f;
	sprite.texture = asset::Texture_Id::Gold_Icon;
	sprite.texture_rect.pos  = {0, 0};
	sprite.texture_rect.size = {1, 1};
	orders.push(sprite, 3);

	// top bar gold string
	text.pos.x = ui_camera.frame_size.x / 2 + info_bar_height / 2 + 0.01f;
	text.pos.y = ui_camera.frame_size.y - info_bar_height / 2;
	text.origin = { 0.f, .5f};
	text.color = {1, 1, 1, 1};
	text.height = info_bar_height * 0.5f;
	text.font_id = asset::Font_Id::Consolas;
	text.text = orders.string(temp_string.c_str());
	text.text_length = temp_string.size();
	orders.push(text, 3);

	temp_string = "Wave: #" + std::to_string(current_wave);
	text.pos.x = 0.01f;
	text.pos.y = ui_camera.frame_size.y - info_bar_height / 2;
	text.text = orders.string(temp_string.c_str());
	text.text_length = temp_string.size();
	orders.push(text, 3);
}

void Interface::init_buttons() noexcept {
	auto& b = action.state_button;
	b[Ui_State::Build].texture_id             = asset::Texture_Id::Build_Icon;
	b[Ui_State::Main].texture_id              = asset::Texture_Id::Back_Icon;
	b[Ui_State::Null].texture_id              = asset::Texture_Id::Null_Icon;
	b[Ui_State::Up].texture_id                = asset::Texture_Id::Up_Icon;
	b[Ui_State::Left].texture_id              = asset::Texture_Id::Left_Icon;
	b[Ui_State::Down].texture_id              = asset::Texture_Id::Down_Icon;
	b[Ui_State::Right].texture_id             = asset::Texture_Id::Right_Icon;
	b[Ui_State::Archer_Build].texture_id      = asset::Texture_Id::Archer_Build_Icon;
	b[Ui_State::Splash_Build].texture_id      = asset::Texture_Id::Splash_Build_Icon;
	b[Ui_State::Cancel].texture_id            = asset::Texture_Id::Cancel_Icon;
	b[Ui_State::Send].texture_id              = asset::Texture_Id::Send_Icon;
	b[Ui_State::Send_First].texture_id        = asset::Texture_Id::Methane_Icon;
	b[Ui_State::Sell].texture_id              = asset::Texture_Id::Gold_Icon;
	b[Ui_State::Next_Wave].texture_id         = asset::Texture_Id::Left_Icon;
	b[Ui_State::Pick_Target_Mode].texture_id  = asset::Texture_Id::Target_Icon;
	b[Ui_State::Target_First].texture_id      = asset::Texture_Id::First_Icon;
	b[Ui_State::Target_Random].texture_id     = asset::Texture_Id::Dice_Icon;
}

Ui_Table Tower_Interface::get_table(const Tower& tower) noexcept {
	std::unordered_map<Tower_Base::Target_Mode, Ui_State> target_mode_to_icon = {
		{Tower_Base::First,  Ui_State::Target_First},
		{Tower_Base::Random, Ui_State::Target_Random}
	};
	
	Ui_Table table = TABLE(
		Ui_State::Sell,   Ui_State::Null, Ui_State::Null, Ui_State::Null,
		Ui_State::Cancel, Ui_State::Null, Ui_State::Null, Ui_State::Null,
		Ui_State::Null,   Ui_State::Null, Ui_State::Null, Ui_State::Null,
		Ui_State::Pick_Target_Mode,   Ui_State::Null, Ui_State::Null, Ui_State::Null
	);

	if (picking_target_mode) {
		for (size_t i = 0; i < Tower_Base::Target_Mode::Count; ++i) {
			table[4 + i] = target_mode_to_icon[(Tower_Base::Target_Mode)i];
		}
	}

	return table;
}
