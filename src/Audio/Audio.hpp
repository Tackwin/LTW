#pragma once
#include "miniaudio.h"

#include "std/vector.hpp"
#include "Managers/AssetsManager.hpp"

extern void sound_callback(
	ma_device* device, void* output, const void* input, ma_uint32 frame_count
);

namespace sound {
	struct Orders {
		bool healthy = false;
		ma_device device;

		xstd::vector<size_t> playing_sound;

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