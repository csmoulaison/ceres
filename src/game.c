#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "renderer/render_list.c"
#include "ui_text.c"
#include "debug_sound.c"
#include "input.c"
#include "main_menu.c"
#include "session.c"

GAME_INIT(game_init) {
	memset(memory, 0, sizeof(GameMemory));

	input_init(&memory->input);
	audio_init(&memory->audio);
	fast_random_init();

	memory->mode_type = GAME_MENU;
	main_menu_init((MainMenu*)memory->mode.memory);
	
	for(i32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		FontData* font = &memory->fonts[font_index];
		FontAsset* f_asset = (FontAsset*)&assets->buffer[assets->font_buffer_offsets[font_index]];
		TextureAsset* t_asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}
}

GAME_UPDATE(game_update) {
	input_poll_events(&memory->input, events_head);
	switch(memory->mode_type) {
		case GAME_MENU: {
			main_menu_update((MainMenu*)memory->mode.memory, output, &memory->input, &memory->audio, dt);
			if(((MainMenu*)memory->mode.memory)->start_session) {
				memory->mode_type = GAME_SESSION;
				LevelAsset* level_asset = (LevelAsset*)&assets->buffer[assets->level_buffer_offsets[0]];
				session_init((Session*)memory->mode.memory, &memory->input, level_asset);
			}
		} break;
		case GAME_SESSION: {
			Session* session = (Session*)memory->mode.memory;
			session_update(session, output, &memory->input, &memory->audio, dt);
			if(session->quit_requested) {
				main_menu_init((MainMenu*)memory->mode.memory);
				memory->mode_type = GAME_MENU;
			}
		} break;
		default: break;
	}
	audio_update(&memory->audio);
	memory->frames_since_init++;
}

GAME_GENERATE_RENDER_LIST(game_generate_render_list) {
	render_list_init(list);
	StackAllocator draw_stack = stack_init(memory->frame.memory, GAME_FRAME_MEMSIZE, "Draw");
	switch(memory->mode_type) {
		case GAME_MENU: {
			draw_main_menu((MainMenu*)memory->mode.memory, list, memory->fonts, &draw_stack);
		} break;
		case GAME_SESSION: {
			session_draw((Session*)memory->mode.memory, list, memory->fonts, &draw_stack);
		} break;
		default: break;
	}
	//debug_draw_sound_channels(&memory->audio, list, memory->fonts, &draw_stack);
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;
	f32 global_attenuation = 0.62f;

	f32 channel_rates[SOUND_MAX_CHANNELS];
	for(i32 channel_index = 0; channel_index < SOUND_MAX_CHANNELS; channel_index++) {
		SoundChannel* channel = &memory->audio.channels[channel_index];
		channel->amplitude = fclamp(channel->amplitude, 0.0f, channel->amplitude);
		channel->pan = fclamp(channel->pan, -1.0f, 1.0f);
		channel->pan = (channel->pan + 1.0f) / 2.0f;

		if(channel->phase > 2.0f * M_PI) {
			channel->phase -= 2.0f * M_PI;
		}

		// NOW: Remove hardcoded sample rate by driving alsa and this from config.h
		channel_rates[channel_index] = 2.0f * M_PI * channel->actual_frequency / 48000;

		if(channel_index >= memory->audio.active_channels_len) {
			//channel->phase = 0.0f;
			//channel->amplitude = 0.0f;
			//channel->frequency = 0.0f;
		}
	}

	for(i32 sample_index = 0; sample_index < samples_count; sample_index++) {
		buffer[sample_index * 2] = 0.0f;
		buffer[sample_index * 2 + 1] = 0.0f;
		for(i32 ch = 0; ch < memory->audio.active_channels_len; ch++) {
			SoundChannel* channel = &memory->audio.channels[ch];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);

			channel->phase += 2.0f * M_PI * channel->actual_frequency / 48000;
			//channel->phase += channel_rates[ch];
			f32 sample = channel->actual_amplitude * sinf(channel->phase);
			sample = fclamp(sample, -channel->shelf, channel->shelf);
			sample += sample * fast_random_f32() * channel->volatility;

			buffer[sample_index * 2] += sample * (1.0f - channel->pan);
			buffer[sample_index * 2 + 1] += sample * channel->pan;
		}

		buffer[sample_index * 2] *= global_attenuation;
		buffer[sample_index * 2] = fclamp(buffer[sample_index * 2], -global_shelf, global_shelf);
		buffer[sample_index * 2 + 1] *= global_attenuation;
		buffer[sample_index * 2 + 1] = fclamp(buffer[sample_index * 2 + 1], -global_shelf, global_shelf);

		if(buffer[sample_index * 2] == global_shelf || buffer[sample_index * 2 + 1] == global_shelf) {
			printf("Global shelf reached (up)\n");
		}
		if(buffer[sample_index * 2] == -global_shelf || buffer[sample_index * 2 + 1] == -global_shelf) {
			printf("Global shelf reached (up)\n");
		}
	}
}
