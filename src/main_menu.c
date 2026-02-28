typedef enum {
	MENU_SELECTION_ONE_PLAYER,
	MENU_SELECTION_TWO_PLAYER,
	MENU_SELECTION_QUIT,
	NUM_MENU_SELECTIONS
} MenuSelection;

char* menu_selection_strings[NUM_MENU_SELECTIONS] = {
	"One Player",
	"Two Player",
	"Quit"
};

typedef struct {
	u8 player_count;
} SessionSettings;

typedef struct {
	MenuSelection selection;
	bool input_up;
	bool input_down;
	bool input_select;

	bool start_session;
	SessionSettings session_settings;
} MainMenu;

void main_menu_init(MainMenu* menu) {
	memset(menu, 0, sizeof(MainMenu));
}

void main_menu_update(MainMenu* menu, FrameOutput* output, Input* input, Audio* audio, f32 dt) {
	menu->input_up = false;
	menu->input_down = false;
	menu->input_select = false;
	InputDevice* device = &input->devices[0];
	if(input_button_pressed(device->buttons[BUTTON_FORWARD])) menu->input_up = true;
	if(input_button_pressed(device->buttons[BUTTON_BACK])) menu->input_down = true;
	if(input_button_pressed(device->buttons[BUTTON_SHOOT])) menu->input_select = true;

	switch(menu_list((u8*)&menu->selection, NUM_MENU_SELECTIONS, menu->input_up, menu->input_down, menu->input_select)) {
		case MENU_SELECTION_ONE_PLAYER: {
			menu->session_settings.player_count = 1;
			menu->start_session = true;
		} break;
		case MENU_SELECTION_TWO_PLAYER: {
			menu->session_settings.player_count = 2;
			menu->start_session = true;
		} break;
		case MENU_SELECTION_QUIT: {
			output->close_requested = true;
		} break;
		case -1: break;
		default: { panic(); };
	}
}

void draw_main_menu(MainMenu* menu, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	draw_menu_list(menu->selection, NUM_MENU_SELECTIONS, menu_selection_strings, list, fonts, draw_stack);
	list->clear_color = v3_new(0.0f, 0.0f, 0.0f);
}
