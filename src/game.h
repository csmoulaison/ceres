#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "asset_format.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.h"
#include "input.c"

#define GAME_SOUND_CHANNELS_COUNT 16
#define GAME_UI_MEMSIZE MEGABYTE
#define GAME_EDITOR_TOOLS true
#define MAX_GAME_LEVEL_SIDE_LENGTH 1024

#if GAME_EDITOR_TOOLS
typedef struct {
	v2 camera_position;
	u32 cursor_x;
	u32 cursor_y;
	u32 cursor_object;
} LevelEditor;
#endif

typedef struct {
	f32 phase;
	f32 frequency;
	f32 amplitude;
	f32 shelf;
	f32 volatility;
	f32 pan;

	f32 actual_frequency;
	f32 actual_amplitude;
} GameSoundChannel;

typedef struct {
	v2 position;
	v2 velocity;
	f32 direction;
	f32 rotation_velocity;
	f32 strafe_tilt;

	f32 health;
	i32 score;

	f32 visible_health;
	f32 shoot_cooldown;
	f32 hit_cooldown;
	f32 momentum_cooldown_sound;
	f32 shoot_cooldown_sound;

	// TODO: Might we want to have this be part of an input handler which just has
	// a list of button states per player?
	ButtonState button_states[NUM_BUTTONS];
} GamePlayer;

typedef struct {
	v2 offset;
} GameCamera;

typedef struct {
	v3 position;
	f32 pitch;
	f32 yaw;
} DebugCamera;

typedef enum {
	GAME_ACTIVE,
#if GAME_EDITOR_TOOLS
	GAME_LEVEL_EDITOR
#endif
} GameMode;

typedef struct {
	u16 side_length;
	u8 tiles[MAX_GAME_LEVEL_SIDE_LENGTH * MAX_GAME_LEVEL_SIDE_LENGTH];
} GameLevel;

typedef struct {
	f32 opacity;
	u8 mesh;
	u8 texture;
	v3 position;
	v3 velocity;
	v3 orientation;
	v3 rotation_velocity;
} GameDestructMesh;

typedef struct {
	GameMode mode;
	GameLevel level;

	GamePlayer players[2];
	GameCamera cameras[2];
	GameDestructMesh destruct_meshes[6];

	GameKeyMapping key_mappings[MAX_KEY_MAPPINGS];
	u32 key_mappings_len;

	GameSoundChannel sound_channels[GAME_SOUND_CHANNELS_COUNT];

	FontData fonts[ASSET_NUM_FONTS];
	u32 frame;

#if GAME_EDITOR_TOOLS
	LevelEditor level_editor;
#endif
} GameState;

typedef struct {
	u8 ui_memory[GAME_UI_MEMSIZE];
} GameTransient;

typedef struct {
	GameState state;
	GameTransient transient;
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

#define GAME_INIT(name) void name(GameMemory* memory, AssetMemory* assets)
typedef GAME_INIT(GameInitFunction);
GAME_INIT(game_init_stub) {}

#define GAME_UPDATE(name) void name(GameMemory* memory, GameEvent* events_head, GameOutput* output, f32 dt)
typedef GAME_UPDATE(GameUpdateFunction);
GAME_UPDATE(game_update_stub) {}

#define GAME_GENERATE_SOUND_SAMPLES(name) void name(GameMemory* memory, i16* buffer, i32 samples_count)
typedef GAME_GENERATE_SOUND_SAMPLES(GameGenerateSoundSamplesFunction);
GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples_stub) {}

#endif // game_h_INCLUDED
