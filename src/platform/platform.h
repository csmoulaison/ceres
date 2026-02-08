#include "game.h"

#define GAME_EVENTS_MEMSIZE sizeof(GameEvent) * 256

typedef struct {
	void* backend;

	bool viewport_update_requested;
	i32 window_width;
	i32 window_height;
	u64 frames_since_init;

	GameEvent* head_event;
	GameEvent* current_event;
	u8 event_memory[GAME_EVENTS_MEMSIZE];
} Platform;
