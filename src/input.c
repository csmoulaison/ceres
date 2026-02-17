#define CONFIG_DEFAULT_INPUT_FILENAME "data/def_input.conf"

#define INPUT_MAX_PLAYERS 2
#define INPUT_MAX_KEY_MAPPINGS NUM_BUTTONS * 4

#define INPUT_DOWN_BIT 0b00000001
#define INPUT_PRESSED_BIT 0b00000010
#define INPUT_RELEASED_BIT 0b00000100

typedef enum {
	BUTTON_FORWARD,
	BUTTON_BACK,
	BUTTON_TURN_LEFT,
	BUTTON_TURN_RIGHT,
	BUTTON_STRAFE_LEFT,
	BUTTON_STRAFE_RIGHT,
	BUTTON_SHOOT,
	BUTTON_DEBUG,
	BUTTON_QUIT,
	BUTTON_CLEAR,
	BUTTON_SWITCH,
	NUM_BUTTONS
} InputButtonType;

typedef u8 InputButton;

typedef struct {
	u64 key_id;
	u8 player_index;
	InputButtonType button_type;
} InputKeyMapping;

typedef struct {
	InputButton buttons[NUM_BUTTONS];
} InputPlayer;

typedef struct {
	InputPlayer players[INPUT_MAX_PLAYERS];
	InputKeyMapping key_mappings[INPUT_MAX_KEY_MAPPINGS];
	u32 key_mappings_len;
} InputState;

bool input_button_down(InputButton button) {
	return button & INPUT_DOWN_BIT;
}

bool input_button_pressed(InputButton button) {
	return button & INPUT_PRESSED_BIT;
}

bool input_button_released(InputButton button) {
	return button & INPUT_RELEASED_BIT;
}

void input_init(InputState* input) {
	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&input->key_mappings_len, sizeof(u32), 1, file);
	for(i32 km = 0; km < input->key_mappings_len; km++) {
		fread(&input->key_mappings[km], sizeof(InputKeyMapping), 1, file);
	}

	for(i32 pl = 0; pl < 2; pl++) {
		InputButton* buttons = input->players[pl].buttons;
		for(i32 btn = 0; btn < NUM_BUTTONS; btn++) {
			buttons[btn] = 0;
		}
	}

}

void input_poll_events(InputState* input, GameEvent* events_head) {
	// Reset button pressed and released states
	for(i32 pl = 0; pl < 2; pl++) {
		InputButton* buttons = input->players[pl].buttons;
		for(i32 btn = 0; btn < NUM_BUTTONS; btn++) {
			buttons[btn] = buttons[btn] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
		}
	}

	// Poll key down and up events
	GameEvent* event = events_head;
	while(event != NULL) {
		switch(event->type) {
			case GAME_EVENT_KEY_DOWN: {
				for(i32 km = 0; km < input->key_mappings_len; km++) {
					InputKeyMapping* mapping = &input->key_mappings[km];
					if(mapping->key_id == *((u64*)event->data)) {
						InputButton* button = &input->players[mapping->player_index].buttons[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							break;
						}
						*button = (*button) | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
						break;
					}
				}
			} break;
			case GAME_EVENT_KEY_UP: {
				for(i32 km = 0; km < input->key_mappings_len; km++) {
					InputKeyMapping* mapping = &input->key_mappings[km];
					if(mapping->key_id == *((u64*)event->data)) {
						InputButton* button = &input->players[mapping->player_index].buttons[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							*button = INPUT_RELEASED_BIT;
						}
						break;
					}
				}
			} break;
			default: break;
		}
		event = event->next;
	}
}
