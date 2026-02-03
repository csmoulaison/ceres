#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "renderer/render_list.c"
#include "ui_text.c"

v2 player_direction_vector(GamePlayer* player) {
	return v2_normalize(v2_new(sin(player->direction), cos(player->direction)));
}

GAME_INIT(game_init) {
	GameState* game = &memory->state;
	game->frame = 0;

	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		FontData* font = &game->fonts[i];
		FontAsset* f_asset = (FontAsset*)&asset_pack->buffer[asset_pack->font_buffer_offsets[i]];
		TextureAsset* t_asset = (TextureAsset*)&asset_pack->buffer[asset_pack->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		player->direction = 0.0f;
		player->rotation_velocity = 0.0f;
		player->position = v2_zero();
		player->velocity = v2_zero();
		player->momentum_cooldown_sound = 0.0f;

		for(i32 i = 0; i < NUM_BUTTONS; i++) {
			player->button_states[i] = 0;
		}

		GameCamera* camera = &game->cameras[i];
		camera->offset = v2_zero();
	}

	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* channel = &game->sound_channels[i];
		channel->phase = 0.0f;
		channel->frequency = 0.0f;
		channel->amplitude = 0.0f;
		channel->shelf = 0.0f;
		channel->volatility = 0.0f;
		channel->actual_frequency = 0.0f;
		channel->actual_amplitude = 0.0f;
	}

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&game->key_mappings_len, sizeof(u32), 1, file);
	for(i32 i = 0; i < game->key_mappings_len; i++) {
		fread(&game->key_mappings[i], sizeof(GameKeyMapping), 1, file);
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
	
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		// Quit control
		if(input_button_down(player->button_states[BUTTON_QUIT])) {
			output->close_requested = true;
		}

		// Shooting
		if(player->shoot_cooldown_sound > 0.0f) {
			player->shoot_cooldown_sound = lerp(player->shoot_cooldown_sound, 0.0f, 4.0f * dt);
		}
		if(input_button_pressed(player->button_states[BUTTON_SHOOT])) {
			player->shoot_cooldown_sound = 1.0f;
		}

		// Calculate ship rotational acceleration
		f32 rotate_speed = 32.0f;
		f32 rot_acceleration = 0.0f;
		if(input_button_down(player->button_states[BUTTON_TURN_LEFT])) {
			rot_acceleration += rotate_speed;
		}
		if(input_button_down(player->button_states[BUTTON_TURN_RIGHT])) {
			rot_acceleration -= rotate_speed;
		}

		// Rotational damping
		f32 rot_damping = 2.0f;
		rot_acceleration += player->rotation_velocity * -rot_damping;

		// Apply rotational acceleration
		player->direction = 0.5f * rot_acceleration * dt * dt + player->rotation_velocity * dt + player->direction;
		player->rotation_velocity += rot_acceleration * dt;

		// Calculate ship acceleration
		// Forward/back thruster control
		v2 acceleration = v2_zero();

		v2 direction_vector = player_direction_vector(player);
		f32 forward_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_FORWARD])) {
			forward_mod += 32.0f;
		}
		if(input_button_down(player->button_states[BUTTON_BACK])) {
			forward_mod -= 16.0f;
		}
		acceleration = v2_scale(direction_vector, forward_mod);

		// Side thruster control
		v2 side_vector = { -direction_vector.y, direction_vector.x };
		f32 strafe_speed = 32.0f;
		f32 strafe_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_STRAFE_LEFT])) {
			strafe_mod -= 1.0f;
		}
		if(input_button_down(player->button_states[BUTTON_STRAFE_RIGHT])) {
			strafe_mod += 1.0f;
		}
		acceleration = v2_add(acceleration, v2_scale(side_vector, strafe_mod * strafe_speed));

		// TODO: Make strafing tilt curve based off of the same equations as motion 
		f32 strafe_tilt_target = -strafe_mod * 0.66f;
		player->strafe_tilt = lerp(player->strafe_tilt, strafe_tilt_target, dt * 6.0f);

		// Damp acceleration
		f32 damping = 1.4f;
		acceleration = v2_add(acceleration, v2_scale(player->velocity, -damping));

		// Apply acceleration to position
		// (acceleration / 2) * dt^2 + velocity * t + position
		v2 accel_dt = v2_scale(v2_scale(acceleration, 0.5f), dt * dt);
		v2 velocity_dt = v2_scale(player->velocity, dt);
		player->position = v2_add(player->position, v2_add(accel_dt, velocity_dt));

		// Update player velocity
		player->velocity = v2_add(player->velocity, v2_scale(acceleration, dt));

		f32 ship_energy = v2_magnitude(player->velocity) + abs(player->rotation_velocity);
		if(player->momentum_cooldown_sound < ship_energy) {
			player->momentum_cooldown_sound = lerp(player->momentum_cooldown_sound, ship_energy, 10.0f * dt);
		} else {
			player->momentum_cooldown_sound = lerp(player->momentum_cooldown_sound, 0.0f, 1.0f * dt);
		}

		// Update camera
		// 
		// TODO: Not every player will necessarily get a camera in the future. Bots
		// and online play, for instance.
		GameCamera* camera = &game->cameras[i];
		f32 camera_lookahead = 4.0f;
		v2 camera_target_offset = v2_scale(direction_vector, camera_lookahead);

		f32 camera_speed_mod = 2.0f;
		v2 camera_target_delta = v2_sub(camera_target_offset, camera->offset);
		camera_target_delta = v2_scale(camera_target_delta, camera_speed_mod * dt);
		camera->offset = v2_add(camera->offset, camera_target_delta);
	}

	// Update sound channels
	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* ch = &game->sound_channels[i];
		ch->amplitude = 0.0f;
		ch->frequency = 0.0f;
		ch->volatility = 0.0f;
	}
	
	GameSoundChannel* velocity_channel = &game->sound_channels[0];
	f32 v_mag = v2_magnitude(game->players[0].velocity);
	velocity_channel->amplitude = 2000.0f * v_mag;
	velocity_channel->frequency = 7.144123f * v_mag;
	velocity_channel->shelf = 6000.0f;
	velocity_channel->volatility = 0.002f * v_mag;

	GameSoundChannel* momentum_channel = &game->sound_channels[1];
	f32 m = game->players[0].momentum_cooldown_sound;
	momentum_channel->amplitude = 1000.0f * m;
	momentum_channel->frequency = 3.0f * m;
	momentum_channel->shelf = 1000.0f;
	momentum_channel->volatility = 0.2f * m;

	GameSoundChannel* rotation_channel = &game->sound_channels[2];
	f32 rot = game->players[0].rotation_velocity;
	rotation_channel->amplitude = 1000.0f * rot;
	rotation_channel->frequency = 10.0f * abs(rot);
	rotation_channel->shelf = 4000.0f;
	rotation_channel->volatility = 0.002f * rot;

	GameSoundChannel* shoot_channel = &game->sound_channels[3];
	f32 shoot = game->players[0].shoot_cooldown_sound;
	rotation_channel->amplitude = 10000.0f * shoot * shoot;
	rotation_channel->frequency = 1000.0f * shoot * shoot;
	rotation_channel->shelf = 6000.0f;
	rotation_channel->volatility = 0.1f * shoot;

#if 1
	i32 mod_phase = 6;
	i32 mod_half = mod_phase / 2;
	GameSoundChannel* music_channel = &game->sound_channels[4];
	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(game->frame / mod_half) % 16];
	music_channel->amplitude = 7000.0f - (game->frame % mod_half) * 100.0f * freq;
	music_channel->frequency = freq * 100.0f;
	music_channel->shelf = 5000.0f;
	music_channel->volatility = freq * 0.4f;

	GameSoundChannel* ch2 = &game->sound_channels[5];
	ch2->amplitude = 7000.0f - (game->frame % mod_half) * 100.0f * freq;
	ch2->frequency = freq * 33.3f;
	ch2->shelf = 4000.0f;
	ch2->volatility = freq * 0.1f;

	GameSoundChannel* ch3 = &game->sound_channels[6];
	f32 fs3[16] = { 4, 0, 1, 0, 8, 0, 1, 0, 4, 0, 1, 1, 8, 1, 2, 1 };
	f32 f3 = fs3[(game->frame / mod_phase) % 16];
	ch3->amplitude = 1000.0f * f3 + 4000.0f - (game->frame % mod_phase) * 1000.0f;
	ch3->frequency = f3 * ((f32)rand() / RAND_MAX) * 25.0f;
	ch3->shelf = 4000.0f;
	ch3->volatility = f3 * 0.2f;
#endif

	// Populate render list
	RenderList* list = &output->render_list;
	render_list_init(list);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	bool splitscreen = false;
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		f32 tilt = player->strafe_tilt + clamp(player->rotation_velocity, -90.0f, 90.0f) * 0.05f;
		v3 rot = v3_new(-tilt, player->direction, 0.0f);
		render_list_draw_model(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, pos, rot);

		if(splitscreen || i == 0) {
			GameCamera* camera = &game->cameras[i];
			v3 cam_target = v3_new(camera->offset.x + player->position.x, 0.0f, camera->offset.y + player->position.y);
			v3 cam_pos = v3_new(cam_target.x + 4.0f, 8.0f, cam_target.z);
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

	i32 floor_instances = 1024;
	for(i32 i = 0; i < floor_instances; i++) {
		v3 floor_pos = v3_new(-15.5f + (i % 32), 0.0f, -15.5f + (i / 32));
		v3 floor_rot = v3_new(0.0f , 0.0f, 0.0f);
		render_list_draw_model(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos, floor_rot);
	}

	Arena ui_arena;
	arena_init(&ui_arena, MEGABYTE, NULL, "UI");

	GamePlayer* pl_primary = &game->players[0];
	GameCamera* cam_primary = &game->cameras[0];
#define PRINT_VALUES_LEN 5
	f32 print_values[PRINT_VALUES_LEN] = {
		pl_primary->position.x,
		pl_primary->position.y,
		pl_primary->velocity.x,
		pl_primary->velocity.y,
		pl_primary->momentum_cooldown_sound
	};
	char* print_labels[PRINT_VALUES_LEN] = {
		"pos_x: ",
		"pos_y: ",
		"vel_x: ",
		"vel_y: ",
		"col_s: "
	};
	v2 debug_screen_anchor = v2_zero();
	for(i32 i = 0; i < PRINT_VALUES_LEN; i++) {
		char str[256];
		sprintf(str, "%s%.2f", print_labels[i], print_values[i]);

		v2 debug_position = v2_new(32.0f, 12.0f + (PRINT_VALUES_LEN - i) * 24.0f);
		v2 debug_inner_anchor = v2_zero();
		TextLinePlacements placements = ui_text_line_placements(game->fonts, ASSET_FONT_MONO_SMALL, str,
			debug_position, debug_inner_anchor, &ui_arena);

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

	v4 color = v4_new(0.4f, 0.5f, 0.7f, 1.0f);
	v2 title_position = v2_new(32.0f, -32.0f);
	v2 title_inner_anchor = v2_new(0.0f, 1.0f);
	v2 title_screen_anchor = v2_new(0.0f, 1.0f);
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_LARGE, "Sector Seven or some shit",
		title_position, title_inner_anchor, title_screen_anchor, color, &ui_arena);

	v4 color_neu = v4_new(0.4f, 0.7f, 0.5f, 1.0f);
	title_position.y -= 64.0f;
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_REGULAR, "A game about love, life, and loss.",
		title_position, title_inner_anchor, title_screen_anchor, color_neu, &ui_arena);

	arena_destroy(&ui_arena);
		
	game->frame++;
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	GameState* game = &memory->state;

	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;

	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* channel = &game->sound_channels[i];
		//channel->actual_frequency = channel->frequency;
		//channel->actual_amplitude = channel->amplitude;
	}

	for(i32 i = 0; i < samples_count; i++) {
		buffer[i * 2] = 0.0f;
		buffer[i * 2 + 1] = 0.0f;
		for(i32 j = 0; j < GAME_SOUND_CHANNELS_COUNT; j++) {
		//for(i32 j = 0; j < 4; j++) {
			GameSoundChannel* channel = &game->sound_channels[j];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);
			// NOW: Add panning and shelf. Remove hardcoded sample rate, obviously.
			channel->phase += 2.0f * M_PI * channel->actual_frequency / 48000;
			if(channel->phase > 2.0f * M_PI) {
				channel->phase -= 2.0f * M_PI;
			}
			f32 sample = channel->actual_amplitude * sinf(channel->phase);
			if(sample > channel->shelf) sample = channel->shelf;
			if(sample < -channel->shelf) sample = -channel->shelf;
			sample += sample * ((f32)rand() / RAND_MAX) * channel->volatility;
			buffer[i * 2] += sample;
			buffer[i * 2 + 1] += sample;
		}
		if(buffer[i * 2] > global_shelf) buffer[i * 2] = global_shelf;
		if(buffer[i * 2 + 1] > global_shelf) buffer[i * 2 + 1] = global_shelf;
		if(buffer[i * 2] < -global_shelf) buffer[i * 2] = -global_shelf;
		if(buffer[i * 2 + 1] < -global_shelf) buffer[i * 2 + 1] = -global_shelf;
	}

}
