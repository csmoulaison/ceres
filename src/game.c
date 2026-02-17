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

GAME_INIT(game_init) {
	GameState* game = &memory->state;
	game->mode = GAME_ACTIVE;
	game->frame = 0;

	fast_random_init();

	for(i32 fnt = 0; fnt < ASSET_NUM_FONTS; fnt++) {
		FontData* font = &game->fonts[fnt];
		FontAsset* f_asset = (FontAsset*)&assets->buffer[assets->font_buffer_offsets[fnt]];
		TextureAsset* t_asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}

	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		player->position = v2_new(pl * 2.0f + 32.0f, pl * 2.0f + 32.0f);
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

		GameCamera* camera = &game->cameras[pl];
		camera->offset = v2_zero();
	}

	for(i32 dm = 0; dm < 6; dm++) {
		GameDestructMesh* destruct_mesh = &game->destruct_meshes[dm];
		destruct_mesh->opacity = 0.0f;
	}

	for(i32 ch = 0; ch < GAME_SOUND_CHANNELS_COUNT; ch++) {
		GameSoundChannel* channel = &game->sound_channels[ch];
		channel->phase = 0.0f;
		channel->frequency = 0.0f;
		channel->amplitude = 0.0f;
		channel->shelf = 0.0f;
		channel->volatility = 0.0f;
		channel->pan = 0.0f;
		channel->actual_frequency = 0.0f;
		channel->actual_amplitude = 0.0f;
	}

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

	// Update sound channels
	for(i32 ch = 0; ch < GAME_SOUND_CHANNELS_COUNT; ch++) {
		GameSoundChannel* channel = &game->sound_channels[ch];
		channel->amplitude = 0.0f;
		channel->frequency = 0.0f;
		channel->volatility = 0.0f;
		//*channel = (GameSoundChannel){};
	}

#if 1
	f32 current_player_velocity;

#define player_channels 5
	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		f32 v_mag = v2_magnitude(player->velocity);
		if(pl == 0) current_player_velocity = v_mag;

		GameSoundChannel* velocity_channel = &game->sound_channels[0 + pl * player_channels];
		velocity_channel->amplitude = 2000.0f * v_mag;
		velocity_channel->frequency = 7.144123f * v_mag;
		velocity_channel->shelf = 6000.0f;
		velocity_channel->volatility = 0.002f * v_mag;

		GameSoundChannel* momentum_channel = &game->sound_channels[1 + pl * player_channels];
		f32 m = player->momentum_cooldown_sound;
		momentum_channel->amplitude = 2000.0f * m;
		momentum_channel->frequency = 3.0f * m;
		momentum_channel->shelf = 2000.0f;
		momentum_channel->volatility = 0.01f * m;

		GameSoundChannel* rotation_channel = &game->sound_channels[2 + pl * player_channels];
		f32 rot = player->rotation_velocity;
		rotation_channel->amplitude = 2000.0f * rot;
		rotation_channel->frequency = 10.0f * abs(rot);
		rotation_channel->shelf = 6000.0f;
		rotation_channel->volatility = 0.002f * rot;

		GameSoundChannel* shoot_channel = &game->sound_channels[3 + pl * player_channels];
		f32 shoot = player->shoot_cooldown_sound;
		shoot_channel->amplitude = 10000.0f * shoot * shoot;
		shoot_channel->frequency = 1500.0f * shoot * shoot;
		shoot_channel->shelf = 2000.0f + 2000.0f * shoot;
		shoot_channel->volatility = 0.2f * shoot;

		GameSoundChannel* hit_channel = &game->sound_channels[4 + pl * player_channels];
		f32 hit = player->hit_cooldown;
		hit_channel->amplitude = 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f;
		hit_channel->frequency = 200.0f * hit * hit;
		hit_channel->shelf = 2000.0f + 2000.0f * hit;
		hit_channel->volatility = 2.0f * hit * ((f32)rand() / RAND_MAX);

		for(i32 ch = 0; ch < player_channels; ch++) {
			GameSoundChannel* channel = &game->sound_channels[ch + pl * player_channels];

			f32 distance_1 = 1.0f + v2_distance(game->players[0].position, player->position);
			f32 distance_2 = 1.0f + v2_distance(game->players[1].position, player->position);
			f32 distance = fmin(distance_1, distance_2);
			// TODO: This is for singleplayer case.
			distance = distance_1;


			channel->amplitude /= distance * 1.0f;
			channel->shelf += distance * 100.0f;
			channel->volatility -= distance * 0.01f;
			channel->pan = (game->players[0].position.y - player->position.y) * 0.2f;
			if(channel->volatility < 0.0f) channel->volatility = 0.0f;
		}
	}

#endif

	i32 mod_phase = 12;
	i32 mod_half = mod_phase / 2;
#if 1
	GameSoundChannel* music_channel = &game->sound_channels[player_channels * 2];
	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(game->frame / mod_half) % 16];
	music_channel->amplitude = 4000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 2000.0f;
	music_channel->frequency = freq * 100.0f;
	music_channel->shelf = 4000.0f;
	music_channel->volatility = freq * 0.01f - (game->frame % mod_half) * freq * 0.1f;
	music_channel->volatility = 0.0f;
	music_channel->pan = 0.0f;

	GameSoundChannel* ch2 = &game->sound_channels[player_channels * 2 + 1];
	ch2->amplitude = 5000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 1000.0f;
	ch2->frequency = freq * 33.3f;
	ch2->shelf = 4000.0f;
	ch2->volatility = freq * 0.05f;
	ch2->volatility = 0.0f;
	ch2->pan = 0.0f;

	GameSoundChannel* ch4 = &game->sound_channels[player_channels * 2 + 2];
	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(game->frame / mod_phase) % 16];
	ch4->amplitude = 100.0f + current_player_velocity * 400.0f - (game->frame % mod_phase) * 4000.0f;
	ch4->frequency = f4 * ((f32)rand() / RAND_MAX) * 400.0f;
	ch4->shelf = 4000.0f + current_player_velocity * 50.0f;
	ch4->volatility = f4 * 0.05f - (game->frame % mod_phase) * 0.01f;
	ch4->volatility = 0.0f;
	ch4->pan = 0.0f;
#endif

#if 1
	GameSoundChannel* ch3 = &game->sound_channels[player_channels * 2 + 3];
	f32 fs3[16] = { 2, 1, 4, 1, 6, 1, 4, 1, 2, 3, 4, 1, 6, 3, 2, 3 };
	f32 f3 = fs3[(game->frame / mod_phase) % 16];
	ch3->amplitude = 2000.0f * f3 - (game->frame % mod_phase) * 8000.0f;
	ch3->frequency = f3 * ((f32)rand() / RAND_MAX) * 25.0f;
	ch3->shelf = 1000.0f + current_player_velocity * 100.0f;
	ch3->volatility = f3 * (0.1f + current_player_velocity * 0.02f - (game->frame % mod_phase) * 0.2f);
	ch3->pan = 0.0f;
#endif

	// Populate render list
	RenderList* list = &output->render_list;
	render_list_init(list);

	render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, 8);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP_2, 8);
	render_list_allocate_instance_type(list, ASSET_MESH_CYLINDER, 0, 64);

	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_BODY, ASSET_TEXTURE_SHIP, 2);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_L, ASSET_TEXTURE_SHIP, 2);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_R, ASSET_TEXTURE_SHIP, 2);

	render_list_allocate_instance_type(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, 4096);

	i32 floor_length = game->level.side_length;
	i32 floor_instances = floor_length * floor_length;
	render_list_allocate_instance_type(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_instances);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	bool splitscreen = false;
	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		v3 rot = player_orientation(player);
		u8 texture = ASSET_TEXTURE_SHIP;
		if(pl == 1) texture = ASSET_TEXTURE_SHIP_2;
		render_list_draw_model_colored(list, ASSET_MESH_SHIP, texture, pos, rot, v4_new(1.0f, 1.0f, 1.0f, player->hit_cooldown * player->hit_cooldown * 4.0f));

		v2 direction = player_direction_vector(player);
		v2 side = v2_new(-direction.y, direction.x);
		v2 target = v2_add(player->position, v2_scale(direction, 50.0f));

		v2 laser_pos_1 = v2_add(v2_add(player->position, v2_scale(side, 0.39f)), v2_scale(direction, 0.10f));
		v2 laser_pos_2 = v2_add(v2_add(player->position, v2_scale(side, -0.39f)), v2_scale(direction, 0.10f));

		render_list_draw_laser(list, v3_new(laser_pos_1.x, 0.5f, laser_pos_1.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);
		render_list_draw_laser(list, v3_new(laser_pos_2.x, 0.5f, laser_pos_2.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);

		// NOW: Debug player velocity line.
		//v3 vel_laser_start = v3_new(player->position.x, 0.5f, player->position.y);
		//v3 vel_laser_end = v3_add(vel_laser_start, v3_new(player->velocity.x, 0.0f, player->velocity.y));
		//render_list_draw_laser(list, laser_instance_type, vel_laser_start, vel_laser_end, 0.05f);
	}

	for(i32 pos = 0; pos < floor_instances; pos++) {
		v3 floor_pos = v3_new(pos % floor_length, 0.0f, pos / floor_length);
		render_list_draw_model_aligned(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos);
	}

	// TODO: When this is put below cube rendering, the meshes are overwritten by
	// cubes. Pretty important we figure out why, obviously.
	for(i32 dm = 0; dm < 6; dm++) {
		GameDestructMesh* mesh = &game->destruct_meshes[dm];
		if(mesh->opacity <= 0.0f) continue;
		render_list_draw_model_colored(list, mesh->mesh, mesh->texture, mesh->position, mesh->orientation, v4_new(1.0f, 1.0f, 1.0f, mesh->opacity * mesh->opacity));
	}

	for(i32 pos = 0; pos < floor_instances; pos++) {
		for(i32 height = 1; height <= game->level.tiles[pos]; height++) {
			v3 cube_pos = v3_new(pos % floor_length, (f32)height - 1.0f, pos / floor_length);
			render_list_draw_model_aligned(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, cube_pos);
		}
	}

	switch(game->mode) {
		case GAME_ACTIVE: {
			for(i32 player_index = 0; player_index < 2; player_index++) {
				if(splitscreen || player_index == 0) {
					GameCamera* camera = &game->cameras[player_index];
					GamePlayer* player = &game->players[player_index];
					v3 cam_target = v3_new(camera->offset.x + player->position.x, 0.0f, camera->offset.y + player->position.y);
					//v3 cam_pos = v3_new(player->position.x - camera->offset.x * 1.5f, 5.0f, player->position.y - camera->offset.y * 1.5f);
					v3 cam_pos = v3_new(cam_target.x, 8.0f, cam_target.z - 4.0f + player_index * 8.0f);
					f32 gap = 0.0025f;
					v4 screen_rect;
					if(splitscreen) {
						screen_rect = v4_new(0.5f * player_index + gap * player_index, 0.0f, 0.5f - gap * (1 - player_index), 1.0f);
					} else {
						screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
					}
					render_list_add_camera(list, cam_pos, cam_target, screen_rect);
				}
			}
		} break;
#if GAME_EDITOR_TOOLS
		case GAME_LEVEL_EDITOR: {
			level_editor_draw(game, list);
		} break;
#endif
		default: break;
	}

	// TODO: Layouts for splitscreen vs single and others, sets up anchors and
	// positions per item, which are referenced here in the drawing commands.
	StackAllocator ui_stack = stack_init(memory->transient.ui_memory, GAME_UI_MEMSIZE, "UI");

	// Score
	for(i32 player_index = 0; player_index < 2; player_index++) {
		v2 position = v2_new(-24.0f + player_index * 48.0f, -32.0f);
		v2 inner_anchor = v2_new(1.0f * (1.0f - player_index), 1.0f);
		v2 screen_anchor = v2_new(0.5f, 1.0f);
		v4 color = v4_new(1.0f, 1.0f, 0.0f, 1.0f);
		char buf[16];
		sprintf(buf, "%i", game->players[player_index].score);
		ui_draw_text_line(list, game->fonts, ASSET_FONT_QUANTICO_LARGE, buf,
			position, inner_anchor, screen_anchor, color, &ui_stack);
	}

	for(i32 player_index = 0; player_index < 2; player_index++) {
		if(!splitscreen && player_index != 0) continue;

		GamePlayer* player = &game->players[player_index];
		player->visible_health = lerp(player->visible_health, player->health, 20.0f * dt);
		v4 health_bar_root = v4_new(32.0f, 32.0f, 64.0f, 400.0f);
		render_list_draw_box(list, health_bar_root, v2_new(player_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f, 1.0f - player->hit_cooldown, 0.0f, 1.0f));
		f32 segments = 40.0f;
		for(i32 seg = 0; seg < (i32)(player->visible_health * segments); seg++) {
			v4 health_bar_sub = health_bar_root;
			health_bar_sub.x += 8.0f;
			health_bar_sub.y += 8.0f + seg * ((health_bar_root.w - 8.0f) / segments);
			health_bar_sub.z -= 16.0f;
			health_bar_sub.w = health_bar_root.w / segments - 8.0f;
			render_list_draw_box(list, health_bar_sub, v2_new(player_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f - seg / segments, seg / segments, 0.0f, 1.0f));
		}
	}

	game->frame++;
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	GameState* game = &memory->state;

	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;
	f32 global_attenuation = 0.62f;

	f32 channel_rates[GAME_SOUND_CHANNELS_COUNT];
	for(i32 channel_index = 0; channel_index < GAME_SOUND_CHANNELS_COUNT; channel_index++) {
		GameSoundChannel* channel = &game->sound_channels[channel_index];
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
		for(i32 ch = 0; ch < GAME_SOUND_CHANNELS_COUNT; ch++) {
			GameSoundChannel* channel = &game->sound_channels[ch];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);

			channel->phase += channel_rates[ch];
			f32 sample_indexle = channel->actual_amplitude * sinf(channel->phase);
			sample_indexle = fclamp(sample_indexle, -channel->shelf, channel->shelf);
			sample_indexle += sample_indexle * fast_random_f32() * channel->volatility;

			buffer[sample_index * 2] += sample_indexle * (1.0f - channel->pan);
			buffer[sample_index * 2 + 1] += sample_indexle * channel->pan;
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
