void draw_active_session(Session* session, RenderList* list, FontData* fonts, StackAllocator* frame_stack, f32 dt) {
	i32 floor_length = session->level.side_length;
	i32 floor_instances = floor_length * floor_length;

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	for(i32 player_index = 0; player_index < session->players_len; player_index++) {
		Player* player = &session->players[player_index];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		v3 rot = player_orientation(player);
		u8 texture = ASSET_TEXTURE_SHIP;
		if(player->team == 1) texture = ASSET_TEXTURE_SHIP_2;
		render_list_draw_model_colored(list, ASSET_MESH_SHIP, texture, pos, rot, v4_new(1.0f, 1.0f, 1.0f, player->hit_cooldown * player->hit_cooldown * 4.0f));

		v2 direction = player_direction_vector(player);
		v2 side = v2_new(-direction.y, direction.x);
		v2 target = v2_add(player->position, v2_scale(direction, 50.0f));

		v2 laser_pos_1 = v2_add(v2_add(player->position, v2_scale(side, 0.39f)), v2_scale(direction, 0.10f));
		v2 laser_pos_2 = v2_add(v2_add(player->position, v2_scale(side, -0.39f)), v2_scale(direction, 0.10f));

		render_list_draw_laser(list, v3_new(laser_pos_1.x, 0.5f, laser_pos_1.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown * player->shoot_cooldown * player->shoot_cooldown * 0.08f);
		render_list_draw_laser(list, v3_new(laser_pos_2.x, 0.5f, laser_pos_2.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown * player->shoot_cooldown * player->shoot_cooldown * 0.08f);

		//v3 vel_laser_start = v3_new(player->position.x, 0.5f, player->position.y);
		//v3 vel_laser_end = v3_add(vel_laser_start, v3_new(player->velocity.x, 0.0f, player->velocity.y));
		//render_list_draw_laser(list, laser_instance_type, vel_laser_start, vel_laser_end, 0.05f);
	}

	for(i32 tile_index = 0; tile_index < floor_instances; tile_index++) {
		v3 floor_pos = v3_new(tile_index % floor_length, 0.0f, tile_index / floor_length);
		render_list_draw_model_aligned(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos);
	}

	// TODO: When this is put below cube rendering, the meshes are overwritten by
	// cubes. Pretty important we figure out why, obviously.
	for(i32 mesh_index = 0; mesh_index < 6; mesh_index++) {
		DestructMesh* mesh = &session->destruct_meshes[mesh_index];
		if(mesh->opacity <= 0.0f) continue;
		render_list_draw_model_colored(list, mesh->mesh, mesh->texture, mesh->position, mesh->orientation, v4_new(1.0f, 1.0f, 1.0f, mesh->opacity * mesh->opacity));
	}

	for(i32 tile_index = 0; tile_index < floor_instances; tile_index++) {
		for(i32 height = 1; height <= session->level.tiles[tile_index]; height++) {
			v3 cube_pos = v3_new(tile_index % floor_length, (f32)height - 1.0f, tile_index / floor_length);
			render_list_draw_model_aligned(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, cube_pos);
		}
	}

	// TODO: This is some logic we want to be more thoughtful about regarding
	// placement.
	if(session->mode != SESSION_LEVEL_EDITOR) {
		for(i32 view_index = 0; view_index < session->player_views_len; view_index++) {
			PlayerView* view = &session->player_views[view_index];
			Player* player = &session->players[view->player];

			// Camera update
			v2 direction_vector = player_direction_vector(player);
			f32 camera_lookahead = 4.0f;
			v2 camera_target_offset = v2_scale(direction_vector, camera_lookahead);

			f32 camera_speed_mod = 2.0f;
			v2 camera_target_delta = v2_sub(camera_target_offset, view->camera_offset);
			camera_target_delta = v2_scale(camera_target_delta, camera_speed_mod * dt);
			view->camera_offset = v2_add(view->camera_offset, camera_target_delta);

			// Camera viewport
			v3 cam_target = v3_new(view->camera_offset.x + player->position.x, 0.0f, view->camera_offset.y + player->position.y);
			v3 cam_pos = v3_new(cam_target.x, 8.0f, cam_target.z - 4.0f + player->team * 8.0f);
			v4 screen_rect;
			if(session->player_views_len == 1) {
				screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
			} else if(session->player_views_len == 2) {
				f32 gap = 0.0025f;
				screen_rect = v4_new(0.5f * view_index + gap * view_index, 0.0f, 0.5f - gap * (1 - view_index), 1.0f);
			} else {
				panic();
			}
			render_list_add_camera(list, cam_pos, cam_target, screen_rect);

			// Health
			view->visible_health = lerp(view->visible_health, player->health, 20.0f * dt);
			v4 health_bar_root = v4_new(32.0f, 32.0f, 64.0f, 400.0f);
			render_list_draw_box(list, health_bar_root, v2_new(view_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f, 1.0f - player->hit_cooldown, 0.0f, 0.4f + player->hit_cooldown));
			f32 segments = 40.0f;
			for(i32 seg = 0; seg < (i32)(view->visible_health * segments); seg++) {
				v4 health_bar_sub = health_bar_root;
				health_bar_sub.x += 8.0f;
				health_bar_sub.y += 8.0f + seg * ((health_bar_root.w - 8.0f) / segments);
				health_bar_sub.z -= 16.0f;
				health_bar_sub.w = health_bar_root.w / segments - 8.0f;
				render_list_draw_box(list, health_bar_sub, v2_new(view_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f - seg / segments, seg / segments, 0.0f, 0.8f));
			}
		}
	} 

	// Player scores
	// TODO: This currently assumes exactly two competing teams.
	for(i32 team_index = 0; team_index < 2; team_index++) {
		v2 position = v2_new(-24.0f + team_index * 48.0f, -32.0f);
		v2 inner_anchor = v2_new(1.0f * (1.0f - team_index), 1.0f);
		v2 screen_anchor = v2_new(0.5f, 1.0f);
		v4 color = v4_new(1.0f, 1.0f, 0.0f, 0.5f);
		char buf[16];
		sprintf(buf, "%i", session->team_scores[team_index]);
		ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_LARGE, buf,
			position, inner_anchor, screen_anchor, color, frame_stack);
	}
}
