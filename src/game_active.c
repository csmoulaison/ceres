v2 player_direction_vector(GamePlayer* player) {
	return v2_normalize(v2_new(sin(player->direction), cos(player->direction)));
}

void game_active_update(GameState* game, f32 dt) {
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		v2 acceleration = v2_zero();
		v2 direction_vector = player_direction_vector(player);
		f32 rot_acceleration = 0.0f;
		f32 strafe_mod = 0.0f;

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
				if(v2_distance_squared(closest, other->position) < 1.15f) {
					f32 cross = v2_cross(direction, v2_sub(other->position, player->position));
					other->rotation_velocity -= cross;
					
					other->health -= 0.2f;
					other->hit_cooldown = 1.0f;
					other->velocity = v2_add(other->velocity, v2_scale(direction, 1.0f));
					if(other->health <= 0.0f) {
						other->health = 1.0f;
						f32 pos = 14.0f;
						if(i == 0) pos = 50.0f;
						other->position = v2_new(pos, pos);
						other->velocity = v2_zero();
						other->hit_cooldown = 1.2f;
					}
				}
			}
		}

		// Calculate ship rotational acceleration
		f32 rotate_speed = 16.0f;
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

		// Rotational damping
		f32 rot_damping = 1.2f;
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

		// RESOLVE VELOCITIES!!
		physics_resolve_velocities(game);

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

	if(input_button_pressed(game->players[0].button_states[BUTTON_DEBUG])) {
		game->level_editor.cursor_x = (u32)game->players[0].position.x;
		game->level_editor.cursor_y = (u32)game->players[0].position.y;
		game->mode = GAME_LEVEL_EDITOR;
	}
}
