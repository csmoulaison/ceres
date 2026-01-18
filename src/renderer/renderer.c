#include "renderer/renderer.h"
#include "renderer/mesh_loader.c"
#include "renderer/render_list.c"
#include "generated/asset_handles.h"

#define RENDER_PROGRAM_MODEL 0

#define RENDER_UBO_WORLD 0
#define RENDER_UBO_INSTANCE 1

#define RENDER_HOST_BUFFER_WORLD 0
#define RENDER_HOST_BUFFER_INSTANCE 1

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

// TODO: Define what's being loaded from a data format.
// - Programs (vert file, frag file)
// - Meshes (file)
// - Textures (file)
// Ubos and host buffers, as well as some specifics of the other resources (mesh
// vertex_attribute_len for instance) are still hardcoded here, though these
// specifics remain data driven on graphics backend side. What's hardcoded in
// load_init_data ought to match the program context which is known about by
// prepare_frame_data.
RenderInitData* render_load_init_data(Arena* init_arena) {
	RenderInitData* data = (RenderInitData*)arena_alloc(init_arena, sizeof(RenderInitData));

	data->programs_len = 1;
	data->programs = (RenderProgramInitData*)arena_alloc(init_arena, sizeof(RenderProgramInitData) * data->programs_len);
	for(i32 i = 0; i < data->programs_len; i++) {
		RenderProgramInitData* program = &data->programs[i];

		switch(i) {
			case RENDER_PROGRAM_MODEL: {
				program->vertex_shader_filename = "data/shaders/cube.vert";
				program->fragment_shader_filename = "data/shaders/cube.frag";
			} break;
			default: break;
		}

		if(i == data->programs_len - 1) {
			program->next = NULL;
		} else {
			program->next = &data->programs[i + 1];
		}
	}

	data->meshes_len = 2;
	data->meshes = (RenderMeshInitData*)arena_alloc(init_arena, sizeof(RenderMeshInitData) * data->meshes_len);
	char* mesh_filenames[2] = { "data/meshes/ship.obj", "data/meshes/floor.obj" };
	for(i32 i = 0; i < data->meshes_len; i++) {
		MeshData* mesh_data = (MeshData*)arena_alloc(init_arena, sizeof(MeshData));
		load_mesh(mesh_data, mesh_filenames[i], init_arena, true);

		RenderMeshInitData* mesh = &data->meshes[i];
		mesh->flat_shading = mesh_data->flat_shading;

		mesh->vertex_attributes_len = 3;
		mesh->vertex_attribute_sizes[0] = 3;
		mesh->vertex_attribute_sizes[1] = 3;
		mesh->vertex_attribute_sizes[2] = 2;
		u32 total_vertex_size = 8;

		mesh->vertices_len = mesh_data->vertices_len;
		mesh->vertex_data = (f32*)arena_alloc(init_arena, sizeof(f32) * total_vertex_size * mesh->vertices_len);
		for(i32 i = 0; i < total_vertex_size * mesh->vertices_len; i++) {
			mesh->vertex_data[i] = mesh_data->vertices[i / total_vertex_size].data[i % total_vertex_size];
		}

		mesh->indices_len = mesh_data->indices_len;
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

	data->textures_len = 2;
	data->textures = (RenderTextureInitData*)arena_alloc(init_arena, sizeof(RenderTextureInitData) * data->textures_len);
	char* texture_filenames[2] = { "data/textures/ship.tex", "data/textures/metal.tex" };
	for(i32 i = 0; i < data->textures_len; i++) {
		FILE* file = fopen(texture_filenames[i], "r");
		assert(file != NULL);
		u32 dimensions[2];
		fread(dimensions, sizeof(u32), 2, file);

		RenderTextureInitData* texture = &data->textures[i];
		texture->width = dimensions[0];
		texture->height = dimensions[1];

		u64 pixel_data_size = sizeof(32) * texture->width * texture->height;
		texture->pixel_data = (u8*)arena_alloc(init_arena, pixel_data_size);
		assert(fread(texture->pixel_data, pixel_data_size, 1, file) == 1);

		fclose(file);

		if(i == data->textures_len - 1) {
			texture->next = NULL;
		} else {
			texture->next = &data->textures[i + 1];
		}
	}

	data->ubos_len = 2;
	data->ubos = (RenderUboInitData*)arena_alloc(init_arena, sizeof(RenderUboInitData) * data->ubos_len);
	for(i32 i = 0; i < data->ubos_len; i++) {
		RenderUboInitData* ubo = &data->ubos[i];

		switch(i) {
			case RENDER_UBO_WORLD: {
				ubo->size = sizeof(f32) * 20;
				ubo->binding = 0;
			} break;
			case RENDER_UBO_INSTANCE: {
				ubo->size = sizeof(f32) * 16;
				ubo->binding = 1;
			} break;
			default: break;
		}

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

void render_prepare_frame_data(Renderer* renderer, Platform* platform, RenderList* list) {
	// World ubo
	f32* world_ubo = (f32*)arena_alloc(&renderer->frame_arena, sizeof(f32) * 20);
	f32* ubo_projection = &world_ubo[0];
	f32* ubo_camera_position = &world_ubo[16];

	f32 perspective[16];
	mat4_perspective(radians_from_degrees(75.0f), (f32)platform->window_width / (f32)platform->window_height, 100.00f, 0.05f, perspective);
	f32 view[16];
	mat4_identity(view);
	float up[3];
	v3_init(up, 0.0f, 1.0f, 0.0f);
	mat4_lookat(list->world.camera_position, list->world.camera_target, up, view);
	mat4_mul(perspective, view, ubo_projection);
	v3_copy(list->world.camera_position, ubo_camera_position);
	renderer->host_buffers[RENDER_WORLD_UBO_BUFFER].data = (u8*)world_ubo;

	// Model ubo
	f32* instances_ubo = (f32*)arena_alloc(&renderer->frame_arena, sizeof(f32) * 16 * list->models_len);
	for(i32 i = 0; i < list->models_len; i++) {
		RenderListModel* model = &list->models[i];
		f32* instance = &instances_ubo[i * 16];
		mat4_translation(model->position, instance);

		f32 rotation[16];
		mat4_rotation(
			model->orientation[0], 
			model->orientation[1], 
			model->orientation[2], 
			rotation);
		mat4_mul(instance, rotation, instance);
	}
	renderer->host_buffers[RENDER_INSTANCE_UBO_BUFFER].data = (u8*)instances_ubo;

	// Render graph
	renderer->graph = (RenderGraph*)arena_alloc(&renderer->frame_arena, sizeof(RenderGraph));

	RenderCommandClear clear = { .color = { 0.05f, 0.05f, 0.05f, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, sizeof(clear));

	RenderCommandUseProgram use_program = { .program = RENDER_PROGRAM_MODEL };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program, sizeof(use_program));

	RenderCommandUseUbo use_ubo_world = { .ubo = RENDER_UBO_WORLD };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_world, sizeof(use_ubo_world));

	RenderCommandUseUbo use_ubo_instance = { .ubo = RENDER_UBO_INSTANCE };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_instance, sizeof(use_ubo_instance));

	RenderCommandBufferUboData buffer_ubo_data_world = { .ubo = 0, .host_buffer_index = 0, .host_buffer_offset = 0 };
	render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_world, sizeof(buffer_ubo_data_world));

	// Draw models
	for(i32 i = 0; i < list->models_len; i++) {
		RenderCommandBufferUboData buffer_ubo_data_instance = { .ubo = 1, .host_buffer_index = 1, .host_buffer_offset = i * 64 };
		render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_instance, sizeof(buffer_ubo_data_instance));

		RenderListModel* model = &list->models[i];
		RenderCommandUseTexture use_texture= { .texture = model->texture };
		render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture, sizeof(use_texture));

		RenderCommandDrawMesh draw_mesh = { .mesh = model->mesh };
		render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh, sizeof(draw_mesh));
	}
}
