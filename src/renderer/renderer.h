#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "renderer/primitive_vbo_data.h"

#define RENDER_MAX_VERTEX_ATTRIBUTES 16

typedef enum {
	RENDER_UBO_CAMERA = 0,
	NUM_RENDER_UBOS
} RenderUboType;

typedef enum {
	RENDER_SSBO_TEXT = 0,
	RENDER_SSBO_MODEL,
	NUM_RENDER_SSBOS
} RenderSsboType;

typedef enum {
	RENDER_HOST_BUFFER_CAMERA = 0,
	RENDER_HOST_BUFFER_MODEL,
	RENDER_HOST_BUFFER_LASER,
	RENDER_HOST_BUFFER_TEXT,
	NUM_RENDER_HOST_BUFFERS
} RenderHostBufferType;

// Renderer (not backend) side references to data
typedef struct {
	u8* data;
} RenderHostBuffer;

typedef enum {
	GRAPHICS_API_OPENGL
} GraphicsApi;

// Render commands are populated by the host and read in sequential order by the
// renderer implementation. Later, we might want to change this to a render
// graph with dependencies and all of that.
typedef enum {
	RENDER_COMMAND_NULL,
	RENDER_COMMAND_CLEAR,
	RENDER_COMMAND_SET_VIEWPORT,
	RENDER_COMMAND_USE_PROGRAM,
	RENDER_COMMAND_USE_UBO,
	RENDER_COMMAND_USE_SSBO,
	RENDER_COMMAND_USE_TEXTURE,
	RENDER_COMMAND_BUFFER_UBO_DATA,
	RENDER_COMMAND_BUFFER_SSBO_DATA,
	RENDER_COMMAND_DRAW_MESH,
	RENDER_COMMAND_DRAW_MESH_INSTANCED
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
	v4 rect;
} RenderCommandSetViewport;

typedef struct {
	i32 program;
} RenderCommandUseProgram;

typedef struct {
	i32 ubo;
} RenderCommandUseUbo;

typedef struct {
	i32 ssbo;
} RenderCommandUseSsbo;

typedef struct {
	i32 texture;
} RenderCommandUseTexture;

typedef struct {
	i32 ubo;
	u32 host_buffer_index;
	u64 host_buffer_offset;
} RenderCommandBufferUboData;

typedef struct {
	i32 ssbo;
	u64 size;
	u32 host_buffer_index;
	u64 host_buffer_offset;
} RenderCommandBufferSsboData;

typedef struct {
	i32 mesh;
} RenderCommandDrawMesh;

typedef struct {
	i32 mesh;
	i32 count;
} RenderCommandDrawMeshInstanced;

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

	u32 model_to_mesh_map[ASSET_NUM_MESHES];
	u32 primitive_to_mesh_map[NUM_RENDER_PRIMITIVES];
	RenderHostBuffer host_buffers[NUM_RENDER_HOST_BUFFERS];
} Renderer;

// Initialization data which is only used during renderer startup. Right now
// these are being populated manually, but we want to move towards loading these
// from a file at startup.
typedef struct RenderProgramInitData {
	struct RenderProgramInitData* next;
	char* vertex_shader_src;
	char* fragment_shader_src;
	i32 vertex_shader_src_len;
	i32 fragment_shader_src_len;
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
	u8 channel_count;;
	u8* pixel_data;
} RenderTextureInitData;

typedef struct RenderUboInitData {
	struct RenderUboInitData* next;
	u64 size;
	u64 binding;
} RenderUboInitData;

typedef struct RenderSsboInitData {
	struct RenderSsboInitData* next;
	u64 size;
	u64 binding;
} RenderSsboInitData;

typedef struct {
	RenderProgramInitData* programs;
	u32 programs_len;

	RenderMeshInitData* meshes;
	u32 meshes_len;

	RenderTextureInitData* textures;
	u32 textures_len;

	RenderUboInitData* ubos;
	u32 ubos_len;

	RenderSsboInitData* ssbos;
	u32 ssbos_len;
} RenderBackendInitData;

#endif // renderer_h_INCLUDED
