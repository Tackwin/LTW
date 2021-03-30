
#include "Ease.hpp"

EASE_WATCH_ME;

/*
clang++ Build.cpp -o Build.exe -std=c++17
*/

void common_build_options(Build& b, Flags& flags) noexcept;

Build build_game(Flags& flags) noexcept;

Build build(Flags flags) noexcept {
	if (Env::Win32 && flags.generate_debug) {
		flags.no_default_lib = true;
	}

	auto b = Build::get_default(flags);
	b.name = "LTW";

	b.add_header("./src/");
	b.add_source_recursively("./src/");

	common_build_options(b, flags);
	auto game = build_game(flags);
	game.next = b;
//	return game;
	return b;
}

Build build_game(Flags& flags) noexcept {
	auto b = Build::get_default(flags);
	b.name = "game";
	b.target = Build::Target::Shared;
	b.flags.output = "assets/dll/";

	b.add_header("./src/");
	b.add_source_recursively("./src/");
	b.del_source_recursively("./src/Entry/");

	b.add_define("BUILD_DLL");

	common_build_options(b, flags);
	return b;
}

void common_build_options(Build& b, Flags& flags) noexcept {
	b.add_define("GLEW_STATIC");
	b.add_define("NOMINMAX");
	b.add_define("PROFILER");
	b.add_define("CONSOLE");
	if (flags.generate_debug) b.add_define("GL_DEBUG");
	b.no_warnings_win32();
	if (Env::Win32 && flags.generate_debug) {
		if (flags.release) {
			b.add_library("libucrt");
			b.add_library("libvcruntime");
			b.add_library("libcmt");
			b.add_library("libcpmt");
			b.add_library("libconcrt");
			b.add_library("Kernel32");
		} else {
			b.add_debug_defines();
			b.add_library("libucrtd");
			b.add_library("libvcruntimed");
			b.add_library("libcmtd");
			b.add_library("libcpmtd");
			b.add_library("libconcrtd");
			b.add_library("Kernel32");
		}
	}

	if (Env::Win32) {
		b.add_define("WIN32_LEAN_AND_MEAN");
		b.add_define("UNICODE");
		b.add_define("_UNICODE");

		b.add_library("Imm32");
		b.add_library("Dwmapi");
		b.add_library("Xinput");
		b.add_library("opengl32");
		b.add_library("gdi32");
		b.add_library("shell32");
		b.add_library("user32");
		b.add_library("kernel32");
		b.add_library("Comdlg32");
		b.add_library("Ole32");
		b.add_library("Uuid");
	}
}
