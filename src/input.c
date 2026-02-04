#define CONFIG_DEFAULT_INPUT_FILENAME "data/def_input.conf"

#define INPUT_MAX_PLAYERS 2
#define MAX_KEY_MAPPINGS NUM_BUTTONS * 4

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
	NUM_BUTTONS
} InputButtonType;

typedef u8 ButtonState;

typedef struct {
	u64 key_id;
	u8 player_index;
	InputButtonType button_type;
} GameKeyMapping;

bool input_button_down(ButtonState button) {
	return button & INPUT_DOWN_BIT;
}

bool input_button_pressed(ButtonState button) {
	return button & INPUT_PRESSED_BIT;
}

bool input_button_released(ButtonState button) {
	return button & INPUT_RELEASED_BIT;
}
