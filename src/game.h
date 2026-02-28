#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "asset_format.h"
#include "generated/asset_handles.h"
#include "renderer/render_list.h"
#include "game_event.h"
#include "input.h"
#include "audio.c"

#define GAME_MODE_MEMSIZE MEGABYTE
#define GAME_FRAME_MEMSIZE MEGABYTE

typedef enum {
	GAME_MENU,
	GAME_SESSION
} GameModeType;

// TODO: Calculate memory needed for a given session by accounting for things
// like level data, number of players, etc.
typedef struct {
	u8 memory[GAME_MODE_MEMSIZE];
} GameModeMemory;

typedef struct {
	u8 memory[GAME_FRAME_MEMSIZE];
} GameFrameMemory;

typedef struct {
	Input input;
	Audio audio;
	FontData fonts[ASSET_NUM_FONTS];
	u32 frames_since_init;

	GameModeType mode_type;
	GameModeMemory mode;
	GameFrameMemory frame;
} GameMemory;

typedef struct {
	bool close_requested;
} FrameOutput;

#define GAME_INIT(name) void name(GameMemory* memory, AssetMemory* assets)
typedef GAME_INIT(GameInitFunction);
GAME_INIT(game_init_stub) {}

#define GAME_UPDATE(name) void name(GameMemory* memory, GameEvent* events_head, FrameOutput* output, AssetMemory* assets, f32 dt)
typedef GAME_UPDATE(GameUpdateFunction);
GAME_UPDATE(game_update_stub) {}

#define GAME_GENERATE_RENDER_LIST(name) void name(GameMemory* memory, RenderList* list)
typedef GAME_GENERATE_RENDER_LIST(GameGenerateRenderListFunction);
GAME_GENERATE_RENDER_LIST(game_generate_render_list_stub) {}

#define GAME_GENERATE_SOUND_SAMPLES(name) void name(GameMemory* memory, i16* buffer, i32 samples_count)
typedef GAME_GENERATE_SOUND_SAMPLES(GameGenerateSoundSamplesFunction);
GAME_GENERATE_SOUND_SAMPLES(game_generate_sound_samples_stub) {}

#endif // game_h_INCLUDED
