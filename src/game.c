#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "renderer/render_list.c"
#include "ui_text.c"
#include "player.c"
#include "physics.c"
#include "level.c"
#include "game_active.c"
#include "level_editor.c"
#include "draw.c"
#include "debug_sound.c"

GAME_INIT(game_init) {
	GameState* game = &memory->state;
	memset(game, 0, sizeof(GameState));
	sound_init(&game->sound);
	input_init(&game->input);
	fast_random_init();

	// Fonts
	for(i32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		FontData* font = &game->fonts[font_index];
		FontAsset* f_asset = (FontAsset*)&assets->buffer[assets->font_buffer_offsets[font_index]];
		TextureAsset* t_asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}

	// Level
	LevelAsset* level_asset = (LevelAsset*)&assets->buffer[assets->level_buffer_offsets[0]];
	GameLevel* level = &game->level;
	level->spawns_len = level_asset->spawns_len;
	for(i32 spawn_index = 0; spawn_index < level->spawns_len; spawn_index++) {
		level->spawns[spawn_index] = level_asset->spawns[spawn_index];
	}

	level->side_length = level_asset->side_length;
	level->side_length = 64;
	u16 side_length = level->side_length;
	assert(side_length <= GAME_MAX_LEVEL_SIDE_LENGTH);
	for(i32 tile_index = 0; tile_index < side_length * side_length; tile_index++) {
		i32 x = tile_index % side_length;
		i32 y = tile_index / side_length;
		if(x < 2 || x > 61 || y < 2 || y > 61) {
			level->tiles[tile_index] = 1 + rand() / (RAND_MAX / 3);
		} else {
			level->tiles[tile_index] = level_asset->buffer[tile_index];
		}
	}

	// Players
	game->players_len = 4;
	game->players[0].team = 0;
	game->players[1].team = 0;
	game->players[2].team = 1;
	game->players[3].team = 1;
	for(i32 player_index = 0; player_index < game->players_len; player_index++) {
		GamePlayer* player = &game->players[player_index];
		player_spawn(player, level);
	}

	// Views
	// 
	// NOW: Setting views[0].player to anything other than 0 breaks any input
	// checks that are manually indexing into the 0th player input state.
	game->player_views_len = 1;
	game->player_views[0].player = 0; 
	game->player_views[1].player = 2; 
	for(i32 view_index = 0; view_index < game->player_views_len; view_index++) {
		GamePlayerView* view = &game->player_views[view_index];

		GamePlayer* player = &game->players[view->player];
		view->camera_offset = player->position;
		view->visible_health = 0.0f;

		input_attach_map(&game->input, view_index, view->player);
	}

	// Editor
#if GAME_EDITOR_TOOLS
	game->level_editor.tool = EDITOR_TOOL_CUBES;
#endif
}

GAME_UPDATE(game_update) {
	GameState* game = &memory->state;
	input_poll_events(&game->input, events_head);

	if(input_button_down(game->input.players[0].buttons[BUTTON_QUIT])) {
		output->close_requested = true;
	}

	switch(game->mode) {
		case GAME_ACTIVE: {
			game_active_update(game, dt);
		} break;
#if GAME_EDITOR_TOOLS
		case GAME_LEVEL_EDITOR: {
			level_editor_update(game);
		} break;
#endif
		default: break;
	}

	sound_update(&game->sound);

	StackAllocator ui_stack = stack_init(memory->transient.ui_memory, GAME_UI_MEMSIZE, "UI");
	draw_active_game(game, &output->render_list, &ui_stack, dt);
	debug_draw_sound_channels(&game->sound, &output->render_list, game->fonts, &ui_stack);

	game->frame++;
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	GameState* game = &memory->state;

	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;
	f32 global_attenuation = 0.62f;

	f32 channel_rates[SOUND_MAX_CHANNELS];
	for(i32 channel_index = 0; channel_index < SOUND_MAX_CHANNELS; channel_index++) {
		SoundChannel* channel = &game->sound.channels[channel_index];
		channel->amplitude = fclamp(channel->amplitude, 0.0f, channel->amplitude);
		channel->pan = fclamp(channel->pan, -1.0f, 1.0f);
		channel->pan = (channel->pan + 1.0f) / 2.0f;

		if(channel->phase > 2.0f * M_PI) {
			channel->phase -= 2.0f * M_PI;
		}

		// NOW: Remove hardcoded sample rate by driving alsa and this from config.h
		channel_rates[channel_index] = 2.0f * M_PI * channel->actual_frequency / 48000;

		if(channel_index >= game->sound.active_channels_len) {
			//channel->phase = 0.0f;
			//channel->amplitude = 0.0f;
			//channel->frequency = 0.0f;
		}
	}

	for(i32 sample_index = 0; sample_index < samples_count; sample_index++) {
		buffer[sample_index * 2] = 0.0f;
		buffer[sample_index * 2 + 1] = 0.0f;
		for(i32 ch = 0; ch < game->sound.active_channels_len; ch++) {
			SoundChannel* channel = &game->sound.channels[ch];
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
