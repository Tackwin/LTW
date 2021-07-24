#include <stdio.h>

#include <GLES3/gl3.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <string>
#include "xstd.hpp"
#include "Profiler/Tracer.hpp"

bool init_gl_context() noexcept;

int main() {
	PROFILER_SESSION_BEGIN("Startup");
	defer { PROFILER_SESSION_END("output/trace/"); };

	init_gl_context();
	printf("Hello world\n");
	return 0;
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