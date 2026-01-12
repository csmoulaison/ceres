typedef struct {
	f32 ship_direction;
	f32 ship_rotation_velocity;
	f32 ship_position[2];
	f32 ship_velocity[2];
	f32 camera_offset[2];
} Game;

Game* game_init(Arena* arena) {
	Game* game = (Game*)arena_alloc(arena, sizeof(Game));
	game->ship_direction = 0.0f;
	game->ship_rotation_velocity = 0.0f;
	v2_zero(game->ship_position);
	v2_zero(game->ship_velocity);
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

RenderList game_update(Game* game, bool up, bool down, bool turn_left, bool turn_right, bool strafe_left, bool strafe_right, f32 dt) {
	// Camera/ship control
	f32 rotate_speed = 32.0f;
	if(turn_left)
		game->ship_rotation_velocity += rotate_speed * dt;
	if(turn_right)
		game->ship_rotation_velocity -= rotate_speed * dt;

	f32 rotate_max_speed = 12.0f;
	f32 rotate_friction = 8.0f;
	game->ship_rotation_velocity = apply_friction(game->ship_rotation_velocity, rotate_friction, dt);
	if(game->ship_rotation_velocity > rotate_max_speed)
		game->ship_rotation_velocity = rotate_max_speed;
	if(game->ship_rotation_velocity < -rotate_max_speed)
		game->ship_rotation_velocity = -rotate_max_speed;
	
	game->ship_direction += game->ship_rotation_velocity * dt;

	f32 direction_vector[2];
	v2_init(direction_vector, sin(game->ship_direction), cos(game->ship_direction));
	v2_normalize(direction_vector, direction_vector);

	f32 forward_mod = 0.0f;
	if(up)
		forward_mod += 0.3f;
	if(down)
		forward_mod -= 0.2f;
	f32 forward[2];
	v2_copy(forward, direction_vector);
	v2_scale(forward, forward_mod);
	v2_add(game->ship_velocity, game->ship_velocity, forward);

	f32 side_vector[2];
	v2_init(side_vector, -direction_vector[1], direction_vector[0]);

	f32 strafe_speed = 0.3f;
	f32 strafe_mod = 0.0f;
	if(strafe_left)
		strafe_mod -= 1.0f;
	if(strafe_right)
		strafe_mod += 1.0f;
	f32 strafe[2];
	v2_copy(strafe, side_vector);
	v2_scale(strafe, strafe_mod * strafe_speed);
	v2_add(game->ship_velocity, game->ship_velocity, strafe);

	f32 velocity_normalized[2];
	v2_normalize(game->ship_velocity, velocity_normalized);

	f32 max_speed = 14.0f;
	f32 velocity_mag = v2_magnitude(game->ship_velocity);
	if(velocity_mag > max_speed) {
		v2_copy(game->ship_velocity, velocity_normalized);
		v2_scale(game->ship_velocity, max_speed);
	}

	f32 movement_friction = 6.0f;
	if(velocity_mag > 0.02f) {
		f32 friction[2];
		v2_copy(friction, velocity_normalized);
		v2_scale(friction, -movement_friction * dt);
		v2_add(game->ship_velocity, game->ship_velocity, friction);
	} else {
		v2_zero(game->ship_velocity);
	}

	f32 camera_lookahead = 4.0f;
	f32 camera_target_position[2];

	f32 camera_target_offset[2];
	v2_copy(camera_target_offset, direction_vector);
	v2_scale(camera_target_offset, camera_lookahead);

	f32 camera_speed_mod = 2.0f;
	f32 camera_target_delta[2];	
	v2_copy(camera_target_delta, camera_target_offset);
	v2_sub(camera_target_delta, camera_target_offset, game->camera_offset);
	v2_scale(camera_target_delta, camera_speed_mod * dt);
	v2_add(game->camera_offset, game->camera_offset, camera_target_delta);

	f32 delta_velocity[2];
	v2_copy(delta_velocity, game->ship_velocity);
	v2_scale(delta_velocity, dt);
	v2_add(game->ship_position, game->ship_position, delta_velocity);

	// Populate render list
	RenderList list;
	render_list_init(&list);

	f32 clear_color[3];
	v3_init(clear_color, 0.1f, 0.1f, 0.2f);
	f32 cam_target[3];
	v3_init(cam_target, game->camera_offset[0] + game->ship_position[0], 0.0f, game->camera_offset[1] + game->ship_position[1]);
	f32 cam_pos[3];
	v3_init(cam_pos, cam_target[0] + 4.0f, 8.0f, cam_target[2]);
	render_list_update_world(&list, clear_color, cam_pos, cam_target);

	f32 ship_pos[3];
	v3_init(ship_pos, game->ship_position[0], 0.5f, game->ship_position[1]);
	f32 ship_rot[3];
	f32 ship_tilt = clamp(game->ship_rotation_velocity, -8.0f, 8.0f);
	v3_init(ship_rot, ship_tilt * -0.1f, game->ship_direction, 0.0f);
	render_list_draw_model(&list, 0, 0, ship_pos, ship_rot);

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
