typedef enum {
	MENU_SELECTION_PLAY,
	MENU_SELECTION_QUIT,
	NUM_MENU_SELECTIONS
} MenuSelection;

char* menu_selection_strings[NUM_MENU_SELECTIONS] = {
	"Play",
	"Quit"
};

typedef struct {
	MenuSelection selection;
	bool start_session;
	bool input_up;
	bool input_down;
	bool input_select;
} MainMenu;

// TODO: Factor this and pause menu into generic menu stuff. Just a shared
// function for selection index wrapping and stuff would work.
void main_menu_update(MainMenu* menu, FrameOutput* output, Input* input, Audio* audio, f32 dt) {
	menu->input_up = false;
	menu->input_down = false;
	menu->input_select = false;
	InputDevice* device = &input->devices[0];
	if(input_button_pressed(device->buttons[BUTTON_FORWARD])) menu->input_up = true;
	if(input_button_pressed(device->buttons[BUTTON_BACK])) menu->input_down = true;
	if(input_button_pressed(device->buttons[BUTTON_SHOOT])) menu->input_select = true;

	if(menu->input_up) {
		if(menu->selection <= 0) {
			menu->selection = NUM_MENU_SELECTIONS - 1;
		} else {
			menu->selection--;
		}
	}
	if(menu->input_down) {
		if(menu->selection >= NUM_MENU_SELECTIONS - 1) {
			menu->selection = 0;
		} else {
			menu->selection++;
		}
	}

	if(menu->input_select) {
		switch(menu->selection) {
			case MENU_SELECTION_PLAY: {
				menu->start_session = true;
			} break;
			case MENU_SELECTION_QUIT: {
				output->close_requested = true;
			} break;
			default: { panic(); };
		}
	}
}

void draw_main_menu(MainMenu* menu, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	f32 line_h = 64.0f;
	f32 root_y = ((f32)NUM_MENU_SELECTIONS * line_h) / 2.0f;
	for(i32 selection_index = 0; selection_index < NUM_MENU_SELECTIONS; selection_index++) {
		v4 color = v4_new(0.0f, 1.0f, 0.0f, 0.5f);
		if(selection_index == menu->selection) color = v4_new(1.0f, 1.0f, 0.0f, 0.5f);
		ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_LARGE, menu_selection_strings[selection_index],
			v2_new(0.0f, root_y - line_h * selection_index), v2_new(0.5f, 0.5f), v2_new(0.5f, 0.5f), color, draw_stack);
	}
}
