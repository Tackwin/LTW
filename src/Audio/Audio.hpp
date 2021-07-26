#pragma once
#include "miniaudio.h"

#include <atomic>
#include <string>

#include "xstd.hpp"
#include "std/vector.hpp"

extern void sound_callback(
	ma_device* device, void* output, const void* input, ma_uint32 frame_count
);

namespace audio {
	struct Sound {
		size_t id = 0;
		size_t asset_id = 0;
		float volume = 1;
		size_t current_frame = 0;

		bool to_remove = false;
	};

	struct Orders {
		bool healthy = false;
		ma_device device;

		xstd::Pool<Sound> playing_sounds;

		Sound circular_sound_buffer[100];
		std::atomic<size_t> consume_idx = 0;
		std::atomic<size_t> produce_idx = 0;


		void init() noexcept;
		void uninit() noexcept;

		void callback(void* output, const void* input, ma_uint32 frame_count);

		void add_sound(Sound s) noexcept;
	};
};