#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "input.c"

#include <GL/glx.h>

#define NUM_MAPPINGS 14

i32 main(i32 argc, char** argv) {
	if(argc != 2) {
		printf("usage: input_serializer <output.conf>");
		return 1;
	}

	FILE* file = fopen(argv[1], "w");
	assert(file != NULL);

	GameKeyMapping mappings[NUM_MAPPINGS] = {
		{ .key_id = 0xff52, .player_index = 0, .button_type = BUTTON_FORWARD },
		{ .key_id = 0xff54, .player_index = 0, .button_type = BUTTON_BACK },
		{ .key_id = 0xff51, .player_index = 0, .button_type = BUTTON_TURN_LEFT },
		{ .key_id = 0xff53, .player_index = 0, .button_type = BUTTON_TURN_RIGHT },
		{ .key_id = 0xff55, .player_index = 0, .button_type = BUTTON_STRAFE_LEFT },
		{ .key_id = 0xff56, .player_index = 0, .button_type = BUTTON_STRAFE_RIGHT },
		{ .key_id = 0xff1b, .player_index = 0, .button_type = BUTTON_QUIT },
		{ .key_id = 0x0077, .player_index = 1, .button_type = BUTTON_FORWARD },
		{ .key_id = 0x0073, .player_index = 1, .button_type = BUTTON_BACK },
		{ .key_id = 0x0061, .player_index = 1, .button_type = BUTTON_TURN_LEFT },
		{ .key_id = 0x0064, .player_index = 1, .button_type = BUTTON_TURN_RIGHT },
		{ .key_id = 0x0071, .player_index = 1, .button_type = BUTTON_STRAFE_LEFT },
		{ .key_id = 0x0065, .player_index = 1, .button_type = BUTTON_STRAFE_RIGHT },
		{ .key_id = 0xff1b, .player_index = 1, .button_type = BUTTON_QUIT }
	};
	u32 num_mappings = NUM_MAPPINGS;

	fwrite(&num_mappings, sizeof(u32), 1, file);
	fwrite(&mappings, sizeof(GameKeyMapping) * NUM_MAPPINGS, 1, file);

	fclose(file);
}
