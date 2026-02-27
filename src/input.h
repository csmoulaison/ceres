#ifndef input_h_INCLUDED
#define input_h_INCLUDED

#include "config.c"

#define CONFIG_DEFAULT_INPUT_FILENAME "data/def_input.conf"

#define INPUT_MAX_DEVICES 8
#define INPUT_MAX_MAPS 2
#define INPUT_MAX_KEY_MAPPINGS NUM_BUTTONS * INPUT_MAX_MAPS

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
	u8 map_index;
	InputButtonType button_type;
} InputKeyMapping;

typedef struct {
	InputButton buttons[NUM_BUTTONS];
} InputDevice;

typedef struct {
	InputDevice devices[INPUT_MAX_DEVICES];
	InputKeyMapping key_mappings[INPUT_MAX_KEY_MAPPINGS];
	u32 key_mappings_len;
} Input;

#endif // input_h_INCLUDED
