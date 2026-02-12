void level_editor_serialize(GameLevel* level) {
	FILE* file = fopen("../build/data/levels/level.lvl", "w");
	assert(file != NULL);
	fwrite(&level->side_length, sizeof(level->side_length), 1, file);
	fwrite(level->tiles, sizeof(u8) * level->side_length * level->side_length, 1, file);
}

void level_editor_update(GameState* game) {
	LevelEditor* editor = &game->level_editor;
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

	editor->camera_position.x = lerp(editor->camera_position.x, (f32)editor->cursor_x, 0.20f);
	editor->camera_position.y = lerp(editor->camera_position.y, (f32)editor->cursor_y, 0.20f);

	if(input_button_pressed(button_states[BUTTON_SHOOT])) {
		u8* tile = &game->level.tiles[editor->cursor_y * game->level.side_length + editor->cursor_x];
		if(*tile == 0) {
			*tile = 1;
		} else {
			*tile = 0;
		}
	}

	if(input_button_pressed(game->players[0].button_states[BUTTON_DEBUG])) {
		game->mode = GAME_ACTIVE;
		level_editor_serialize(&game->level);
	}
}
