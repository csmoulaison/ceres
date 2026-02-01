#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "game.h"
#include "renderer/render_list.c"
#include "ui_text.c"

v2 player_direction_vector(GamePlayer* player) {
	return v2_normalize(v2_new(sin(player->direction), cos(player->direction)));
}

GAME_INIT(game_init) {
	GameState* game = &memory->state;
	game->frame = 0;

	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		FontData* font = &game->fonts[i];
		FontAsset* f_asset = (FontAsset*)&asset_pack->buffer[asset_pack->font_buffer_offsets[i]];
		TextureAsset* t_asset = (TextureAsset*)&asset_pack->buffer[asset_pack->texture_buffer_offsets[f_asset->texture_id]];

		font->texture_id = f_asset->texture_id;
		font->texture_width = t_asset->width;
		font->texture_height = t_asset->height;
		font->size = f_asset->buffer['O'].size[1];
		memcpy(font->glyphs, f_asset->buffer, sizeof(FontGlyph) * f_asset->glyphs_len);
	}

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		player->direction = 0.0f;
		player->rotation_velocity = 0.0f;
		player->position = v2_zero();
		player->velocity = v2_zero();

		for(i32 i = 0; i < NUM_BUTTONS; i++) {
			player->button_states[i] = 0;
		}

		GameCamera* camera = &game->cameras[i];
		camera->offset = v2_zero();
	}

	FILE* file = fopen(CONFIG_DEFAULT_INPUT_FILENAME, "r");
	assert(file != NULL);

	fread(&game->key_mappings_len, sizeof(u32), 1, file);
	for(i32 i = 0; i < game->key_mappings_len; i++) {
		fread(&game->key_mappings[i], sizeof(GameKeyMapping), 1, file);
	}

}

GAME_UPDATE(game_update) {
	GameState* game = &memory->state;

	// Update input events
	for(u32 i = 0; i < NUM_BUTTONS; i++) {
		game->players[0].button_states[i] = game->players[0].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
		game->players[1].button_states[i] = game->players[1].button_states[i] & ~INPUT_PRESSED_BIT & ~INPUT_RELEASED_BIT;
	}
	GameEvent* event = events_head;
	while(event != NULL) {
		switch(event->type) {
			case GAME_EVENT_KEY_DOWN: {
				for(i32 i = 0; i < game->key_mappings_len; i++) {
					GameKeyMapping* mapping = &game->key_mappings[i];
					if(mapping->key_id == *((u64*)event->data)) {
						ButtonState* button = &game->players[mapping->player_index].button_states[mapping->button_type];
						if((*button) & INPUT_DOWN_BIT) {
							break;
						}
						*button = (*button) | INPUT_DOWN_BIT | INPUT_PRESSED_BIT;
						break;
					}
				}
			} break;
			case GAME_EVENT_KEY_UP: {
				for(i32 i = 0; i < game->key_mappings_len; i++) {
					GameKeyMapping* mapping = &game->key_mappings[i];
					if(mapping->key_id == *((u64*)event->data)) {
						ButtonState* button = &game->players[mapping->player_index].button_states[mapping->button_type];
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
	
	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];

		// Quit control
		if(input_button_down(player->button_states[BUTTON_QUIT])) {
			output->close_requested = true;
		}

		// Calculate ship rotational acceleration
		f32 rotate_speed = 32.0f;
		f32 rot_acceleration = 0.0f;
		if(input_button_down(player->button_states[BUTTON_TURN_LEFT])) {
			rot_acceleration += rotate_speed;
		}
		if(input_button_down(player->button_states[BUTTON_TURN_RIGHT])) {
			rot_acceleration -= rotate_speed;
		}

		// Rotational damping
		f32 rot_damping = 2.0f;
		rot_acceleration += player->rotation_velocity * -rot_damping;

		// Apply rotational acceleration
		player->direction = 0.5f * rot_acceleration * dt * dt + player->rotation_velocity * dt + player->direction;
		player->rotation_velocity += rot_acceleration * dt;

		// Calculate ship acceleration
		// Forward/back thruster control
		v2 acceleration = v2_zero();

		v2 direction_vector = player_direction_vector(player);
		f32 forward_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_FORWARD])) {
			forward_mod += 32.0f;
		}
		if(input_button_down(player->button_states[BUTTON_BACK])) {
			forward_mod -= 16.0f;
		}
		acceleration = v2_scale(direction_vector, forward_mod);

		// Side thruster control
		v2 side_vector = { -direction_vector.y, direction_vector.x };
		f32 strafe_speed = 32.0f;
		f32 strafe_mod = 0.0f;
		if(input_button_down(player->button_states[BUTTON_STRAFE_LEFT])) {
			strafe_mod -= 1.0f;
		}
		if(input_button_down(player->button_states[BUTTON_STRAFE_RIGHT])) {
			strafe_mod += 1.0f;
		}
		acceleration = v2_add(acceleration, v2_scale(side_vector, strafe_mod * strafe_speed));

		// TODO: Make strafing tilt curve based off of the same equations as motion 
		f32 strafe_tilt_target = -strafe_mod * 0.66f;
		player->strafe_tilt = lerp(player->strafe_tilt, strafe_tilt_target, dt * 6.0f);

		// Damp acceleration
		f32 damping = 2.0f;
		acceleration = v2_add(acceleration, v2_scale(player->velocity, -damping));

		// Apply acceleration to position
		// (acceleration / 2) * dt^2 + velocity * t + position
		v2 accel_dt = v2_scale(v2_scale(acceleration, 0.5f), dt * dt);
		v2 velocity_dt = v2_scale(player->velocity, dt);
		player->position = v2_add(player->position, v2_add(accel_dt, velocity_dt));

		// Update player velocity
		player->velocity = v2_add(player->velocity, v2_scale(acceleration, dt));

		// Update camera
		// 
		// TODO: Not every player will necessarily get a camera in the future. Bots
		// and online play, for instance.
		GameCamera* camera = &game->cameras[i];
		f32 camera_lookahead = 4.0f;
		v2 camera_target_offset = v2_scale(direction_vector, camera_lookahead);

		f32 camera_speed_mod = 2.0f;
		v2 camera_target_delta = v2_sub(camera_target_offset, camera->offset);
		camera_target_delta = v2_scale(camera_target_delta, camera_speed_mod * dt);
		camera->offset = v2_add(camera->offset, camera_target_delta);

	}

	// Populate render list
	RenderList* list = &output->render_list;
	render_list_init(list);

	v3 clear_color = v3_new(0.0f, 0.0f, 0.0f);
	render_list_set_clear_color(list, clear_color);

	for(i32 i = 0; i < 2; i++) {
		GamePlayer* player = &game->players[i];
		v3 pos = v3_new(player->position.x, 0.5f, player->position.y);
		f32 tilt = player->strafe_tilt + clamp(player->rotation_velocity, -90.0f, 90.0f) * 0.05f;
		v3 rot = v3_new(-tilt, player->direction, 0.0f);
		render_list_draw_model(list, ASSET_MESH_SHIP, ASSET_TEXTURE_SHIP, pos, rot);

		// TODO: Disassociate camera from player index.
		GameCamera* camera = &game->cameras[i];
		v3 cam_target = v3_new(camera->offset.x + player->position.x, 0.0f, camera->offset.y + player->position.y);
		v3 cam_pos = v3_new(cam_target.x + 4.0f, 8.0f, cam_target.z);
		f32 gap = 0.0025f;
		v4 screen_rect = v4_new(0.5f * i + gap * i, 0.0f, 0.5f - gap * (1 - i), 1.0f);
		render_list_add_camera(list, cam_pos, cam_target, screen_rect);
	}

	i32 floor_instances = 1024;
	for(i32 i = 0; i < floor_instances; i++) {
		v3 floor_pos = v3_new(-15.5f + (i % 32), 0.0f, -15.5f + (i / 32));
		v3 floor_rot = v3_new(0.0f , 0.0f, 0.0f);
		render_list_draw_model(list, ASSET_MESH_FLOOR, ASSET_TEXTURE_FLOOR, floor_pos, floor_rot);
	}

	Arena ui_arena;
	arena_init(&ui_arena, MEGABYTE, NULL, "UI");

	GamePlayer* pl_primary = &game->players[0];
	GameCamera* cam_primary = &game->cameras[0];
#define PRINT_VALUES_LEN 4
	f32 print_values[PRINT_VALUES_LEN] = {
		pl_primary->position.x,
		pl_primary->position.y,
		pl_primary->velocity.x,
		pl_primary->velocity.y,
	};
	char* print_labels[PRINT_VALUES_LEN] = {
		"pos_x: ",
		"pos_y: ",
		"vel_x: ",
		"vel_y: "
	};
	v2 debug_screen_anchor = v2_zero();
	for(i32 i = 0; i < PRINT_VALUES_LEN; i++) {
		char str[256];
		sprintf(str, "%s%.2f", print_labels[i], print_values[i]);

		v2 debug_position = v2_new(32.0f, 12.0f + (PRINT_VALUES_LEN - i) * 24.0f);
		v2 debug_inner_anchor = v2_zero();
		TextLinePlacements placements = ui_text_line_placements(game->fonts, ASSET_FONT_MONO_SMALL, str,
			debug_position, debug_inner_anchor, &ui_arena);

		f32 gb_mod = 1.0f;
		for(i32 j = 0; j < placements.len; j++) {
			v2 position = v2_new(placements.x[j], placements.y[j]);
			v4 color = v4_new(0.75f, 0.75f * gb_mod, 0.75f * gb_mod, 1.0f);

			if(str[j] == ':') {
				gb_mod = 0.1f;
			}

			render_list_draw_glyph(list, game->fonts, ASSET_FONT_MONO_SMALL, str[j], position, debug_screen_anchor, color);
		}
	}

	v4 color = v4_new(0.4f, 0.5f, 0.7f, 1.0f);
	v2 title_position = v2_new(32.0f, -32.0f);
	v2 title_inner_anchor = v2_new(0.0f, 1.0f);
	v2 title_screen_anchor = v2_new(0.0f, 1.0f);
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_LARGE, "Sector Seven or some shit",
		title_position, title_inner_anchor, title_screen_anchor, color, &ui_arena);

	v4 color_neu = v4_new(0.4f, 0.7f, 0.5f, 1.0f);
	title_position.y -= 64.0f;
	ui_draw_text_line(list, game->fonts, ASSET_FONT_OVO_REGULAR, "A game about love, life, and loss.",
		title_position, title_inner_anchor, title_screen_anchor, color_neu, &ui_arena);

	arena_destroy(&ui_arena);
		
	game->frame++;
}
