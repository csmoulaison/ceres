#define SOUND_MAX_CHANNELS 8
#define SOUND_SPARSE_LEN 64

typedef struct {
	u16 index;
	bool assigned;
} SoundHandle;

typedef struct {
	f32 amplitude;
	f32 frequency;
	f32 shelf;
	f32 volatility;
} SoundSettings;

// TODO: Try changing some of these into u8 values and profile to see if saving
// the cache space is worth it.
typedef struct {
	f32 phase;
	f32 amplitude;
	f32 frequency;
	f32 shelf;
	f32 volatility;
	f32 pan;

	f32 actual_frequency;
	f32 actual_amplitude;
} SoundChannel;

// SoundChannel and SoundData are separated to keep SoundChannel smaller for
// cache locality because generating sound samples is as of 2/17/26 the slowest
// part of the codebase.
typedef struct {
	SoundSettings settings;
	u8 channel;
	bool active;
	bool channel_assigned;
	u32 priority;
} SoundData;

typedef struct {
	SoundData sounds[SOUND_SPARSE_LEN];
	v2 listener;
	SoundChannel channels[SOUND_MAX_CHANNELS];
	u8 active_channels_len;
} Audio;

typedef struct {
	u16 handle;
	u32 priority;
} SortedSound;

void audio_init(Audio* audio) {
}

void audio_start_sound(Audio* audio, SoundHandle* handle, u32 priority, f32 amplitude, f32 frequency, f32 shelf, f32 volatility) {
	SoundData* sound = NULL;
	if(handle->assigned) { 
		assert(audio->sounds[handle->index].active);
		//assert(audio->sounds[handle->index].channel_assigned);
		sound = &audio->sounds[handle->index];
	} else {
		for(i32 sound_index = 0; sound_index < SOUND_SPARSE_LEN; sound_index++) {
			SoundData* slot = &audio->sounds[sound_index];
			if(!slot->active) {
				sound = slot;
				sound->active = true;
				handle->index = sound_index;
				handle->assigned = true;
				break;
			}
		}
		if(sound != NULL) { 
			assert(!sound->channel_assigned);
		}
	}
	if(sound == NULL) {
		printf("No handle available for sound type %u.\n", handle->index);
		return;
	}

	sound->priority = priority;
	sound->settings.amplitude = amplitude;
	sound->settings.frequency = frequency;
	sound->settings.shelf = shelf;
	sound->settings.volatility = volatility;
}

void audio_stop_sound(Audio* audio, SoundHandle* handle) {
	if(handle->assigned) {
		SoundData* sound = &audio->sounds[handle->index];
		memset(sound, 0, sizeof(SoundData));
		memset(handle, 0, sizeof(SoundHandle));
	}
}

/*
void sound_spatialize_channel(SoundChannel* channel, v2 source_position, v2 listener_position) {
	f32 distance = v2_distance(source_position, listener_position);
	if(distance <= 0.1) {
		channel->pan = 0.5f;
		return;
	}

	channel->amplitude /= distance * 1.0f;
	channel->shelf += distance * 100.0f;
	channel->volatility -= distance * 0.01f;
	channel->pan = (listener_position.x - source_position.x) * 0.2f;
	if(channel->volatility < 0.0f) channel->volatility = 0.0f;
}
*/

void audio_update(Audio* audio) {
	SoundData* packed_sounds[SOUND_MAX_CHANNELS];
	u8 packed_sounds_len = 0;
	u32 priority_threshold = UINT32_MAX;
	u8 priority_threshold_index;

	for(i32 sound_index = 0; sound_index < SOUND_SPARSE_LEN; sound_index++) {
		SoundData* sound = &audio->sounds[sound_index];
		if(!sound->active) continue;

		if(packed_sounds_len < SOUND_MAX_CHANNELS) {
			packed_sounds[packed_sounds_len] = sound;
			if(sound->priority < priority_threshold) {
				priority_threshold = sound->priority;
				priority_threshold_index = packed_sounds_len;
			}
			packed_sounds_len++;
		} else {
			if(sound->priority > priority_threshold) {
				packed_sounds[priority_threshold_index]->channel_assigned = false;
				packed_sounds[priority_threshold_index] = sound;
				priority_threshold = sound->priority;

				for(i32 packed_index = 0; packed_index < packed_sounds_len; packed_index++) {
					if(packed_sounds[packed_index]->priority < priority_threshold) {
						priority_threshold = packed_sounds[packed_index]->priority;
						priority_threshold_index = packed_index;
					}
				}
			} else {
				sound->channel_assigned = false;
			}
		}
	}

	SoundChannel cached_channels[SOUND_MAX_CHANNELS];
	memcpy(cached_channels, audio->channels, sizeof(SoundChannel) * SOUND_MAX_CHANNELS);
	bool channels_assigned[SOUND_MAX_CHANNELS] = {0};

	audio->active_channels_len = packed_sounds_len;
	for(i32 channel_index = 0; channel_index < audio->active_channels_len; channel_index++) {
		SoundChannel* channel = &audio->channels[channel_index];
		SoundData* sound = packed_sounds[channel_index];
		if(sound->channel_assigned) {
			SoundChannel* cached_channel = &cached_channels[sound->channel];
			channel->phase = cached_channel->phase;
			channel->actual_frequency = cached_channel->actual_frequency;
			channel->actual_amplitude = cached_channel->actual_amplitude;
			*channel = *cached_channel;
		} else {
			sound->channel_assigned = true;
			channel->phase = 0.0f;
			channel->actual_amplitude = 0.0f;
			channel->actual_frequency = 0.0f;
		}
		sound->channel = channel_index;
		channels_assigned[channel_index] = true;

		channel->amplitude = sound->settings.amplitude;
		channel->frequency = sound->settings.frequency;
		channel->shelf = sound->settings.shelf;
		channel->volatility = sound->settings.volatility;
		channel->pan = 0.5f;
	}
	for(i32 channel_index = 0; channel_index < audio->active_channels_len; channel_index++) {
		if(!channels_assigned[channel_index]) {
			SoundChannel* channel = &audio->channels[channel_index];
			channel->amplitude = 0.0f;
			channel->frequency = 0.0f;
		}
	}

	/*
	f32 primary_player_velocity = v_mags[0];

	i32 mod_phase = 12;
	i32 mod_half = mod_phase / 2;

	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(frame / mod_half) % 16];
	ch = sound_push_channel(audio, 4000.0f - (frame % mod_half) * 100.0f * freq + primary_player_velocity * 2000.0f, freq * 100.0f, 4000.0f, 0.0f);

	ch = sound_push_channel(audio, 5000.0f - (frame % mod_half) * 100.0f * freq + primary_player_velocity * 1000.0f, freq * 33.3f, 4000.0f, 0.0f);

	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(frame / mod_phase) % 16];
	ch = sound_push_channel(audio, 100.0f + primary_player_velocity * 400.0f - (frame % mod_phase) * 4000.0f, f4 * ((f32)rand() / RAND_MAX) * 400.0f, 4000.0f + primary_player_velocity * 50.0f, 0.0f);

	f32 fs3[16] = { 2, 1, 4, 1, 6, 1, 4, 1, 2, 3, 4, 1, 6, 3, 2, 3 };
	f32 f3 = fs3[(frame / mod_phase) % 16];
	ch = sound_push_channel(audio, 2000.0f * f3 - (frame % mod_phase) * 8000.0f, f3 * ((f32)rand() / RAND_MAX) * 25.0f, 1000.0f + primary_player_velocity * 100.0f, f3 * (0.1f + primary_player_velocity * 0.02f - (frame % mod_phase) * 0.2f));
	*/
}
