#include "Interface.hpp"
#include "Managers/AssetsManager.hpp"

#include <map>
#include <iterator>

Rectanglef Action::get_zone() noexcept {
	Rectanglef zone;
	zone.pos = {0.f, 0.f};
	zone.size = N * Vector2f{button_bounds, button_bounds};
	return zone;
}

void Action::back_to_main() noexcept {
	current_state = Ui_State::Main;
}


void Tower_Selection::input(const Input_Info& info) noexcept {

}

void Tower_Selection::render(render::Orders& orders) noexcept {
	Rectanglef zone;
	zone.pos = pos;
	zone.size = V2F(content_size);

	render::Rectangle rec;
	render::Sprite sprite;
	render::Text text;

	if (!pool) return;
	if (selection.empty()) return;

	rec.rec = zone;
	rec.color = {0.0f, 0.15f, 0.15f, 1.0f};
	orders.push(rec);

	thread_local std::map<Tower::Kind, size_t> n_by_kind;
	thread_local xstd::vector<std::pair<size_t, Tower::Kind>> kind_by_n;
	n_by_kind.clear();
	kind_by_n.clear();
	for (auto& x : selection) if (pool->exist(x)) n_by_kind[pool->id(x).kind]++;
	for (auto& [a, b] : n_by_kind) kind_by_n.push_back({b, a});
	std::sort(BEG_END(kind_by_n), [] (const auto& a, const auto& b) { return a.first > b.first; });

	sprite.shader = asset::Shader_Id::Default;
	sprite.pos.x = 0.1f * content_size;
	sprite.pos.y = zone.y + (N - 1) * zone.h / N;
	sprite.size = V2F(content_size * 0.9f / N);
	text.font_id = asset::Font_Id::Consolas;
	text.height = sprite.size.y * 0.3f;
	text.origin = {0, 0.5f};
	for (size_t i = 0; i < N && i < kind_by_n.size(); ++i) {
		auto it = kind_by_n[i];
		sprite.texture = Tower(it.second)->texture_icon_id;

		auto str = std::to_string(it.first);

		text.pos = sprite.pos;
		text.pos.y += sprite.size.y / 2;
		text.pos.x += sprite.size.x + zone.w * 0.1f;
		text.text = orders.string(str.c_str());
		text.text_length = str.size();

		orders.push(sprite);
		orders.push(text);
		sprite.pos.y -= content_size / N;
	}

}

Ui_Table Tower_Selection::get_selected_table(
	xstd::unordered_map<Ui_State, Action::Button>& buttons
) noexcept {
	// >TOWER_TARGET_MODE:
	xstd::unordered_map<Tower_Target::Target_Mode, Ui_State> target_mode_to_icon = {
		{Tower_Target::First,  Ui_State::Target_First},
		{Tower_Target::Random, Ui_State::Target_Random},
		{Tower_Target::Closest, Ui_State::Target_Closest},
		{Tower_Target::Farthest, Ui_State::Target_Farthest}
	};
	
	Ui_Table table = TABLE(
		Ui_State::Sell,   Ui_State::Null, Ui_State::Null, Ui_State::Null,
		Ui_State::Cancel, Ui_State::Upgrade, Ui_State::Null, Ui_State::Null,
		Ui_State::Null,   Ui_State::Null, Ui_State::Null, Ui_State::Null,
		Ui_State::Pick_Target_Mode,   Ui_State::Null, Ui_State::Null, Ui_State::Null
	);

	if (picking_target_mode) {
		for (size_t i = 0; i < Tower_Target::Target_Mode::Count; ++i) {
			table[4 + i] = target_mode_to_icon[(Tower_Target::Target_Mode)i];
		}
	}

	for (auto& i : selection) if (pool->exist(i) && pool->id(i).kind == Tower::Volter_Kind) {
		table[8 + 1] = Ui_State::Surge_Spell;
		buttons[Ui_State::Surge_Spell].ready_percentage =
			1 - xstd::max(0.f, pool->id(i).Volter_.surge_timer) / pool->id(i).Volter_.surge_time;
	}

	return table;
}

bool Interface::input(const Input_Info& info) noexcept {
	tower_selection.input(info);

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

	if (!tower_selection.selection.empty())
		table = tower_selection.get_selected_table(action.state_button);

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

	tower_selection.pos.y = action.get_zone().h * 1.1f;

	if (!tower_selection.selection.empty()) {
		auto table = tower_selection.get_selected_table(action.state_button);

		if (action.state_button[Ui_State::Pick_Target_Mode].just_pressed) {
			tower_selection.picking_target_mode = !tower_selection.picking_target_mode;
		} else {
			if (tower_selection.picking_target_mode) {
				if (action.any_just_pressed) {
					tower_selection.picking_target_mode = false;
				}
			}
		}
	}

	for (auto& [s, x] : action.state_button) if (s != Ui_State::Null) {
		x.actual_color += (x.target_color - x.actual_color) * dt;
		if (x.hovered) x.actual_color = xstd::max(x.target_color * 1.3f, x.actual_color);
		if (x.pressed) x.actual_color = xstd::max(x.target_color * 1.8f, x.actual_color);
	}

	if (tower_selection.selection.empty()) {
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
	orders.push(render::Push_Ui{});
	defer { orders.push(render::Pop_Ui{}); };

	orders.push(ui_camera);
	defer { orders.push(render::Pop_Camera{}); };
	
	tower_selection.render(orders);

	auto action_zone = action.get_zone();

	render::Rectangle rec;
	render::Sprite sprite;
	render::Text text;

	rec.rec = action_zone;
	rec.color = { 0.1f, 0.1f, 0.1f, 1.0f };
	orders.push(rec, 2);

	// left down actions buttons
	auto table = action.Button_Nav_Map[action.current_state];
	if (!tower_selection.selection.empty())
		table = tower_selection.get_selected_table(action.state_button);
	for2(x, y, V2F(Action::N)) {
		auto& it  = action.state_button[table[x + y * Action::N]];
		rec.pos.x = action.button_bounds * x + action.button_padding / 2;
		rec.pos.y = action.button_bounds * y + action.button_padding / 2;
		rec.pos  += action_zone.pos,
		rec.size  =
		 V2F(action.button_content);
		rec.color = it.actual_color * it.ready_percentage;

		orders.push(rec, 3);

		if (it.texture_id) {
			sprite.rec = rec.rec;
			sprite.texture = it.texture_id;
			sprite.color = { it.ready_percentage, it.ready_percentage, it.ready_percentage, 1 };
			orders.push(sprite, 4);
		}
	}

	// rectangle selection
	if (dragging) {
		rec.color = {0, 1, 0, 0.1f};
		rec.rec = drag_selection;
		orders.push(rec, 2);
	}

	render_top_bar(orders);
}

void Interface::render_top_bar(render::Orders& orders) noexcept {
	render::Rectangle rec;
	render::Sprite sprite;
	render::Text text;
	thread_local std::string temp_string;
	temp_string.clear();

	if (current_player) {
		// top bar info
		rec.pos.x = 0;
		rec.pos.y = ui_camera.frame_size.y - info_bar_height;
		rec.size.x = ui_camera.frame_size.x;
		rec.size.y = info_bar_height;
		rec.color = { 0.1f, 0.1f, 0.1f, 1.0f };
		orders.push(rec, 2);

		Vector2f pos_gold =
			{ ui_camera.frame_size.x / 2, ui_camera.frame_size.y - info_bar_height / 2 };
		Vector2f pos_carbon = pos_gold;
		pos_carbon.x += 0.075f;
		Vector2f pos_hydro = pos_carbon;
		pos_hydro.x += 0.075f;
		Vector2f pos_oxy= pos_hydro;
		pos_oxy.x += 0.075f;

		auto push_ressource = [&](size_t texture_id, Vector2f pos, size_t n) {
			temp_string.clear();
			temp_string = std::to_string(n);
			if (temp_string.size() > 4) {
				temp_string = std::to_string((501 + n) / 1000);
				temp_string += 'k';
			}

			// top bar gold icon
			sprite.color = V4F(1);
			sprite.origin = V2F(0.5f);
			sprite.pos = pos;
			sprite.size = V2F(info_bar_height) / 1.5f;
			sprite.texture = texture_id;
			sprite.texture_rect.pos  = {0, 0};
			sprite.texture_rect.size = {1, 1};
			orders.push(sprite, 3);

			// top bar gold string
			text.pos = pos;
			text.pos.x += 0.01f;
			text.origin = { 0.f, .75f};
			text.color = {1, 1, 1, 1};
			text.height = info_bar_height * 0.4f;
			text.font_id = asset::Font_Id::Consolas;
			text.text = orders.string(temp_string.c_str());
			text.text_length = temp_string.size();
			orders.push(text, 4);
		};

		using namespace asset;
		push_ressource(Texture_Id::Gold_Icon,     pos_gold,   current_player->ressources.gold);
		push_ressource(Texture_Id::Carbon_Icon,   pos_carbon, current_player->ressources.carbons);
		push_ressource(Texture_Id::Hydrogen_Icon, pos_hydro,  current_player->ressources.hydrogens);
		push_ressource(Texture_Id::Oxygen_Icon,   pos_oxy,    current_player->ressources.oxygens);
	}

	temp_string = "Wave: #" + std::to_string(current_wave + 1);
	temp_string += " Next in: ";
	if (seconds_to_wave < 1) {
		temp_string += std::to_string((size_t)(seconds_to_wave * 10) / 10.f).substr(0, 3);
	} else {
		temp_string += std::to_string((size_t)seconds_to_wave);
	}
	text.pos.x = 0.01f;
	text.pos.y = ui_camera.frame_size.y - info_bar_height / 2;
	text.text = orders.string(temp_string.c_str());
	text.text_length = temp_string.size();
	orders.push(text, 3);
}

// >ADD_BUTTON(Tackwin):
void Interface::init_buttons() noexcept {
	auto& b = action.state_button;
	b[Ui_State::Build].texture_id             = asset::Texture_Id::Build_Icon;
	b[Ui_State::Main].texture_id              = asset::Texture_Id::Back_Icon;
	b[Ui_State::Null].texture_id              = asset::Texture_Id::Null_Icon;
	b[Ui_State::Up].texture_id                = asset::Texture_Id::Up_Icon;
	b[Ui_State::Left].texture_id              = asset::Texture_Id::Left_Icon;
	b[Ui_State::Down].texture_id              = asset::Texture_Id::Down_Icon;
	b[Ui_State::Right].texture_id             = asset::Texture_Id::Right_Icon;
	b[Ui_State::Mirror_Build].texture_id      = asset::Texture_Id::Mirror_Build_Icon;
	b[Ui_State::Splash_Build].texture_id      = asset::Texture_Id::Splash_Build_Icon;
	b[Ui_State::Cancel].texture_id            = asset::Texture_Id::Cancel_Icon;
	b[Ui_State::Send].texture_id              = asset::Texture_Id::Send_Icon;
	b[Ui_State::Send_First].texture_id        = asset::Texture_Id::Methane_Icon;
	b[Ui_State::Sell].texture_id              = asset::Texture_Id::Gold_Icon;
	b[Ui_State::Next_Wave].texture_id         = asset::Texture_Id::Left_Icon;
	b[Ui_State::Pick_Target_Mode].texture_id  = asset::Texture_Id::Target_Icon;
	b[Ui_State::Target_First].texture_id      = asset::Texture_Id::First_Icon;
	b[Ui_State::Target_Random].texture_id     = asset::Texture_Id::Dice_Icon;
	b[Ui_State::Target_Closest].texture_id    = asset::Texture_Id::Closest_Icon;
	b[Ui_State::Target_Farthest].texture_id   = asset::Texture_Id::Farthest_Icon;
	b[Ui_State::Volter_Build].texture_id      = asset::Texture_Id::Volter_Icon;
	b[Ui_State::Surge_Spell].texture_id       = asset::Texture_Id::Dummy;
	b[Ui_State::Upgrade].texture_id           = asset::Texture_Id::Upgrade_Icon;
	b[Ui_State::Heat_Build].texture_id        = asset::Texture_Id::Heat_Icon;
	b[Ui_State::Radiation_Build].texture_id   = asset::Texture_Id::Radiation_Icon;
	b[Ui_State::Circuit_Build].texture_id     = asset::Texture_Id::Circuit_Icon;
	b[Ui_State::Block_Build].texture_id     = asset::Texture_Id::Dummy;
}
