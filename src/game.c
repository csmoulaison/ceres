#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "renderer/render_list.c"
#include "ui_text.c"
#include "level.c"

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
		player->health = 1.0f;
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

	game->debug_camera.position = v3_new(1.0f, 1.0f, 1.0f);
	game->debug_camera.pitch = 0.0f;
	game->debug_camera.yaw = 90.0f;
	game->debug_camera_mode = false;
	game->debug_camera_moving = false;

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&game->key_mappings_len, sizeof(u32), 1, file);
	for(i32 i = 0; i < game->key_mappings_len; i++) {
		fread(&game->key_mappings[i], sizeof(GameKeyMapping), 1, file);
	}

	for(i32 i = 0; i < 64 * 64; i++) {
		i32 x = i % 64;
		i32 y = i / 64;
		if(x < 12 || x > 51 || y < 12 || y > 51) {
			game->level[i] = 1 + rand() / (RAND_MAX / 3);
		} else {
			game->level[i] = level[i];
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

	if(game->debug_camera_mode) {
		v3 move = v3_zero();
		if(game->debug_camera_moving) {
			if(input_button_down(game->players[0].button_states[BUTTON_FORWARD])) {
				move.x -= 1.0f;
			}
			if(input_button_down(game->players[0].button_states[BUTTON_BACK])) {
				move.x += 1.0f;
			}
			if(input_button_down(game->players[0].button_states[BUTTON_TURN_LEFT])) {
				move.z += 1.0f;
			}
			if(input_button_down(game->players[0].button_states[BUTTON_TURN_RIGHT])) {
				move.z -= 1.0f;
			}
			if(input_button_down(game->players[0].button_states[BUTTON_STRAFE_LEFT])) {
				move.y += 1.0f;
			}
			if(input_button_down(game->players[0].button_states[BUTTON_STRAFE_RIGHT])) {
				move.y -= 1.0f;
			}
		}
		game->debug_camera.position = v3_add(game->debug_camera.position, v3_scale(move, 10.0f * dt));
	}
	
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		// Quit control
		if(input_button_down(player->button_states[BUTTON_QUIT])) {
			output->close_requested = true;
		}

		if(input_button_pressed(player->button_states[BUTTON_DEBUG])) {
			game->debug_camera_mode = !game->debug_camera_mode;
		}

		if(game->debug_camera_mode && input_button_pressed(player->button_states[BUTTON_SHOOT])) {
			game->debug_camera_moving = true;
		}
		if(game->debug_camera_mode && input_button_released(player->button_states[BUTTON_SHOOT])) {
			game->debug_camera_moving = false;
		}

		v2 acceleration = v2_zero();
		v2 direction_vector = player_direction_vector(player);
		f32 rot_acceleration = 0.0f;
		f32 strafe_mod = 0.0f;
		if(!game->debug_camera_moving) {
			// Shooting
			if(player->shoot_cooldown_sound > 0.0f) {
				player->shoot_cooldown_sound = lerp(player->shoot_cooldown_sound, 0.0f, 4.0f * dt);
			}
			if(player->hit_cooldown > 0.0f) {
				player->hit_cooldown = lerp(player->hit_cooldown, 0.0f, 4.0f * dt);
			}

			player->shoot_cooldown -= dt;
			if(player->shoot_cooldown < 0.0f && input_button_down(player->button_states[BUTTON_SHOOT])) {
				player->shoot_cooldown = 0.08f;
				player->shoot_cooldown_sound = 0.99f + ((f32)rand() / RAND_MAX) * 0.01f;

				for(i32 j = 0; j < 2; j++) {
					if(j == i) continue;

					GamePlayer* other = &game->players[j];
					v2 direction = player_direction_vector(player);
					f32 t = v2_dot(direction, v2_sub(other->position, player->position));
					if(t < 0.5f) continue;

					v2 closest = v2_add(player->position, v2_scale(direction, t));
					if(v2_distance_squared(closest, other->position) < 1.66f) {
						other->health -= 0.2f;
						other->hit_cooldown = 1.0f;
						if(other->health <= 0.0f) {
							other->health = 1.0f;
							f32 pos = -18.0f;
							if(i == 0) pos = -pos;
							other->position = v2_new(-pos, pos);
							other->velocity = v2_zero();
							other->hit_cooldown = 1.2f;
						}
					}
				}
			}

			// Calculate ship rotational acceleration
			f32 rotate_speed = 24.0f;
			if(input_button_down(player->button_states[BUTTON_TURN_LEFT])) {
				rot_acceleration += rotate_speed;
			}
			if(input_button_down(player->button_states[BUTTON_TURN_RIGHT])) {
				rot_acceleration -= rotate_speed;
			}

			// Forward/back thruster control
			f32 forward_mod = 0.0f;
			if(input_button_down(player->button_states[BUTTON_FORWARD])) {
				forward_mod += 32.0f;
			}
			if(input_button_down(player->button_states[BUTTON_BACK])) {
				forward_mod -= 16.0f;
			}
			acceleration = v2_scale(direction_vector, forward_mod);

			// Side thruster control
			if(input_button_down(player->button_states[BUTTON_STRAFE_LEFT])) {
				strafe_mod -= 1.0f;
			}
			if(input_button_down(player->button_states[BUTTON_STRAFE_RIGHT])) {
				strafe_mod += 1.0f;
			}
		}

		// Rotational damping
		f32 rot_damping = 1.5f;
		rot_acceleration += player->rotation_velocity * -rot_damping;

		// Apply rotational acceleration
		player->direction = 0.5f * rot_acceleration * dt * dt + player->rotation_velocity * dt + player->direction;
		player->rotation_velocity += rot_acceleration * dt;

		// Calculate ship acceleration

		// Side thruster control
		v2 side_vector = v2_new(-direction_vector.y, direction_vector.x);
		f32 strafe_speed = 32.0f;
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

	i32 mod_phase = 20;
	i32 mod_half = mod_phase / 2;
#endif

#if 1
	GameSoundChannel* music_channel = &game->sound_channels[player_channels * 2];
	f32 frequencies[32] = { 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 0, 2, 0, 2, 0, 1, 0, 2, 0, 2, 0, 1 };
	f32 freq = frequencies[(game->frame / mod_half) % 16];
	music_channel->amplitude = 4000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 2000.0f;
	music_channel->frequency = freq * 100.0f;
	music_channel->shelf = 4000.0f;
	music_channel->volatility = freq * 0.05f - (game->frame % mod_half) * freq * 0.1f;

	GameSoundChannel* ch2 = &game->sound_channels[player_channels * 2 + 1];
	ch2->amplitude = 5000.0f - (game->frame % mod_half) * 100.0f * freq + current_player_velocity * 1000.0f;
	ch2->frequency = freq * 33.3f;
	ch2->shelf = 4000.0f;
	ch2->volatility = freq * 0.05f;

	GameSoundChannel* ch4 = &game->sound_channels[player_channels * 2 + 2];
	f32 fs4[16] = { 6, 1, 0, 4, 0, 0, 3, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f4 = fs4[(game->frame / mod_phase) % 16];
	ch4->amplitude = 100.0f + current_player_velocity * 400.0f - (game->frame % mod_phase) * 4000.0f;
	ch4->frequency = f4 * ((f32)rand() / RAND_MAX) * 400.0f;
	ch4->shelf = 4000.0f + current_player_velocity * 50.0f;
	ch4->volatility = f4 * 0.05f - (game->frame % mod_phase) * 0.01f;
#endif

#if 1
	GameSoundChannel* ch3 = &game->sound_channels[player_channels * 2 + 3];
	f32 fs3[16] = { 4, 0, 2, 0, 6, 0, 2, 0, 4, 0, 2, 1, 6, 1, 2, 1 };
	f32 f3 = fs3[(game->frame / mod_phase) % 16];
	ch3->amplitude = 2000.0f * f3 - (game->frame % mod_phase) * 8000.0f;
	ch3->frequency = f3 * ((f32)rand() / RAND_MAX) * 25.0f;
	ch3->shelf = 1000.0f + current_player_velocity * 100.0f;
	ch3->volatility = f3 * 0.6f - (game->frame % mod_phase) * 0.2f;
#endif

	// Populate render list
	RenderList* list = &output->render_list;
	render_list_init(list);

	u8 ship_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, 2);
	u8 laser_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_CYLINDER, 0, 64);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	bool splitscreen = false;
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		f32 tilt = player->strafe_tilt + clamp(player->rotation_velocity, -90.0f, 90.0f) * 0.05f;
		v3 rot = v3_new(-tilt, player->direction, 0.0f);
		render_list_draw_model(list, ship_instance_type, pos, rot);

		if(!game->debug_camera_mode && (splitscreen || i == 0)) {
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
		if(game->debug_camera_mode) {
			//render_list_add_camera(list, game->debug_camera.position, v3_add(game->debug_camera.position, debug_cam_direction), v4_new(0.0f, 0.0f, 1.0f, 1.0f));
			render_list_add_camera(list, game->debug_camera.position, v3_add(game->debug_camera.position, v3_new(-1.0f, 0.0f, 0.0f)), v4_new(0.0f, 0.0f, 1.0f, 1.0f));
		}

		v2 direction = player_direction_vector(player);
		v2 side = v2_new(-direction.y, direction.x);
		v2 target = v2_add(player->position, v2_scale(direction, 50.0f));

		v2 laser_pos_1 = v2_add(v2_add(player->position, v2_scale(side, 0.48f)), v2_scale(direction, 0.30f));
		v2 laser_pos_2 = v2_add(v2_add(player->position, v2_scale(side, -0.48f)), v2_scale(direction, 0.30f));

		render_list_draw_laser(list, laser_instance_type, v3_new(laser_pos_1.x, 0.5f, laser_pos_1.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);
		render_list_draw_laser(list, laser_instance_type, v3_new(laser_pos_2.x, 0.5f, laser_pos_2.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);
	}


	i32 floor_length = 64;
	i32 floor_instances = floor_length * floor_length;
	u8 floor_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_instances);
	for(i32 i = 0; i < floor_instances; i++) {
		v3 floor_pos = v3_new(-32.5f + (i % floor_length), 0.0f, -32.5f + (i / floor_length));
		v3 floor_rot = v3_new(0.0f , 0.0f, 0.0f);
		render_list_draw_model(list, floor_instance_type, floor_pos, floor_rot);
	}


	u8 cube_instance_type = render_list_allocate_instance_type(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, 1024);
	for(i32 i = 0; i < floor_instances; i++) {
		for(i32 j = 1; j <= game->level[i]; j++) {
			v3 floor_pos = v3_new(-32.0f + (i % floor_length), (f32)j - 1.0f, -32.5f + (i / floor_length));
			render_list_draw_model(list, cube_instance_type, floor_pos, v3_zero());
		}
	}

	Arena ui_arena;
	arena_init(&ui_arena, MEGABYTE, NULL, "UI");

	GamePlayer* pl_primary = &game->players[0];
#define PRINT_VALUES_LEN 6
	f32 print_values[PRINT_VALUES_LEN] = {
		game->debug_camera.position.x,
		game->debug_camera.position.y,
		pl_primary->velocity.x,
		pl_primary->velocity.y,
		game->debug_camera_mode,
		game->debug_camera_moving,
	};
	char* print_labels[PRINT_VALUES_LEN] = {
		"dbg cam pos_x: ",
		"dbg cam pos_y: ",
		"vel_x: ",
		"vel_y: ",
		"dbg mode: ",
		"dbg moving: "
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
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_LARGE, "Level editing",
		title_position, title_inner_anchor, title_screen_anchor, color, &ui_arena);

	v4 color_neu = v4_new(0.4f, 0.7f, 0.5f, 1.0f);
	title_position.y -= 64.0f;
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_REGULAR, "Next up: collision handling.",
		title_position, title_inner_anchor, title_screen_anchor, color_neu, &ui_arena);

	arena_destroy(&ui_arena);
		
	game->frame++;
}

GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples) {
	GameState* game = &memory->state;

	// NOW: Figure out a good max amplitude for limiting
	f32 global_shelf = 30000.0f;
	f32 global_attenuation = 0.62f;

	for(i32 i = 0; i < GAME_SOUND_CHANNELS_COUNT; i++) {
		GameSoundChannel* channel = &game->sound_channels[i];
		channel->amplitude = clamp(channel->amplitude, 0.0f, channel->amplitude);
	}

	for(i32 i = 0; i < samples_count; i++) {
		buffer[i * 2] = 0.0f;
		buffer[i * 2 + 1] = 0.0f;
		for(i32 j = 0; j < GAME_SOUND_CHANNELS_COUNT; j++) {
		//for(i32 j = 0; j < 4; j++) {
			GameSoundChannel* channel = &game->sound_channels[j];
			channel->actual_frequency = lerp(channel->actual_frequency, channel->frequency, 0.01f);
			channel->actual_amplitude = lerp(channel->actual_amplitude, channel->amplitude, 0.01f);
			// NOW: Remove hardcoded sample rate.
			channel->phase += 2.0f * M_PI * channel->actual_frequency / 48000;
			if(channel->phase > 2.0f * M_PI) {
				channel->phase -= 2.0f * M_PI;
			}
			f32 sample = channel->actual_amplitude * sinf(channel->phase);
			if(sample > channel->shelf) sample = channel->shelf;
			if(sample < -channel->shelf) sample = -channel->shelf;
			sample += sample * ((f32)rand() / RAND_MAX) * channel->volatility;

			channel->pan = clamp(channel->pan, -1.0f, 1.0f);
			f32 actual_pan = (channel->pan + 1.0f) / 2.0f;
			assert(actual_pan >= 0.0f && actual_pan <= 1.0f);
			buffer[i * 2] += sample * global_attenuation * (1.0f - actual_pan);
			buffer[i * 2 + 1] += sample * global_attenuation * actual_pan;
		}
		if(buffer[i * 2] > global_shelf) buffer[i * 2] = global_shelf;
		if(buffer[i * 2 + 1] > global_shelf) buffer[i * 2 + 1] = global_shelf;
		if(buffer[i * 2] < -global_shelf) buffer[i * 2] = -global_shelf;
		if(buffer[i * 2 + 1] < -global_shelf) buffer[i * 2 + 1] = -global_shelf;

		if(buffer[i * 2] == global_shelf || buffer[i * 2 + 1] == global_shelf) {
			printf("Global shelf reached (up)\n");
		}
		if(buffer[i * 2] == -global_shelf || buffer[i * 2 + 1] == -global_shelf) {
			printf("Global shelf reached (up)\n");
		}
	}

}
