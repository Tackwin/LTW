#include <stdio.h>
#include <Windows.h>
#include <optional>
#include <string>

#include "GL/glew.h"
#include "GL/wglew.h"

#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_win32.h"

#include "global.hpp"

Environment_t Environment;
float wheel_scroll = 0;

void* platform::handle_window = nullptr;
void* platform::handle_dc_window = nullptr;
void* platform::main_opengl_context = nullptr;
void* platform::asset_opengl_context = nullptr;

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

LRESULT WINAPI window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);

	switch (msg) {
	case WM_KEYDOWN: {
		if (wParam == VK_F11) toggle_fullscren(hWnd);
		break;
	}
	case WM_SIZE: {
		Environment.windows_size.x = (size_t)((int)LOWORD(lParam));
		Environment.windows_size.y = (size_t)((int)HIWORD(lParam));
		break;
	}
	case WM_SIZING: {
		RECT* rect = (RECT*)lParam;
		Environment.windows_size.x = rect->right - rect->left;
		Environment.windows_size.y = rect->bottom - rect->top;
		break;
	}
	case WM_CHAR:
		// post_char((std::uint32_t)(wchar_t)wParam);
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


#ifdef CONSOLE
int main(int, char**
#else
int WINAPI WinMain(
	HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd
#endif
) {
	printf("Hello world\n");
	return 0;
}