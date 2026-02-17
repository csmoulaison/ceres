#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "asset_format.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.h"
#include "game_event.h"
#include "input.c"

#define SOUND_CHANNELS_COUNT 16
#define GAME_UI_MEMSIZE MEGABYTE
#define GAME_EDITOR_TOOLS true
#define MAX_GAME_LEVEL_SIDE_LENGTH 1024

typedef struct {
	f32 phase;
	f32 frequency;
	f32 amplitude;
	f32 shelf;
	f32 volatility;
	f32 pan;

	f32 actual_frequency;
	f32 actual_amplitude;
} SoundChannel;

typedef enum {
	EDITOR_TOOL_CUBES,
	EDITOR_TOOL_SPAWNS
} LevelEditorTool;

#if GAME_EDITOR_TOOLS
typedef struct {
	v3 camera_position;
	u32 cursor_x;
	u32 cursor_y;
	u32 cursor_object;
	LevelEditorTool tool;
} LevelEditor;
#endif

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
	LevelSpawn spawns[MAX_LEVEL_SPAWNS];
	u8 spawns_len;

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
	InputState input;

	GamePlayer players[2];
	GameCamera cameras[2];
	GameDestructMesh destruct_meshes[6];

	SoundChannel sound_channels[SOUND_CHANNELS_COUNT];

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
