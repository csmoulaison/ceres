#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define RENDER_MAX_VERTEX_ATTRIBUTES 16
#define RENDER_MAX_HOST_BUFFERS 32

#define RENDER_WORLD_UBO_BUFFER 0
#define RENDER_INSTANCE_UBO_BUFFER 1

typedef enum {
	GRAPHICS_API_OPENGL
} GraphicsApi;

// Renderer (not backend) side references to data
typedef u8 RenderProgram;
typedef u8 RenderMesh;
typedef u8 RenderTexture;
typedef u8 RenderUbo;
typedef struct {
	u8 id;
	u8* data;
} RenderHostBuffer;

// Render commands are populated by the host and read in sequential order by the
// renderer implementation. Later, we might want to change this to a render
// graph with dependencies and all of that.
typedef enum {
	RENDER_COMMAND_NULL,
	RENDER_COMMAND_CLEAR,
	RENDER_COMMAND_USE_PROGRAM,
	RENDER_COMMAND_USE_UBO,
	RENDER_COMMAND_BUFFER_UBO_DATA,
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

// Render command types
typedef struct {
	float color[4];
} RenderCommandClear;

typedef struct {
	RenderProgram program;
} RenderCommandUseProgram;

typedef struct {
	RenderUbo ubo;
} RenderCommandUseUbo;

typedef struct {
	RenderUbo ubo;
	u32 host_buffer_index;
	u64 host_buffer_offset;
} RenderCommandBufferUboData;

typedef struct {
	RenderMesh mesh;
} RenderCommandDrawMesh;

// Key here is the data oriented approach. The association between the index of
// a resource is known by the host of the renderer, either by hardcoded enums or
// a loaded data format that keeps track of the linkages.
typedef struct {
	void* backend;
	GraphicsApi graphics_api;
	RenderGraph* graph;
	i32 frames_since_init;

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

	RenderHostBuffer* host_buffers;
	u32 host_buffers_len;
} Renderer;

// Initialization data which is only used during renderer startup. Right now
// these are being populated manually, but we want to move towards loading these
// from a file at startup.
typedef struct RenderProgramInitData {
	struct RenderProgramInitData* next;
	char* vertex_shader_filename;
	char* fragment_shader_filename;
} RenderProgramInitData;

typedef struct RenderMeshInitData {
	struct RenderMeshInitData* next;

	f32* vertex_data;
	u32 vertex_attribute_sizes[RENDER_MAX_VERTEX_ATTRIBUTES];
	u32 vertex_attributes_len;
	u32 vertices_len;

	u32* indices;
	u32 indices_len;

	bool flat_shading;
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
	u64 binding;
} RenderUboInitData;

typedef struct RenderHostBufferInitData {
	struct RenderHostBufferInitData* next;
} RenderHostBufferInitData;

typedef struct {
	RenderProgramInitData* programs;
	u32 programs_len;

	RenderMeshInitData* meshes;
	u32 meshes_len;

	RenderTextureInitData* textures;
	u32 textures_len;

	RenderUboInitData* ubos;
	u32 ubos_len;

	RenderHostBufferInitData* host_buffers;
	u32 host_buffers_len;
} RenderInitData;

#endif // renderer_h_INCLUDED
