#define INPUT_MAX_PLAYERS 2
// NOTE: PLAYER_INPUT_BUTTONS_LEN must match the amount of buttons in the
// GamePlayer struct.
#define PLAYER_INPUT_BUTTONS_LEN 6

#define INPUT_DOWN_BIT 0b00000001
#define INPUT_PRESSED_BIT 0b00000010
#define INPUT_RELEASED_BIT 0b00000100

typedef u8 ButtonState;

typedef struct {
	f32 ship_direction;
	f32 ship_rotation_velocity;
	f32 ship_position[2];
	f32 ship_velocity[2];

	// NOTE: The number of buttons here must match PLAYER_INPUT_BUTTONS_LEN.
	union {
		struct {
			ButtonState up;
			ButtonState down;
			ButtonState turn_left;
			ButtonState turn_right;
			ButtonState strafe_left;
			ButtonState strafe_right;
		};
		ButtonState button_states[PLAYER_INPUT_BUTTONS_LEN];
	};
} GamePlayer;

typedef struct {
	bool close_requested;
	GamePlayer players[2];
	f32 camera_offset[2];
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

		player->up = false;
		player->down = false;
		player->turn_left = false;
		player->turn_right = false;
		player->strafe_left = false;
		player->strafe_right = false;
	}
	v2_zero(game->camera_offset);
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

void handle_button_down_event(ButtonState* button) {
	if((*button) & INPUT_DOWN_BIT) {
		return;
	}
	*button = (*button) | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
}

void handle_button_up_event(ButtonState* button) {
	if((*button) & INPUT_DOWN_BIT) {
		*button = INPUT_RELEASED_BIT;
	}
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
	for(u32 i = 0; i < PLAYER_INPUT_BUTTONS_LEN; i++) {
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
			case PLATFORM_EVENT_BUTTON_DOWN: {
				switch(*((u64*)event->data)) {
					// quit
					case 0xff1b: { 
						game->close_requested = true;
					} break;
					// up
					case 0xff52: {
						handle_button_down_event(&game->players[1].up);
					} break;
					// down
					case 0xff54: {
						handle_button_down_event(&game->players[1].down);
					} break;
					// left
					case 0xff51: {
						handle_button_down_event(&game->players[1].turn_left);
					} break;
					// right
					case 0xff53: {
						handle_button_down_event(&game->players[1].turn_right);
					} break;
					// pageUp
					case 0xff55: {
						handle_button_down_event(&game->players[1].strafe_left);
					} break;
					// pageDown
					case 0xff56: {
						handle_button_down_event(&game->players[1].strafe_right);
					} break;
					// w
					case 0x0077: {
						handle_button_down_event(&game->players[0].up);
					} break;
					// s
					case 0x0073: {
						handle_button_down_event(&game->players[0].down);
					} break;
					// a
					case 0x0061: {
						handle_button_down_event(&game->players[0].turn_left);
					} break;
					// d
					case 0x0064: {
						handle_button_down_event(&game->players[0].turn_right);
					} break;
					// q
					case 0x0071: {
						handle_button_down_event(&game->players[0].strafe_left);
					} break;
					// e
					case 0x0065: {
						handle_button_down_event(&game->players[0].strafe_right);
					} break;
					default: break;
				}
			} break;
			case PLATFORM_EVENT_BUTTON_UP: {
				switch(*((u64*)event->data)) {
					// quit
					case 0xff1b: { 
						game->close_requested = true;
					} break;
					// up
					case 0xff52: {
						handle_button_up_event(&game->players[1].up);
					} break;
					// down
					case 0xff54: {
						handle_button_up_event(&game->players[1].down);
					} break;
					// left
					case 0xff51: {
						handle_button_up_event(&game->players[1].turn_left);
					} break;
					// right
					case 0xff53: {
						handle_button_up_event(&game->players[1].turn_right);
					} break;
					// pageUp
					case 0xff55: {
						handle_button_up_event(&game->players[1].strafe_left);
					} break;
					// pageDown
					case 0xff56: {
						handle_button_up_event(&game->players[1].strafe_right);
					} break;
					// w
					case 0x0077: {
						handle_button_up_event(&game->players[0].up);
					} break;
					// s
					case 0x0073: {
						handle_button_up_event(&game->players[0].down);
					} break;
					// a
					case 0x0061: {
						handle_button_up_event(&game->players[0].turn_left);
					} break;
					// d
					case 0x0064: {
						handle_button_up_event(&game->players[0].turn_right);
					} break;
					// q
					case 0x0071: {
						handle_button_up_event(&game->players[0].strafe_left);
					} break;
					// e
					case 0x0065: {
						handle_button_up_event(&game->players[0].strafe_right);
					} break;
					default: break;
				}
			} break;
			default: break;
		}
	}
	
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		f32 rotate_speed = 32.0f;
		if(input_button_down(player->turn_left))
			player->ship_rotation_velocity += rotate_speed * dt;
		if(input_button_down(player->turn_right))
			player->ship_rotation_velocity -= rotate_speed * dt;

		f32 rotate_max_speed = 12.0f;
		f32 rotate_friction = 8.0f;
		player->ship_rotation_velocity = apply_friction(player->ship_rotation_velocity, rotate_friction, dt);
		if(player->ship_rotation_velocity > rotate_max_speed)
			player->ship_rotation_velocity = rotate_max_speed;
		if(player->ship_rotation_velocity < -rotate_max_speed)
			player->ship_rotation_velocity = -rotate_max_speed;
		player->ship_direction += player->ship_rotation_velocity * dt;

		f32 direction_vector[2];
		player_direction_vector(direction_vector, player);

		f32 forward_mod = 0.0f;
		if(input_button_down(player->up))
			forward_mod += 0.3f;
		if(input_button_down(player->down))
			forward_mod -= 0.2f;
		f32 forward[2];
		v2_copy(forward, direction_vector);
		v2_scale(forward, forward_mod);
		v2_add(player->ship_velocity, player->ship_velocity, forward);

		f32 side_vector[2];
		v2_init(side_vector, -direction_vector[1], direction_vector[0]);

		f32 strafe_speed = 0.3f;
		f32 strafe_mod = 0.0f;
		if(input_button_down(player->strafe_left))
			strafe_mod -= 1.0f;
		if(input_button_down(player->strafe_right))
			strafe_mod += 1.0f;
		f32 strafe[2];
		v2_copy(strafe, side_vector);
		v2_scale(strafe, strafe_mod * strafe_speed);
		v2_add(player->ship_velocity, player->ship_velocity, strafe);

		f32 velocity_normalized[2];
		v2_normalize(player->ship_velocity, velocity_normalized);

		f32 max_speed = 14.0f;
		f32 velocity_mag = v2_magnitude(player->ship_velocity);
		if(velocity_mag > max_speed) {
			v2_copy(player->ship_velocity, velocity_normalized);
			v2_scale(player->ship_velocity, max_speed);
		}

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
