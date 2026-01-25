#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "asset_format.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.h"
#include "input.c"

typedef struct {
	f32 ship_direction;
	f32 ship_rotation_velocity;
	f32 ship_position[2];
	f32 ship_velocity[2];
	// TODO: Might we want to have this be part of an input handler which just has
	// a list of button states per player?
	ButtonState button_states[NUM_BUTTONS];
} GamePlayer;

typedef struct {
	GamePlayer players[2];
	f32 camera_offset[2];
	GameKeyMapping key_mappings[MAX_KEY_MAPPINGS];
	u32 key_mappings_len;
	FontData fonts[ASSET_NUM_FONTS];
	u8 frame;
} GameState;

typedef struct {
} GameScratch;

typedef struct {
	GameState state;
	GameScratch scratch;
} GameMemory;

typedef struct {
	bool close_requested;
	RenderList render_list;
} GameOutput;

typedef enum {
	GAME_EVENT_MOUSE_MOVED,
	GAME_EVENT_KEY_DOWN,
	GAME_EVENT_KEY_UP,
	GAME_EVENT_NEW_CONNECTION,
	GAME_EVENT_PACKET_RECEIVED
} GameEventType;

typedef struct GameEvent {
	struct GameEvent* next;
	GameEventType type;
	void* data;
} GameEvent;

#define GAME_INIT(name) void name(GameMemory* memory, AssetPack* asset_pack)
typedef GAME_INIT(GameInitFunction);
GAME_INIT(game_init_stub) {}

#define GAME_UPDATE(name) void name(GameMemory* memory, GameEvent* events_head, GameOutput* output, f32 dt)
typedef GAME_UPDATE(GameUpdateFunction);
GAME_UPDATE(game_update_stub) {}

#endif // game_h_INCLUDED
