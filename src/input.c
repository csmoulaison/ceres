#define INPUT_MAX_PLAYERS 2
#define MAX_KEY_MAPPINGS NUM_BUTTONS * 4

#define INPUT_DOWN_BIT 0b00000001
#define INPUT_PRESSED_BIT 0b00000010
#define INPUT_RELEASED_BIT 0b00000100

typedef enum {
	BUTTON_FORWARD = 0,
	BUTTON_BACK,
	BUTTON_TURN_LEFT,
	BUTTON_TURN_RIGHT,
	BUTTON_STRAFE_LEFT,
	BUTTON_STRAFE_RIGHT,
	BUTTON_QUIT,
	NUM_BUTTONS
} ButtonType;

typedef u8 ButtonState;

typedef struct {
	u64 key_id;
	u8 player_index;
	ButtonType button_type;
} GameKeyMapping;
