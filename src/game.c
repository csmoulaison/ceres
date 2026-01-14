#define INPUT_MAX_PLAYERS 2
#define MAX_KEY_MAPPINGS NUM_BUTTONS * 8

#define INPUT_DOWN_BIT 0b00000001
#define INPUT_PRESSED_BIT 0b00000010
#define INPUT_RELEASED_BIT 0b00000100

typedef enum {
	BUTTON_FORWARD = 0,
	BUTTON_BACK,
	BUTTON_TURN_LEFT,
	BUTTON_TURN_RIGHT,
	BUTTON_STRAFE_LEFT,
	BUTTON_STRAFE_RIGHT,
	BUTTON_QUIT,
	NUM_BUTTONS
} ButtonType;

typedef u8 ButtonState;

typedef struct {
	f32 ship_direction;
	f32 ship_rotation_velocity;
	f32 ship_position[2];
	f32 ship_velocity[2];
	ButtonState button_states[NUM_BUTTONS];
} GamePlayer;

typedef struct {
	u64 key_id;
	u8 player_index;
	ButtonType button_type;
} GameKeyMapping;

typedef struct {
	bool close_requested;
	GamePlayer players[2];
	f32 camera_offset[2];
	GameKeyMapping key_mappings[MAX_KEY_MAPPINGS];
	u32 key_mappings_len;
} Game;

Game* game_init(Arena* arena) {
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));
	game->close_requested = false;
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		player->ship_direction = 0.0f;
		player->ship_rotation_velocity = 0.0f;
		v2_zero(player->ship_position);
		v2_zero(player->ship_velocity);

		for(i32 i = 0; i < NUM_BUTTONS; i++) {
			player->button_states[i] = 0;
		}
	}

	game->key_mappings[0] = (GameKeyMapping){ .key_id = 0xff52, .player_index = 0, .button_type = BUTTON_FORWARD };
	game->key_mappings[1] = (GameKeyMapping){ .key_id = 0xff54, .player_index = 0, .button_type = BUTTON_BACK };
	game->key_mappings[2] = (GameKeyMapping){ .key_id = 0xff51, .player_index = 0, .button_type = BUTTON_TURN_LEFT };
	game->key_mappings[3] = (GameKeyMapping){ .key_id = 0xff53, .player_index = 0, .button_type = BUTTON_TURN_RIGHT };
	game->key_mappings[4] = (GameKeyMapping){ .key_id = 0xff55, .player_index = 0, .button_type = BUTTON_STRAFE_LEFT };
	game->key_mappings[5] = (GameKeyMapping){ .key_id = 0xff56, .player_index = 0, .button_type = BUTTON_STRAFE_RIGHT };
	game->key_mappings[6] = (GameKeyMapping){ .key_id = 0xff1b, .player_index = 0, .button_type = BUTTON_QUIT };
	game->key_mappings[7] = (GameKeyMapping){ .key_id = 0x0077, .player_index = 1, .button_type = BUTTON_FORWARD };
	game->key_mappings[8] = (GameKeyMapping){ .key_id = 0x0073, .player_index = 1, .button_type = BUTTON_BACK };
	game->key_mappings[9] = (GameKeyMapping){ .key_id = 0x0061, .player_index = 1, .button_type = BUTTON_TURN_LEFT };
	game->key_mappings[10] = (GameKeyMapping){ .key_id = 0x0064, .player_index = 1, .button_type = BUTTON_TURN_RIGHT };
	game->key_mappings[11] = (GameKeyMapping){ .key_id = 0x0071, .player_index = 1, .button_type = BUTTON_STRAFE_LEFT };
	game->key_mappings[12] = (GameKeyMapping){ .key_id = 0x0065, .player_index = 1, .button_type = BUTTON_STRAFE_RIGHT };
	game->key_mappings[13] = (GameKeyMapping){ .key_id = 0xff1b, .player_index = 1, .button_type = BUTTON_QUIT };
	game->key_mappings_len = 14;

	v2_zero(game->camera_offset);

	return game;
}

f32 apply_friction(f32 v, f32 f, f32 dt) {
	if(v > 0.0f) {
		v -= f * dt;
		if(v < 0.0f) {
			v = 0.0f;
		}
	}
	if(v < 0.0f) {
		v += f * dt;
		if(v > 0.0f) {
			v = 0.0f;
		}
	}
	return v;
}

bool input_button_down(ButtonState button) {
	return button & INPUT_DOWN_BIT;
}

bool input_button_pressed(ButtonState button) {
	return button & INPUT_PRESSED_BIT;
}

bool input_button_released(ButtonState button) {
	return button & INPUT_RELEASED_BIT;
}

void player_direction_vector(f32* dst, GamePlayer* player) {
	v2_init(dst, sin(player->ship_direction), cos(player->ship_direction));
	v2_normalize(dst, dst);
}

RenderList game_update(Game* game, Platform* platform, f32 dt) {
	for(u32 i = 0; i < NUM_BUTTONS; i++) {
		game->players[0].button_states[i] = game->players[0].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
		game->players[1].button_states[i] = game->players[1].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}
	PlatformEvent* event;
	while((event = platform_poll_next_event(platform)) != NULL) {
		switch(event->type) {
			// Issue3 - NOW: Put these into a format associating buttons with
			// (multiple) keysyms, then load that format from a file, then create a
			// simple tool for editing that file by capturing button presses.
			// 
			// Eventually we need to support controller input.
			case PLATFORM_EVENT_KEY_DOWN: {
				// TODO: Replace this and the KEY_RELEASE case with a hash table or some
				// other fast solution.
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
			case PLATFORM_EVENT_KEY_UP: {
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
	}
	
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		// Quit control
		if(input_button_down(player->button_states[BUTTON_QUIT])) {
			game->close_requested = true;
		}

		// Rotation thruster control
		f32 rotate_speed = 32.0f;
		if(input_button_down(player->button_states[BUTTON_TURN_LEFT]))
			player->ship_rotation_velocity += rotate_speed * dt;
		if(input_button_down(player->button_states[BUTTON_TURN_RIGHT]))
			player->ship_rotation_velocity -= rotate_speed * dt;

		f32 rotate_max_speed = 12.0f;
		f32 rotate_friction = 8.0f;
		player->ship_rotation_velocity = apply_friction(player->ship_rotation_velocity, rotate_friction, dt);
		if(player->ship_rotation_velocity > rotate_max_speed)
			player->ship_rotation_velocity = rotate_max_speed;
		if(player->ship_rotation_velocity < -rotate_max_speed)
			player->ship_rotation_velocity = -rotate_max_speed;
		player->ship_direction += player->ship_rotation_velocity * dt;

		// Forward/back thruster control
		f32 direction_vector[2];
		player_direction_vector(direction_vector, player);

		f32 forward_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_FORWARD]))
			forward_mod += 0.3f;
		if(input_button_down(player->button_states[BUTTON_BACK]))
			forward_mod -= 0.2f;
		f32 forward[2];
		v2_copy(forward, direction_vector);
		v2_scale(forward, forward_mod);
		v2_add(player->ship_velocity, player->ship_velocity, forward);

		// Side thruster control
		f32 side_vector[2];
		v2_init(side_vector, -direction_vector[1], direction_vector[0]);

		f32 strafe_speed = 0.3f;
		f32 strafe_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_STRAFE_LEFT]))
			strafe_mod -= 1.0f;
		if(input_button_down(player->button_states[BUTTON_STRAFE_RIGHT]))
			strafe_mod += 1.0f;
		f32 strafe[2];
		v2_copy(strafe, side_vector);
		v2_scale(strafe, strafe_mod * strafe_speed);
		v2_add(player->ship_velocity, player->ship_velocity, strafe);

		// Normalize to max veloity and apply friction.
		f32 velocity_normalized[2];
		v2_normalize(player->ship_velocity, velocity_normalized);

		f32 velocity_mag = v2_magnitude(player->ship_velocity);
		f32 max_speed = 14.0f;
		if(velocity_mag > max_speed) {
			v2_copy(player->ship_velocity, velocity_normalized);
			v2_scale(player->ship_velocity, max_speed);
		}

		// NOW: Move this above normalization?
		f32 movement_friction = 6.0f;
		if(velocity_mag > 0.02f) {
			f32 friction[2];
			v2_copy(friction, velocity_normalized);
			v2_scale(friction, -movement_friction * dt);
			v2_add(player->ship_velocity, player->ship_velocity, friction);
		} else {
			v2_zero(player->ship_velocity);
		}

		f32 delta_velocity[2];
		v2_copy(delta_velocity, player->ship_velocity);
		v2_scale(delta_velocity, dt);
		v2_add(player->ship_position, player->ship_position, delta_velocity);
	}

	// Camera control
	GamePlayer* primary_player = &game->players[0];
	f32 camera_lookahead = 4.0f;
	f32 camera_target_position[2];
	f32 camera_target_offset[2];
	f32 direction_vector[2];
	player_direction_vector(direction_vector, primary_player);
	v2_copy(camera_target_offset, direction_vector);
	v2_scale(camera_target_offset, camera_lookahead);

	f32 camera_speed_mod = 2.0f;
	f32 camera_target_delta[2];	
	v2_copy(camera_target_delta, camera_target_offset);
	v2_sub(camera_target_delta, camera_target_offset, game->camera_offset);
	v2_scale(camera_target_delta, camera_speed_mod * dt);
	v2_add(game->camera_offset, game->camera_offset, camera_target_delta);

	// Populate render list
	RenderList list;
	render_list_init(&list);

	f32 clear_color[3];
	v3_init(clear_color, 0.1f, 0.1f, 0.2f);
	f32 cam_target[3];
	v3_init(cam_target, game->camera_offset[0] + primary_player->ship_position[0], 0.0f, game->camera_offset[1] + primary_player->ship_position[1]);
	f32 cam_pos[3];
	v3_init(cam_pos, cam_target[0] + 4.0f, 8.0f, cam_target[2]);
	render_list_update_world(&list, clear_color, cam_pos, cam_target);

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		f32 ship_pos[3];
		v3_init(ship_pos, player->ship_position[0], 0.5f, player->ship_position[1]);
		f32 ship_rot[3];
		f32 ship_tilt = clamp(player->ship_rotation_velocity, -12.0f, 12.0f);
		v3_init(ship_rot, ship_tilt * -0.1f, player->ship_direction, 0.0f);
		render_list_draw_model(&list, 0, 0, ship_pos, ship_rot);
	}

	i32 floor_instances = 1024;
	for(i32 i = 0; i < floor_instances; i++) {
		f32 floor_pos[3];
		v3_init(floor_pos, -15.5f + (i % 32), 0.0f, -15.5f + (i / 32));
		f32 floor_rot[3];
		v3_zero(floor_rot);
		render_list_draw_model(&list, 1, 1, floor_pos, floor_rot);
	}
	return list;
}
