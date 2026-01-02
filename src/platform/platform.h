#define PLATFORM_MAX_BUTTONS 32
#define PLATFORM_INPUT_DOWN_BIT     0b00000001
#define PLATFORM_INPUT_PRESSED_BIT  0b00000010
#define PLATFORM_INPUT_RELEASED_BIT 0b00000100
#define PLATFORM_INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN 256
#define PLATFORM_INPUT_KEYCODE_UNREGISTERED -1

typedef struct {
	void* backend;

	bool viewport_update_requested;
	i32 window_width;
	i32 window_height;

	u32 input_buttons_len;
	u8 input_button_states[PLATFORM_MAX_BUTTONS];
	i16 input_keycode_to_button_lookup[PLATFORM_INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN];
} Platform;
