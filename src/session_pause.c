char* pause_selection_strings[NUM_PAUSE_SELECTIONS] = {
	"Resume",
	"Quit"
};

void session_pause_update(Session* session, FrameOutput* output, Input* input, Audio* audio, f32 dt) {
	PauseMenu* menu = &session->pause_menu;
	menu->input_up = false;
	menu->input_down = false;
	menu->input_select = false;
	menu->input_resume = false;
	InputDevice* device = &input->devices[0];
	if(input_button_pressed(device->buttons[BUTTON_FORWARD])) menu->input_up = true;
	if(input_button_pressed(device->buttons[BUTTON_BACK])) menu->input_down = true;
	if(input_button_pressed(device->buttons[BUTTON_SHOOT])) menu->input_select = true;
	if(input_button_pressed(device->buttons[BUTTON_QUIT])) menu->input_resume = true;

	if(menu->input_up) {
		if(menu->selection <= 0) {
			menu->selection = NUM_PAUSE_SELECTIONS - 1;
		} else {
			menu->selection--;
		}
	}
	if(menu->input_down) {
		if(menu->selection >= NUM_PAUSE_SELECTIONS - 1) {
			menu->selection = 0;
		} else {
			menu->selection++;
		}
	}

	if(menu->input_select) {
		switch(menu->selection) {
			case PAUSE_SELECTION_RESUME: {
				session->mode = SESSION_ACTIVE;
			} break;
			case PAUSE_SELECTION_QUIT: {
				session->quit_requested = true;
			} break;
			default: { panic(); };
		}
	}

	if(menu->input_resume) {
		session->mode = SESSION_ACTIVE;
	}
}

void draw_pause_menu(Session* session, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	PauseMenu* menu = &session->pause_menu;
	f32 line_h = 64.0f;
	f32 root_y = ((f32)NUM_PAUSE_SELECTIONS * line_h) / 2.0f;
	for(i32 selection_index = 0; selection_index < NUM_PAUSE_SELECTIONS; selection_index++) {
		v4 color = v4_new(0.0f, 1.0f, 0.0f, 0.5f);
		if(selection_index == menu->selection) color = v4_new(1.0f, 1.0f, 0.0f, 0.5f);
		ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_LARGE, pause_selection_strings[selection_index],
			v2_new(0.0f, root_y - line_h * selection_index), v2_new(0.5f, 0.5f), v2_new(0.5f, 0.5f), color, draw_stack);
	}
}
