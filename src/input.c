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

void input_init(InputState* input) {
	memset(input, 0, sizeof(InputState));
	for(i32 map_index = 0; map_index < INPUT_MAX_MAPS; map_index++) {
		input->map_to_player[map_index] = -1;
	}

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&input->key_mappings_len, sizeof(u32), 1, file);
	for(i32 km = 0; km < input->key_mappings_len; km++) {
		fread(&input->key_mappings[km], sizeof(InputKeyMapping), 1, file);
	}
}

void input_attach_map(InputState* input, i8 map, u8 player) {
	input->map_to_player[map] = player;
}

void input_poll_events(InputState* input, GameEvent* events_head) {
	// Reset button pressed and released states
	for(i32 player_index = 0; player_index < MAX_PLAYERS; player_index++) {
		InputButton* buttons = input->players[player_index].buttons;
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
						InputButton* button = &input->players[input->map_to_player[mapping->map_index]].buttons[mapping->button_type];
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
						InputButton* button = &input->players[input->map_to_player[mapping->map_index]].buttons[mapping->button_type];
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
