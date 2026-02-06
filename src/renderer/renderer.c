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

i32 render_push_host_buffer(Renderer* renderer, u8* data) {
	strict_assert(RENDER_MAX_HOST_BUFFERS < 255);
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
	Arena* init_arena) 
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
	mesh->vertex_data = (f32*)arena_alloc(init_arena, vertex_buffer_size);
	memcpy(mesh->vertex_data, vertex_data, vertex_buffer_size);

	mesh->indices_len = indices_len;
	u64 index_buffer_size = sizeof(u32) * indices_len;
	mesh->indices = (u32*)arena_alloc(init_arena, index_buffer_size);
	memcpy(mesh->indices, indices, index_buffer_size);
}

Renderer* render_init(RenderBackendInitData* init, Arena* init_arena, Arena* render_arena, AssetPack* asset_pack) {
	Renderer* renderer = (Renderer*)arena_alloc(render_arena, sizeof(Renderer));
	renderer->frames_since_init = 0;

	arena_init(&renderer->persistent_arena, RENDER_PERSISTENT_ARENA_SIZE, render_arena, "RenderPersistent");
	arena_init(&renderer->viewport_arena, RENDER_VIEWPORT_ARENA_SIZE, render_arena, "RenderViewport");
	arena_init(&renderer->frame_arena, RENDER_FRAME_ARENA_SIZE, render_arena, "RenderFrame");

	init->programs_len = asset_pack->render_programs_len;
	init->programs = (RenderProgramInitData*)arena_alloc(init_arena, sizeof(RenderProgramInitData) * init->programs_len);
	for(i32 i = 0; i < init->programs_len; i++) {
		RenderProgramAsset* asset = (RenderProgramAsset*)&asset_pack->buffer[asset_pack->render_program_buffer_offsets[i]];
		RenderProgramInitData* program = &init->programs[i];

		program->vertex_shader_src = (char*)arena_alloc(init_arena, asset->vertex_shader_src_len);
		program->vertex_shader_src_len = asset->vertex_shader_src_len;
		memcpy(program->vertex_shader_src, asset->buffer, sizeof(char) * asset->vertex_shader_src_len);

		program->fragment_shader_src = (char*)arena_alloc(init_arena, asset->fragment_shader_src_len);
		program->fragment_shader_src_len = asset->fragment_shader_src_len;
		memcpy(program->fragment_shader_src, asset->buffer + asset->vertex_shader_src_len, sizeof(char) * asset->fragment_shader_src_len);

		if(i == init->programs_len - 1) program->next = NULL; else program->next = &init->programs[i + 1];
	}

	init->meshes_len = asset_pack->meshes_len + NUM_RENDER_PRIMITIVES;
	init->meshes = (RenderMeshInitData*)arena_alloc(init_arena, sizeof(RenderMeshInitData) * init->meshes_len);
	for(i32 i = 0; i < asset_pack->meshes_len; i++) {
		renderer->model_to_mesh_map[i] = i;

		MeshAsset* asset = (MeshAsset*)&asset_pack->buffer[asset_pack->mesh_buffer_offsets[i]];
		RenderMeshInitData* mesh = &init->meshes[i];
		u32 vert_attrib_sizes[3] = { 3, 3, 2 };
		u32 vert_attribs_len = 3;
		u64 vertex_buffer_size = sizeof(f32) * 8 * asset->vertices_len;
		render_push_mesh_init_data(mesh, (f32*)(asset->buffer), asset->vertices_len, (u32*)(asset->buffer + vertex_buffer_size), asset->indices_len, vert_attrib_sizes, vert_attribs_len, init_arena);

		if(i == init->meshes_len - 1) {
			mesh->next = NULL; 
		} else {
			mesh->next = &init->meshes[i + 1];
		}
	}
	for(i32 i = 0; i < NUM_RENDER_PRIMITIVES; i++) {
		i32 mesh_index = asset_pack->meshes_len + i;
		renderer->primitive_to_mesh_map[i] = mesh_index;
		
		RenderMeshInitData* mesh = &init->meshes[mesh_index];
		u32 vert_attrib_sizes[1] = { 2 };
		u32 vert_attribs_len = 1;
		f32* vertex_data = render_primitives[i];
		u32 vertices_len = sizeof(render_primitives[i]) * sizeof(f32);
		render_push_mesh_init_data(mesh, render_primitives[i], vertices_len, render_primitives_indices[i], 1, vert_attrib_sizes, vert_attribs_len, init_arena);

		if(mesh_index == init->meshes_len - 1) {
			mesh->next = NULL;
		} else {
			mesh->next = &init->meshes[mesh_index + 1];
		}
	}

	init->textures_len = asset_pack->textures_len;
	init->textures = (RenderTextureInitData*)arena_alloc(init_arena, sizeof(RenderTextureInitData) * init->textures_len);
	for(i32 i = 0; i < init->textures_len; i++) {
		TextureAsset* asset = (TextureAsset*)&asset_pack->buffer[asset_pack->texture_buffer_offsets[i]];
		RenderTextureInitData* texture = &init->textures[i];
		texture->width = asset->width;
		texture->height = asset->height;
		texture->channel_count = asset->channel_count;

		u64 pixel_data_size = sizeof(u8) * asset->channel_count * texture->width * texture->height;
		texture->pixel_data = (u8*)arena_alloc(init_arena, pixel_data_size);
		memcpy(texture->pixel_data, asset->buffer, pixel_data_size);

		if(i == init->textures_len - 1) {
			texture->next = NULL;
		} else {
			texture->next = &init->textures[i + 1];
		}
	}

	init->ubos_len = NUM_RENDER_UBOS;
	init->ubos = (RenderUboInitData*)arena_alloc(init_arena, sizeof(RenderUboInitData) * init->ubos_len);
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
	init->ssbos = (RenderSsboInitData*)arena_alloc(init_arena, sizeof(RenderSsboInitData) * init->ssbos_len);
	for(i32 i = 0; i < init->ssbos_len; i++) {
		RenderSsboInitData* ssbo = &init->ssbos[i];

		switch(i) {
			case RENDER_SSBO_MODEL: {
				ssbo->size = sizeof(RenderModelTransform) * RENDER_LIST_MAX_MODELS;
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

	return renderer;
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
void render_prepare_frame_data(Renderer* renderer, Platform* platform, RenderList* list) {
	// =========================
	// PREPARE HOST DATA BUFFERS
	// =========================
	renderer->host_buffers_len = 0;

	// Camera ubo
	RenderCameraUbo* camera_ubos = (RenderCameraUbo*)arena_alloc(&renderer->frame_arena, sizeof(RenderCameraUbo) * list->cameras_len);
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
	RenderModelTransform* model_ssbos = (RenderModelTransform*)arena_alloc(&renderer->frame_arena, sizeof(RenderModelTransform) * list->models_len);

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
	u8 model_host_buffer = render_push_host_buffer(renderer, (u8*)model_ssbos);

	// Laser ubo
	RenderModelTransform* laser_ubos = (RenderModelTransform*)arena_alloc(&renderer->frame_arena, sizeof(RenderModelTransform) * list->lasers_len);
	for(i32 i = 0; i < list->lasers_len; i++) {
		RenderListLaser* laser = &list->lasers[i];
		RenderModelTransform* ubo = &laser_ubos[i];

		v3 line_delta = v3_sub(laser->end, laser->start);
		m4_scale(v3_new(v3_magnitude(line_delta), laser->stroke, laser->stroke), ubo->transform);

		f32 rotation[16];
		v3 line_norm = v3_normalize(line_delta);
		//v2 2d_delta = v2_new(line_delta.x, line_delta.z);
		m4_rotation(0.0f, atan2(-line_norm.z, line_norm.x), 0.0f, rotation);

		f32 translation[16];
		m4_translation(laser->start, translation);

		m4_mul(rotation, ubo->transform, ubo->transform);
		m4_mul(translation, ubo->transform, ubo->transform);
	}
	u8 laser_host_buffer = render_push_host_buffer(renderer, (u8*)laser_ubos);

	// Text ssbo
	RenderGlyph* text_ssbo = (RenderGlyph*)arena_alloc(&renderer->frame_arena, sizeof(RenderGlyph) * ASSET_NUM_FONTS * RENDER_LIST_MAX_GLYPHS_PER_FONT);
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
	renderer->graph = (RenderGraph*)arena_alloc(&renderer->frame_arena, sizeof(RenderGraph));

	RenderCommandClear clear = { .color = { list->clear_color.r, list->clear_color.g, list->clear_color.b, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, sizeof(clear));

	RenderCommandUseUbo use_ubo_camera = { .ubo = RENDER_UBO_CAMERA };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_camera, sizeof(use_ubo_camera));

	// Draw viewports
	for(i32 i = 0; i < list->cameras_len; i++) {
		RenderListCamera* cam = &list->cameras[i];
		v4 viewport_rect = cam->screen_rect;
		viewport_rect.x *= platform->window_width;
		viewport_rect.y *= platform->window_height;
		viewport_rect.z *= platform->window_width;
		viewport_rect.w *= platform->window_height;

		RenderCommandSetViewport set_viewport = { .rect = viewport_rect };
		render_push_command(renderer, RENDER_COMMAND_SET_VIEWPORT, &set_viewport, sizeof(set_viewport));
		
		RenderCommandBufferUboData buffer_ubo_data_camera = { .ubo = RENDER_UBO_CAMERA, .host_buffer_index = camera_host_buffer, .host_buffer_offset = sizeof(RenderCameraUbo) * i };
		render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_camera, sizeof(buffer_ubo_data_camera));

		// Draw models
		RenderCommandUseProgram use_program_model = { .program = ASSET_RENDER_PROGRAM_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_model, sizeof(use_program_model));

		RenderCommandUseSsbo use_ssbo_model = { .ssbo = RENDER_SSBO_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_model, sizeof(use_ssbo_model));

		for(i32 i = 0; i < ASSET_NUM_MESHES; i++) {
			/*
			RenderCommandBufferUboData buffer_ubo_data_instance = { .ubo = RENDER_UBO_MODEL, .host_buffer_index = model_host_buffer, .host_buffer_offset = i * 64 };
			render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_instance, sizeof(buffer_ubo_data_instance));

			RenderListModel* model = &list->models[i];
			RenderCommandUseTexture use_texture = { .texture = model->texture };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture, sizeof(use_texture));

			RenderCommandDrawMesh draw_mesh = { .mesh = renderer->model_to_mesh_map[model->id] };
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh, sizeof(draw_mesh));
			*/

			if(list->model_lens_by_type[i] < 1) break;

			RenderCommandBufferSsboData buffer_ssbo_data_model = { 
				.ssbo = RENDER_SSBO_MODEL, 
				.size = sizeof(RenderModelTransform) * list->model_lens_by_type[i], 
				.host_buffer_index = model_host_buffer, 
				.host_buffer_offset = sizeof(RenderModelTransform) * model_ssbo_offsets_by_type[i]
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_model, sizeof(buffer_ssbo_data_model));

			// TODO: This assumes textures and models are listed 1 by 1 in perfect sync,
			// ignoring the texture passed to the render list.
			//
			// Avoiding this would require us to do something like bindless rendering or
			// a texture array, which would mean the textures are laid out next to each
			// other in memory on a per mesh basis.
			//
			// For now, we can also just have them be a part of the render list.
			RenderCommandUseTexture use_texture = { .texture = i };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture, sizeof(use_texture));

			RenderCommandDrawMeshInstanced draw_mesh_instanced_model = { .mesh = renderer->model_to_mesh_map[i], .count = list->model_lens_by_type[i] };
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_model, sizeof(draw_mesh_instanced_model));
		}

		// NOW: Get lasers working as same as other instanced stuff. Implement scaling
		// as a regular part of model transforms. The calculations for doing so are to
		// be derived on the game side.
		 
		/* Draw lasers
		RenderCommandUseProgram use_program_laser = { .program = ASSET_RENDER_PROGRAM_LASER };
		render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_laser, sizeof(use_program_laser));

		//RenderCommandUseUbo use_ubo_instance = { .ubo = RENDER_UBO_MODEL };
		//render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_instance, sizeof(use_ubo_instance));

		for(i32 i = 0; i < list->lasers_len; i++) {
			RenderCommandBufferUboData buffer_ubo_data_laser = { .ubo = RENDER_UBO_MODEL, .host_buffer_index = laser_host_buffer, .host_buffer_offset = i * 64 };
			render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_laser, sizeof(buffer_ubo_data_laser));

			RenderCommandDrawMesh draw_mesh = { .mesh = renderer->model_to_mesh_map[ASSET_MESH_CYLINDER] };
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH, &draw_mesh, sizeof(draw_mesh));
		}
		*/
	}

	// Draw text
	RenderCommandSetViewport set_viewport_text = { .rect = v4_new(0.0f, 0.0f, platform->window_width, platform->window_height) };
	render_push_command(renderer, RENDER_COMMAND_SET_VIEWPORT, &set_viewport_text, sizeof(set_viewport_text));

	RenderCommandUseProgram use_program_text = { .program = ASSET_RENDER_PROGRAM_TEXT };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_text, sizeof(use_program_text));

	RenderCommandUseSsbo use_ssbo_text = { .ssbo = RENDER_SSBO_TEXT };
	render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_text, sizeof(use_ssbo_text));

	u64 text_buffer_offset = 0;
	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		if(list->glyph_list_lens[i] > 0) {
			RenderCommandBufferSsboData buffer_ssbo_data_text = { 
				.ssbo = RENDER_SSBO_TEXT, 
				.size = sizeof(RenderGlyph) * list->glyph_list_lens[i], 
				.host_buffer_index = text_host_buffer, 
				.host_buffer_offset = text_buffer_offset 
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_text, sizeof(buffer_ssbo_data_text));

			RenderCommandUseTexture use_texture_text = { .texture = list->glyph_list_textures[i] };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture_text, sizeof(use_texture_text));

			RenderCommandDrawMeshInstanced draw_mesh_instanced_text = { 
				.mesh = renderer->primitive_to_mesh_map[RENDER_PRIMITIVE_QUAD], 
				.count = list->glyph_list_lens[i] 
			};
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_text, sizeof(draw_mesh_instanced_text));
		}
		text_buffer_offset += sizeof(RenderGlyph) * RENDER_LIST_MAX_GLYPHS_PER_FONT;
	}
}
