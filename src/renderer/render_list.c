/*
The render list is the API from the game layer to the renderer frontend, and the 
render graph is the API from the renderer frontend to a renderer backend.

As a rule of thumb, the render list encodes information about specific instances
of a particular type of draw, with information about specific mesh/textures and
the like.

The renderer frontend is agnostic to particular instances, it only knows how to 
encode data into commands, which is still specific to a set of capabilities 
provided by the game.

It's hard to strictly delineate this split in my head, but here's the pragmatic
heuristic: the render list is the API which allows us the programmer to work on
the game layer whenever we want, in a way that isn't brittle to inevitable
changes made to the render layer.
*/

#include "renderer/render_list.h"
#include "font.h"

void render_list_init(RenderList* list) {
	list->cameras_len = 0;
	list->instance_index_offset = 0;
	list->instance_types_len = 0;
	list->rects_len = 0;
	for(i32 type_index = 0; type_index < RENDER_LIST_MAX_INSTANCE_TYPES; type_index++) {
		list->instance_types[type_index].instances_len = 0;
	}
	for(i32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		list->glyph_list_lens[font_index] = 0;
		list->glyph_list_textures[font_index] = 0;
	}
}

// NOW: Interpolation is quite choppy, less so when camera isn't moving. It
// doesn't appear when the frame time is in sync with the monitor.
void render_list_interpolated(RenderList* previous, RenderList* current, RenderList* res, f64 t) {
	*res = *current;

	res->cameras_len = previous->cameras_len;
	res->instance_index_offset = previous->instance_index_offset;
	res->instance_types_len = previous->instance_types_len;
	res->rects_len = previous->rects_len;
	for(i32 type_index = 0; type_index < RENDER_LIST_MAX_INSTANCE_TYPES; type_index++) {
		res->instance_types[type_index].instances_len = previous->instance_types[type_index].instances_len;
	}
	for(i32 font_index = 0; font_index < ASSET_NUM_FONTS; font_index++) {
		res->glyph_list_lens[font_index] = previous->glyph_list_lens[font_index];
		res->glyph_list_textures[font_index] = previous->glyph_list_textures[font_index];
	}

	for(i32 cam_index = 0; cam_index < res->cameras_len; cam_index++) {
		RenderListCamera* prev_cam = &previous->cameras[cam_index];
		RenderListCamera* cur_cam = &current->cameras[cam_index];
		RenderListCamera* res_cam = &res->cameras[cam_index];
		res_cam->position = v3_lerp(prev_cam->position, cur_cam->position, t);
		//res_cam->position = v3_new(40.0f, 5.0f, 40.0f);
		res_cam->target = v3_lerp(prev_cam->target, cur_cam->target, t);
		//res_cam->target = v3_new(32.0f, 0.0f, 32.0f);
		//res_cam->position = cur_cam->position;
		//res_cam->target = cur_cam->target;
		res_cam->screen_rect = v4_lerp(prev_cam->screen_rect, cur_cam->screen_rect, t);
	}

	for(i32 type_index = 0; type_index < res->instance_types_len; type_index++) {
		RenderListInstanceType* type = &res->instance_types[type_index];
		u32 offset = type->instance_index_offset;
		u32 len = type->instances_len;
		for(i32 inst_index = 0; inst_index < len; inst_index++) {
			RenderListInstanceData* prev_inst = &previous->instances[offset + inst_index];
			RenderListInstanceData* cur_inst = &current->instances[offset + inst_index];
			RenderListInstanceData* res_inst = &res->instances[offset + inst_index];
			res_inst->position = v3_lerp(prev_inst->position, cur_inst->position, t);
			res_inst->rotation = v3_lerp(prev_inst->rotation, cur_inst->rotation, t);
			res_inst->scale = v3_lerp(prev_inst->scale, cur_inst->scale, t);
			res_inst->color = v4_lerp(prev_inst->color, cur_inst->color, t);
		}
	}
}

void render_list_set_clear_color(RenderList* list, v3 clear_color) {
	list->clear_color = clear_color;
}

void render_list_add_camera(RenderList* list, v3 position, v3 target, v4 screen_rect) {
	assert(list->cameras_len < RENDER_LIST_MAX_CAMERAS);
	RenderListCamera* camera = &list->cameras[list->cameras_len];
	camera->position = position;
	camera->target = target;
	camera->screen_rect = screen_rect;
	list->cameras_len++;
}

// NOW: Check if matching type has already been allocated.
void render_list_allocate_instance_type(RenderList* list, u8 model, u8 texture, i32 count) {
	assert(list->instance_types_len < RENDER_LIST_MAX_INSTANCE_TYPES);
	strict_assert(list->instance_types_len < 255);

	RenderListInstanceType* type = &list->instance_types[list->instance_types_len];
	list->instance_types_len++;
	type->model = model;
	type->texture = texture;
	type->instance_index_offset = list->instance_index_offset;
	list->instance_index_offset += count;
}

RenderListInstanceData* render_list_push_instance(RenderList* list, u8 model, u8 texture) {
	RenderListInstanceType* type = NULL;
	for(i32 type_index = 0; type_index < list->instance_types_len; type_index++) {
		type = &list->instance_types[type_index];
		if(type->model == model && type->texture == texture) {
			break;
		} else {
			type = NULL;
		}
	}
	assert(type != NULL);
	
	// TODO: Bounds checking on instances? Right now we aren't storing the count
	// when the type is allocated. Maybe we can only store when in debug mode.
	RenderListInstanceData* instance = &list->instances[type->instance_index_offset + type->instances_len];
	instance->color = v4_new(1.0f, 1.0f, 1.0f, 0.0f);
	type->instances_len++;
	return instance;
}

void render_list_draw_model(RenderList* list, u8 model, u8 texture, v3 position, v3 orientation) {
	RenderListInstanceData* instance = render_list_push_instance(list, model, texture);
	instance->position = position;
	instance->rotation = orientation;
	instance->scale = v3_identity();
}

void render_list_draw_model_aligned(RenderList* list, u8 model, u8 texture, v3 position) {
	RenderListInstanceData* instance = render_list_push_instance(list, model, texture);
	instance->position = position;
	instance->rotation = v3_zero();
	instance->scale = v3_identity();
}

void render_list_draw_model_colored(RenderList* list, u8 model, u8 texture, v3 position, v3 orientation, v4 color) {
	RenderListInstanceData* instance = render_list_push_instance(list, model, texture);
	instance->position = position;
	instance->rotation = orientation;
	instance->scale = v3_identity();
	instance->color = color;
}

void render_list_draw_laser(RenderList* list, v3 start, v3 end, f32 stroke) {
	RenderListInstanceData* instance = render_list_push_instance(list, ASSET_MESH_CYLINDER, 0);
	instance->position = start;
	instance->color = v4_new(2.0f, 0.0f, 0.0f, 1.0f);

	v3 line_delta = v3_sub(end, start);
	instance->scale = v3_new(v3_magnitude(line_delta), stroke, stroke);

	f32 rotation[16];
	v3 line_norm = v3_normalize(line_delta);
	instance->rotation = v3_new(0.0f, atan2(-line_norm.z, line_norm.x), 0.0f);
}

void render_list_draw_glyph(RenderList* list, FontData* fonts, FontAssetHandle font_handle, char c, v2 position, v2 screen_anchor, v4 color) {
	assert(list->glyph_list_lens[font_handle] < RENDER_LIST_MAX_GLYPHS_PER_FONT);

	FontData* font = &fonts[font_handle];
	FontGlyph* font_glyph = &font->glyphs[(u8)c];
	RenderListGlyph* list_glyph = &list->glyph_lists[font_handle][list->glyph_list_lens[font_handle]];
	list->glyph_list_lens[font_handle] += 1;
	// TODO: this is quite unideal, setting every time we draw a single glyph, when
	// really this is just statically derivable data.
	list->glyph_list_textures[font_handle] = font->texture_id;

	list_glyph->screen_anchor = screen_anchor;
	list_glyph->offset = position;
	list_glyph->size.x = font_glyph->size[0];
	list_glyph->size.y = font_glyph->size[1];
	list_glyph->color = color;

	list_glyph->src.x = ((f32)font_glyph->position[0]) / font->texture_width;
	list_glyph->src.y = ((f32)font_glyph->position[1]) / font->texture_height;
	list_glyph->src.z = ((f32)font_glyph->size[0]) / font->texture_width;
	list_glyph->src.w = ((f32)font_glyph->size[1]) / font->texture_height;
}

void render_list_draw_rect(RenderList* list, v4 dst, v2 screen_anchor, v4 color) {
	assert(list->rects_len < RENDER_LIST_MAX_RECTS);

	RenderListRect* rect = &list->rects[list->rects_len];
	list->rects_len++;
	rect->dst = dst;
	rect->color = color;
	rect->screen_anchor = screen_anchor;
}

void render_list_draw_box(RenderList* list, v4 dst, v2 screen_anchor, f32 stroke, v4 color) {
	f32 hstroke = stroke / 2.0f;
	render_list_draw_rect(list, v4_new(dst.x - hstroke,         dst.y - hstroke,         dst.z + stroke, stroke),         screen_anchor, color);
	render_list_draw_rect(list, v4_new(dst.x - hstroke,         dst.y + dst.w - hstroke, dst.z + stroke, stroke),         screen_anchor, color);
	render_list_draw_rect(list, v4_new(dst.x - hstroke,         dst.y + hstroke,         stroke,         dst.w - stroke), screen_anchor, color);
	render_list_draw_rect(list, v4_new(dst.x + dst.z - hstroke, dst.y + hstroke,         stroke,         dst.w - stroke), screen_anchor, color);
}
