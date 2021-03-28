#include "InputsManager.hpp"

#include "OS/file.hpp"
#include "OS/RealTimeIO.hpp"
#include "xstd.hpp"

std::string to_string(Mouse::Button b) noexcept {
	switch (b) {
	case Mouse::Left:
		return "Left";
	case Mouse::Right:
		return "Right";
	case Mouse::Middle:
		return "Middle";
	default:
		return "Unknown";
	}
}
std::string to_string(Keyboard::Key k) noexcept {
	switch (k) {
		case Keyboard::Key::A : {
			return "A";
		}
		case Keyboard::Key::B : {
			return "B";
		}
		case Keyboard::Key::C : {
			return "C";
		}
		case Keyboard::Key::D : {
			return "D";
		}
		case Keyboard::Key::E : {
			return "E";
		}
		case Keyboard::Key::F : {
			return "F";
		}
		case Keyboard::Key::G : {
			return "G";
		}
		case Keyboard::Key::H : {
			return "H";
		}
		case Keyboard::Key::I : {
			return "I";
		}
		case Keyboard::Key::J : {
			return "J";
		}
		case Keyboard::Key::K : {
			return "K";
		}
		case Keyboard::Key::L : {
			return "L";
		}
		case Keyboard::Key::M : {
			return "M";
		}
		case Keyboard::Key::N : {
			return "N";
		}
		case Keyboard::Key::O : {
			return "O";
		}
		case Keyboard::Key::P : {
			return "P";
		}
		case Keyboard::Key::Q : {
			return "Q";
		}
		case Keyboard::Key::R : {
			return "R";
		}
		case Keyboard::Key::S : {
			return "S";
		}
		case Keyboard::Key::T : {
			return "T";
		}
		case Keyboard::Key::U : {
			return "U";
		}
		case Keyboard::Key::V : {
			return "V";
		}
		case Keyboard::Key::W : {
			return "W";
		}
		case Keyboard::Key::X : {
			return "X";
		}
		case Keyboard::Key::Y : {
			return "Y";
		}
		case Keyboard::Key::Z : {
			return "Z";
		}
		case Keyboard::Key::Num0 : {
			return "Num0";
		}
		case Keyboard::Key::Num1 : {
			return "Num1";
		}
		case Keyboard::Key::Num2 : {
			return "Num2";
		}
		case Keyboard::Key::Num3 : {
			return "Num3";
		}
		case Keyboard::Key::Num4 : {
			return "Num4";
		}
		case Keyboard::Key::Num5 : {
			return "Num5";
		}
		case Keyboard::Key::Num6 : {
			return "Num6";
		}
		case Keyboard::Key::Num7 : {
			return "Num7";
		}
		case Keyboard::Key::Num8 : {
			return "Num8";
		}
		case Keyboard::Key::Num9 : {
			return "Num9";
		}
		case Keyboard::Key::Escape : {
			return "Escape";
		}
		case Keyboard::Key::LCTRL : {
			return "LCTRL";
		}
		case Keyboard::Key::LSHIFT : {
			return "LSHIFT";
		}
		case Keyboard::Key::LALT : {
			return "LALT";
		}
		case Keyboard::Key::LSYS : {
			return "LSYS";
		}
		case Keyboard::Key::RCTRL : {
			return "RCTRL";
		}
		case Keyboard::Key::RSHIFT : {
			return "RSHIFT";
		}
		case Keyboard::Key::RALT : {
			return "RALT";
		}
		case Keyboard::Key::RSYS : {
			return "RSYS";
		}
		case Keyboard::Key::Menu : {
			return "Menu";
		}
		case Keyboard::Key::LBracket : {
			return "LBracket";
		}
		case Keyboard::Key::RBracket : {
			return "RBracket";
		}
		case Keyboard::Key::Semicolon : {
			return "Semicolon";
		}
		case Keyboard::Key::Comma : {
			return "Comma";
		}
		case Keyboard::Key::Period : {
			return "Period";
		}
		case Keyboard::Key::Quote : {
			return "Quote";
		}
		case Keyboard::Key::Slash : {
			return "Slash";
		}
		case Keyboard::Key::Backslash : {
			return "Backslash";
		}
		case Keyboard::Key::Tilde : {
			return "Tilde";
		}
		case Keyboard::Key::Equal : {
			return "Equal";
		}
		case Keyboard::Key::Hyphen : {
			return "Hyphen";
		}
		case Keyboard::Key::Space : {
			return "Space";
		}
		case Keyboard::Key::Return : {
			return "Return";
		}
		case Keyboard::Key::Backspace : {
			return "Backspace";
		}
		case Keyboard::Key::Tab : {
			return "Tab";
		}
		case Keyboard::Key::PageUp : {
			return "PageUp";
		}
		case Keyboard::Key::PageDown : {
			return "PageDown";
		}
		case Keyboard::Key::End : {
			return "End";
		}
		case Keyboard::Key::Home : {
			return "Home";
		}
		case Keyboard::Key::Insert : {
			return "Insert";
		}
		case Keyboard::Key::DEL : {
			return "DEL";
		}
		case Keyboard::Key::Add : {
			return "Add";
		}
		case Keyboard::Key::Subtract : {
			return "Subtract";
		}
		case Keyboard::Key::Multiply : {
			return "Multiply";
		}
		case Keyboard::Key::Divide : {
			return "Divide";
		}
		case Keyboard::Key::Left : {
			return "Left";
		}
		case Keyboard::Key::Right : {
			return "Right";
		}
		case Keyboard::Key::Up : {
			return "Up";
		}
		case Keyboard::Key::Down : {
			return "Down";
		}
		case Keyboard::Key::Numpad0 : {
			return "Numpad0";
		}
		case Keyboard::Key::Numpad1 : {
			return "Numpad1";
		}
		case Keyboard::Key::Numpad2 : {
			return "Numpad2";
		}
		case Keyboard::Key::Numpad3 : {
			return "Numpad3";
		}
		case Keyboard::Key::Numpad4 : {
			return "Numpad4";
		}
		case Keyboard::Key::Numpad5 : {
			return "Numpad5";
		}
		case Keyboard::Key::Numpad6 : {
			return "Numpad6";
		}
		case Keyboard::Key::Numpad7 : {
			return "Numpad7";
		}
		case Keyboard::Key::Numpad8 : {
			return "Numpad8";
		}
		case Keyboard::Key::Numpad9 : {
			return "Numpad9";
		}
		case Keyboard::Key::F1 : {
			return "F1";
		}
		case Keyboard::Key::F2 : {
			return "F2";
		}
		case Keyboard::Key::F3 : {
			return "F3";
		}
		case Keyboard::Key::F4 : {
			return "F4";
		}
		case Keyboard::Key::F5 : {
			return "F5";
		}
		case Keyboard::Key::F6 : {
			return "F6";
		}
		case Keyboard::Key::F7 : {
			return "F7";
		}
		case Keyboard::Key::F8 : {
			return "F8";
		}
		case Keyboard::Key::F9 : {
			return "F9";
		}
		case Keyboard::Key::F10 : {
			return "F10";
		}
		case Keyboard::Key::F11 : {
			return "F11";
		}
		case Keyboard::Key::F12 : {
			return "F12";
		}
		case Keyboard::Key::F13 : {
			return "F13";
		}
		case Keyboard::Key::F14 : {
			return "F14";
		}
		case Keyboard::Key::F15 : {
			return "F15";
		}
		case Keyboard::Key::Pause : {
			return "Pause";
		}
		default: {
			return "Unknown";
		}
	}
}

std::string to_string(Joystick::Button b) noexcept {
	switch (b) {
		case Joystick::Button::A : {
			return "A";
		}
		case Joystick::Button::B : {
			return "B";
		}
		case Joystick::Button::X : {
			return "X";
		}
		case Joystick::Button::Y : {
			return "Y";
		}
		case Joystick::Button::DPAD_UP : {
			return "DPAD_UP";
		}
		case Joystick::Button::DPAD_DOWN : {
			return "DPAD_DOWN";
		}
		case Joystick::Button::DPAD_LEFT : {
			return "DPAD_LEFT";
		}
		case Joystick::Button::DPAD_RIGHT : {
			return "DPAD_RIGHT";
		}
		case Joystick::Button::START : {
			return "START";
		}
		case Joystick::Button::BACK : {
			return "BACK";
		}
		case Joystick::Button::LB : {
			return "LB";
		}
		case Joystick::Button::RB : {
			return "RB";
		}
		case Joystick::Button::LT : {
			return "LT";
		}
		case Joystick::Button::RT : {
			return "RT";
		}
		default:
			return "Unknown";
			break;
	}
}
