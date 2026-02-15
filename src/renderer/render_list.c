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
	list->instances_len = 0;
	list->instance_index_offset = 0;
	list->instance_types_len = 0;
	list->rects_len = 0;
	for(i32 i = 0; i < RENDER_LIST_MAX_INSTANCE_TYPES; i++) {
		list->instance_types[i].instances_len = 0;
	}
	for(i32 i = 0; i < ASSET_NUM_FONTS; i++) {
		list->glyph_list_lens[i] = 0;
		list->glyph_list_textures[i] = 0;
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
	assert(list->instances_len + count < RENDER_LIST_MAX_INSTANCES);
	assert(list->instance_types_len < RENDER_LIST_MAX_INSTANCE_TYPES);
	strict_assert(list->instances_len < 65535);
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
	for(i32 i = 0; i < list->instance_types_len; i++) {
		type = &list->instance_types[i];
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
	m4_translation(position, instance->transform);
	f32 rotation[16];
	m4_rotation(
		orientation.x, 
		orientation.y, 
		orientation.z, 
		rotation);
	m4_mul(instance->transform, rotation, instance->transform);
}

void render_list_draw_model_aligned(RenderList* list, u8 model, u8 texture, v3 position) {
	RenderListInstanceData* instance = render_list_push_instance(list, model, texture);
	m4_translation(position, instance->transform);
}

void render_list_draw_model_colored(RenderList* list, u8 model, u8 texture, v3 position, v3 orientation, v4 color) {
	RenderListInstanceData* instance = render_list_push_instance(list, model, texture);
	instance->color = color;
	m4_translation(position, instance->transform);
	f32 rotation[16];
	m4_rotation(
		orientation.x, 
		orientation.y, 
		orientation.z, 
		rotation);
	m4_mul(instance->transform, rotation, instance->transform);
}

void render_list_draw_laser(RenderList* list, v3 start, v3 end, f32 stroke) {
	RenderListInstanceData* instance = render_list_push_instance(list, ASSET_MESH_CYLINDER, 0);
	instance->color = v4_new(2.0f, 0.0f, 0.0f, 1.0f);
	v3 line_delta = v3_sub(end, start);
	m4_scale(v3_new(v3_magnitude(line_delta), stroke, stroke), instance->transform);

	f32 rotation[16];
	v3 line_norm = v3_normalize(line_delta);
	m4_rotation(0.0f, atan2(-line_norm.z, line_norm.x), 0.0f, rotation);

	f32 translation[16];
	m4_translation(start, translation);

	m4_mul(rotation, instance->transform, instance->transform);
	m4_mul(translation, instance->transform, instance->transform);
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
