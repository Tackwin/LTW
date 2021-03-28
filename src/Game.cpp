#include "Game.hpp"

#include <stdio.h>

#include "Profiler/Tracer.hpp"
#include "global.hpp"

Environment_t Environment;
float wheel_scroll = 0;
std::mutex Main_Mutex;

void* platform::handle_window = nullptr;
void* platform::handle_dc_window = nullptr;
void* platform::main_opengl_context = nullptr;
void* platform::asset_opengl_context = nullptr;

void Game::startup() noexcept {
	PROFILER_BEGIN_SEQ("monitor");
	PROFILER_END_SEQ();
}

void Game::shutdown() noexcept {
	PROFILER_BEGIN_SEQ("shutup");
	PROFILER_END_SEQ();
}

void Game::post_char(size_t arg) noexcept {
	if (arg > 0xFF) {
		fprintf(stderr, "Fuck it just received a non ascii char not gonna handle it.\n");
		return;
	}
}

void Game::update(double) noexcept {}
void Game::render(render::Orders&) noexcept {}