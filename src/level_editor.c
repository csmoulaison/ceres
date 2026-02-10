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
		game->level[editor->cursor_y * 64 + editor->cursor_x] = 1;
	}
}
