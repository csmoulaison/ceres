char* pause_selection_strings[NUM_PAUSE_SELECTIONS] = {
	"Resume",
	"Quit"
};

void session_pause_update(Session* session, GameOutput* output, Input* input, Audio* audio, FontData* fonts, StackAllocator* frame_stack, f32 dt) {
	if(input_button_pressed(input->players[0].buttons[BUTTON_FORWARD])) {
		if(session->pause_selection <= 0) {
			session->pause_selection = NUM_PAUSE_SELECTIONS - 1;
		} else {
			session->pause_selection--;
		}
	}
	if(input_button_pressed(input->players[0].buttons[BUTTON_BACK])) {
		if(session->pause_selection >= NUM_PAUSE_SELECTIONS - 1) {
			session->pause_selection = 0;
		} else {
			session->pause_selection++;
		}
	}

	if(input_button_pressed(input->players[0].buttons[BUTTON_SHOOT])) {
		switch(session->pause_selection) {
			case PAUSE_SELECTION_RESUME: {
				session->mode = SESSION_ACTIVE;
			} break;
			case PAUSE_SELECTION_QUIT: {
				output->close_requested = true;
			} break;
			default: { panic(); };
		}
	}

	if(input_button_pressed(input->players[0].buttons[BUTTON_QUIT])) {
		session->mode = SESSION_ACTIVE;
	}

	draw_active_session(session, &output->render_list, fonts, frame_stack, dt);
	draw_player_views(session, &output->render_list, fonts, frame_stack, dt);

	f32 line_h = 64.0f;
	f32 root_y = ((f32)NUM_PAUSE_SELECTIONS * line_h) / 2.0f;
	for(i32 selection_index = 0; selection_index < NUM_PAUSE_SELECTIONS; selection_index++) {
		v4 color = v4_new(0.0f, 1.0f, 0.0f, 0.5f);
		if(selection_index == session->pause_selection) color = v4_new(1.0f, 1.0f, 0.0f, 0.5f);
		ui_draw_text_line(&output->render_list, fonts, ASSET_FONT_QUANTICO_LARGE, pause_selection_strings[selection_index],
			v2_new(0.0f, root_y - line_h * selection_index), v2_new(0.5f, 0.5f), v2_new(0.5f, 0.5f), color, frame_stack);
	}
}
