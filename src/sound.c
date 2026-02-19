// NOW: We are clicking a bit. Idk why. Not the right channels being copied?
// Seems to happen both when channels activate and deactivate.
// We also need to add all the other sounds.

#define SOUND_MAX_CHANNELS 16
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
	u8 priority;
} SoundData;

typedef struct {
	SoundData sounds[SOUND_SPARSE_LEN];
	v2 listener;
	SoundChannel channels[SOUND_MAX_CHANNELS];
	u8 active_channels_len;
} SoundState;

typedef struct {
	u16 handle;
	u32 priority;
} SortedSound;

void sound_init(SoundState* state) {
}

void sound_start(SoundState* state, SoundHandle* handle, u8 priority, f32 amplitude, f32 frequency, f32 shelf, f32 volatility) {
	SoundData* sound = NULL;
	if(handle->assigned) { 
		assert(state->sounds[handle->index].active);
		assert(state->sounds[handle->index].channel_assigned);
		sound = &state->sounds[handle->index];
	} else {
		for(i32 sound_index = 0; sound_index < SOUND_SPARSE_LEN; sound_index++) {
			SoundData* slot = &state->sounds[sound_index];
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

void sound_stop(SoundState* state, SoundHandle* handle) {
	if(handle->assigned) {
		SoundData* sound = &state->sounds[handle->index];
		memset(sound, 0, sizeof(SoundData));
		memset(handle, 0, sizeof(SoundHandle));
	}
}

//void sound_set_channel_state(SoundChannel* channel, f32 amplitude, f32 frequency, f32 shelf, f32 volatility) {
	//channel->amplitude = amplitude;
	//channel->frequency = frequency;
	//channel->shelf = shelf;
	//channel->volatility = volatility;
	//channel->pan = 0.5f;
//}

/*
SoundChannel* sound_push_channel(SoundState* state, f32 amplitude, f32 frequency, f32 shelf, f32 volatility) {
	SoundChannel* channel = &state->channels[state->active_channels_len - 1];
	channel->pan = 0.5f;

	channel->amplitude = amplitude;
	channel->frequency = frequency;
	channel->shelf = shelf;
	channel->volatility = volatility;

	state->active_channels_len++;
	return channel;
}

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

void sound_update(SoundState* state) {
	SoundData* packed_sounds[SOUND_MAX_CHANNELS];
	u8 packed_sounds_len = 0;
	//u32 priority_threshold = UINT32_MAX;
	//SoundData* priority_threshold_sound;

	printf("starting\n");
	for(i32 sound_index = 0; sound_index < SOUND_SPARSE_LEN; sound_index++) {
		SoundData* sound = &state->sounds[sound_index];
		if(!sound->active) continue;

		if(packed_sounds_len < SOUND_MAX_CHANNELS) {
			packed_sounds[packed_sounds_len] = sound;
			packed_sounds_len++;

			//if(sound->priority < priority_threshold) {
			//	priority_threshold = sound->priority;
			//	priority_threshold_sound = sound;
			//}
		} else {
			panic();
			/* THIS IS THE CASE WHERE WE HAVE MORE SOUNDS THAN CHANNELS. We aren't there yet.
			if(active[active_index].priority > priority_threshold) {
				state->sounds[active[priority_threshold_index].handle].channel_assigned = false;
				priority_threshold = active[active_index].priority;
				channels_to_sounds[priority_threshold_index] = active_index;
				for(i32 prioritized_index = 0; prioritized_index < active_len; prioritized_index++) {
					if(active[channels_to_sounds[prioritized_index]].priority < priority_threshold) {
						priority_threshold = active[active_index].priority;
						priority_threshold_index = active_index;
					}
				}
			} else {
				state->sounds[active[active_index].handle].channel_assigned = false;
			}
			*/
		}
	}

	SoundChannel cached_channels[SOUND_MAX_CHANNELS];
	memcpy(cached_channels, state->channels, sizeof(SoundChannel) * SOUND_MAX_CHANNELS);
	bool channels_assigned[SOUND_MAX_CHANNELS] = {0};

	state->active_channels_len = packed_sounds_len;
	for(i32 channel_index = 0; channel_index < state->active_channels_len; channel_index++) {
		SoundChannel* channel = &state->channels[channel_index];
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
	for(i32 channel_index = 0; channel_index < state->active_channels_len; channel_index++) {
		if(!channels_assigned[channel_index]) {
			SoundChannel* channel = &state->channels[channel_index];
			channel->amplitude = 0.0f;
			channel->frequency = 0.0f;
		}
	}

	/*
	f32 primary_player_velocity = v_mags[0];

	SoundChannel* ch;
	for(i32 pl = 0; pl < 2; pl++) {
		v2 source = positions[pl];
		v2 listener = positions[0];
			

		// Collision/hit/damage
		f32 hit = hit_cools[pl];
		ch = sound_push_channel(state, 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f, 200.0f * hit * hit, 2000.0f + 2000.0f * hit, 2.0f * hit * ((f32)rand() / RAND_MAX));
		sound_spatialize_channel(ch, source, listener);
	}

	i32 mod_phase = 12;
	i32 mod_half = mod_phase / 2;

	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(frame / mod_half) % 16];
	ch = sound_push_channel(state, 4000.0f - (frame % mod_half) * 100.0f * freq + primary_player_velocity * 2000.0f, freq * 100.0f, 4000.0f, 0.0f);

	ch = sound_push_channel(state, 5000.0f - (frame % mod_half) * 100.0f * freq + primary_player_velocity * 1000.0f, freq * 33.3f, 4000.0f, 0.0f);

	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(frame / mod_phase) % 16];
	ch = sound_push_channel(state, 100.0f + primary_player_velocity * 400.0f - (frame % mod_phase) * 4000.0f, f4 * ((f32)rand() / RAND_MAX) * 400.0f, 4000.0f + primary_player_velocity * 50.0f, 0.0f);

	f32 fs3[16] = { 2, 1, 4, 1, 6, 1, 4, 1, 2, 3, 4, 1, 6, 3, 2, 3 };
	f32 f3 = fs3[(frame / mod_phase) % 16];
	ch = sound_push_channel(state, 2000.0f * f3 - (frame % mod_phase) * 8000.0f, f3 * ((f32)rand() / RAND_MAX) * 25.0f, 1000.0f + primary_player_velocity * 100.0f, f3 * (0.1f + primary_player_velocity * 0.02f - (frame % mod_phase) * 0.2f));
	*/
}
