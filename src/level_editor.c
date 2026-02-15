void level_editor_serialize(GameLevel* level) {
	FILE* file = fopen("../build/data/levels/level.lvl", "w");
	assert(file != NULL);
	fwrite(&level->side_length, sizeof(level->side_length), 1, file);
	fwrite(&level->spawns, sizeof(LevelSpawn) * MAX_LEVEL_SPAWNS, 1, file);
	fwrite(&level->spawns_len, sizeof(level->spawns_len), 1, file);
	fwrite(level->tiles, sizeof(u8) * level->side_length * level->side_length, 1, file);
}

void level_editor_update(GameState* game) {
	LevelEditor* editor = &game->level_editor;
	GameLevel* level = &game->level;
	ButtonState* button_states = game->players[0].button_states;

	if(input_button_pressed(button_states[BUTTON_FORWARD])) {
		editor->cursor_y++;
	}
	if(input_button_pressed(button_states[BUTTON_BACK])) {
		editor->cursor_y--;
	}
	if(input_button_pressed(button_states[BUTTON_TURN_LEFT])) {
		editor->cursor_x++;
	}
	if(input_button_pressed(button_states[BUTTON_TURN_RIGHT])) {
		editor->cursor_x--;
	}

	u8* tile = &game->level.tiles[editor->cursor_y * game->level.side_length + editor->cursor_x];
	editor->camera_position.x = lerp(editor->camera_position.x, (f32)editor->cursor_x, 0.20f);
	editor->camera_position.y = lerp(editor->camera_position.y, *tile, 0.10f);
	editor->camera_position.z = lerp(editor->camera_position.z, (f32)editor->cursor_y, 0.20f);

	if(input_button_pressed(button_states[BUTTON_CLEAR])) {
		for(i32 i = 0; i < game->level.side_length * game->level.side_length; i++) {
			level->tiles[i] = 0;
		}
		level->spawns_len = 0;
		printf("Editor: Cleared level\n");
	}

	switch(editor->tool) {
		case EDITOR_TOOL_CUBES: {
			if(input_button_pressed(button_states[BUTTON_SHOOT])) {
				*tile += 1;
			}

			if(input_button_pressed(game->players[1].button_states[BUTTON_SHOOT])) {
				if(*tile > 0) {
					*tile -= 1;
				}
			}

		} break;
		case EDITOR_TOOL_SPAWNS: {
			if(input_button_pressed(button_states[BUTTON_SHOOT]) && *tile == 0) {
				bool matched_spawn = false;
				for(i32 i = 0; i < level->spawns_len; i++) {
					LevelSpawn* spawn = &level->spawns[i];
					if(spawn->x == editor->cursor_x && spawn->y == editor->cursor_y) {
						matched_spawn = true;
						spawn->team++;
						if(spawn->team > 1) {
							printf("Editor: Deleted spawn %i\n", i);
							level->spawns[i] = level->spawns[level->spawns_len - 1];
							level->spawns_len--;
						} else {
							printf("Editor: Changed spawn team to %i\n", spawn->team);
						}
					}
				}

				if(!matched_spawn && level->spawns_len < MAX_LEVEL_SPAWNS) {
					printf("Editor: Added spawn %u\n", level->spawns_len);
					LevelSpawn* spawn = &level->spawns[level->spawns_len];
					level->spawns_len++;
					spawn->x = editor->cursor_x;
					spawn->y = editor->cursor_y;
					spawn->team = 0;
				}
			}
		} break;
		default: { panic(); }
	}
	if(input_button_pressed(button_states[BUTTON_SWITCH])) {
		editor->tool++;
		if(editor->tool > 1) editor->tool = 0;
	}

	if(input_button_pressed(button_states[BUTTON_DEBUG])) {
		game->mode = GAME_ACTIVE;
		level_editor_serialize(&game->level);
	}
}

void level_editor_draw(GameState* game, RenderList* list) {
	LevelEditor* editor = &game->level_editor;
	GameLevel* level = &game->level;
	u8 cursor_tile = level->tiles[editor->cursor_y * level->side_length + editor->cursor_x];

	v3 cam_pos = v3_new((f32)editor->camera_position.x, (f32)editor->camera_position.y + 8.0f, (f32)editor->camera_position.z - 4.0f);
	v3 cam_target = v3_new((f32)editor->camera_position.x, (f32)editor->camera_position.y, (f32)editor->camera_position.z);
	v4 screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
	render_list_add_camera(list, cam_pos, cam_target, screen_rect);

	v3 mesh_pos = v3_new(editor->cursor_x, cursor_tile, editor->cursor_y);
	u8 mesh;
	u8 texture;
	f32 bluegreen_attenuation = 0.0f;
	switch(editor->tool) {
		case EDITOR_TOOL_CUBES: {
			mesh = ASSET_MESH_CUBE;
			texture = ASSET_TEXTURE_CRATE;
		} break;
		case EDITOR_TOOL_SPAWNS: {
			mesh = ASSET_MESH_SHIP;
			texture = ASSET_TEXTURE_SHIP;
			if(cursor_tile != 0) {
				bluegreen_attenuation = 1.0f;
			}
		} break;
		default: { panic(); }
	}
	render_list_draw_model_colored(list, mesh, texture, mesh_pos, v3_zero(), v4_new(1.0f, 1.0f - bluegreen_attenuation, 1.0f - bluegreen_attenuation, sin((f32)game->frame / 4.0f)));

	for(i32 i = 0; i < level->spawns_len; i++) {
		LevelSpawn* spawn = &level->spawns[i];
		v4 color = v4_new(1.0f, 0.0f, 0.0f, 1.0f);
		if(spawn->team == 1) color = v4_new(0.0f, 0.5f, 0.5f, 1.0f);
		render_list_draw_model_colored(list, mesh, texture, v3_new(spawn->x, 0.5f, spawn->y), v3_new(0.0f, (f32)game->frame / 10.0f, 0.0f), color);
	}
}
