#include "renderer/renderer.h"

// TODO: This should come from a file or whatever.
#define RENDER_CUBE_VERTEX_DATA_LEN 108
const f32 cube_vertex_data[RENDER_CUBE_VERTEX_DATA_LEN] = {
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,

	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,

	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,	
};

#define RENDER_NO_INTERPOLATION false

void render_push_command(Renderer* renderer, RenderCommandType type, void* data, u64 data_size) {
	RenderCommand* cmd = (RenderCommand*)arena_alloc(&renderer->frame_arena, sizeof(RenderCommand));
	cmd->type = type;
	cmd->data = arena_alloc(&renderer->frame_arena, data_size);
	memcpy(cmd->data, data, data_size);

	if(renderer->graph->root == NULL) {
		renderer->graph->root = cmd;
		renderer->graph->tail = cmd;
	} else {
		renderer->graph->tail->next = cmd;
		renderer->graph->tail = cmd;
	}
}


// TODO: Replace this with a data format.
RenderInitData* render_load_init_data(Arena* init_arena) {
	RenderInitData* data = (RenderInitData*)arena_alloc(init_arena, sizeof(RenderInitData));

	data->programs_len = 1;
	data->programs = (RenderProgramInitData*)arena_alloc(init_arena, sizeof(RenderProgramInitData) * data->programs_len);
	data->programs[0].vertex_shader_filename = "shaders/cube.vert";
	data->programs[0].fragment_shader_filename = "shaders/cube.frag";
	data->programs[0].next = NULL;

	data->meshes_len = 1;
	data->meshes = (RenderMeshInitData*)arena_alloc(init_arena, sizeof(RenderMeshInitData) * data->meshes_len);
	data->meshes[0].vertex_size = 3;
	data->meshes[0].vertices_len = 36;
	data->meshes[0].vertex_data = (f32*)arena_alloc(init_arena, sizeof(f32) * data->meshes[0].vertex_size * data->meshes[0].vertices_len);
	for(i32 i = 0; i < RENDER_CUBE_VERTEX_DATA_LEN; i++) {
		data->meshes[0].vertex_data[i] = cube_vertex_data[i];
	}
	data->meshes[0].next = NULL;

	data->textures_len = 0;

	data->ubos_len = 1;
	data->ubos = (RenderUboInitData*)arena_alloc(init_arena, sizeof(RenderUboInitData) * data->ubos_len);
	data->ubos[0].size = sizeof(f32) * 32;
	data->ubos[0].binding = 0;
	data->ubos[0].next = NULL;

	data->host_buffers_len = 1;
	data->host_buffers = (RenderHostBufferInitData*)arena_alloc(init_arena, sizeof(RenderHostBufferInitData) * data->host_buffers_len);
	data->host_buffers[0].next = NULL;

	return data;
}

// TODO: Replace this with a data format.
void render_load_frame_graph(Renderer* renderer) {
	renderer->graph = (RenderGraph*)arena_alloc(&renderer->frame_arena, sizeof(RenderGraph));

	RenderCommandClear clear = { .color = { 0.0f, 0.0f, 0.5f, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, sizeof(clear));

	RenderCommandUseProgram use_program = { .program = 0 };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program, sizeof(use_program));

	RenderCommandUseUbo use_ubo = { .ubo = 0 };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo, sizeof(use_ubo));

	RenderCommandBufferUboData buffer_ubo_data = { .ubo = 0, .host_buffer_index = 0, .host_offset = 0 };
	render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data, sizeof(buffer_ubo_data));

	RenderCommandDrawMesh draw_mesh = { .mesh = 0 };
	render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh, sizeof(draw_mesh));
}
