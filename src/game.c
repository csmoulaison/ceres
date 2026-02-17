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
#include "sound.c"
#include "draw.c"

GAME_INIT(game_init) {
	GameState* game = &memory->state;
	game->mode = GAME_ACTIVE;
	game->frame = 0;

	fast_random_init();

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

	for(i32 player_index = 0; player_index < 2; player_index++) {
		GamePlayer* player = &game->players[player_index];
		player->position = v2_new(player_index * 2.0f + 32.0f, player_index * 2.0f + 32.0f);
		player->velocity = v2_zero();
		player->direction = 0.0f;
		player->rotation_velocity = 0.0f;
		player->health = 1.0f;
		player->score = 0;
		player->visible_health = player->health;
		player->shoot_cooldown = 0.0f;
		player->hit_cooldown = 0.0f;
		player->shoot_cooldown_sound = 0.0f;
		player->momentum_cooldown_sound = 0.0f;

		GameCamera* camera = &game->cameras[player_index];
		camera->offset = v2_zero();
	}

	for(i32 dm = 0; dm < 6; dm++) {
		GameDestructMesh* destruct_mesh = &game->destruct_meshes[dm];
		destruct_mesh->opacity = 0.0f;
	}

	sound_init(game->sound_channels);
	input_init(&game->input);

	// Load level
	LevelAsset* level_asset = (LevelAsset*)&assets->buffer[assets->level_buffer_offsets[0]];
	GameLevel* level = &game->level;
	level->spawns_len = level_asset->spawns_len;
	for(i32 sp = 0; sp < level->spawns_len; sp++) {
		level->spawns[sp] = level_asset->spawns[sp];
	}

	level->side_length = level_asset->side_length;
	level->side_length = 64;
	u16 side_length = level->side_length;
	assert(side_length <= MAX_GAME_LEVEL_SIDE_LENGTH);
	for(i32 pos = 0; pos < side_length * side_length; pos++) {
		i32 x = pos % side_length;
		i32 y = pos / side_length;
		if(x < 2 || x > 61 || y < 2 || y > 61) {
			level->tiles[pos] = 1 + rand() / (RAND_MAX / 3);
		} else {
			level->tiles[pos] = level_asset->buffer[pos];
		}
	}

	// init editor
	game->level_editor.tool = EDITOR_TOOL_CUBES;
}

GAME_UPDATE(game_update) {
	GameState* game = &memory->state;
	input_poll_events(&game->input, events_head);

	// Quit control
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

	for(i32 dm = 0; dm < 6; dm++) {
		GameDestructMesh* mesh = &game->destruct_meshes[dm];
		if(mesh->opacity <= 0.0f) continue;
		mesh->velocity.y -= 10.0f * dt;
		mesh->position = v3_add(mesh->position, v3_scale(mesh->velocity, dt));
		mesh->orientation = v3_add(mesh->orientation, v3_scale(mesh->rotation_velocity, dt));
		mesh->opacity -= dt;
	}

	sound_update(game->sound_channels, game);

	StackAllocator ui_stack = stack_init(memory->transient.ui_memory, GAME_UI_MEMSIZE, "UI");
	draw_active_game(game, &output->render_list, &ui_stack, dt);

	game->frame++;
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	GameState* game = &memory->state;

	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;
	f32 global_attenuation = 0.62f;

	f32 channel_rates[SOUND_CHANNELS_COUNT];
	for(i32 channel_index = 0; channel_index < SOUND_CHANNELS_COUNT; channel_index++) {
		SoundChannel* channel = &game->sound_channels[channel_index];
		channel->amplitude = fclamp(channel->amplitude, 0.0f, channel->amplitude);
		channel->pan = fclamp(channel->pan, -1.0f, 1.0f);
		channel->pan = (channel->pan + 1.0f) / 2.0f;

		if(channel->phase > 2.0f * M_PI) {
			channel->phase -= 2.0f * M_PI;
		}

		// NOW: Remove hardcoded sample rate by driving alsa and this from config.h
		channel_rates[channel_index] = 2.0f * M_PI * channel->actual_frequency / 48000;
	}

	for(i32 sample_index = 0; sample_index < samples_count; sample_index++) {
		buffer[sample_index * 2] = 0.0f;
		buffer[sample_index * 2 + 1] = 0.0f;
		for(i32 ch = 0; ch < SOUND_CHANNELS_COUNT; ch++) {
			SoundChannel* channel = &game->sound_channels[ch];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);

			channel->phase += channel_rates[ch];
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
