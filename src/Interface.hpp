#pragma once

#include <array>

#include "Graphic/Render.hpp"

#include "Math/Vector.hpp"
#include "Math/Rectangle.hpp"

#include "Managers/InputsManager.hpp"

struct Action {
	static constexpr size_t N = 3;
	enum class State {
		Null,
		Main,
		Build,
		Up,
		Left,
		Down,
		Right,
		Archer_Build,
		Splash_Build,
		Cancel,
		Count
	};

	using Tranistion_Table =
		std::array<State, N*N>;

	static constexpr Tranistion_Table Main_Table = {
		State::Build, State::Null, State::Null,
		State::Left,  State::Down, State::Right,
		State::Cancel,  State::Up,   State::Null
	};
	static constexpr Tranistion_Table Build_Table = {
		State::Main,         State::Null,         State::Null,
		State::Archer_Build, State::Splash_Build, State::Null,
		State::Null,         State::Null,         State::Null
	};

	inline static std::unordered_map<State, Tranistion_Table> Button_Nav_Map = {
		{State::Main,  Main_Table},
		{State::Build, Build_Table}
	};
	
	float button_content = 0.18f / 3;
	float button_padding = button_content / 20;
	float button_bounds  = button_content + button_padding;

	struct Button {
		Vector4f target_color = { 0.2f, 0.05f, 0.05f, 1.f };
		Vector4f actual_color = { 0.2f, 0.05f, 0.05f, 1.f };

		size_t texture_id = 0;

		bool hovered = false;
		bool pressed = false;
		bool just_pressed = false;
	};


	std::unordered_map<State, Button> state_button;
	State current_state = State::Main;

	Rectanglef get_zone() noexcept;
	void back_to_main() noexcept;
};

struct Interface {
	struct Ressource_Info {
		size_t golds = 0;
	};

	render::Camera ui_camera;

	Action action;
	float info_bar_height = 0.025f;

	Ressource_Info ressources;


	bool input(const Input_Info& info) noexcept;
	void update(double dt) noexcept;
	void render(render::Orders& order) noexcept;

	void init_buttons() noexcept;

	Interface() noexcept {
		ui_camera.frame_size = {1.6f, 0.9f};
		ui_camera.pos = ui_camera.frame_size / 2;
	}
	
};