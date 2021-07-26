#include <stdio.h>

#include <GLES3/gl3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <string>
#include "xstd.hpp"
#include "Profiler/Tracer.hpp"
#include "global.hpp"
#include "Game.hpp"
#include "OS/RealTimeIO.hpp"


io::Keyboard_State emscripten_keyboard_state{};
io::Controller_State emscripten_controller_state{};
Vector2f emscripten_mouse_pos{};
bool init_gl_context() noexcept;
void setup_controls_callback() noexcept;

Game game;

struct Game_Proc {
	decltype(&game_startup)  startup  = game_startup;
	decltype(&game_shutdown) shutdown = game_shutdown;
	decltype(&game_update)   update   = game_update;
	decltype(&game_render)   render   = game_render;
} game_proc;

void loop() noexcept;

int main() {
	PROFILER_SESSION_BEGIN("Startup");
	defer { PROFILER_SESSION_END("output/trace/"); };

	init_gl_context();

	game_proc.startup(game);
	emscripten_set_main_loop(loop, 0, 1);
	game_proc.shutdown(game);
	return 0;
}

void loop() noexcept {
	thread_local render::Render_Param render_param;
	thread_local audio::Orders sound_orders;
	thread_local render::Orders render_orders;
	render_orders.reserve(10'000);

	game_proc.update(game, sound_orders, 0.016f);
	game_proc.render(game, render_orders);
}

bool init_gl_context() noexcept {
	EmscriptenWebGLContextAttributes attribs;
	emscripten_webgl_init_context_attributes(&attribs);

	attribs.majorVersion = 2;
	attribs.minorVersion = 0;

	auto context = emscripten_webgl_create_context("canvas", &attribs);
	if (context <= 0) return false;
	auto result = emscripten_webgl_make_context_current(context);
	if (result != EMSCRIPTEN_RESULT_SUCCESS) return false;
	return result == EMSCRIPTEN_RESULT_SUCCESS;
}

void setup_controls_callback() noexcept {
	auto mouse = [](int event_type, const EmscriptenMouseEvent* event, void*) -> int {
		emscripten_keyboard_state.keys[io::Keyboard_State::Max_Key - 1] = event->buttons & 1;
		emscripten_keyboard_state.keys[io::Keyboard_State::Max_Key - 2] = event->buttons & 2;
		emscripten_keyboard_state.keys[io::Keyboard_State::Max_Key - 3] = event->buttons & 4;
		return 1;
	};
	auto wheel = [](int event_type, const EmscriptenWheelEvent* event, void*) -> int {
		if (event->deltaY > 0)
			wheel_scroll = -1.f / 5.f;
		else if (event->deltaY < 0)
			wheel_scroll = 1.f / 5.f;
		return 1;
	};
	auto key = [](int event_type, const EmscriptenKeyboardEvent* event, void*) -> int {
		switch (event_type) {
		case EMSCRIPTEN_EVENT_KEYDOWN: {
			emscripten_keyboard_state.keys[event->which] = 1;
			break;
		}
		case EMSCRIPTEN_EVENT_KEYUP: {
			emscripten_keyboard_state.keys[event->which] = 0;
			break;
		}
		}
		return 1;
	};

	emscripten_set_mousemove_callback(nullptr, nullptr, false, mouse);
	emscripten_set_mousedown_callback(nullptr, nullptr, false, mouse);
	emscripten_set_mouseup_callback(nullptr, nullptr, false, mouse);

	emscripten_set_wheel_callback("canvas", nullptr, false, wheel);
	emscripten_set_wheel_callback("canvas", nullptr, false, wheel);

	//emscripten_set_keypress_callback(nullptr, nullptr, false, &FunImGui::keyboardCallback);
	emscripten_set_keydown_callback(nullptr, nullptr, false, key);
	emscripten_set_keyup_callback(nullptr, nullptr, false, key);
}