#include "Math/Vector.hpp"

struct Environment_t {
	Vector2u windows_size;
};

namespace platform {
	extern void* handle_window;
	extern void* handle_dc_window;
	extern void* main_opengl_context;
	extern void* asset_opengl_context;
}

extern float wheel_scroll;
extern Environment_t Environment;