#include "OS/RealTimeIO.hpp"

#define NOGDI
#include <cassert>
#include <Windows.h>
#include <Xinput.h>
#include <tchar.h>

#include "global.hpp"

using namespace io;

extern io::Keyboard_State emscripten_keyboard_state;

Keyboard_State io::get_keyboard_state() noexcept {
	BYTE windows_state[Keyboard_State::Max_Key];
	GetKeyboardState(windows_state);

	Keyboard_State state{};
	for (size_t i = 0; i < Keyboard_State::Max_Key; ++i) {
		state.keys[i] = windows_state[i] & 0b10000000;
	}
	return state;
}

Controller_State io::get_controller_state(size_t i) noexcept {
	XINPUT_STATE windows_state = {};
	XInputGetState(i, &windows_state);

	Controller_State state = {};
	state.buttons_mask = (size_t)windows_state.Gamepad.wButtons;
	state.left_trigger = (std::uint8_t)windows_state.Gamepad.bLeftTrigger;
	state.right_trigger = (std::uint8_t)windows_state.Gamepad.bRightTrigger;
	state.left_thumb_x = (std::int16_t)windows_state.Gamepad.sThumbLX;
	state.left_thumb_y = (std::int16_t)windows_state.Gamepad.sThumbLY;
	state.right_thumb_x = (std::int16_t)windows_state.Gamepad.sThumbRX;
	state.right_thumb_y = (std::int16_t)windows_state.Gamepad.sThumbRY;
	return state;
}

size_t io::map_key(size_t x) noexcept {
	switch (x)
	{
	case Keyboard::A:          return 'A';
	case Keyboard::B:          return 'B';
	case Keyboard::C:          return 'C';
	case Keyboard::D:          return 'D';
	case Keyboard::E:          return 'E';
	case Keyboard::F:          return 'F';
	case Keyboard::G:          return 'G';
	case Keyboard::H:          return 'H';
	case Keyboard::I:          return 'I';
	case Keyboard::J:          return 'J';
	case Keyboard::K:          return 'K';
	case Keyboard::L:          return 'L';
	case Keyboard::M:          return 'M';
	case Keyboard::N:          return 'N';
	case Keyboard::O:          return 'O';
	case Keyboard::P:          return 'P';
	case Keyboard::Q:          return 'Q';
	case Keyboard::R:          return 'R';
	case Keyboard::S:          return 'S';
	case Keyboard::T:          return 'T';
	case Keyboard::U:          return 'U';
	case Keyboard::V:          return 'V';
	case Keyboard::W:          return 'W';
	case Keyboard::X:          return 'X';
	case Keyboard::Y:          return 'Y';
	case Keyboard::Z:          return 'Z';
	case Keyboard::Left:       return VK_LEFT;
	case Keyboard::Right:      return VK_RIGHT;
	case Keyboard::Up:         return VK_UP;
	case Keyboard::Down:       return VK_DOWN;
	case Keyboard::Escape:     return VK_ESCAPE;
	case Keyboard::LCTRL:      return VK_LCONTROL;
	case Keyboard::LSHIFT:     return VK_LSHIFT;
	case Keyboard::LALT:       return VK_LMENU;
	case Keyboard::LSYS:       return VK_LWIN;
	case Keyboard::RCTRL:      return VK_RCONTROL;
	case Keyboard::RSHIFT:     return VK_RSHIFT;
	case Keyboard::RALT:       return VK_RMENU;
	case Keyboard::RSYS:       return VK_RWIN;
	case Keyboard::Quote:      return VK_OEM_7;
	case Keyboard::F1:         return VK_F1;
	case Keyboard::F2:         return VK_F2;
	case Keyboard::F3:         return VK_F3;
	case Keyboard::F4:         return VK_F4;
	case Keyboard::F5:         return VK_F5;
	case Keyboard::F6:         return VK_F6;
	case Keyboard::F7:         return VK_F7;
	case Keyboard::F8:         return VK_F8;
	case Keyboard::F9:         return VK_F9;
	case Keyboard::F10:        return VK_F10;
	case Keyboard::F11:        return VK_F11;
	case Keyboard::F12:        return VK_F12;
	case Keyboard::Space:      return VK_SPACE;
	case Keyboard::DEL:        return VK_DELETE;
	case Keyboard::Return:     return VK_RETURN;
		/*case Keyboard::Num0:       return '0';
		case Keyboard::Num1:       return '1';
		case Keyboard::Num2:       return '2';
		case Keyboard::Num3:       return '3';
		case Keyboard::Num4:       return '4';
		case Keyboard::Num5:       return '5';
		case Keyboard::Num6:       return '6';
		case Keyboard::Num7:       return '7';
		case Keyboard::Num8:       return '8';
		case Keyboard::Num9:       return '9';
		case Keyboard::Menu:       return VK_APPS;
		case Keyboard::LBracket:   return VK_OEM_4;
		case Keyboard::RBracket:   return VK_OEM_6;
		case Keyboard::Semicolon:  return VK_OEM_1;
		case Keyboard::Comma:      return VK_OEM_COMMA;
		case Keyboard::Period:     return VK_OEM_PERIOD;
		case Keyboard::Slash:      return VK_OEM_2;
		case Keyboard::Backslash:  return VK_OEM_5;
		case Keyboard::Tilde:      return VK_OEM_3;
		case Keyboard::Equal:      return VK_OEM_PLUS;
		case Keyboard::Hyphen:     return VK_OEM_MINUS;
		case Keyboard::Enter:      return VK_RETURN;
		case Keyboard::Backspace:  return VK_BACK;
		case Keyboard::Tab:        return VK_TAB;
		case Keyboard::PageUp:     return VK_PRIOR;
		case Keyboard::PageDown:   return VK_NEXT;
		case Keyboard::End:        return VK_END;
		case Keyboard::Home:       return VK_HOME;
		case Keyboard::Insert:     return VK_INSERT;
		case Keyboard::Add:        return VK_ADD;
		case Keyboard::Subtract:   return VK_SUBTRACT;
		case Keyboard::Multiply:   return VK_MULTIPLY;
		case Keyboard::Divide:     return VK_DIVIDE;
		case Keyboard::Left:       return VK_LEFT;
		case Keyboard::Right:      return VK_RIGHT;
		case Keyboard::Up:         return VK_UP;
		case Keyboard::Down:       return VK_DOWN;
		case Keyboard::Numpad0:    return VK_NUMPAD0;
		case Keyboard::Numpad1:    return VK_NUMPAD1;
		case Keyboard::Numpad2:    return VK_NUMPAD2;
		case Keyboard::Numpad3:    return VK_NUMPAD3;
		case Keyboard::Numpad4:    return VK_NUMPAD4;
		case Keyboard::Numpad5:    return VK_NUMPAD5;
		case Keyboard::Numpad6:    return VK_NUMPAD6;
		case Keyboard::Numpad7:    return VK_NUMPAD7;
		case Keyboard::Numpad8:    return VK_NUMPAD8;
		case Keyboard::Numpad9:    return VK_NUMPAD9;
		case Keyboard::F13:        return VK_F13;
		case Keyboard::F14:        return VK_F14;
		case Keyboard::F15:        return VK_F15;
		case Keyboard::Pause:      return VK_PAUSE;     */
	default: return 0;
	}
	assert("Logic error.");
	return 0;
}

size_t io::map_mouse(size_t x) noexcept {
	switch (x) {
	case Mouse::Button::Left:   return VK_LBUTTON;
	case Mouse::Button::Right:  return VK_RBUTTON;
	case Mouse::Button::Middle: return VK_MBUTTON;
	default: return 0;
	}
	assert("Logic error.");
	return 0;
}

size_t io::map_controller(size_t x) noexcept {
	switch (x) {
	case Controller::Button::A:          return XINPUT_GAMEPAD_A;
	case Controller::Button::B:          return XINPUT_GAMEPAD_B;
	case Controller::Button::X:          return XINPUT_GAMEPAD_X;
	case Controller::Button::Y:          return XINPUT_GAMEPAD_Y;
	case Controller::Button::BACK:       return XINPUT_GAMEPAD_BACK;
	case Controller::Button::START:      return XINPUT_GAMEPAD_START;
	case Controller::Button::DPAD_DOWN:  return XINPUT_GAMEPAD_DPAD_DOWN;
	case Controller::Button::DPAD_LEFT:  return XINPUT_GAMEPAD_DPAD_LEFT;
	case Controller::Button::DPAD_RIGHT: return XINPUT_GAMEPAD_DPAD_RIGHT;
	case Controller::Button::DPAD_UP:    return XINPUT_GAMEPAD_DPAD_UP;
	case Controller::Button::LT:         return XINPUT_GAMEPAD_LEFT_THUMB;
	case Controller::Button::RT:         return XINPUT_GAMEPAD_RIGHT_THUMB;
	case Controller::Button::LB:         return XINPUT_GAMEPAD_LEFT_SHOULDER;
	case Controller::Button::RB:         return XINPUT_GAMEPAD_RIGHT_SHOULDER;
	default: return 0;
	}
	assert("Logic error.");
	return 0;
}

Vector2f io::get_mouse_pos() noexcept {
	POINT p;
	GetCursorPos(&p);
	ScreenToClient((HWND)platform::handle_window, &p);
	return {
		(float)(p.x),
		(float)(Environment.window_size.y - p.y)
	};
}
bool io::is_window_focused() noexcept {
	auto handle = GetActiveWindow();
	TCHAR buffer[1000];
	GetClassName(handle, buffer, 1000);
	return _tcscmp(buffer, TEXT("Roshar Class")) == 0;
}

