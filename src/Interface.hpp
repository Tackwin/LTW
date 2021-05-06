#pragma once

#include <array>

#include "Graphic/Render.hpp"
#include "std/vector.hpp"

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

#include "Managers/InputsManager.hpp"
#include "Tower.hpp"

#include "Player.hpp"

// >ADD_TOWER(Tackwin):
// >ADD_BUTTON(Tackwin):
enum class Ui_State {
	Null,
	Main,
	Build,
	Up,
	Left,
	Down,
	Right,
	Mirror_Build,
	Splash_Build,
	Volter_Build,
	Heat_Build,
	Radiation_Build,
	Circuit_Build,
	Block_Build,
	Surge_Spell,
	Cancel,
	Send,
	Send_First,
	Sell,
	Next_Wave,
	Pick_Target_Mode,
	Target_First,
	Target_Random,
	Target_Closest,
	Upgrade,
	Target_Farthest,
	Count
};
using Ui_Table = std::array<Ui_State, 4*4>;
#define TABLE(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) {\
	(m), (n), (o), (p),\
	(i), (j), (k), (l),\
	(e), (f), (g), (h),\
	(a), (b), (c), (d)\
}

struct Action {
	using U = Ui_State;
	static constexpr size_t N = 4;
	static constexpr Ui_Table Main_Table = TABLE(
		U::Null,    U::Null, U::Null,  U::Null,
		U::Cancel,  U::Up,   U::Null,  U::Null,
		U::Left,    U::Down, U::Right, U::Null,
		U::Build,   U::Send, U::Next_Wave,  U::Null
	);
// >ADD_TOWER(Tackwin):
	static constexpr Ui_Table Build_Table = TABLE(
		U::Null,         U::Null,         U::Null, U::Null,
		U::Radiation_Build, U::Circuit_Build,         U::Block_Build, U::Null,
		U::Mirror_Build, U::Splash_Build, U::Volter_Build, U::Heat_Build,
		U::Main,         U::Null,         U::Null, U::Null
	);
	static constexpr Ui_Table Send1_Table = TABLE(
		U::Null,       U::Null, U::Null, U::Null,
		U::Send_First, U::Null, U::Null, U::Null,
		U::Null,       U::Null, U::Null, U::Null,
		U::Main,       U::Null, U::Null, U::Null
	);

	inline static std::unordered_map<U, Ui_Table> Button_Nav_Map = {
		{U::Main,  Main_Table},
		{U::Build, Build_Table},
		{U::Send,  Send1_Table}
	};
	
	float button_content = 0.22f / N;
	float button_padding = button_content / 20;
	float button_bounds  = button_content + button_padding;

	struct Button {
		Vector4f target_color = { 0.2f, 0.05f, 0.05f, 1.f };
		Vector4f actual_color = { 0.2f, 0.05f, 0.05f, 1.f };

		size_t texture_id = 0;

		float ready_percentage = 1;

		bool hovered = false;
		bool pressed = false;
		bool just_pressed = false;
	};

	bool any_just_pressed = false;

	std::unordered_map<Ui_State, Button> state_button;
	Ui_State current_state = Ui_State::Main;

	Rectanglef get_zone() noexcept;
	void back_to_main() noexcept;
};

struct Tower_Selection {
	Vector2f pos;

	xstd::Pool<Tower>* pool = nullptr;
	xstd::vector<size_t> selection;

	static constexpr size_t N = 4;

	float content_size = 0.232f;
	bool picking_target_mode = false;

	void input(const Input_Info& info) noexcept;
	void render(render::Orders& orders) noexcept;

	Ui_Table get_selected_table(std::unordered_map<Ui_State, Action::Button>& buttons) noexcept;
};

struct Interface {
	render::Camera ui_camera;

	Action action;
	float info_bar_height = 0.025f;

	Player* current_player = nullptr;

	float dragging = false;
	Rectanglef drag_selection;

	Tower_Selection tower_selection;

	size_t current_wave = 0;
	float seconds_to_wave = 0;

	bool input(const Input_Info& info) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& order) noexcept;
	void render_top_bar(render::Orders& order) noexcept;

	void init_buttons() noexcept;

	Interface() noexcept {
		ui_camera.frame_size = {1.6f, 0.9f};
		ui_camera.pos = ui_camera.frame_size / 2;
	}
};