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

	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		FontData* font = &game->fonts[i];
		FontAsset* f_asset = (FontAsset*)&assets->buffer[assets->font_buffer_offsets[i]];
		TextureAsset* t_asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		player->position = v2_new(i * 2.0f + 32.0f, i * 2.0f + 32.0f);
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

		for(i32 i = 0; i < NUM_BUTTONS; i++) {
			player->button_states[i] = 0;
		}

		GameCamera* camera = &game->cameras[i];
		camera->offset = v2_zero();
	}

	for(i32 i = 0; i < 6; i++) {
		GameDestructMesh* destruct_mesh = &game->destruct_meshes[i];
		destruct_mesh->opacity = 0.0f;
	}

	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* channel = &game->sound_channels[i];
		channel->phase = 0.0f;
		channel->frequency = 0.0f;
		channel->amplitude = 0.0f;
		channel->shelf = 0.0f;
		channel->volatility = 0.0f;
		channel->pan = 0.0f;
		channel->actual_frequency = 0.0f;
		channel->actual_amplitude = 0.0f;
	}

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&game->key_mappings_len, sizeof(u32), 1, file);
	for(i32 i = 0; i < game->key_mappings_len; i++) {
		fread(&game->key_mappings[i], sizeof(GameKeyMapping), 1, file);
	}

	LevelAsset* level_asset = (LevelAsset*)&assets->buffer[assets->level_buffer_offsets[0]];
	game->level.side_length = level_asset->side_length;
	u16 side_length = game->level.side_length;
	assert(side_length <= MAX_GAME_LEVEL_SIDE_LENGTH);
	for(i32 i = 0; i < side_length * side_length; i++) {
		i32 x = i % side_length;
		i32 y = i / side_length;
		if(x < 12 || x > 51 || y < 12 || y > 51) {
			game->level.tiles[i] = 1 + rand() / (RAND_MAX / 3);
		} else {
			game->level.tiles[i] = level_asset->buffer[i];
		}
	}
}

GAME_UPDATE(game_update) {
	GameState* game = &memory->state;

	// Update input events
	for(u32 i = 0; i < NUM_BUTTONS; i++) {
		game->players[0].button_states[i] = game->players[0].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
		game->players[1].button_states[i] = game->players[1].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}
	GameEvent* event = events_head;
	while(event != NULL) {
		switch(event->type) {
			case GAME_EVENT_KEY_DOWN: {
				for(i32 i = 0; i < game->key_mappings_len; i++) {
					GameKeyMapping* mapping = &game->key_mappings[i];
					if(mapping->key_id == *((u64*)event->data)) {
						ButtonState* button = &game->players[mapping->player_index].button_states[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							break;
						}
						*button = (*button) | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
						break;
					}
				}
			} break;
			case GAME_EVENT_KEY_UP: {
				for(i32 i = 0; i < game->key_mappings_len; i++) {
					GameKeyMapping* mapping = &game->key_mappings[i];
					if(mapping->key_id == *((u64*)event->data)) {
						ButtonState* button = &game->players[mapping->player_index].button_states[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							*button = INPUT_RELEASED_BIT;
						}
						break;
					}
				}
			} break;
			default: break;
		}
		event = event->next;
	}

	// Quit control
	if(input_button_down(game->players[0].button_states[BUTTON_QUIT])) {
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

	for(i32 i = 0; i < 6; i++) {
		GameDestructMesh* mesh = &game->destruct_meshes[i];
		if(mesh->opacity <= 0.0f) continue;
		mesh->velocity.y -= 10.0f * dt;
		mesh->position = v3_add(mesh->position, v3_scale(mesh->velocity, dt));
		mesh->orientation = v3_add(mesh->orientation, v3_scale(mesh->rotation_velocity, dt));
		mesh->opacity -= dt;
	}

	// Update sound channels
	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* ch = &game->sound_channels[i];
		ch->amplitude = 0.0f;
		ch->frequency = 0.0f;
		ch->volatility = 0.0f;
	}

#if 1
	f32 current_player_velocity;

#define player_channels 5
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		f32 v_mag = v2_magnitude(player->velocity);
		if(i == 0) current_player_velocity = v_mag;

		GameSoundChannel* velocity_channel = &game->sound_channels[0 + i * player_channels];
		velocity_channel->amplitude = 2000.0f * v_mag;
		velocity_channel->frequency = 7.144123f * v_mag;
		velocity_channel->shelf = 6000.0f;
		velocity_channel->volatility = 0.002f * v_mag;

		GameSoundChannel* momentum_channel = &game->sound_channels[1 + i * player_channels];
		f32 m = player->momentum_cooldown_sound;
		momentum_channel->amplitude = 2000.0f * m;
		momentum_channel->frequency = 3.0f * m;
		momentum_channel->shelf = 2000.0f;
		momentum_channel->volatility = 0.01f * m;

		GameSoundChannel* rotation_channel = &game->sound_channels[2 + i * player_channels];
		f32 rot = player->rotation_velocity;
		rotation_channel->amplitude = 2000.0f * rot;
		rotation_channel->frequency = 10.0f * abs(rot);
		rotation_channel->shelf = 6000.0f;
		rotation_channel->volatility = 0.002f * rot;

		GameSoundChannel* shoot_channel = &game->sound_channels[3 + i * player_channels];
		f32 shoot = player->shoot_cooldown_sound;
		shoot_channel->amplitude = 10000.0f * shoot * shoot;
		shoot_channel->frequency = 1500.0f * shoot * shoot;
		shoot_channel->shelf = 2000.0f + 2000.0f * shoot;
		shoot_channel->volatility = 0.2f * shoot;

		GameSoundChannel* hit_channel = &game->sound_channels[4 + i * player_channels];
		f32 hit = player->hit_cooldown;
		hit_channel->amplitude = 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f;
		hit_channel->frequency = 200.0f * hit * hit;
		hit_channel->shelf = 2000.0f + 2000.0f * hit;
		hit_channel->volatility = 2.0f * hit * ((f32)rand() / RAND_MAX);

		for(i32 j = 0; j < player_channels; j++) {
			GameSoundChannel* ch = &game->sound_channels[j + i * player_channels];

			f32 distance_1 = 1.0f + v2_distance(game->players[0].position, player->position);
			f32 distance_2 = 1.0f + v2_distance(game->players[1].position, player->position);
			f32 distance = fmin(distance_1, distance_2);
			// TODO: This is for singleplayer case.
			distance = distance_1;


			ch->amplitude /= distance * 1.0f;
			ch->shelf += distance * 100.0f;
			ch->volatility -= distance * 0.01f;
			ch->pan = (game->players[0].position.y - player->position.y) * 0.2f;
			if(ch->volatility < 0.0f) ch->volatility = 0.0f;
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

	u8 ship_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, 1);
	u8 ship2_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP_2, 1);
	u8 laser_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_CYLINDER, 0, 64);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	bool splitscreen = false;
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		v3 rot = player_orientation(player);
		u8 instance_type = ship_instance_type;
		if(i == 1) instance_type = ship2_instance_type;
		render_list_draw_model_colored(list, instance_type, pos, rot, v4_new(1.0f, 1.0f, 1.0f, player->hit_cooldown * player->hit_cooldown * 4.0f));

		v2 direction = player_direction_vector(player);
		v2 side = v2_new(-direction.y, direction.x);
		v2 target = v2_add(player->position, v2_scale(direction, 50.0f));

		v2 laser_pos_1 = v2_add(v2_add(player->position, v2_scale(side, 0.39f)), v2_scale(direction, 0.10f));
		v2 laser_pos_2 = v2_add(v2_add(player->position, v2_scale(side, -0.39f)), v2_scale(direction, 0.10f));

		render_list_draw_laser(list, laser_instance_type, v3_new(laser_pos_1.x, 0.5f, laser_pos_1.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);
		render_list_draw_laser(list, laser_instance_type, v3_new(laser_pos_2.x, 0.5f, laser_pos_2.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);

		// NOW: Debug player velocity line.
		//v3 vel_laser_start = v3_new(player->position.x, 0.5f, player->position.y);
		//v3 vel_laser_end = v3_add(vel_laser_start, v3_new(player->velocity.x, 0.0f, player->velocity.y));
		//render_list_draw_laser(list, laser_instance_type, vel_laser_start, vel_laser_end, 0.05f);
	}

	i32 floor_length = game->level.side_length;
	i32 floor_instances = floor_length * floor_length;
	u8 floor_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_instances);
	for(i32 i = 0; i < floor_instances; i++) {
		v3 floor_pos = v3_new(i % floor_length, 0.0f, i / floor_length);
		render_list_draw_model_aligned(list, floor_instance_type, floor_pos);
	}

	// TODO: When this is put below cube rendering, the meshes are overwritten by
	// cubes. Pretty important we figure out why, obviously.
	u8 body_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP_BODY, ASSET_TEXTURE_SHIP, 2);
	u8 wing_l_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_L, ASSET_TEXTURE_SHIP, 2);
	u8 wing_r_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_R, ASSET_TEXTURE_SHIP, 2);
	for(i32 i = 0; i < 6; i++) {
		GameDestructMesh* mesh = &game->destruct_meshes[i];
		if(mesh->opacity <= 0.0f) continue;

		u8 instance_type = 0;
		switch(mesh->mesh) {
			case ASSET_MESH_SHIP_BODY: {
				instance_type = body_instance_type;
			} break;
			case ASSET_MESH_SHIP_WING_L: {
				instance_type = wing_l_instance_type;
			} break;
			case ASSET_MESH_SHIP_WING_R: {
				instance_type = wing_r_instance_type;
			} break;
			default: { panic(); }
		}
		render_list_draw_model_colored(list, instance_type, mesh->position, mesh->orientation, v4_new(1.0f, 1.0f, 1.0f, mesh->opacity * mesh->opacity));
	}

	u8 cube_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, 1024);
	for(i32 i = 0; i < floor_instances; i++) {
		for(i32 j = 1; j <= game->level.tiles[i]; j++) {
			v3 cube_pos = v3_new(i % floor_length, (f32)j - 1.0f, i / floor_length);
			render_list_draw_model_aligned(list, cube_instance_type, cube_pos);
		}
	}

	switch(game->mode) {
		case GAME_ACTIVE: {
			for(i32 i = 0; i < 2; i++) {
				if(splitscreen || i == 0) {
					GameCamera* camera = &game->cameras[i];
					GamePlayer* player = &game->players[i];
					v3 cam_target = v3_new(camera->offset.x + player->position.x, 0.0f, camera->offset.y + player->position.y);
					//v3 cam_pos = v3_new(player->position.x - camera->offset.x * 1.5f, 5.0f, player->position.y - camera->offset.y * 1.5f);
					v3 cam_pos = v3_new(cam_target.x, 8.0f, cam_target.z - 4.0f + i * 8.0f);
					f32 gap = 0.0025f;
					v4 screen_rect;
					if(splitscreen) {
						screen_rect = v4_new(0.5f * i + gap * i, 0.0f, 0.5f - gap * (1 - i), 1.0f);
					} else {
						screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
					}
					render_list_add_camera(list, cam_pos, cam_target, screen_rect);
				}
			}
		} break;
#if GAME_EDITOR_TOOLS
		case GAME_LEVEL_EDITOR: {
			LevelEditor* editor = &game->level_editor;
			v3 cam_pos = v3_new((f32)editor->camera_position.x, 8.0f, (f32)editor->camera_position.y - 4.0f);
			v3 cam_target = v3_new((f32)editor->camera_position.x, 0.0f, (f32)editor->camera_position.y);
			v4 screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
			render_list_add_camera(list, cam_pos, cam_target, screen_rect);

			v3 cube_pos = v3_new(editor->cursor_x, 0.0f, editor->cursor_y);
			render_list_draw_model_aligned(list, cube_instance_type, cube_pos);
		} break;
#endif
		default: break;
	}

	// TODO: Layouts for splitscreen vs single and others, sets up anchors and
	// positions per item, which are referenced here in the drawing commands.
	StackAllocator ui_stack = stack_init(memory->transient.ui_memory, GAME_UI_MEMSIZE, "UI");

	GamePlayer* pl_primary = &game->players[0];
#define PRINT_VALUES_LEN 2
	f32 print_values[PRINT_VALUES_LEN] = {
		pl_primary->velocity.x,
		pl_primary->velocity.y
	};
	char* print_labels[PRINT_VALUES_LEN] = {
		"vel_x: ",
		"vel_y: "
	};
	v2 debug_screen_anchor = v2_new(1.0f, 0.0f);
	for(i32 i = 0; i < PRINT_VALUES_LEN; i++) {
		char str[256];
		sprintf(str, "%s%.2f", print_labels[i], print_values[i]);

		v2 debug_position = v2_new(32.0f, 12.0f + (PRINT_VALUES_LEN - i) * 24.0f);
		// TODO: anchor should be 1.0f, but is misaligned when we set it to that. Why?
		v2 debug_inner_anchor = v2_new(1.5f, 0.0f);
		TextLinePlacements placements = ui_text_line_placements(game->fonts, ASSET_FONT_MONO_SMALL, str,
			debug_position, debug_inner_anchor, &ui_stack);

		f32 gb_mod = 1.0f;
		for(i32 j = 0; j < placements.len; j++) {
			v2 position = v2_new(placements.x[j], placements.y[j]);
			v4 color = v4_new(0.75f, 0.75f * gb_mod, 0.75f * gb_mod, 1.0f);

			if(str[j] == ':') {
				gb_mod = 0.1f;
			}

			render_list_draw_glyph(list, game->fonts, ASSET_FONT_MONO_SMALL, str[j], position, debug_screen_anchor, color);
		}
	}

	// Score
	for(i32 i = 0; i < 2; i++) {
		v2 position = v2_new(-24.0f + i * 48.0f, -32.0f);
		v2 inner_anchor = v2_new(1.0f * (1.0f - i), 1.0f);
		v2 screen_anchor = v2_new(0.5f, 1.0f);
		v4 color = v4_new(1.0f, 1.0f, 0.0f, 1.0f);
		char buf[16];
		sprintf(buf, "%i", game->players[i].score);
		ui_draw_text_line(list, game->fonts, ASSET_FONT_QUANTICO_LARGE, buf,
			position, inner_anchor, screen_anchor, color, &ui_stack);
	}

	for(i32 i = 0; i < 2; i++) {
		if(!splitscreen && i != 0) continue;

		GamePlayer* player = &game->players[i];
		player->visible_health = lerp(player->visible_health, player->health, 20.0f * dt);
		v4 health_bar_root = v4_new(32.0f, 32.0f, 64.0f, 400.0f);
		render_list_draw_box(list, health_bar_root, v2_new(i * 0.5f, 0.0f), 4.0f, v4_new(1.0f, 1.0f - player->hit_cooldown, 0.0f, 1.0f));
		f32 segments = 40.0f;
		for(i32 j = 0; j < (i32)(player->visible_health * segments); j++) {
			v4 health_bar_sub = health_bar_root;
			health_bar_sub.x += 8.0f;
			health_bar_sub.y += 8.0f + j * ((health_bar_root.w - 8.0f) / segments);
			health_bar_sub.z -= 16.0f;
			health_bar_sub.w = health_bar_root.w / segments - 8.0f;
			render_list_draw_box(list, health_bar_sub, v2_new(i * 0.5f, 0.0f), 4.0f, v4_new(1.0f - j / segments, j / segments, 0.0f, 1.0f));
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
	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* channel = &game->sound_channels[i];
		channel->amplitude = fclamp(channel->amplitude, 0.0f, channel->amplitude);
		channel->pan = fclamp(channel->pan, -1.0f, 1.0f);
		channel->pan = (channel->pan + 1.0f) / 2.0f;

		if(channel->phase > 2.0f * M_PI) {
			channel->phase -= 2.0f * M_PI;
		}

		// NOW: Remove hardcoded sample rate by driving alsa and this from config.h
		channel_rates[i] = 2.0f * M_PI * channel->actual_frequency / 48000;
	}

	for(i32 i = 0; i < samples_count; i++) {
		buffer[i * 2] = 0.0f;
		buffer[i * 2 + 1] = 0.0f;
		for(i32 j = 0; j < GAME_SOUND_CHANNELS_COUNT; j++) {
			GameSoundChannel* channel = &game->sound_channels[j];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);

			channel->phase += channel_rates[j];
			f32 sample = channel->actual_amplitude * sinf(channel->phase);
			sample = fclamp(sample, -channel->shelf, channel->shelf);
			sample += sample * fast_random_f32() * channel->volatility;

			buffer[i * 2] += sample * (1.0f - channel->pan);
			buffer[i * 2 + 1] += sample * channel->pan;
		}

		buffer[i * 2] *= global_attenuation;
		buffer[i * 2] = fclamp(buffer[i * 2], -global_shelf, global_shelf);
		buffer[i * 2 + 1] *= global_attenuation;
		buffer[i * 2 + 1] = fclamp(buffer[i * 2 + 1], -global_shelf, global_shelf);

		if(buffer[i * 2] == global_shelf || buffer[i * 2 + 1] == global_shelf) {
			printf("Global shelf reached (up)\n");
		}
		if(buffer[i * 2] == -global_shelf || buffer[i * 2 + 1] == -global_shelf) {
			printf("Global shelf reached (up)\n");
		}
	}
}
