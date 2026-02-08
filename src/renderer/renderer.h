#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#include "renderer/primitive_vbo_data.h"

#define RENDER_INIT_MEMSIZE MEGABYTE * 32
#define RENDER_TRANSIENT_MEMSIZE MEGABYTE * 4

#define RENDER_MAX_VERTEX_ATTRIBUTES 16
#define RENDER_MAX_HOST_BUFFERS 16

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
} RenderCommandList;

// Render command types
typedef struct {
	float color[4];
} RenderCommandClear;

typedef struct {
	v4 rect;
} RenderCommandSetViewport;

typedef struct {
	u32 program;
} RenderCommandUseProgram;

typedef struct {
	u32 ubo;
} RenderCommandUseUbo;

typedef struct {
	u32 ssbo;
} RenderCommandUseSsbo;

typedef struct {
	u32 texture;
} RenderCommandUseTexture;

typedef struct {
	u32 ubo;
	u32 host_buffer_index;
	u64 host_buffer_offset;
} RenderCommandBufferUboData;

typedef struct {
	u32 ssbo;
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

typedef struct {
	void* backend;
	GraphicsApi api;
	i32 frames_since_init;

	u32 model_to_mesh_map[ASSET_NUM_MESHES];
	u32 primitive_to_mesh_map[NUM_RENDER_PRIMITIVES];

	u8* host_buffers[RENDER_MAX_HOST_BUFFERS];
	u8 host_buffers_len;

	RenderCommandList commands;
	u8 transient[RENDER_TRANSIENT_MEMSIZE];
} RenderMemory;

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
	RenderMeshInitData* meshes;
	RenderTextureInitData* textures;
	RenderUboInitData* ubos;
	RenderSsboInitData* ssbos;

	u32 meshes_len;
	u32 programs_len;
	u32 textures_len;
	u32 ubos_len;
	u32 ssbos_len;

	u8 memory[RENDER_INIT_MEMSIZE];
} RenderInitMemory;

#endif // renderer_h_INCLUDED
