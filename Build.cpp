
#include "Ease.hpp"

EASE_WATCH_ME;

/*
clang++ Build.cpp -o Build.exe -std=c++17
*/

enum Target {
	Windows,
	Emscripten
} target_to = Windows;

void common_build_options(Build& b, Flags& flags) noexcept;

Build build_game(Flags& flags) noexcept;
Build build_emscripten(Flags& flags) noexcept;

Build build(Flags flags) noexcept {
	// return build_emscripten(flags);

	if (Env::Win32 && flags.generate_debug) {
		flags.no_default_lib = true;
	}

	auto b = Build::get_default(flags);
	b.name = "LTW";

	b.add_header("./src/");
	b.add_source_recursively("./src/");
	b.del_source_recursively("./src/Entry/");
	b.del_source_recursively("./src/OS/Emscripten");
	b.add_source("./src/Entry/win32_main.cpp");

	common_build_options(b, flags);

	return b;
}

Build build_emscripten(Flags& flags) noexcept {
	if (Env::Win32 && flags.generate_debug) {
		flags.no_default_lib = false;
	}

	auto b = Build::get_default(flags);
	b.compiler = "emcc";
	b.name = "LTW_em";

	b.add_header("./src/");
	b.add_source_recursively("./src/");
	b.del_source_recursively("./src/Entry/");
	b.del_source_recursively("./src/OS/Windows");
	b.del_source_recursively("./src/imgui");
	b.del_source("./src/Inspector.cpp");
	b.del_source("./src/GL/gl3w.cpp");
	b.add_source("./src/Entry/em_main.cpp");

	b.add_define("WEB");
	b.add_define("IMGUI_IMPL_OPENGL_LOADER_GL3W");
	b.add_define("ES");
	b.add_define("NOMINMAX");
	b.add_define("PROFILER");
	b.add_define("CONSOLE");
	if (flags.generate_debug) b.add_define("GL_DEBUG");

	b.add_link_flag("-s TOTAL_MEMORY=32768000");
	b.add_link_flag("-s ASSERTIONS=2");
	b.add_link_flag("-s TOTAL_STACK=32MB");
	b.add_link_flag("-s TOTAL_MEMORY=128MB");
	b.add_link_flag("-s SAFE_HEAP=1");
	b.add_link_flag("-s USE_WEBGL2=1");
	b.add_link_flag("-s DEMANGLE_SUPPORT=1");
	b.add_link_flag("-s SAFE_HEAP=1");
	b.add_link_flag("-s WASM=1");
	b.add_link_flag("-s FULL_ES3=1");
	b.add_link_flag("-s FULL_ES2=1");
	b.add_link_flag("-s GL_ASSERTIONS=2");
	b.add_link_flag("-s ERROR_ON_UNDEFINED_SYMBOLS=0");
	b.add_link_flag("--embed-file ./assets/");
	b.add_link_flag("--emrun");

	if (flags.run_after_compilation) {
		Commands c;
		c.add_command("emrun --port 2356 ./LTW.html", "Run", "LTW.html", "LTW.html");
		b.post_link.push_back(c);
	}
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

	return b;
}

void common_build_options(Build& b, Flags& flags) noexcept {
	b.add_define("IMGUI_IMPL_OPENGL_LOADER_GL3W");
	b.add_define("NOMINMAX");
	b.add_define("PROFILER");
	b.add_define("CONSOLE");
	if (flags.generate_debug) b.add_define("GL_DEBUG");
	b.no_warnings_win32();
	if (target_to == Target::Windows && flags.generate_debug) {
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
    
	if (target_to == Target::Windows) {
		b.add_define("WIN32_LEAN_AND_MEAN");
		b.add_define("UNICODE");
		b.add_define("_UNICODE");
        
		b.add_library("Psapi");
		b.add_library("Dbghelp");
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
		b.add_library("Winmm");
	} else if (target_to == Target::Emscripten) {

	}
}
