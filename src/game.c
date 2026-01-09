typedef struct {
	f32 ship_direction;
	f32 ship_position[2];
	f32 ship_velocity[2];
	f32 ship_rotation_velocity;
	f32 camera_offset[2];
} Game;

Game* game_init(Arena* arena) {
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));
	game->ship_direction = 0.0f;
	game->ship_position[0] = 0.0f;
	game->ship_position[1] = 0.0f;
	game->ship_velocity[0] = 0.0f;
	game->ship_velocity[1] = 0.0f;
	game->ship_rotation_velocity = 0.0f;

	//game->camera_position[0] = 0.0f;
	//game->camera_position[1] = 0.0f;
	game->camera_offset[0] = 0.0f;
	game->camera_offset[1] = 0.0f;
	//game->camera_lookahead_direction = game->ship_direction;
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

void game_update(Game* game, bool up, bool down, bool left, bool right, f32 dt) {
	f32 rotate_speed = 32.0f;
	if(left)
		game->ship_rotation_velocity += rotate_speed * dt;
	if(right)
		game->ship_rotation_velocity -= rotate_speed * dt;

	f32 rotate_max_speed = 12.0f;
	f32 rotate_friction = 8.0f;
	game->ship_rotation_velocity = apply_friction(game->ship_rotation_velocity, rotate_friction, dt);
	if(game->ship_rotation_velocity > rotate_max_speed) {
		game->ship_rotation_velocity = rotate_max_speed;
	}
	if(game->ship_rotation_velocity < -rotate_max_speed) {
		game->ship_rotation_velocity = -rotate_max_speed;
	}
	
	game->ship_direction += game->ship_rotation_velocity * dt;

	f32 direction_vector[2];
	direction_vector[0] = sin(game->ship_direction);
	direction_vector[1] = cos(game->ship_direction);
	v2_normalize(direction_vector, direction_vector);

	if(up) {
		f32 forward_speed = 0.3f;
		game->ship_velocity[0] += direction_vector[0] * forward_speed;
		game->ship_velocity[1] += direction_vector[1] * forward_speed;
	}
	if(down) {
		f32 back_speed = 0.15f;
		game->ship_velocity[0] -= direction_vector[0] * back_speed;
		game->ship_velocity[1] -= direction_vector[1] * back_speed;
	}

	f32 velocity_normalized[2];
	v2_normalize(game->ship_velocity, velocity_normalized);

	f32 max_speed = 14.0f;
	f32 velocity_mag = v2_magnitude(game->ship_velocity);
	if(velocity_mag > max_speed) {
		game->ship_velocity[0] = velocity_normalized[0] * max_speed;
		game->ship_velocity[1] = velocity_normalized[1] * max_speed;
	}

	f32 movement_friction = 6.0f;
	if(velocity_mag > 0.02f) {
		game->ship_velocity[0] -= velocity_normalized[0] * movement_friction * dt;
		game->ship_velocity[1] -= velocity_normalized[1] * movement_friction * dt;
	} else {
		game->ship_velocity[0] = 0.0f;
		game->ship_velocity[1] = 0.0f;
	}

	f32 camera_lookahead = 4.0f;
	f32 camera_target_position[2];

	//f32 camera_direction_speed = 8.0f;
	//game->camera_lookahead_direction = lerp(game->camera_lookahead_direction, game->ship_direction, camera_direction_speed * dt);
	//v2_normalize(camera_direction_vector, camera_direction_vector);
	//f32 camera_direction_vector[2];
	//camera_direction_vector[0] = sin(game->camera_lookahead_direction);
	//camera_direction_vector[1] = cos(game->camera_lookahead_direction);

	//camera_target_position[0] = game->ship_position[0] + camera_direction_vector[0] * camera_lookahead;
	//camera_target_position[1] = game->ship_position[1] + camera_direction_vector[1] * camera_lookahead;

	f32 camera_target_offset[2];
	camera_target_offset[0] = direction_vector[0] * camera_lookahead;
	camera_target_offset[1] = direction_vector[1] * camera_lookahead;

	f32 camera_target_delta[2];	
	f32 camera_delta_pronouncement = 1.0f;
	camera_target_delta[0] = (camera_target_offset[0] - game->camera_offset[0]) * camera_delta_pronouncement;
	camera_target_delta[1] = (camera_target_offset[1] - game->camera_offset[1]) * camera_delta_pronouncement;

	f32 camera_speed_mod = 2.0f;
	game->camera_offset[0] += camera_target_delta[0] * camera_speed_mod * dt;
	game->camera_offset[1] += camera_target_delta[1] * camera_speed_mod * dt;

	game->ship_position[0] += game->ship_velocity[0] * dt;
	game->ship_position[1] += game->ship_velocity[1] * dt;
}
