#pragma once

#include <filesystem>
#include <string>
#include <array>

#include <dyn_struct.hpp>
#include <chrono>

#ifdef _MSC_VER
# include <intrin.h>
#else
# include <x86intrin.h>
#endif
#include "OS/Process.hpp"

struct Scoped_Timer {

	std::string cat;
	std::string name;
	std::size_t pid;
	std::size_t tid;
	std::chrono::time_point<std::chrono::steady_clock> ts;
	struct Phase {
		static constexpr auto B = "B";
		static constexpr auto E = "E";
		static constexpr auto I = "I";
		static constexpr auto S = "S";
		static constexpr auto F = "F";
		static constexpr auto X = "X";
		static constexpr auto Args = "Args";
	};
	const char* ph;

	Scoped_Timer(std::string cat, std::string name) noexcept;
	~Scoped_Timer() noexcept;
};

struct Tracer {
private:
	Tracer() noexcept;

	dyn_struct current_session;
	std::string current_session_name;
	bool session_running = false;
	std::vector<Scoped_Timer*> timers;

public:
	static Tracer& get() noexcept { static Tracer t; return t; };

	void begin_session(std::string name) noexcept;
	void end_session(std::filesystem::path path) noexcept;

	void push(dyn_struct str) noexcept;

	void begin(std::string name) noexcept;
	void end() noexcept;
};

struct Scoped_Session {
	std::filesystem::path path;

	Scoped_Session(std::string name, std::filesystem::path path) noexcept : path(path) {
		Tracer::get().begin_session(name);
	}

	~Scoped_Session() noexcept {
		Tracer::get().end_session(path);
	}
};


#ifdef PROFILER
#define PROFILER_SCOPE_SESSION(n, p) Scoped_Session scoped_session##__COUNTER__ (n, p);
#define PROFILER_SESSION_BEGIN(n) Tracer::get().begin_session(n);
#define PROFILER_SESSION_END(n) Tracer::get().end_session(n);
#define PROFILER_FUNCTION() Scoped_Timer scoped_timer##__COUNTER__ ("function", __FUNCSIG__);
#define PROFILER_SCOPE(n) Scoped_Timer scoped_timer##__COUNTER__ ("scope", n);
#define PROFILER_BEGIN(n) Tracer::get().begin(n);
#define PROFILER_END() Tracer::get().end();
#define PROFILER_BEGIN_SEQ(n) Tracer::get().begin(n);
#define PROFILER_SEQ(n) Tracer::get().end(); Tracer::get().begin(n);
#define PROFILER_END_SEQ() Tracer::get().end();
#else
#define PROFILER_SCOPE_SESSION(x, y)
#define PROFILER_SESSION_BEGIN(x)
#define PROFILER_SESSION_END(x)
#define PROFILER_FUNCTION(x)
#define PROFILER_SCOPE(x)
#define PROFILER_BEGIN(x)
#define PROFILER_END()
#define PROFILER_BEGIN_SEQ(n)
#define PROFILER_SEQ(n)
#define PROFILER_END_SEQ()
#endif

struct Sample {
	const char* function_name = nullptr;
	size_t thread_id   = 0;
	std::uint64_t cycle_start = 0;
	std::uint64_t cycle_end   = 0;
};

struct Sample_Log {
	static constexpr size_t MAX_SAMPLE = 4096;
	std::array<Sample, MAX_SAMPLE> samples;
	std::atomic<size_t> sample_count = 0;
};
extern Sample_Log frame_sample_log;

struct Timed_Block {
	Sample s;

	Timed_Block(const char* function_name) noexcept {
		s.function_name = function_name;
		s.cycle_start = __rdtsc();
		s.thread_id = get_thread_id();
	}

	~Timed_Block() noexcept {
		s.cycle_end = __rdtsc();

		frame_sample_log.samples[frame_sample_log.sample_count++] = s;
	}
};

#define TIMED_FUNCTION() Timed_Block scoped_timed_bloc_##__COUNTER__(__FUNCSIG__);