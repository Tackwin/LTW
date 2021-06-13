#include "Audio/Audio.hpp"


void audio::Orders::init() noexcept {
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

void audio::Orders::uninit() noexcept {
	ma_device_uninit(&device);
}

void audio::Orders::callback(void* output, const void* input, ma_uint32 frame_count) {
	if (!asset::Store.ready) return;

	size_t n = produce_idx;
	for (; consume_idx < n; ++consume_idx) {
		playing_sounds.push_back(circular_sound_buffer[consume_idx % 100]);
	}

	std::int16_t* temp_output = (std::int16_t*)output;
	thread_local xstd::vector<std::int16_t> frame_out;
	frame_out.clear();
	frame_out.resize(frame_count);

	for (auto& s : playing_sounds) if (!s.to_remove) {
		auto& decoder = asset::Store.get_sound(s.asset_id);
		ma_decoder_seek_to_pcm_frame(&decoder, s.current_frame);
		size_t frame_read = ma_decoder_read_pcm_frames(
			&decoder, temp_output, frame_count
		);

		for (size_t i = 0; i < frame_read; ++i) frame_out[i] += s.volume * temp_output[i];

		if (frame_read < frame_count) {
			s.to_remove = true;
		}
		s.current_frame += frame_read;
	}

	for (size_t i = 0; i < frame_count; ++i)
		((std::int16_t*)output)[i] = frame_out[i];

	playing_sounds.remove_all([](auto& x) { return x.to_remove; });
}

void audio::Orders::add_sound(audio::Sound s) noexcept {
	circular_sound_buffer[produce_idx % 100] = s;
	produce_idx++;
}
