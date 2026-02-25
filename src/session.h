#ifndef session_h_INCLUDED
#define session_h_INCLUDED

#define MAX_TEAMS 2
#define MAX_PLAYER_VIEWS 2
#define MAX_LEVEL_SIDE_LENGTH 1024
#define MAX_DESTRUCT_MESHES MAX_PLAYERS * 3

typedef struct {
	u8 team;
	f32 health;

	v2 position;
	v2 velocity;
	f32 direction;
	f32 rotation_velocity;
	f32 strafe_tilt;

	f32 shoot_cooldown;
	f32 hit_cooldown;
	f32 thruster_cooldown;

	SoundHandle sound_forward_thruster;
	SoundHandle sound_rotation_thruster;
	SoundHandle sound_thruster_cooldown;
	SoundHandle sound_shoot;
	SoundHandle sound_hit;
} Player;

typedef struct {
	u8 player;
	v2 camera_offset;
	f32 visible_health;
} PlayerView;

typedef struct {
	LevelSpawn spawns[MAX_LEVEL_SPAWNS];
	u8 spawns_len;

	u16 side_length;
	u8 tiles[MAX_LEVEL_SIDE_LENGTH * MAX_LEVEL_SIDE_LENGTH];
} Level;

typedef enum {
	EDITOR_TOOL_CUBES,
	EDITOR_TOOL_SPAWNS
} LevelEditorTool;

typedef struct {
	v3 camera_position;
	u32 cursor_x;
	u32 cursor_y;
	u32 cursor_object;
	f32 cursor_animation;
	LevelEditorTool tool;
} LevelEditor;

typedef struct {
	f32 opacity;
	u8 mesh;
	u8 texture;
	v3 position;
	v3 velocity;
	v3 orientation;
	v3 rotation_velocity;
} DestructMesh;

typedef enum {
	SESSION_ACTIVE,
	SESSION_PAUSE,
	SESSION_GAME_OVER,
	SESSION_LEVEL_EDITOR
} SessionMode;

typedef enum {
	PAUSE_SELECTION_RESUME,
	PAUSE_SELECTION_QUIT,
	NUM_PAUSE_SELECTIONS
} PauseSelection;

typedef struct {
	SessionMode mode;
	Level level;
	i32 team_scores[MAX_TEAMS];

	// TODO: Make this specifically a destroyed player struct which has 3 meshes
	// and a sound source
	DestructMesh destruct_meshes[32];
	LevelEditor level_editor;

	Player players[MAX_PLAYERS];
	PlayerView player_views[MAX_PLAYER_VIEWS];
	u8 teams_len;
	u8 players_len;
	u8 player_views_len;

	PauseSelection pause_selection;
	float game_over_rotation_position;
} Session;

#endif // session_h_INCLUDED
