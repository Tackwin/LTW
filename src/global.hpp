#include "Math/Vector.hpp"
#include <shared_mutex>

struct Environment_t {
	Vector2u window_size;
	Vector2f viewport_size;
	Vector2u buffer_size = { 1920, 1080 };
};

namespace platform {
	extern void* handle_window;
	extern void* handle_dc_window;
	extern void* main_opengl_context;
	extern void* asset_opengl_context;
}

extern float wheel_scroll;
extern Environment_t Environment;
extern std::mutex Main_Mutex;