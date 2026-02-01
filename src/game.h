#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "asset_format.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.h"
#include "input.c"

typedef struct {
	f32 direction;
	f32 rotation_velocity;
	f32 strafe_tilt;
	v2 position;
	v2 velocity;
	// TODO: Might we want to have this be part of an input handler which just has
	// a list of button states per player?
	ButtonState button_states[NUM_BUTTONS];
} GamePlayer;

typedef struct {
	v2 offset;
} GameCamera;

typedef struct {
	GamePlayer players[2];
	GameCamera cameras[2];
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

// NOW: We are addding audio now to the game. Start by creating a function like
// game_generate_sound_samples(f32* buffer, u32 samples_requested, f32 start_t?)
//
// The platform must be able to call this function whenever it wants so that it
// can request samples relative to its place in a circular audio samples buffer.
// 
// To reiterate, upcoming audio samples are stored in a ring buffer which is
// populated by the game. The closer the audio hardware "position" is to the
// time of sample generation, the less latency there is between audio and their
// consequent game effects; but, the lower the latency, the less tolerant the
// audio is to frame time variance.
//
// 

#endif // game_h_INCLUDED
