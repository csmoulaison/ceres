void session_active_update(Session* session, Input* input, Audio* audio, f32 dt) {
	for(i32 view_index = 0; view_index < session->player_views_len; view_index++) {
		PlayerView* view = &session->player_views[view_index];
		PlayerInput* player_input = &session->players[view->player].input;
		InputDevice* device = &input->devices[view->input_device];
		//memset(player_input, 0, sizeof(PlayerInput));
		player_input->forward_axis = 0.0f;
		player_input->turn_axis = 0.0f;
		player_input->strafe_axis = 0.0f;
		player_input->shoot = 0.0f;
		if(input_button_down(device->buttons[BUTTON_FORWARD])) player_input->forward_axis += 1.0f;
		if(input_button_down(device->buttons[BUTTON_BACK])) player_input->forward_axis -= 1.0f;
		if(input_button_down(device->buttons[BUTTON_TURN_LEFT])) player_input->turn_axis -= 1.0f;
		if(input_button_down(device->buttons[BUTTON_TURN_RIGHT])) player_input->turn_axis += 1.0f;
		if(input_button_down(device->buttons[BUTTON_STRAFE_LEFT])) player_input->strafe_axis -= 1.0f;
		if(input_button_down(device->buttons[BUTTON_STRAFE_RIGHT])) player_input->strafe_axis += 1.0f;
		if(input_button_down(device->buttons[BUTTON_SHOOT])) player_input->shoot = true;
	}

	for(i32 player_index = 0; player_index < session->players_len; player_index++) {
		Player* player = &session->players[player_index];
		PlayerInput* player_input = &player->input;

		v2 acceleration = v2_zero();
		v2 direction_vector = player_direction_vector(player);
		f32 rot_acceleration = 0.0f;

		// Shooting
		if(player->hit_cooldown > 0.0f) {
			player->hit_cooldown = lerp(player->hit_cooldown, 0.0f, 4.0f * dt);
		}

		player->shoot_cooldown -= dt * 10.0f;
		if(player->shoot_cooldown < 0.0f) player->shoot_cooldown = 0.0f;
		if(player->shoot_cooldown <= 0.0f && player_input->shoot) {
			player->shoot_cooldown = 0.08f;
			player->shoot_cooldown = 0.99f + ((f32)rand() / RAND_MAX) * 0.01f;

			v2 direction = player_direction_vector(player);
			//player->velocity = v2_add(player->velocity, v2_scale(direction, -0.33f));

			for(i32 other_index = 0; other_index < session->players_len; other_index++) {
				if(other_index == player_index) continue;

				Player* other = &session->players[other_index];
				f32 t = v2_dot(direction, v2_sub(other->position, player->position));
				if(t < 0.5f) continue;

				v2 closest = v2_add(player->position, v2_scale(direction, t));
				if(v2_distance_squared(closest, other->position) < 1.15f) {
					f32 cross = v2_cross(direction, v2_sub(other->position, player->position));
					other->rotation_velocity -= cross;
					
					other->velocity = v2_add(other->velocity, v2_scale(direction, 1.0f));
					player_damage(other, 0.2f);
				}
			}
		}

		// Calculate ship rotational acceleration
		f32 rotate_speed = 20.0f;
		rot_acceleration = -player_input->turn_axis * rotate_speed;
		//if(input_button_down(player_input->buttons[BUTTON_TURN_LEFT])) {
		//	rot_acceleration += rotate_speed;
		//}
		//if(input_button_down(player_input->buttons[BUTTON_TURN_RIGHT])) {
		//	rot_acceleration -= rotate_speed;
		//}

		// Forward/back thruster control
		//f32 forward_mod = 0.0f;
		//if(input_button_down(player_input->buttons[BUTTON_FORWARD])) {
		//	forward_mod += 32.0f;
		//}
		//if(input_button_down(player_input->buttons[BUTTON_BACK])) {
		//	forward_mod -= 16.0f;
		//}
		f32 forward_mod = 0.0f;
		if(player_input->forward_axis > 0.0f) forward_mod += 32.0f;
		if(player_input->forward_axis < 0.0f) forward_mod -= 16.0f;
		acceleration = v2_scale(direction_vector, forward_mod);

		// Side thruster control
		f32 strafe_mod = player_input->strafe_axis;;
		//if(input_button_down(player_input->buttons[BUTTON_STRAFE_LEFT])) {
		//	strafe_mod -= 1.0f;
		//}
		//if(input_button_down(player_input->buttons[BUTTON_STRAFE_RIGHT])) {
		//	strafe_mod += 1.0f;
		//}

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

		// Resolve velocities
		physics_resolve_velocities(session);
		player->velocity = v2_add(player->velocity, v2_scale(acceleration, dt));

		f32 ship_energy = v2_magnitude(player->velocity) + abs(player->rotation_velocity);
		if(player->thruster_cooldown < ship_energy) {
			player->thruster_cooldown = lerp(player->thruster_cooldown, ship_energy, 10.0f * dt);
		} else {
			player->thruster_cooldown = lerp(player->thruster_cooldown, 0.0f, 1.0f * dt);
		}

		f32 mag = v2_magnitude(player->velocity);
		if(mag > 0.01f) {
			audio_start_sound(audio, &player->sound_forward_thruster, mag * 5.0f, 2000.0f * mag, 7.144123f * mag, 6000.0f, 0.000f * mag);
		} else {
			audio_stop_sound(audio, &player->sound_forward_thruster);
		}

		f32 rot = fabs(player->rotation_velocity);
		if(rot > 0.01f) {
			audio_start_sound(audio, &player->sound_rotation_thruster, rot * 5.0f, 2000.0f * rot, 10.0f * rot, 6000.0f, 0.000f * rot);
		} else {
			audio_stop_sound(audio, &player->sound_rotation_thruster);
		}

		f32 cool = player->thruster_cooldown;
		if(cool > 0.01f) {
			audio_start_sound(audio, &player->sound_thruster_cooldown, cool * 4.0f, 2000.0f * cool, 3.0f * cool, 2000.0f, 0.01f * cool);
		} else {
			audio_stop_sound(audio, &player->sound_thruster_cooldown);
		}

		f32 shoot = player->shoot_cooldown;
		if(shoot > 0.01f) {
			audio_start_sound(audio, &player->sound_shoot, shoot * 1000.0f, 3200.0f * shoot * shoot, 1500.0f * shoot * shoot, 3000.0f + 2000.0f * shoot, 0.2f * shoot);
		} else {
			audio_stop_sound(audio, &player->sound_shoot);
		}

		f32 hit = player->hit_cooldown * 0.5f;
		if(hit > 0.01f) {
			audio_start_sound(audio, &player->sound_hit, hit * 500.0f, 1000.0f * hit * hit * ((f32)rand() / RAND_MAX) * 2500.0f, 200.0f * hit * hit, 2000.0f + 2000.0f * hit, 2.0f * hit * ((f32)rand() / RAND_MAX));
		} else {
			audio_stop_sound(audio, &player->sound_hit);
		}

		if(player->health <= 0.0f) {
			u8 destruct_mesh_ids[3] = { ASSET_MESH_SHIP_BODY, ASSET_MESH_SHIP_WING_L, ASSET_MESH_SHIP_WING_R };
			for(i32 mesh_index = 0; mesh_index < 3; mesh_index++) {
				DestructMesh* destruct_mesh = &session->destruct_meshes[mesh_index * player_index];
				destruct_mesh->opacity = 1.0f;
				destruct_mesh->mesh = destruct_mesh_ids[mesh_index];
				destruct_mesh->texture = ASSET_TEXTURE_SHIP;
				destruct_mesh->position = v3_new(player->position.x, 0.5f, player->position.y);
				destruct_mesh->orientation = player_orientation(player);
				destruct_mesh->velocity.x = (random_f32() * 2.0f - 1.0f) * 2.0f;
				destruct_mesh->velocity.y = random_f32() * 2.0f;
				destruct_mesh->velocity.z = (random_f32() * 2.0f - 1.0f) * 2.0f;
				destruct_mesh->velocity = v3_add(destruct_mesh->velocity, v3_new(player->velocity.x, 0.0f, player->velocity.y));
				destruct_mesh->rotation_velocity.x = (random_f32() * 2.0f - 1.0f) * 2.0f;
				destruct_mesh->rotation_velocity.y = (random_f32() * 2.0f - 1.0f) * 2.0f;
				destruct_mesh->rotation_velocity.z = (random_f32() * 2.0f - 1.0f) * 2.0f;
			}

			player_spawn(player, session->players, session->players_len, &session->level);

			if(player->team == 0) session->team_scores[1] += 1;
			if(player->team == 1) session->team_scores[0] += 1;

			for(i32 team_index = 0; team_index < session->teams_len; team_index++) {
				if(session->team_scores[team_index] >= 2) {
					session->mode = SESSION_GAME_OVER;
					break;
				}
			}
		}
	}

	for(i32 view_index = 0; view_index < session->player_views_len; view_index++) {
		PlayerView* view = &session->player_views[view_index];
		Player* player = &session->players[view->player];
		view->visible_health = lerp(view->visible_health, player->health, 20.0f * dt);

		v2 direction_vector = player_direction_vector(player);
		f32 camera_lookahead = 4.0f;
		v2 camera_target_offset = v2_scale(direction_vector, camera_lookahead);

		f32 camera_speed_mod = 2.0f;
		v2 camera_target_delta = v2_sub(camera_target_offset, view->camera_offset);
		camera_target_delta = v2_scale(camera_target_delta, camera_speed_mod * dt);
		view->camera_offset = v2_add(view->camera_offset, camera_target_delta);
	}

	for(i32 mesh_index = 0; mesh_index < 6; mesh_index++) {
		DestructMesh* mesh = &session->destruct_meshes[mesh_index];
		if(mesh->opacity <= 0.0f) continue;
		mesh->velocity.y -= 10.0f * dt;
		mesh->position = v3_add(mesh->position, v3_scale(mesh->velocity, dt));
		mesh->orientation = v3_add(mesh->orientation, v3_scale(mesh->rotation_velocity, dt));
		mesh->opacity -= dt;
	}

	if(input_button_pressed(input->devices[0].buttons[BUTTON_DEBUG])) {
		session->level_editor.cursor_x = (u32)session->players[0].position.x;
		session->level_editor.cursor_y = (u32)session->players[0].position.y;
		session->mode = SESSION_LEVEL_EDITOR;
	}

	if(input_button_pressed(input->devices[0].buttons[BUTTON_QUIT])) {
		session->mode = SESSION_PAUSE;
	}
}
