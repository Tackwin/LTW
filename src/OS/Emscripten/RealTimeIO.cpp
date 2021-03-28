#ifdef WEB

#include <OS/RealTimeIO.hpp>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <cassert>


extern io::Keyboard_State emscripten_keyboard_state;
extern io::Controller_State emscripten_controller_state;
extern Vector2f emscripten_mouse_pos;

Vector2f io::get_mouse_pos() noexcept {
	EmscriptenMouseEvent mouse_state = {};
	emscripten_get_mouse_status(&mouse_state);

	return {
		(float)mouse_state.canvasX,
		(float)(Environment.window_height - mouse_state.canvasY)
	};
}

io::Keyboard_State io::get_keyboard_state() noexcept {
	return emscripten_keyboard_state;
}
io::Controller_State io::get_controller_state(size_t i) noexcept {
	return emscripten_controller_state;
}
size_t io::map_key(size_t x) noexcept {
	switch(x) {
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
	case Keyboard::Escape:     return 0x1B;
	case Keyboard::LCTRL:      return 0x11;
	case Keyboard::LSHIFT:     return 0x10;
	case Keyboard::LALT:       return 0x12;
	case Keyboard::LSYS:       return 0x5B;
	case Keyboard::RCTRL:      return 0x11;
	case Keyboard::RSHIFT:     return 0x10;
	case Keyboard::RALT:       return 0x12;
	case Keyboard::RSYS:       return 0x5b;
	case Keyboard::Quote:      return 0xDE;
	case Keyboard::F1:         return 0x70;
	case Keyboard::F2:         return 0x71;
	case Keyboard::F3:         return 0x72;
	case Keyboard::F4:         return 0x73;
	case Keyboard::F5:         return 0x74;
	case Keyboard::F6:         return 0x75;
	case Keyboard::F7:         return 0x76;
	case Keyboard::F8:         return 0x77;
	case Keyboard::F9:         return 0x78;
	case Keyboard::F10:        return 0x79;
	case Keyboard::F11:        return 0x7A;
	case Keyboard::F12:        return 0x7B;
	case Keyboard::Space:      return 0x20;
	case Keyboard::DEL:        return 0x2E;
	case Keyboard::Return:     return 0x0D;
	default: return 0;
	}
	assert("logic error");
}
size_t io::map_mouse(size_t x) noexcept {
	switch (x) {
	case Mouse::Button::Left:   return io::Keyboard_State::Max_Key - 1;
	case Mouse::Button::Right:  return io::Keyboard_State::Max_Key - 2;
	case Mouse::Button::Middle: return io::Keyboard_State::Max_Key - 3;
	default: return 0;
	}
	assert("logic error");
}
size_t io::map_controller(size_t x) noexcept {
	return {};
}

bool io::is_window_focused() noexcept {
	// >TODO(Tackwin)
	return true;
}

#endif