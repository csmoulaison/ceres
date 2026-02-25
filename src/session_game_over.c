void session_reset(Session* session, Level* level);

void session_game_over_update(Session* session, Input* input, Audio* audio, f32 dt) {
	session->game_over_rotation_position += dt;

	if(input_button_pressed(input->players[0].buttons[BUTTON_SHOOT])) {
		session_reset(session, &session->level);
	}
}

void session_game_over_draw(Session* session, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_LARGE, "Game Over",
		v2_new(0.0f, 16.0f), v2_new(0.5f, 0.0f), v2_new(0.5f, 0.5f), v4_new(1.0f, 0.0f, 0.0f, 0.5f), draw_stack);

	i32 winning_team_score = 0;
	i32 winning_team_index = 0;
	for(i32 team_index = 0; team_index < session->teams_len; team_index++) {
		if(session->team_scores[team_index] > winning_team_score) {
			winning_team_score = session->team_scores[team_index];
			winning_team_index = team_index;
		}
	}

	char buffer[256];
	sprintf(buffer, "Team %i wins with a score of %i\n", winning_team_index + 1, winning_team_score);
	ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_REGULAR, buffer,
		v2_new(0.0f, -16.0f), v2_new(0.5f, 1.0f), v2_new(0.5f, 0.5f), v4_new(0.0f, 1.0f, 0.0f, 0.5f), draw_stack);

	v2 cam_target = v2_new((f32)session->level.side_length / 2.0f, (f32)session->level.side_length / 2.0f);
	v2 cam_pos = v2_normalize(v2_new(sin(session->game_over_rotation_position), cos(session->game_over_rotation_position)));
	cam_pos = v2_add(cam_target, v2_scale(cam_pos, 10.0f));
	v4 screen_rect = v4_new(0.0f, 0.0f, 1.0f, 1.0f);
	render_list_add_camera(list, v3_new(cam_pos.x, 6.66f, cam_pos.y), v3_new(cam_target.x, 0.0f, cam_target.y), screen_rect);
}
