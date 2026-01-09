typedef struct {
	f32 ship_direction;
	f32 ship_position[2];
	f32 ship_velocity[2];
	f32 ship_rotation_velocity;
} Game;

Game* game_init(Arena* arena) {
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));
	game->ship_direction = 0.0f;
	game->ship_position[0] = 0.0f;
	game->ship_position[1] = 0.0f;
	game->ship_velocity[0] = 0.0f;
	game->ship_velocity[1] = 0.0f;
	game->ship_rotation_velocity = 0.0f;
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

void game_update(Game* game, bool up, bool down, bool left, bool right, float dt) {
	f32 rotate_speed = 32.0f;
	if(left)
		game->ship_rotation_velocity += rotate_speed * dt;
	if(right)
		game->ship_rotation_velocity -= rotate_speed * dt;

	f32 rotate_friction = 8.0f;
	game->ship_rotation_velocity = apply_friction(game->ship_rotation_velocity, rotate_friction, dt);
	game->ship_direction += game->ship_rotation_velocity * dt;

	f32 direction_vector[2];
	direction_vector[0] = sin(game->ship_direction);
	direction_vector[1] = cos(game->ship_direction);
	v2_normalize(direction_vector, direction_vector);

	if(up) {
		f32 forward_speed = 0.5f;
		game->ship_velocity[0] += direction_vector[0] * forward_speed;
		game->ship_velocity[1] += direction_vector[1] * forward_speed;
	}
	if(down) {
		f32 back_speed = 0.25f;
		game->ship_velocity[0] -= direction_vector[0] * back_speed;
		game->ship_velocity[1] -= direction_vector[1] * back_speed;
	}

	f32 velocity_normalized[2];
	v2_normalize(game->ship_velocity, velocity_normalized);

	float max_speed = 8.0f;
	float velocity_mag = v2_magnitude(game->ship_velocity);
	if(velocity_mag > max_speed) {
		game->ship_velocity[0] = velocity_normalized[0] * max_speed;
		game->ship_velocity[1] = velocity_normalized[1] * max_speed;
	}

	f32 movement_friction = 6.0f;
	game->ship_velocity[0] -= velocity_normalized[0] * movement_friction * dt;
	game->ship_velocity[1] -= velocity_normalized[1] * movement_friction * dt;

	game->ship_position[0] += game->ship_velocity[0] * dt;
	game->ship_position[1] += game->ship_velocity[1] * dt;
}
