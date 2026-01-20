#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.c"

void player_direction_vector(f32* dst, GamePlayer* player) {
	v2_init(dst, sin(player->ship_direction), cos(player->ship_direction));
	v2_normalize(dst, dst);
}

GAME_INIT(game_init) {
	printf("Game init from dynamic lib!\n");
	GameState* game = &memory->state;

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

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&game->key_mappings_len, sizeof(u32), 1, file);
	for(i32 i = 0; i < game->key_mappings_len; i++) {
		fread(&game->key_mappings[i], sizeof(GameKeyMapping), 1, file);
	}

	v2_zero(game->camera_offset);
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

		// Rotation thruster control
		f32 rotate_speed = 32.0f;
		if(input_button_down(player->button_states[BUTTON_TURN_LEFT]))
			player->ship_rotation_velocity += rotate_speed * dt;
		if(input_button_down(player->button_states[BUTTON_TURN_RIGHT]))
			player->ship_rotation_velocity -= rotate_speed * dt;

		f32 rotate_max_speed = 12.0f;
		f32 rotate_friction = 8.0f;
		player->ship_rotation_velocity = move_to_zero(player->ship_rotation_velocity, rotate_friction * dt);
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
		f32 side_vector[2] = { -direction_vector[1], direction_vector[0] };

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

		// Normalize to max velocity and apply friction.
		f32 velocity_normalized[2];
		v2_normalize(player->ship_velocity, velocity_normalized);
		f32 velocity_mag = v2_magnitude(player->ship_velocity);

		f32 movement_friction = 6.0f;
		if(velocity_mag > 0.02f) {
			f32 friction[2];
			v2_copy(friction, velocity_normalized);
			v2_scale(friction, -movement_friction * dt);
			v2_add(player->ship_velocity, player->ship_velocity, friction);
		} else {
			v2_zero(player->ship_velocity);
		}


		f32 max_speed = 14.0f;
		if(velocity_mag > max_speed) {
			v2_copy(player->ship_velocity, velocity_normalized);
			v2_scale(player->ship_velocity, max_speed);
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
	RenderList* list = &output->render_list;
	render_list_init(list);

	f32 clear_color[3] = { 0.1f, 0.1f, 0.2f };
	f32 cam_target[3] = { game->camera_offset[0] + primary_player->ship_position[0], 0.0f, game->camera_offset[1] + primary_player->ship_position[1] };
	f32 cam_pos[3] = { cam_target[0] + 4.0f, 8.0f, cam_target[2] };
	render_list_update_world(list, clear_color, cam_pos, cam_target);

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		f32 ship_pos[3] = { player->ship_position[0], 0.5f, player->ship_position[1] };
		f32 ship_tilt = clamp(player->ship_rotation_velocity, -8.0f, 8.0f);
		f32 ship_rot[3] = { ship_tilt * -0.1f, player->ship_direction, 0.0f };
		render_list_draw_model(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, ship_pos, ship_rot);
	}

	i32 floor_instances = 1024;
	for(i32 i = 0; i < floor_instances; i++) {
		f32 floor_pos[3] = { -15.5f + (i % 32), 0.0f, -15.5f + (i / 32) };
		f32 floor_rot[3] = { 0.0f , 0.0f, 0.0f };
		render_list_draw_model(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos, floor_rot);
	}
}
