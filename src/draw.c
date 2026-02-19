void draw_active_game(GameState* game, RenderList* list, StackAllocator* ui_stack, f32 dt) {
	render_list_init(list);

	render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, 8);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP_2, 8);
	render_list_allocate_instance_type(list, ASSET_MESH_CYLINDER, 0, 64);

	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_BODY, ASSET_TEXTURE_SHIP, 2);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_L, ASSET_TEXTURE_SHIP, 2);
	render_list_allocate_instance_type(list, ASSET_MESH_SHIP_WING_R, ASSET_TEXTURE_SHIP, 2);

	render_list_allocate_instance_type(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, 4096);

	i32 floor_length = game->level.side_length;
	i32 floor_instances = floor_length * floor_length;
	render_list_allocate_instance_type(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_instances);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	bool splitscreen = true;
	for(i32 pl = 0; pl < 2; pl++) {
		GamePlayer* player = &game->players[pl];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		v3 rot = player_orientation(player);
		u8 texture = ASSET_TEXTURE_SHIP;
		if(pl == 1) texture = ASSET_TEXTURE_SHIP_2;
		render_list_draw_model_colored(list, ASSET_MESH_SHIP, texture, pos, rot, v4_new(1.0f, 1.0f, 1.0f, player->hit_cooldown * player->hit_cooldown * 4.0f));

		v2 direction = player_direction_vector(player);
		v2 side = v2_new(-direction.y, direction.x);
		v2 target = v2_add(player->position, v2_scale(direction, 50.0f));

		v2 laser_pos_1 = v2_add(v2_add(player->position, v2_scale(side, 0.39f)), v2_scale(direction, 0.10f));
		v2 laser_pos_2 = v2_add(v2_add(player->position, v2_scale(side, -0.39f)), v2_scale(direction, 0.10f));

		render_list_draw_laser(list, v3_new(laser_pos_1.x, 0.5f, laser_pos_1.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);
		render_list_draw_laser(list, v3_new(laser_pos_2.x, 0.5f, laser_pos_2.y), v3_new(target.x, 0.5f, target.y), player->shoot_cooldown_sound * player->shoot_cooldown_sound * player->shoot_cooldown_sound * 0.08f);

		// NOW: Debug player velocity line.
		//v3 vel_laser_start = v3_new(player->position.x, 0.5f, player->position.y);
		//v3 vel_laser_end = v3_add(vel_laser_start, v3_new(player->velocity.x, 0.0f, player->velocity.y));
		//render_list_draw_laser(list, laser_instance_type, vel_laser_start, vel_laser_end, 0.05f);
	}

	for(i32 pos = 0; pos < floor_instances; pos++) {
		v3 floor_pos = v3_new(pos % floor_length, 0.0f, pos / floor_length);
		render_list_draw_model_aligned(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos);
	}

	// TODO: When this is put below cube rendering, the meshes are overwritten by
	// cubes. Pretty important we figure out why, obviously.
	for(i32 dm = 0; dm < 6; dm++) {
		GameDestructMesh* mesh = &game->destruct_meshes[dm];
		if(mesh->opacity <= 0.0f) continue;
		render_list_draw_model_colored(list, mesh->mesh, mesh->texture, mesh->position, mesh->orientation, v4_new(1.0f, 1.0f, 1.0f, mesh->opacity * mesh->opacity));
	}

	for(i32 pos = 0; pos < floor_instances; pos++) {
		for(i32 height = 1; height <= game->level.tiles[pos]; height++) {
			v3 cube_pos = v3_new(pos % floor_length, (f32)height - 1.0f, pos / floor_length);
			render_list_draw_model_aligned(list, ASSET_MESH_CUBE, ASSET_TEXTURE_CRATE, cube_pos);
		}
	}

	switch(game->mode) {
		case GAME_ACTIVE: {
			for(i32 player_index = 0; player_index < 2; player_index++) {
				if(splitscreen || player_index == 0) {
					GameCamera* camera = &game->cameras[player_index];
					GamePlayer* player = &game->players[player_index];
					v3 cam_target = v3_new(camera->offset.x + player->position.x, 0.0f, camera->offset.y + player->position.y);
					//v3 cam_pos = v3_new(player->position.x - camera->offset.x * 1.5f, 5.0f, player->position.y - camera->offset.y * 1.5f);
					v3 cam_pos = v3_new(cam_target.x, 8.0f, cam_target.z - 4.0f + player_index * 8.0f);
					f32 gap = 0.0025f;
					v4 screen_rect;
					if(splitscreen) {
						screen_rect = v4_new(0.5f * player_index + gap * player_index, 0.0f, 0.5f - gap * (1 - player_index), 1.0f);
					} else {
						screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
					}
					render_list_add_camera(list, cam_pos, cam_target, screen_rect);
				}
			}
		} break;
#if GAME_EDITOR_TOOLS
		case GAME_LEVEL_EDITOR: {
			level_editor_draw(game, list);
		} break;
#endif
		default: break;
	}

	// TODO: Layouts for splitscreen vs single and others, sets up anchors and
	// positions per item, which are referenced here in the drawing commands.
	 
	// Player scores
	for(i32 player_index = 0; player_index < 2; player_index++) {
		v2 position = v2_new(-24.0f + player_index * 48.0f, -32.0f);
		v2 inner_anchor = v2_new(1.0f * (1.0f - player_index), 1.0f);
		v2 screen_anchor = v2_new(0.5f, 1.0f);
		v4 color = v4_new(1.0f, 1.0f, 0.0f, 1.0f);
		char buf[16];
		sprintf(buf, "%i", game->players[player_index].score);
		ui_draw_text_line(list, game->fonts, ASSET_FONT_QUANTICO_LARGE, buf,
			position, inner_anchor, screen_anchor, color, ui_stack);
	}

	// Player health bars
	for(i32 player_index = 0; player_index < 2; player_index++) {
		if(!splitscreen && player_index != 0) continue;

		GamePlayer* player = &game->players[player_index];
		player->visible_health = lerp(player->visible_health, player->health, 20.0f * dt);

		v4 health_bar_root = v4_new(32.0f, 32.0f, 64.0f, 400.0f);
		render_list_draw_box(list, health_bar_root, v2_new(player_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f, 1.0f - player->hit_cooldown, 0.0f, 1.0f));
		f32 segments = 40.0f;
		for(i32 seg = 0; seg < (i32)(player->visible_health * segments); seg++) {
			v4 health_bar_sub = health_bar_root;
			health_bar_sub.x += 8.0f;
			health_bar_sub.y += 8.0f + seg * ((health_bar_root.w - 8.0f) / segments);
			health_bar_sub.z -= 16.0f;
			health_bar_sub.w = health_bar_root.w / segments - 8.0f;
			render_list_draw_box(list, health_bar_sub, v2_new(player_index * 0.5f, 0.0f), 4.0f, v4_new(1.0f - seg / segments, seg / segments, 0.0f, 1.0f));
		}
	}
}
