#include "renderer/render_list.h"
#include "renderer/renderer.h"

#define RENDER_NO_INTERPOLATION false

typedef struct {
	v4 src;
	v4 dst;
	v4 color;
} RenderGlyph;

typedef struct {
	v4 dst;
	v4 color;
} RenderRect;

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
	cmd->next = NULL;
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
	for(u32 attr_index = 0; attr_index < vertex_attributes_len; attr_index++) {
		mesh->vertex_attribute_sizes[attr_index] = vertex_attribute_sizes[attr_index];
		total_vertex_size += vertex_attribute_sizes[attr_index];
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
	for(i32 prog_index = 0; prog_index < init->programs_len; prog_index++) {
		RenderProgramAsset* asset = (RenderProgramAsset*)&assets->buffer[assets->render_program_buffer_offsets[prog_index]];
		RenderProgramInitData* program = &init->programs[prog_index];

		program->vertex_shader_src = (char*)stack_alloc(&init_stack, asset->vertex_shader_src_len);
		program->vertex_shader_src_len = asset->vertex_shader_src_len;
		memcpy(program->vertex_shader_src, asset->buffer, sizeof(char) * asset->vertex_shader_src_len);

		program->fragment_shader_src = (char*)stack_alloc(&init_stack, asset->fragment_shader_src_len);
		program->fragment_shader_src_len = asset->fragment_shader_src_len;
		memcpy(program->fragment_shader_src, asset->buffer + asset->vertex_shader_src_len, sizeof(char) * asset->fragment_shader_src_len);

		if(prog_index == init->programs_len - 1) program->next = NULL; else program->next = &init->programs[prog_index + 1];
	}

	init->meshes_len = assets->meshes_len + NUM_RENDER_PRIMITIVES;
	init->meshes = (RenderMeshInitData*)stack_alloc(&init_stack, sizeof(RenderMeshInitData) * init->meshes_len);
	for(i32 mesh_index = 0; mesh_index < assets->meshes_len; mesh_index++) {
		renderer->model_to_mesh_map[mesh_index] = mesh_index;

		MeshAsset* asset = (MeshAsset*)&assets->buffer[assets->mesh_buffer_offsets[mesh_index]];
		RenderMeshInitData* mesh = &init->meshes[mesh_index];
		u32 vert_attrib_sizes[3] = { 3, 3, 2 };
		u32 vert_attribs_len = 3;
		u64 vertex_buffer_size = sizeof(f32) * 8 * asset->vertices_len;
		render_push_mesh_init_data(mesh, (f32*)(asset->buffer), asset->vertices_len, (u32*)(asset->buffer + vertex_buffer_size), asset->indices_len, vert_attrib_sizes, vert_attribs_len, &init_stack);

		if(mesh_index == init->meshes_len - 1) {
			mesh->next = NULL; 
		} else {
			mesh->next = &init->meshes[mesh_index + 1];
		}
	}
	for(i32 primitive_index = 0; primitive_index < NUM_RENDER_PRIMITIVES; primitive_index++) {
		i32 mesh_index = assets->meshes_len + primitive_index;
		renderer->primitive_to_mesh_map[primitive_index] = mesh_index;
		
		RenderMeshInitData* mesh = &init->meshes[mesh_index];
		u32 vert_attrib_sizes[1] = { 2 };
		u32 vert_attribs_len = 1;
		// TODO: Remove the magic number here only used for quad primitive
		u32 vertices_len = 6;
		render_push_mesh_init_data(mesh, render_primitives[primitive_index], vertices_len, render_primitives_indices[primitive_index], 1, vert_attrib_sizes, vert_attribs_len, &init_stack);

		if(mesh_index == init->meshes_len - 1) {
			mesh->next = NULL;
		} else {
			mesh->next = &init->meshes[mesh_index + 1];
		}
	}

	init->textures_len = assets->textures_len;
	init->textures = (RenderTextureInitData*)stack_alloc(&init_stack, sizeof(RenderTextureInitData) * init->textures_len);
	for(i32 tex_index = 0; tex_index < init->textures_len; tex_index++) {
		TextureAsset* asset = (TextureAsset*)&assets->buffer[assets->texture_buffer_offsets[tex_index]];
		RenderTextureInitData* texture = &init->textures[tex_index];
		texture->width = asset->width;
		texture->height = asset->height;
		texture->channel_count = asset->channel_count;

		u64 pixel_data_size = sizeof(u8) * asset->channel_count * texture->width * texture->height;
		texture->pixel_data = (u8*)stack_alloc(&init_stack, pixel_data_size);
		memcpy(texture->pixel_data, asset->buffer, pixel_data_size);

		if(tex_index == init->textures_len - 1) {
			texture->next = NULL;
		} else {
			texture->next = &init->textures[tex_index + 1];
		}
	}

	init->ubos_len = NUM_RENDER_UBOS;
	init->ubos = (RenderUboInitData*)stack_alloc(&init_stack, sizeof(RenderUboInitData) * init->ubos_len);
	for(i32 ubo_index = 0; ubo_index < init->ubos_len; ubo_index++) {
		RenderUboInitData* ubo = &init->ubos[ubo_index];

		switch(ubo_index) {
			case RENDER_UBO_CAMERA: {
				ubo->size = sizeof(RenderCameraUbo);
				ubo->binding = 0;
			} break;
			default: {
				printf("Error: No initialization data defined for uniform buffer %i.\n", ubo_index);
				panic();
			} break;
		}

		if(ubo_index == init->ubos_len - 1) {
			ubo->next = NULL;
		} else {
			ubo->next = &init->ubos[ubo_index + 1];
		}
	}

	init->ssbos_len = NUM_RENDER_SSBOS;
	init->ssbos = (RenderSsboInitData*)stack_alloc(&init_stack, sizeof(RenderSsboInitData) * init->ssbos_len);
	for(i32 ssbo_index = 0; ssbo_index < init->ssbos_len; ssbo_index++) {
		RenderSsboInitData* ssbo = &init->ssbos[ssbo_index];

		switch(ssbo_index) {
			case RENDER_SSBO_MODEL: {
				ssbo->size = sizeof(RenderListInstanceData) * RENDER_LIST_MAX_INSTANCES;
				ssbo->binding = 0;
			} break;
			case RENDER_SSBO_TEXT: {
				ssbo->size = sizeof(RenderGlyph) * ASSET_NUM_FONTS * RENDER_LIST_MAX_GLYPHS_PER_FONT;
				ssbo->binding = 0;
			} break;
			case RENDER_SSBO_RECT: {
				ssbo->size = sizeof(RenderRect) * RENDER_LIST_MAX_RECTS;
				ssbo->binding = 0;
			} break;
			default: {
				printf("Error: No initialization data defined for shader storage buffer %i.\n", ssbo_index);
				panic();
			} break;
		}

		if(ssbo_index == init->ssbos_len - 1) {
			ssbo->next = NULL;
		} else {
			ssbo->next = &init->ssbos[ssbo_index + 1];
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
	// TODO: We need to finish what we started with models and move a lot of this
	// logic to the render list if appropriate.

	// Camera ubo
	RenderCameraUbo* camera_ubos = (RenderCameraUbo*)stack_alloc(&frame_stack, sizeof(RenderCameraUbo) * list->cameras_len);
	for(i32 cam_index = 0; cam_index < list->cameras_len; cam_index++) {
		RenderListCamera* cam = &list->cameras[cam_index];
		RenderCameraUbo* ubo = &camera_ubos[cam_index];
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
	u8 model_host_buffer = render_push_host_buffer(renderer, (u8*)list->instances);

	// Text ssbo
	// NOW: Only allocate amount of glyphs needed, I would imagine!
	RenderGlyph* text_ssbo = (RenderGlyph*)stack_alloc(&frame_stack, sizeof(RenderGlyph) * ASSET_NUM_FONTS * RENDER_LIST_MAX_GLYPHS_PER_FONT);
	for(u32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		for(u32 glyph_index = 0; glyph_index < list->glyph_list_lens[font_index]; glyph_index++) {
			RenderListGlyph* list_glyph = &list->glyph_lists[font_index][glyph_index];
			RenderGlyph* render_glyph = &text_ssbo[font_index * RENDER_LIST_MAX_GLYPHS_PER_FONT + glyph_index];

			render_glyph->dst.x = 2.0f * (list_glyph->offset.x / platform->window_width + list_glyph->screen_anchor.x) - 1.0f;
			render_glyph->dst.y = 2.0f * (list_glyph->offset.y / platform->window_height + list_glyph->screen_anchor.y) - 1.0f;
			render_glyph->dst.z = 2.0f * (list_glyph->size.x / platform->window_width);
			render_glyph->dst.w = 2.0f * (list_glyph->size.y / platform->window_height);

			render_glyph->src = list_glyph->src;
			render_glyph->color = list_glyph->color;
		}
	}
	u8 text_host_buffer = render_push_host_buffer(renderer, (u8*)text_ssbo);

	// Rect ssbo
	RenderRect* rect_ssbo = (RenderRect*)stack_alloc(&frame_stack, sizeof(RenderRect) * list->rects_len);
	for(u32 rect_index = 0; rect_index < list->rects_len; rect_index++) {
		RenderListRect* list_rect = &list->rects[rect_index];
		RenderRect* render_rect = &rect_ssbo[rect_index];
		render_rect->dst.x = 2.0f * (list_rect->dst.x / platform->window_width + list_rect->screen_anchor.x) - 1.0f;
		render_rect->dst.y = 2.0f * (list_rect->dst.y / platform->window_height + list_rect->screen_anchor.y) - 1.0f;
		render_rect->dst.z = 2.0f * (list_rect->dst.z / platform->window_width);
		render_rect->dst.w = 2.0f * (list_rect->dst.w / platform->window_height);
		render_rect->color = list_rect->color;
	}
	u8 rect_host_buffer = render_push_host_buffer(renderer, (u8*)rect_ssbo);

	// =====================
	// GENERATE RENDER GRAPH
	// =====================
	RenderCommandClear clear = { .color = { list->clear_color.r, list->clear_color.g, list->clear_color.b, 1.0f } };
	render_push_command(renderer, RENDER_COMMAND_CLEAR, &clear, &frame_stack);

	RenderCommandUseUbo use_ubo_camera = { .ubo = RENDER_UBO_CAMERA };
	render_push_command(renderer, RENDER_COMMAND_USE_UBO, &use_ubo_camera, &frame_stack);

	// Draw viewports
	for(i32 cam_index = 0; cam_index < list->cameras_len; cam_index++) {
		RenderListCamera* cam = &list->cameras[cam_index];
		v4 viewport_rect = cam->screen_rect;
		viewport_rect.x *= platform->window_width;
		viewport_rect.y *= platform->window_height;
		viewport_rect.z *= platform->window_width;
		viewport_rect.w *= platform->window_height;

		RenderCommandSetViewport set_viewport = { .rect = viewport_rect };
		render_push_command(renderer, RENDER_COMMAND_SET_VIEWPORT, &set_viewport, &frame_stack);
		
		RenderCommandBufferUboData buffer_ubo_data_camera = { .ubo = RENDER_UBO_CAMERA, .host_buffer_index = camera_host_buffer, .host_buffer_offset = sizeof(RenderCameraUbo) * cam_index };
		render_push_command(renderer, RENDER_COMMAND_BUFFER_UBO_DATA, &buffer_ubo_data_camera, &frame_stack);

		// Draw models
		RenderCommandUseProgram use_program_model = { .program = ASSET_RENDER_PROGRAM_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_model, &frame_stack);

		RenderCommandUseSsbo use_ssbo_model = { .ssbo = RENDER_SSBO_MODEL };
		render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_model, &frame_stack);

		for(i32 type_index = 0; type_index < list->instance_types_len; type_index++) {
			RenderListInstanceType* type = &list->instance_types[type_index];
			if(type->instances_len < 1) {
				continue;
				//printf("Instances len less than 1 for instance type %i\n", i);
				//panic();
			}

			RenderCommandBufferSsboData buffer_ssbo_data_model = { 
				.ssbo = RENDER_SSBO_MODEL, 
				.size = sizeof(RenderListInstanceData) * type->instances_len, 
				.host_buffer_index = model_host_buffer, 
				.host_buffer_offset = sizeof(RenderListInstanceData) * type->instance_index_offset
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
	for(i32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		if(list->glyph_list_lens[font_index] > 0) {
			RenderCommandBufferSsboData buffer_ssbo_data_text = { 
				.ssbo = RENDER_SSBO_TEXT, 
				.size = sizeof(RenderGlyph) * list->glyph_list_lens[font_index], 
				.host_buffer_index = text_host_buffer, 
				.host_buffer_offset = text_buffer_offset 
			};
			render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_text, &frame_stack);

			RenderCommandUseTexture use_texture_text = { .texture = list->glyph_list_textures[font_index] };
			render_push_command(renderer, RENDER_COMMAND_USE_TEXTURE, &use_texture_text, &frame_stack);

			RenderCommandDrawMeshInstanced draw_mesh_instanced_text = { 
				.mesh = renderer->primitive_to_mesh_map[RENDER_PRIMITIVE_QUAD], 
				.count = list->glyph_list_lens[font_index] 
			};
			render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_text, &frame_stack);
		}
		text_buffer_offset += sizeof(RenderGlyph) * RENDER_LIST_MAX_GLYPHS_PER_FONT;
	}

	// Draw rects
	RenderCommandUseProgram use_program_rect = { .program = ASSET_RENDER_PROGRAM_RECT };
	render_push_command(renderer, RENDER_COMMAND_USE_PROGRAM, &use_program_rect, &frame_stack);
	
	RenderCommandUseSsbo use_ssbo_rect = { .ssbo = RENDER_SSBO_RECT };
	render_push_command(renderer, RENDER_COMMAND_USE_SSBO, &use_ssbo_rect, &frame_stack);

	RenderCommandBufferSsboData buffer_ssbo_data_rect = { 
		.ssbo = RENDER_SSBO_RECT, 
		.size = sizeof(RenderGlyph) * list->rects_len, 
		.host_buffer_index = rect_host_buffer, 
		.host_buffer_offset = 0 
	};
	render_push_command(renderer, RENDER_COMMAND_BUFFER_SSBO_DATA, &buffer_ssbo_data_rect, &frame_stack);

	RenderCommandDrawMeshInstanced draw_mesh_instanced_rect = { 
		.mesh = renderer->primitive_to_mesh_map[RENDER_PRIMITIVE_QUAD], 
		.count = list->rects_len 
	};
	render_push_command(renderer, RENDER_COMMAND_DRAW_MESH_INSTANCED, &draw_mesh_instanced_rect, &frame_stack);
}
