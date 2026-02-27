#include "input.h"

bool input_button_down(InputButton button) {
	return button & INPUT_DOWN_BIT;
}

bool input_button_pressed(InputButton button) {
	return button & INPUT_PRESSED_BIT;
}

bool input_button_released(InputButton button) {
	return button & INPUT_RELEASED_BIT;
}

void input_init(Input* input) {
	memset(input, 0, sizeof(Input));

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&input->key_mappings_len, sizeof(u32), 1, file);
	for(i32 mapping_index = 0; mapping_index < input->key_mappings_len; mapping_index++) {
		fread(&input->key_mappings[mapping_index], sizeof(InputKeyMapping), 1, file);
	}
}

void input_poll_events(Input* input, GameEvent* events_head) {
	// Reset button pressed and released states
	for(i32 device_index = 0; device_index < MAX_PLAYERS; device_index++) {
		InputButton* buttons = input->devices[device_index].buttons;
		for(i32 button_index = 0; button_index < NUM_BUTTONS; button_index++) {
			buttons[button_index] = buttons[button_index] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
		}
	}

	// Poll key down and up events
	GameEvent* event = events_head;
	while(event != NULL) {
		switch(event->type) {
			case GAME_EVENT_KEY_DOWN: {
				for(i32 mapping_index = 0; mapping_index < input->key_mappings_len; mapping_index++) {
					InputKeyMapping* mapping = &input->key_mappings[mapping_index];
					if(mapping->key_id == *((u64*)event->data)) {
						InputButton* button = &input->devices[mapping->map_index].buttons[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							break;
						}
						*button = (*button) | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
						break;
					}
				}
			} break;
			case GAME_EVENT_KEY_UP: {
				for(i32 mapping_index = 0; mapping_index < input->key_mappings_len; mapping_index++) {
					InputKeyMapping* mapping = &input->key_mappings[mapping_index];
					if(mapping->key_id == *((u64*)event->data)) {
						InputButton* button = &input->devices[mapping->map_index].buttons[mapping->button_type];
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
