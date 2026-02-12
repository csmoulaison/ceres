#include "renderer/render_list.h"
#include "renderer/renderer.h"

#define RENDER_NO_INTERPOLATION false

typedef struct {
	v4 src;
	v4 dst;
	v4 color;
} RenderGlyph;

typedef struct {
	float projection[16];
	v3 position;
	f32 padding;
} RenderCameraUbo;

typedef struct {
	f32 transform[16];
} RenderModelTransform;

void render_push_command(RenderMemory* renderer, RenderCommandType type, void* data, StackAllocator* frame_stack) {
	u64 data_size;
	switch(type) {
		case RENDER_COMMAND_CLEAR: {
			data_size = sizeof(RenderCommandClear);
		} break;
		case RENDER_COMMAND_SET_VIEWPORT: {
			data_size = sizeof(RenderCommandSetViewport);
		} break;
		case RENDER_COMMAND_USE_PROGRAM: {
			data_size = sizeof(RenderCommandUseProgram);
		} break;
		case RENDER_COMMAND_USE_UBO: {
			data_size = sizeof(RenderCommandUseUbo);
		} break;
		case RENDER_COMMAND_USE_SSBO: {
			data_size = sizeof(RenderCommandUseSsbo);
		} break;
		case RENDER_COMMAND_USE_TEXTURE: {
			data_size = sizeof(RenderCommandUseTexture);
		} break;
		case RENDER_COMMAND_BUFFER_UBO_DATA: {
			data_size = sizeof(RenderCommandBufferUboData);
		} break;
		case RENDER_COMMAND_BUFFER_SSBO_DATA: {
			data_size = sizeof(RenderCommandBufferSsboData);
		} break;
		case RENDER_COMMAND_DRAW_MESH: {
			data_size = sizeof(RenderCommandDrawMesh);
		} break;
		case RENDER_COMMAND_DRAW_MESH_INSTANCED: {
			data_size = sizeof(RenderCommandDrawMeshInstanced);
		} break;
		default: { 
			printf("Render command %u data size not set in render_push_command.\n", type);
			panic(); 
		} break;
	}
	
	RenderCommand* cmd = (RenderCommand*)stack_alloc(frame_stack, sizeof(RenderCommand));
	cmd->type = type;
	cmd->data = stack_alloc(frame_stack, data_size);
	memcpy(cmd->data, data, data_size);

	if(renderer->commands.root == NULL) {
		renderer->commands.root = cmd;
		renderer->commands.tail = cmd;
	} else {
		renderer->commands.tail->next = cmd;
		renderer->commands.tail = cmd;
	}
}

i32 render_push_host_buffer(RenderMemory* renderer, u8* data) {
	strict_assert(renderer->host_buffers_len < 255);
	assert(renderer->host_buffers_len < RENDER_MAX_HOST_BUFFERS);
	renderer->host_buffers[renderer->host_buffers_len] = (u8*)data;
	renderer->host_buffers_len++;
	return renderer->host_buffers_len - 1;
}

void render_push_mesh_init_data(
	RenderMeshInitData* mesh, 
	f32* vertex_data, 
	u32 vertices_len, 
	u32* indices, 
	u32 indices_len, 
	u32* vertex_attribute_sizes, 
	u32 vertex_attributes_len,
	StackAllocator* init_stack) 
{
	mesh->flat_shading = true;
	mesh->vertex_attributes_len = vertex_attributes_len;

	u32 total_vertex_size = 0;
	for(u32 i = 0; i < vertex_attributes_len; i++) {
		mesh->vertex_attribute_sizes[i] = vertex_attribute_sizes[i];
		total_vertex_size += vertex_attribute_sizes[i];
	}

	mesh->vertices_len = vertices_len;
	u64 vertex_buffer_size = sizeof(f32) * total_vertex_size * vertices_len;
	mesh->vertex_data = (f32*)stack_alloc(init_stack, vertex_buffer_size);
	memcpy(mesh->vertex_data, vertex_data, vertex_buffer_size);

	mesh->indices_len = indices_len;
	u64 index_buffer_size = sizeof(u32) * indices_len;
	mesh->indices = (u32*)stack_alloc(init_stack, index_buffer_size);
	memcpy(mesh->indices, indices, index_buffer_size);
}

void render_init(RenderMemory* renderer, RenderInitMemory* init, AssetMemory* assets) {
	StackAllocator init_stack = stack_init(init->memory, RENDER_INIT_MEMSIZE, "RenderInit");
	renderer->frames_since_init = 0;

	init->programs_len = assets->render_programs_len;
	init->programs = (RenderProgramInitData*)stack_alloc(&init_stack, sizeof(RenderProgramInitData) * init->programs_len);
	for(i32 i = 0; i < init->programs_len; i++) {
		RenderProgramAsset* asset = (RenderProgramAsset*)&assets->buffer[assets->render_program_buffer_offsets[i]];
		RenderProgramInitData* program = &init->programs[i];

		program->vertex_shader_src = (char*)stack_alloc(&init_stack, asset->vertex_shader_src_len);
		program->vertex_shader_src_len = asset->vertex_shader_src_len;
		memcpy(program->vertex_shader_src, asset->buffer, sizeof(char) * asset->vertex_shader_src_len);

		program->fragment_shader_src = (char*)stack_alloc(&init_stack, asset->fragment_shader_src_len);
		program->fragment_shader_src_len = asset->fragment_shader_src_len;
		memcpy(program->fragment_shader_src, asset->buffer + asset->vertex_shader_src_len, sizeof(char) * asset->fragment_shader_src_len);

		if(i == init->programs_len - 1) program->next = NULL; else program->next = &init->programs[i + 1];
	}

	init->meshes_len = assets->meshes_len + NUM_RENDER_PRIMITIVES;
	init->meshes = (RenderMeshInitData*)stack_alloc(&init_stack, sizeof(RenderMeshInitData) * init->meshes_len);
	for(i32 i = 0; i < assets->meshes_len; i++) {
		renderer->model_to_mesh_map[i] = i;

		MeshAsset* asset = (MeshAsset*)&assets->buffer[assets->mesh_buffer_offsets[i]];
		RenderMeshInitData* mesh = &init->meshes[i];
		u32 vert_attrib_sizes[3] = { 3, 3, 2 };
		u32 vert_attribs_len = 3;
		u64 vertex_buffer_size = sizeof(f32) * 8 * asset->vertices_len;
		render_push_mesh_init_data(mesh, (f32*)(asset->buffer), asset->vertices_len, (u32*)(asset->buffer + vertex_buffer_size), asset->indices_len, vert_attrib_sizes, vert_attribs_len, &init_stack);

		if(i == init->meshes_len - 1) {
			mesh->next = NULL; 
		} else {
			mesh->next = &init->meshes[i + 1];
		}
	}
	for(i32 i = 0; i < NUM_RENDER_PRIMITIVES; i++) {
		i32 mesh_index = assets->meshes_len + i;
		renderer->primitive_to_mesh_map[i] = mesh_index;
		
		RenderMeshInitData* mesh = &init->meshes[mesh_index];
		u32 vert_attrib_sizes[1] = { 2 };
		u32 vert_attribs_len = 1;
		// TODO: Remove the magic number here only used for quad primitive
		u32 vertices_len = 6;
		render_push_mesh_init_data(mesh, render_primitives[i], vertices_len, render_primitives_indices[i], 1, vert_attrib_sizes, vert_attribs_len, &init_stack);

		if(mesh_index == init->meshes_len - 1) {
			mesh->next = NULL;
		} else {
			mesh->next = &init->meshes[mesh_index + 1];
		}
	}

	init->textures_len = assets->textures_len;
	init->textures = (RenderTextureInitData*)stack_alloc(&init_stack, sizeof(RenderTextureInitData) * init->textures_len);
	for(i32 i = 0; i < init->textures_len; i++) {
		TextureAsset* asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[i]];
		RenderTextureInitData* texture = &init->textures[i];
		texture->width = asset->width;
		texture->height = asset->height;
		texture->channel_count = asset->channel_count;

		u64 pixel_data_size = sizeof(u8) * asset->channel_count * texture->width * texture->height;
		texture->pixel_data = (u8*)stack_alloc(&init_stack, pixel_data_size);
		memcpy(texture->pixel_data, asset->buffer, pixel_data_size);

		if(i == init->textures_len - 1) {
			texture->next = NULL;
		} else {
			texture->next = &init->textures[i + 1];
		}
	}

	init->ubos_len = NUM_RENDER_UBOS;
	init->ubos = (RenderUboInitData*)stack_alloc(&init_stack, sizeof(RenderUboInitData) * init->ubos_len);
	for(i32 i = 0; i < init->ubos_len; i++) {
		RenderUboInitData* ubo = &init->ubos[i];

		switch(i) {
			case RENDER_UBO_CAMERA: {
				ubo->size = sizeof(RenderCameraUbo);
				ubo->binding = 0;
			} break;
			default: {
				printf("Error: No initialization data defined for uniform buffer %i.\n", i);
				panic();
			} break;
		}

		if(i == init->ubos_len - 1) {
			ubo->next = NULL;
		} else {
			ubo->next = &init->ubos[i + 1];
		}
	}

	init->ssbos_len = 2;
	init->ssbos = (RenderSsboInitData*)stack_alloc(&init_stack, sizeof(RenderSsboInitData) * init->ssbos_len);
	for(i32 i = 0; i < init->ssbos_len; i++) {
		RenderSsboInitData* ssbo = &init->ssbos[i];

		switch(i) {
			case RENDER_SSBO_MODEL: {
				ssbo->size = sizeof(RenderListTransform) * RENDER_LIST_MAX_TRANSFORMS;
				ssbo->binding = 0;
			} break;
			case RENDER_SSBO_TEXT: {
				ssbo->size = sizeof(RenderGlyph) * ASSET_NUM_FONTS * RENDER_LIST_MAX_GLYPHS_PER_FONT;
				ssbo->binding = 0;
			} break;
			default: {
				printf("Error: No initialization data defined for shader storage buffer %i.\n", i);
				panic();
			} break;
		}

		if(i == init->ssbos_len - 1) {
			ssbo->next = NULL;
		} else {
			ssbo->next = &init->ssbos[i + 1];
		}
	}
}

// TODO #23: This is currently very "immediate mode". At minimum, most of this
// stuff is done every frame with minimal variation, and so we should bake large
// parts of it at initialization. This also allows modern renderers to do that
// sort of thing. We should also refresh ourselves on modern APIs with their
// more retained workflows and make the front end look more like that so that,
// once we move on to DX12 or Vulkan or whatever, the transition is easier.
//
// I don't think it would be appropriate to do some kind of proper frame graph
// architecture, we should just be trying to respect what's most performant and
// try to figure out how to cull boilerplate as we go, as well as eventually
// make things multithreaded.
//
// One thing that might really start to necessitate some more hardcore
// abstractions is allowing the frame graph to encode actual data buffering
// operations. At minimum, we might be defining callback functions to do that
// sort of thing. The reason is to save memory by aliasing stuff. As a simple
// example, in the splitscreen case, the camera ubos needn't all be taking up
// memory at the same time.
void render_prepare_frame_data(RenderMemory* renderer, Platform* platform, RenderList* list) {
	// Initialize
	renderer->host_buffers_len = 0;
	renderer->commands.root = NULL;
	renderer->commands.tail = NULL;
	StackAllocator frame_stack = stack_init(renderer->transient, RENDER_TRANSIENT_MEMSIZE, "RenderFrame");

	// =========================
	// PREPARE HOST DATA BUFFERS
	// =========================

	// Camera ubo
	RenderCameraUbo* camera_ubos = (RenderCameraUbo*)stack_alloc(&frame_stack, sizeof(RenderCameraUbo) * list->cameras_len);
	for(i32 i = 0; i < list->cameras_len; i++) {
		RenderListCamera* cam = &list->cameras[i];
		RenderCameraUbo* ubo = &camera_ubos[i];
		f32 perspective[16];
		m4_perspective(
			radians_from_degrees(75.0f), 
			(cam->screen_rect.z * (f32)platform->window_width) / (cam->screen_rect.w * (f32)platform->window_height),
			100.00f, 0.05f, perspective);
		f32 view[16];
		m4_identity(view);
		v3 up = v3_new(0.0f, 1.0f, 0.0f);
		m4_lookat(cam->position, cam->target, up, view);
		m4_mul(perspective, view, ubo->projection);
		ubo->position = cam->position;
	}
	u8 camera_host_buffer = render_push_host_buffer(renderer, (u8*)camera_ubos);

	// Model ssbos
	/*
	RenderModelTransform* model_ssbos = (RenderModelTransform*)stack_alloc(&frame_stack, sizeof(RenderModelTransform) * list->models_len);

	u32 model_ssbo_offsets_by_type[ASSET_NUM_MESHES];
	u32 model_ssbo_lens_by_type[ASSET_NUM_MESHES];
	u64 offset = 0;
	for(i32 i = 0; i < ASSET_NUM_MESHES; i++) {
		model_ssbo_offsets_by_type[i] = offset;
		offset += list->model_lens_by_type[i];
		model_ssbo_lens_by_type[i] = 0;
	}

	for(i32 i = 0; i < list->models_len; i++) {
		RenderListModel* model = &list->models[i];
		RenderModelTransform* ssbo = &model_ssbos[model_ssbo_offsets_by_type[model->id] + model_ssbo_lens_by_type[model->id]];
		model_ssbo_lens_by_type[model->id]++;

		m4_translation(model->position, ssbo->transform);
		f32 rotation[16];
		m4_rotation(
			model->orientation.x, 
			model->orientation.y, 
			model->orientation.z, 
			rotation);
		m4_mul(ssbo->transform, rotation, ssbo->transform);
	}
	*/
	u8 model_host_buffer = render_push_host_buffer(renderer, (u8*)list->transforms);

	// Text ssbo
	RenderGlyph* text_ssbo = (RenderGlyph*)stack_alloc(&frame_stack, sizeof(RenderGlyph) * ASSET_NUM_FONTS * RENDER_LIST_MAX_GLYPHS_PER_FONT);
	for(u32 i = 0; i < ASSET_NUM_FONTS; i++) {
		for(u32 j = 0; j < list->glyph_list_lens[i]; j++) {
			RenderListGlyph* list_glyph = &list->glyph_lists[i][j];
			RenderGlyph* render_glyph = &text_ssbo[i * RENDER_LIST_MAX_GLYPHS_PER_FONT + j];

			render_glyph->dst.x = 2.0f * (list_glyph->offset.x / platform->window_width + list_glyph->screen_anchor.x) - 1.0f;
			render_glyph->dst.y = 2.0f * (list_glyph->offset.y / platform->window_height + list_glyph->screen_anchor.y) - 1.0f;
			render_glyph->dst.z = 2.0f * (list_glyph->size.x / platform->window_width);
			render_glyph->dst.w = 2.0f * (list_glyph->size.y / platform->window_height);

			render_glyph->src = list_glyph->src;
			render_glyph->color = list_glyph->color;
		}
	}
	u8 text_host_buffer = render_push_host_buffer(renderer, (u8*)text_ssbo);

	// =====================
	// GENERATE RENDER GRAPH
	// =====================
	RenderCommandClear clear = { .color = { list->clear_color.r, list->clear_color.g, list->clear_color.b, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, &frame_stack);

	RenderCommandUseUbo use_ubo_camera = { .ubo = RENDER_UBO_CAMERA };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_camera, &frame_stack);

	// Draw viewports
	for(i32 i = 0; i < list->cameras_len; i++) {
		RenderListCamera* cam = &list->cameras[i];
		v4 viewport_rect = cam->screen_rect;
		viewport_rect.x *= platform->window_width;
		viewport_rect.y *= platform->window_height;
		viewport_rect.z *= platform->window_width;
		viewport_rect.w *= platform->window_height;

		RenderCommandSetViewport set_viewport = { .rect = viewport_rect };
		render_push_command(renderer, RENDER_COMMAND_SET_VIEWPORT, &set_viewport, &frame_stack);
		
		RenderCommandBufferUboData buffer_ubo_data_camera = { .ubo = RENDER_UBO_CAMERA, .host_buffer_index = camera_host_buffer, .host_buffer_offset = sizeof(RenderCameraUbo) * i };
		render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_camera, &frame_stack);

		// Draw models
		RenderCommandUseProgram use_program_model = { .program = ASSET_RENDER_PROGRAM_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_model, &frame_stack);

		RenderCommandUseSsbo use_ssbo_model = { .ssbo = RENDER_SSBO_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_model, &frame_stack);

		/*
		for(i32 i = 0; i < ASSET_NUM_MESHES; i++) {
			if(list->model_lens_by_type[i] < 1) break;

			RenderCommandBufferSsboData buffer_ssbo_data_model = { 
				.ssbo = RENDER_SSBO_MODEL, 
				.size = sizeof(RenderModelTransform) * list->model_lens_by_type[i], 
				.host_buffer_index = model_host_buffer, 
				.host_buffer_offset = sizeof(RenderModelTransform) * model_ssbo_offsets_by_type[i]
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_model, &frame_stack);

			// TODO: This assumes textures and models are listed 1 by 1 in perfect sync,
			// ignoring the texture passed to the render list.
			//
			// Avoiding this would require us to do something like bindless rendering or
			// a texture array, which would mean the textures are laid out next to each
			// other in memory on a per mesh basis.
			//
			// For now, we can also just have them be a part of the render list.
			RenderCommandUseTexture use_texture = { .texture = i };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture, &frame_stack);

			RenderCommandDrawMeshInstanced draw_mesh_instanced_model = { .mesh = renderer->model_to_mesh_map[i], .count = list->model_lens_by_type[i] };
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_model, &frame_stack);
		}
		*/

		for(i32 i = 0; i < list->instance_types_len; i++) {
			RenderListInstanceType* type = &list->instance_types[i];
			if(type->instances_len < 1) {
				printf("Instances len less than 1 for instance type %i\n", i);
				panic();
			}

			RenderCommandBufferSsboData buffer_ssbo_data_model = { 
				.ssbo = RENDER_SSBO_MODEL, 
				.size = sizeof(RenderListTransform) * type->instances_len, 
				.host_buffer_index = model_host_buffer, 
				.host_buffer_offset = sizeof(RenderListTransform) * type->transform_index_offset
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_model, &frame_stack);

			// TODO: Might be nice to make the transform lists have a 1:1 with the asset
			// handles again and use texture arrays to switch between different textures.
			// This would just go back into the RenderListTransform and be passed to the
			// ssbo.
			RenderCommandUseTexture use_texture = { .texture = type->texture };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture, &frame_stack);

			RenderCommandDrawMeshInstanced draw_mesh_instanced_model = { .mesh = renderer->model_to_mesh_map[type->model], .count = type->instances_len };
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_model, &frame_stack);
		}
	}

	// Draw text
	RenderCommandSetViewport set_viewport_text = { .rect = v4_new(0.0f, 0.0f, platform->window_width, platform->window_height) };
	render_push_command(renderer, RENDER_COMMAND_SET_VIEWPORT, &set_viewport_text, &frame_stack);

	RenderCommandUseProgram use_program_text = { .program = ASSET_RENDER_PROGRAM_TEXT };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_text, &frame_stack);

	RenderCommandUseSsbo use_ssbo_text = { .ssbo = RENDER_SSBO_TEXT };
	render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_text, &frame_stack);

	u64 text_buffer_offset = 0;
	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		if(list->glyph_list_lens[i] > 0) {
			RenderCommandBufferSsboData buffer_ssbo_data_text = { 
				.ssbo = RENDER_SSBO_TEXT, 
				.size = sizeof(RenderGlyph) * list->glyph_list_lens[i], 
				.host_buffer_index = text_host_buffer, 
				.host_buffer_offset = text_buffer_offset 
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_text, &frame_stack);

			RenderCommandUseTexture use_texture_text = { .texture = list->glyph_list_textures[i] };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture_text, &frame_stack);

			RenderCommandDrawMeshInstanced draw_mesh_instanced_text = { 
				.mesh = renderer->primitive_to_mesh_map[RENDER_PRIMITIVE_QUAD], 
				.count = list->glyph_list_lens[i] 
			};
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_text, &frame_stack);
		}
		text_buffer_offset += sizeof(RenderGlyph) * RENDER_LIST_MAX_GLYPHS_PER_FONT;
	}
}
