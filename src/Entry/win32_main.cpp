#include <stdio.h>
#include <Windows.h>
#include <optional>
#include <string>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "Profiler/Tracer.hpp"
#include "xstd.hpp"

#include "GL/glew.h"
#include "GL/wglew.h"

#include "global.hpp"

#include "OS/RealTimeIO.hpp"

#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"

#include "Graphic/FrameBuffer.hpp"

#include "Game.hpp"

IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(
	HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam
);

void APIENTRY opengl_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void*
) noexcept;
std::optional<std::string> get_last_error_message() noexcept;
std::optional<HGLRC> create_gl_context(HWND handle_window) noexcept;
void destroy_gl_context(HGLRC gl_context) noexcept;

void sound_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
void windows_loop() noexcept;

void toggle_fullscren(HWND hwnd) {
	static WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

	DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	if (dwStyle & WS_OVERLAPPEDWINDOW) {
		MONITORINFO mi = { sizeof(mi) };
		if (
			GetWindowPlacement(hwnd, &g_wpPrev) &&
			GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi)
		) {
			SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(
				hwnd, HWND_TOP,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED
			);
		}
	} else {
		SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(hwnd, &g_wpPrev);
		SetWindowPos(
			hwnd,
			NULL,
			0,
			0,
			0,
			0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
		);
	}
}

std::vector<size_t> posted_char;
LRESULT WINAPI window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_PAINT: windows_loop(); break;
	case WM_KEYDOWN: {
		if (wParam == VK_F11) toggle_fullscren(hWnd);
		break;
	}
	case WM_SIZE: {
		Environment.window_size.x = (size_t)((int)LOWORD(lParam));
		Environment.window_size.y = (size_t)((int)HIWORD(lParam));
		break;
	}
	case WM_SIZING: {
		RECT* rect = (RECT*)lParam;
		Environment.window_size.x = rect->right - rect->left;
		Environment.window_size.y = rect->bottom - rect->top;
		break;
	}
	case WM_CHAR:
		posted_char.push_back(wParam);
		break;
	case WM_MOUSEWHEEL:
		wheel_scroll = GET_WHEEL_DELTA_WPARAM(wParam) / 120.f;
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

Game game;
render::Orders orders;

namespace sound {
	struct Orders {
		bool healthy = false;
		ma_device device;

		std::vector<size_t> playing_sound;

		void init() noexcept {
			ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
			device_config.playback.format = ma_format_s16;
			device_config.playback.channels = 1;
			device_config.sampleRate = 44100;
			device_config.dataCallback = sound_callback;
			device_config.pUserData = this;

			if (ma_device_init(nullptr, &device_config, &device) != MA_SUCCESS) {
				printf("Failed to load sound :'(\n");
				return;
			}

			if (ma_device_start(&device) != MA_SUCCESS) {
				printf("Failed to load sound :'(\n");
				ma_device_uninit(&device);
				return;
			}
			healthy = true;
		}

		void uninit() {
			ma_device_uninit(&device);
		}

		void callback(void* output, const void* input, ma_uint32 frame_count) {
			return;
			
			//if (playing_sound.empty()) return;
			if (!asset::Store.ready) return;

			auto& decoder = asset::Store.get_sound(asset::Sound_Id::Ui_Action);
			ma_decoder_read_pcm_frames(&decoder, output, frame_count);
		}
	};
};
sound::Orders sound_orders;


struct Game_Proc {
	decltype(&game_startup)  startup  = game_startup;
	decltype(&game_shutdown) shutdown = game_shutdown;
	decltype(&game_update)   update   = game_update;
	decltype(&game_render)   render   = game_render;
} game_proc;

#ifdef CONSOLE
int main(int, char**
#else
int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd
#endif
) {
	game = {};

	PROFILER_SESSION_BEGIN("Startup");
	defer { PROFILER_SESSION_END("output/trace/"); };

	PROFILER_BEGIN("Windows");
	auto Class_Name = TEXT("Roshar Class");
	auto Window_Title = TEXT("Roshar");

	// Create application window
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_CLASSDC | CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		window_proc,
		0L,
		0L,
		GetModuleHandle(nullptr),
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		Class_Name,
		nullptr
	};

	RegisterClassEx(&wc);
	defer{ UnregisterClass(Class_Name, wc.hInstance); };

	RECT desired_size;
	desired_size.left = 0;
	desired_size.top = 0;
	desired_size.right = 1600;
	desired_size.bottom = 900;
	AdjustWindowRect(&desired_size, WS_OVERLAPPEDWINDOW, false);

	HWND window_handle = CreateWindow(
		Class_Name,
		Window_Title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		desired_size.right - desired_size.left,
		desired_size.bottom - desired_size.top,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	platform::handle_window = window_handle;

	RECT window_rect;
	if (!GetClientRect(window_handle, &window_rect)) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return 1;
	}
	Environment.window_size.x = (size_t)(window_rect.right - window_rect.left);
	Environment.window_size.y = (size_t)(window_rect.bottom - window_rect.top);

	PROFILER_END();

	PROFILER_BEGIN("OpenGL");
	auto gl_context = *create_gl_context(window_handle);
	defer{ destroy_gl_context(gl_context); };

#ifndef NDEBUG
	GLint flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glDebugMessageCallback((GLDEBUGPROCARB)opengl_debug, nullptr);
	}
#endif

	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	PROFILER_END();

	GLint x;
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &x);
	printf("Opengl ");
	printf("%s\n", (char*)glGetString(GL_VERSION));
	printf("glGetIntegerv(GL_MAX_VERTEX_ATTRIBS) = %d\n", x);


	PROFILER_BEGIN("Imgui");
	// Setup Dear ImGui context
	//IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	const char* glsl_version = "#version 130";
	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(window_handle);
	ImGui_ImplOpenGL3_Init(glsl_version);
	PROFILER_END();

	auto dc_window = GetDC(window_handle);
	if (!dc_window) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return 1;
	}

	platform::handle_dc_window = dc_window;
	orders.reserve(1000);

	PROFILER_BEGIN("Sound");
	sound_orders.init();
	defer { if (sound_orders.healthy) sound_orders.uninit(); };

	PROFILER_BEGIN("Game");
	game_proc.startup(game);
	PROFILER_END();

	wglSwapIntervalEXT(0);

	MSG msg{};

	PROFILER_BEGIN("ShowWindow");
	ShowWindow(window_handle, SW_SHOWDEFAULT);
	PROFILER_END();
	PROFILER_SESSION_END("output/trace/");

	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		windows_loop();
	}

	PROFILER_SESSION_BEGIN("Shutup");
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	game_proc.shutdown(game);
	// >SEE(Tackwin) >ClipCursor
	ClipCursor(nullptr);
	return 0;
}

void windows_loop() noexcept {
	static render::Render_Param render_param;
	thread_local auto last_time_frame = xstd::seconds();
	auto dt = xstd::seconds() - last_time_frame;
	last_time_frame = xstd::seconds();

	if (dt > 1 / 55.0) {
		printf("Lag Spike :( %10.5lf ms\n", 1000 * dt);
	}

	// >TODO(Tackwin): handle multiple char by frame.
	for (auto& x : posted_char) current_char = x;
	posted_char.clear();
	
	Rectanglef viewport_rect{
		0.f, 0.f, (float)Environment.window_size.x, (float)Environment.window_size.y
	};
	viewport_rect = viewport_rect.fitDownRatio({16, 9});
	Environment.viewport_size = viewport_rect.size;

	Main_Mutex.lock();
	defer{ Main_Mutex.unlock(); };

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	thread_local bool render_window_open = true;
	if (render_window_open) {
		ImGui::Begin("Render", &render_window_open);
		ImGui::SliderFloat("Gamma   ", &render_param.gamma    , 0, 2);
		ImGui::SliderFloat("Exposure", &render_param.exposure , 0, 2);
		ImGui::SliderSize ("Samples ", &render_param.n_samples, 1, 16);
		ImGui::SliderFloat("SSAO R  ", &render_param.ssao_radius, 0, 1);
		ImGui::SliderFloat("SSAo bias ", &render_param.ssao_bias, 0, 1);
		ImGui::SliderFloat("Motion scale", &render_param.motion_scale, 0, 1);
		ImGui::SliderSize("Target FPS", &render_param.target_fps, 60, 240);
		ImGui::End();
	}

	render_param.cam_pos = game.camera3d.pos;
	render_param.current_fps = (size_t)(1.0 / dt);
	auto response = game_proc.update(game, dt);
	game_proc.render(game, orders);

	render::render_orders(orders, render_param);
	orders.clear();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SwapBuffers((HDC)platform::handle_dc_window);

	if (response.confine_cursor) {
		RECT r;
		GetWindowRect((HWND)platform::handle_window, &r);
		ClipCursor(&r);
	} else {
		// >ClipCursor >TODO(Tackwin): If a user had a clip cursor before us we are destroying it
		// we need to restore the old clipCursor by doing GetClipCursor...
		ClipCursor(nullptr);
	}
}


std::optional<std::string> get_last_error_message() noexcept {
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::nullopt; //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	auto flags = FORMAT_MESSAGE_ALLOCATE_BUFFER;
	flags |= FORMAT_MESSAGE_FROM_SYSTEM;
	flags |= FORMAT_MESSAGE_IGNORE_INSERTS;
	size_t size = FormatMessageA(
		flags, nullptr, errorMessageID, 0, (LPSTR)& messageBuffer, 0, nullptr
	);

	std::string message(messageBuffer, size);

	LocalFree(messageBuffer);
	return message;
}


std::optional<HGLRC> create_gl_context(HWND handle_window) noexcept {
	static int attribs[] = {
	#ifndef NDEBUG
		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
	#endif
		0
	};

	PROFILER_BEGIN_SEQ("DC");
	auto dc = GetDC(handle_window);
	if (!dc) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	defer{ ReleaseDC(handle_window, dc); };

	PROFILER_SEQ("Pixel");
	PIXELFORMATDESCRIPTOR pixel_format{};
	pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixel_format.nVersion = 1;
	pixel_format.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pixel_format.cColorBits = 32;
	pixel_format.cAlphaBits = 8;
	pixel_format.iLayerType = PFD_MAIN_PLANE;

	PROFILER_BEGIN_SEQ("choose");
	auto suggested_pixel_format = ChoosePixelFormat(dc, &pixel_format);
	if (!suggested_pixel_format) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	PROFILER_SEQ("describe");
	auto result = DescribePixelFormat(
		dc, suggested_pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format
	);
	if (!result) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	PROFILER_SEQ("set");
	if (!SetPixelFormat(dc, suggested_pixel_format, &pixel_format)) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}
	PROFILER_END_SEQ();

	PROFILER_SEQ("first context");
	auto gl_context = wglCreateContext(dc);
	if (!gl_context) {
		// >TODO error handling
		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	if (!wglMakeCurrent(dc, gl_context)) {
		wglDeleteContext(gl_context);

		printf("%s", get_last_error_message()->c_str());
		return std::nullopt;
	}

	PROFILER_SEQ("glew");
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		printf("Can't init glew\n");
		return gl_context;
	}

	PROFILER_SEQ("second context");
	auto gl = wglCreateContextAttribsARB(dc, nullptr, attribs);
	if (!gl) {
		auto err = glGetError();
		printf("%s", std::to_string(err).c_str());

		return gl_context;
	}
	platform::main_opengl_context = gl;

	if (!wglMakeCurrent(dc, gl)) {
		wglDeleteContext(gl);
		return gl_context;
	}


	PROFILER_SEQ("destroy");
	wglDeleteContext(gl_context);
	PROFILER_END_SEQ();
	return (HGLRC)platform::main_opengl_context;
}
void destroy_gl_context(HGLRC gl_context) noexcept {
	// >TODO error handling
	if (!wglDeleteContext(gl_context)) {
		printf("%s", get_last_error_message()->c_str());
	}
}
const char* debug_source_str(GLenum source) {
	static const char* sources[] = {
	  "API",   "Window System", "Shader Compiler", "Third Party", "Application",
	  "Other", "Unknown"
	};
	auto str_idx = std::min(
		(size_t)(source - GL_DEBUG_SOURCE_API),
		(size_t)(sizeof(sources) / sizeof(const char*) - 1)
	);
	return sources[str_idx];
}

const char* debug_type_str(GLenum type) {
	static const char* types[] = {
	  "Error",       "Deprecated Behavior", "Undefined Behavior", "Portability",
	  "Performance", "Other",               "Unknown"
	};

	auto str_idx = std::min(
		(size_t)(type - GL_DEBUG_TYPE_ERROR),
		(size_t)(sizeof(types) / sizeof(const char*) - 1)
	);
	return types[str_idx];
}

const char* debug_severity_str(GLenum severity) {
	static const char* severities[] = {
	  "High", "Medium", "Low", "Unknown"
	};

	auto str_idx = std::min(
		(size_t)(severity - GL_DEBUG_SEVERITY_HIGH),
		(size_t)(sizeof(severities) / sizeof(const char*) - 1)
	);
	return severities[str_idx];
}

void APIENTRY opengl_debug(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei,
	const GLchar* message,
	const void*
) noexcept {
	constexpr GLenum To_Ignore[] = {
		131185,
		131204,
		131140
	};

	constexpr GLenum To_Break_On[] = {
		1280, 1281, 1282, 1286, 131076
	};

	if (std::find(BEG_END(To_Ignore), id) != std::end(To_Ignore)) return;

	printf("OpenGL:\n");
	printf("=============\n");
	printf(" Object ID: ");
	printf("%s\n", std::to_string(id).c_str());
	printf(" Severity:  ");
	printf("%s\n", debug_severity_str(severity));
	printf(" Type:      ");
	printf("%s\n", debug_type_str(type));
	printf("Source:    ");
	printf("%s\n", debug_source_str(source));
	printf(" Message:   ");
	printf("%s\n", message);
	printf("\n");

	if (std::find(BEG_END(To_Break_On), id) != std::end(To_Break_On)) {
		DebugBreak();
	}
}

void sound_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
	auto sound_orders = (sound::Orders*)device->pUserData;
	if (sound_orders) sound_orders->callback(output, input, frame_count);
}
