#include "session.h"
#include "font.h"
#include "level.c"
#include "level_editor.c"
#include "player.c"
#include "physics.c"
#include "session_active.c"
#include "draw.c"

void session_init(Session* session, Input* input, LevelAsset* level_asset) {
	memset(session, 0, sizeof(Session));

	// Level
	Level* level = &session->level;
	level->spawns_len = level_asset->spawns_len;
	for(i32 spawn_index = 0; spawn_index < level->spawns_len; spawn_index++) {
		level->spawns[spawn_index] = level_asset->spawns[spawn_index];
	}

	level->side_length = level_asset->side_length;
	level->side_length = 64;
	u16 side_length = level->side_length;
	assert(side_length <= MAX_LEVEL_SIDE_LENGTH);
	for(i32 tile_index = 0; tile_index < side_length * side_length; tile_index++) {
		i32 x = tile_index % side_length;
		i32 y = tile_index / side_length;
		if(x < 2 || x > 61 || y < 2 || y > 61) {
			level->tiles[tile_index] = 1 + rand() / (RAND_MAX / 3);
		} else {
			level->tiles[tile_index] = level_asset->buffer[tile_index];
		}
	}

	// Players
	session->players_len = 4;
	session->players[0].team = 0;
	session->players[1].team = 0;
	session->players[2].team = 1;
	session->players[3].team = 1;
	for(i32 player_index = 0; player_index < session->players_len; player_index++) {
		Player* player = &session->players[player_index];
		player_spawn(player, session->players, session->players_len, level);
	}

	// Views
	// 
	// NOW: Setting views[0].player to anything other than 0 breaks any input
	// checks that are manually indexing into the 0th player input state.
	session->player_views_len = 1;
	session->player_views[0].player = 0; 
	session->player_views[1].player = 2; 
	for(i32 view_index = 0; view_index < session->player_views_len; view_index++) {
		PlayerView* view = &session->player_views[view_index];

		Player* player = &session->players[view->player];
		view->camera_offset = player->position;
		view->visible_health = 0.0f;

		input_attach_map(input, view_index, view->player);
	}

	// Editor
	session->level_editor.tool = EDITOR_TOOL_CUBES;
}

void session_update(Session* session, GameOutput* output, Input* input, Audio* audio, FontData* fonts, StackAllocator* frame_stack, f32 dt) {
	switch(session->mode) {
		case SESSION_ACTIVE: {
			session_active_update(session, output, input, audio, dt);
			if(input_button_pressed(input->players[0].buttons[BUTTON_QUIT])) {
				session->mode = SESSION_PAUSE;
			}
		} break;
		case SESSION_PAUSE: {
			if(input_button_pressed(input->players[0].buttons[BUTTON_QUIT])) {
				session->mode = SESSION_ACTIVE;
			}
		} break;
		case SESSION_LEVEL_EDITOR: {
			level_editor_update(session, input);
		} break;
		default: break;
	}
	draw_active_session(session, &output->render_list, fonts, frame_stack, dt);
	if(session->mode == SESSION_PAUSE) {
		ui_draw_text_line(&output->render_list, fonts, ASSET_FONT_QUANTICO_LARGE, "Game Paused",
			v2_zero(), v2_new(0.5f, 0.5f), v2_new(0.5f, 0.5f), v4_new(0.0f, 1.0f, 0.0f, 0.5f), frame_stack);
	}
}
