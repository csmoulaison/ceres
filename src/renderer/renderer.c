#include "renderer/renderer.h"
#include "renderer/mesh_loader.c"

#define RENDER_NO_INTERPOLATION false

// TODO: This should come from a file
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

	data->meshes_len = 2;
	data->meshes = (RenderMeshInitData*)arena_alloc(init_arena, sizeof(RenderMeshInitData) * data->meshes_len);
	char* mesh_filenames[2] = { "meshes/ship.obj", "meshes/floor.obj" };

	for(i32 i = 0; i < data->meshes_len; i++) {
		MeshData* mesh_data = (MeshData*)arena_alloc(init_arena, sizeof(MeshData));
		load_mesh(mesh_data, mesh_filenames[i], init_arena, true);

		RenderMeshInitData* mesh = &data->meshes[i];
		mesh->vertex_attributes_len = 2;
		mesh->vertex_attribute_sizes[0] = 3;
		mesh->vertex_attribute_sizes[1] = 3;
		mesh->flat_shading = mesh_data->flat_shading;
		u32 total_vertex_size = 6;
		mesh->vertices_len = mesh_data->vertices_len;
		mesh->indices_len = mesh_data->indices_len;

		mesh->vertex_data = (f32*)arena_alloc(init_arena, sizeof(f32) * total_vertex_size * mesh->vertices_len);
		for(i32 i = 0; i < total_vertex_size * mesh->vertices_len; i++) {
			mesh->vertex_data[i] = mesh_data->vertices[i / total_vertex_size].data[i % total_vertex_size];
		}
		mesh->indices = (u32*)arena_alloc(init_arena, sizeof(u32) * mesh->indices_len);
		for(i32 i = 0; i < mesh->indices_len; i++) {
			mesh->indices[i] = mesh_data->indices[i];
		}

		if(i == data->meshes_len - 1) {
			mesh->next = NULL;
		} else {
			mesh->next = &data->meshes[i + 1];
		}
	}

	data->textures_len = 0;

	data->ubos_len = 2;
	data->ubos = (RenderUboInitData*)arena_alloc(init_arena, sizeof(RenderUboInitData) * data->ubos_len);

	struct {
		u64 size;
		u32 binding;
	} ubo_data[2] = {
		{ .size = sizeof(f32) * 20, .binding = 0 },
		{ .size = sizeof(f32) * 16, .binding = 1 }
	};

	for(i32 i = 0; i < data->ubos_len; i++) {
		RenderUboInitData* ubo = &data->ubos[i];
		ubo->size = ubo_data[i].size;
		ubo->binding = ubo_data[i].binding;
		if(i == data->ubos_len - 1) {
			ubo->next = NULL;
		} else {
			ubo->next = &data->ubos[i + 1];
		}
	}

	data->host_buffers_len = 2;
	data->host_buffers = (RenderHostBufferInitData*)arena_alloc(init_arena, sizeof(RenderHostBufferInitData) * data->host_buffers_len);
	data->host_buffers[0].next = &data->host_buffers[1];
	data->host_buffers[1].next = NULL;

	return data;
}

Renderer* render_pre_init(RenderInitData* data, Arena* render_arena) {
	Renderer* renderer = (Renderer*)arena_alloc(render_arena, sizeof(Renderer));
	renderer->frames_since_init = 0;

	arena_init(&renderer->persistent_arena, RENDER_PERSISTENT_ARENA_SIZE, render_arena, "RenderPersistent");
	arena_init(&renderer->viewport_arena, RENDER_VIEWPORT_ARENA_SIZE, render_arena, "RenderViewport");
	arena_init(&renderer->frame_arena, RENDER_FRAME_ARENA_SIZE, render_arena, "RenderFrame");

	if(data->programs_len > 0)
		renderer->programs = (RenderProgram*)arena_alloc(&renderer->persistent_arena, sizeof(RenderProgram) * data->programs_len);
	if(data->meshes_len > 0)
		renderer->meshes = (RenderMesh*)arena_alloc(&renderer->persistent_arena, sizeof(RenderMesh) * data->meshes_len);
	if(data->textures_len > 0)
		renderer->textures = (RenderTexture*)arena_alloc(&renderer->persistent_arena, sizeof(RenderTexture) * data->textures_len);
	if(data->ubos_len > 0)
		renderer->ubos = (RenderUbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderUbo) * data->ubos_len);
	if(data->host_buffers_len > 0)
		renderer->host_buffers = (RenderHostBuffer*)arena_alloc(&renderer->persistent_arena, sizeof(RenderHostBuffer) * data->host_buffers_len);

	return renderer;
}

void render_prepare_frame_data(Renderer* renderer, Platform* platform, f32* ship_position, f32 ship_direction) {
	// Prepare the render graph.
	// TODO: Replace this with a data format.
	renderer->graph = (RenderGraph*)arena_alloc(&renderer->frame_arena, sizeof(RenderGraph));

	RenderCommandClear clear = { .color = { 0.05f, 0.05f, 0.05f, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, sizeof(clear));

	RenderCommandUseProgram use_program = { .program = 0 };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program, sizeof(use_program));

	RenderCommandUseUbo use_ubo_world = { .ubo = 0 };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_world, sizeof(use_ubo_world));

	RenderCommandUseUbo use_ubo_instance = { .ubo = 1 };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_instance, sizeof(use_ubo_instance));

	// Draw ship
	RenderCommandBufferUboData buffer_ubo_data_ship_world = { .ubo = 0, .host_buffer_index = 0, .host_buffer_offset = 0 };
	render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_ship_world, sizeof(buffer_ubo_data_ship_world));

	RenderCommandBufferUboData buffer_ubo_data_ship_instance = { .ubo = 1, .host_buffer_index = 1, .host_buffer_offset = 0 };
	render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_ship_instance, sizeof(buffer_ubo_data_ship_instance));

	RenderCommandDrawMesh draw_mesh_ship = { .mesh = 0 };
	render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh_ship, sizeof(draw_mesh_ship));

	// Draw floor
	i32 floor_instances = 1024;
	for(i32 i = 0; i < floor_instances; i++) {
		RenderCommandBufferUboData buffer_ubo_data_floor_instance = { .ubo = 1, .host_buffer_index = 1, .host_buffer_offset = 64 + i * 64 };
		render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_floor_instance, sizeof(buffer_ubo_data_floor_instance));

		RenderCommandDrawMesh draw_mesh_floor = { .mesh = 1 };
		render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh_floor, sizeof(draw_mesh_floor));
	}

	// Prepare the host data buffers.
	f32 target[3] = { 0.0f, 0.0f, 0.0f };

	// World ubo
	f32* world_ubo = (f32*)arena_alloc(&renderer->frame_arena, sizeof(f32) * 20);
	f32* projection = &world_ubo[0];
	f32* camera_position = &world_ubo[16];

	f32 perspective[16];
	mat4_perspective(radians_from_degrees(90.0f), (f32)platform->window_width / (f32)platform->window_height, 100.00f, 0.05f, perspective);
	f32 view[16] = {};
	mat4_identity(view);
	float up[3] = { 0.0f, 1.0f, 0.0f };
	float cam_pos[3] = {3.5f, 8.5f, 0.0f };
	mat4_lookat(cam_pos, target, up, view);

	mat4_mul(perspective, view, projection);

	camera_position[0] = cam_pos[0];
	camera_position[1] = cam_pos[1];
	camera_position[2] = cam_pos[2];

	renderer->host_buffers[0].data = (u8*)projection;

	// Model ubo
	f32* instance_ubo = (f32*)arena_alloc(&renderer->frame_arena, sizeof(f32) * 16 * (1 + floor_instances));
	f32* model_ship = &instance_ubo[0];

	f32 ship_pos[3] = { ship_position[0], 0.5f, ship_position[1] };
	mat4_translation(ship_pos, model_ship);
	f32 ship_rotation[16];
	mat4_rotation(0.00f, ship_direction, 0.00f, ship_rotation);
	mat4_mul(model_ship, ship_rotation, model_ship);

	for(i32 i = 0; i < floor_instances; i++) {
		f32* model_floor = &instance_ubo[16 + i * 16];
		f32 x = -15.5f + (i % 32);
		f32 z = -15.5f + (i / 32);
		f32 floor_pos[3] = { x, 0.0f, z };
		mat4_translation(floor_pos, model_floor);
		f32 floor_rotation[16];
		mat4_rotation(0.0f, 0.0f, 0.0f, floor_rotation);
		mat4_mul(model_floor, floor_rotation, model_floor);
	}

	renderer->host_buffers[1].data = (u8*)instance_ubo;
}
