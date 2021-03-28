#pragma once

#include "Math/Vector.hpp"

namespace io {

	struct Keyboard {
		enum Key {
			Unknown = -1, ///< Unhandled key
			A = 0,        ///< The A key
			B,            ///< The B key
			C,            ///< The C key
			D,            ///< The D key
			E,            ///< The E key
			F,            ///< The F key
			G,            ///< The G key
			H,            ///< The H key
			I,            ///< The I key
			J,            ///< The J key
			K,            ///< The K key
			L,            ///< The L key
			M,            ///< The M key
			N,            ///< The N key
			O,            ///< The O key
			P,            ///< The P key
			Q,            ///< The Q key
			R,            ///< The R key
			S,            ///< The S key
			T,            ///< The T key
			U,            ///< The U key
			V,            ///< The V key
			W,            ///< The W key
			X,            ///< The X key
			Y,            ///< The Y key
			Z,            ///< The Z key
			Num0,         ///< The 0 key
			Num1,         ///< The 1 key
			Num2,         ///< The 2 key
			Num3,         ///< The 3 key
			Num4,         ///< The 4 key
			Num5,         ///< The 5 key
			Num6,         ///< The 6 key
			Num7,         ///< The 7 key
			Num8,         ///< The 8 key
			Num9,         ///< The 9 key
			Escape,       ///< The Escape key
			LCTRL,        ///< The left Control key
			LSHIFT,       ///< The left Shift key
			LALT,         ///< The left Alt key
			LSYS,         ///< The left OS specific key: window (Windows), apple (MacOS X), ...
			RCTRL,        ///< The right Control key
			RSHIFT,       ///< The right Shift key
			RALT,         ///< The right Alt key
			RSYS,         ///< The right OS specific key: window (Windows), apple (MacOS X), ...
			Menu,         ///< The Menu key
			LBracket,     ///< The [ key
			RBracket,     ///< The ] key
			Semicolon,    ///< The ; key
			Comma,        ///< The , key
			Period,       ///< The . key
			Quote,        ///< The ' key
			Slash,        ///< The / key
			Backslash,    ///< The \ key
			Tilde,        ///< The ~ key
			Equal,        ///< The = key
			Hyphen,       ///< The - key (hyphen)
			Space,        ///< The Space key
			Return,       ///< The Enter/Return keys
			Backspace,    ///< The Backspace key
			Tab,          ///< The Tabulation key
			PageUp,       ///< The Page up key
			PageDown,     ///< The Page down key
			End,          ///< The End key
			Home,         ///< The Home key
			Insert,       ///< The Insert key
			DEL,          ///< The Delete key
			Add,          ///< The + key
			Subtract,     ///< The - key (minus, usually from numpad)
			Multiply,     ///< The * key
			Divide,       ///< The / key
			Left,         ///< Left arrow
			Right,        ///< Right arrow
			Up,           ///< Up arrow
			Down,         ///< Down arrow
			Numpad0,      ///< The numpad 0 key
			Numpad1,      ///< The numpad 1 key
			Numpad2,      ///< The numpad 2 key
			Numpad3,      ///< The numpad 3 key
			Numpad4,      ///< The numpad 4 key
			Numpad5,      ///< The numpad 5 key
			Numpad6,      ///< The numpad 6 key
			Numpad7,      ///< The numpad 7 key
			Numpad8,      ///< The numpad 8 key
			Numpad9,      ///< The numpad 9 key
			F1,           ///< The F1 key
			F2,           ///< The F2 key
			F3,           ///< The F3 key
			F4,           ///< The F4 key
			F5,           ///< The F5 key
			F6,           ///< The F6 key
			F7,           ///< The F7 key
			F8,           ///< The F8 key
			F9,           ///< The F9 key
			F10,          ///< The F10 key
			F11,          ///< The F11 key
			F12,          ///< The F12 key
			F13,          ///< The F13 key
			F14,          ///< The F14 key
			F15,          ///< The F15 key
			Pause,        ///< The Pause key
			Count = 101
		};
	};


	struct Mouse {
		enum Button {
			Left = 0,
			Right,
			Middle,
			Count = 5
		};
	};

	struct Controller {
		enum Button {
			A = 0,
			B,
			X,
			Y,
			DPAD_UP,
			DPAD_DOWN,
			DPAD_LEFT,
			DPAD_RIGHT,
			START,
			BACK,
			LB,
			RB,
			LT,
			RT,
			Count = 32
		};
	};

	struct Keyboard_State {
		static constexpr auto Max_Key = 256;
		std::uint8_t keys[Max_Key];
	};

	struct Controller_State {
		static constexpr auto Left_Thumb_Deadzone = 7849;
		static constexpr auto Right_Thumb_Deadzone = 8689;
		static constexpr auto Trigger_Treshold = 30;

		size_t buttons_mask;
		std::uint8_t left_trigger;
		std::uint8_t right_trigger;
		std::int16_t left_thumb_x;
		std::int16_t left_thumb_y;
		std::int16_t right_thumb_x;
		std::int16_t right_thumb_y;
	};

	extern Keyboard_State get_keyboard_state() noexcept;
	extern Controller_State get_controller_state(size_t i = 0) noexcept;
	extern Vector2f get_mouse_pos() noexcept;
	extern size_t map_key(size_t x) noexcept;
	extern size_t map_mouse(size_t x) noexcept;
	extern size_t map_controller(size_t x) noexcept;
	extern bool is_window_focused() noexcept;
}
