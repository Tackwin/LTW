#include "OS/Process.hpp"
#include "Windows.h"

size_t get_process_id() noexcept {
	return (size_t)GetCurrentProcessId();
}

size_t get_thread_id() noexcept {
	return (size_t)GetCurrentThreadId();
}
