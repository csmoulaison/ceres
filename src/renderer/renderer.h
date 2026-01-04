#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

typedef u32 RenderProgram;
typedef u32 RenderMesh;
typedef u32 RenderTexture;
typedef u32 RenderUbo;
typedef u32 RenderSsbo;

typedef enum {
	GRAPHICS_API_OPENGL
} GraphicsApi;

// Render commands are populated by the host and read in sequential order by the
// renderer implementation. Later, we might want to change this to a render
// graph with dependencies and all of that.
typedef enum {
	RENDER_COMMAND_NULL,
	RENDER_COMMAND_CLEAR,
	RENDER_COMMAND_BIND_PROGRAM,
	RENDER_COMMAND_BIND_UBO,
	RENDER_COMMAND_DRAW_MESH
} RenderCommandType;

typedef struct RenderCommand {
	struct RenderCommand* next;
	RenderCommandType type;
	void* data;
} RenderCommand;

typedef struct {
	RenderCommand* root;
	RenderCommand* tail;
} RenderGraph;

// Key here is the data oriented approach. The association between the index of
// a resource is known by the host of the renderer, either by hardcoded enums or
// a loaded data format that keeps track of the linkages.
typedef struct {
	void* backend;
	GraphicsApi graphics_api;
	RenderGraph* graph;

	Arena persistent_arena;
	Arena viewport_arena;
	Arena frame_arena;

	RenderProgram* programs;
	u32 programs_len;

	RenderMesh* meshes;
	u32 meshes_len;

	RenderTexture* textures;
	u32 textures_len;

	RenderUbo* ubos;
	u32 ubos_len;
} Renderer;

typedef struct RenderProgramInitData {
	struct RenderProgramInitData* next;
	char* vertex_shader_filename;
	char* fragment_shader_filename;
} RenderProgramInitData;

typedef struct RenderMeshInitData {
	struct RenderMeshInitData* next;
	f32* vertex_data;
	u32 vertex_size;
	u32 vertices_len;
} RenderMeshInitData;

typedef struct RenderTextureInitData {
	struct RenderTextureInitData* next;
	u16 width;
	u16 height;
	u8* pixels;
} RenderTextureInitData;

typedef struct RenderUboInitData {
	struct RenderUboInitData* next;
	u64 size;
} RenderUboInitData;

typedef struct {
	RenderProgramInitData* programs;
	u32 programs_len;

	RenderMeshInitData* meshes;
	u32 meshes_len;

	RenderTextureInitData* textures;
	u32 textures_len;

	RenderUboInitData* ubos;
	u32 ubos_len;
} RenderInitData;

#endif // renderer_h_INCLUDED
