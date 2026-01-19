#ifndef game_h_INCLUDED
#define game_h_INCLUDED

#include "renderer/render_list.h"

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

#endif // game_h_INCLUDED
