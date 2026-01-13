typedef enum {
	PLATFORM_EVENT_TERMINATOR,
	PLATFORM_EVENT_MOUSE_MOVED,
	PLATFORM_EVENT_BUTTON_DOWN,
	PLATFORM_EVENT_BUTTON_UP,
	PLATFORM_EVENT_NEW_CONNECTION,
	PLATFORM_EVENT_PACKET_RECEIVED
} PlatformEventType;

typedef struct PlatformEvent {
	struct PlatformEvent* next;
	PlatformEventType type;
	void* data;
} PlatformEvent;

typedef struct {
	void* backend;

	bool viewport_update_requested;
	i32 window_width;
	i32 window_height;
	u64 frames_since_init;

	PlatformEvent* head_event;
	PlatformEvent* current_event;
} Platform;

PlatformEvent* platform_poll_next_event(Platform* platform);
