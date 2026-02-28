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

	switch(menu_list((u8*)&menu->selection, NUM_PAUSE_SELECTIONS, menu->input_up, menu->input_down, menu->input_select)) {
		case PAUSE_SELECTION_RESUME: {
			session->mode = SESSION_ACTIVE;
		} break;
		case PAUSE_SELECTION_QUIT: {
			session->quit_requested = true;
		} break;
		case -1: break;
		default: { panic(); };
	}

	if(menu->input_resume) {
		session->mode = SESSION_ACTIVE;
	}
}

void draw_pause_menu(Session* session, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	PauseMenu* menu = &session->pause_menu;
	draw_menu_list(menu->selection, NUM_PAUSE_SELECTIONS, pause_selection_strings, list, fonts, draw_stack);
}
