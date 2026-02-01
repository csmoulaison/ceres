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
	*list = (RenderList){};
}

void render_list_set_clear_color(RenderList* list, v3 clear_color) {
	list->clear_color = clear_color;
}

void render_list_add_camera(RenderList* list, v3 position, v3 target, v4 screen_rect) {
	RenderListCamera* camera = &list->cameras[list->cameras_len];
	camera->position = position;
	camera->target = target;
	camera->screen_rect = screen_rect;
	list->cameras_len++;
}

void render_list_draw_model(RenderList* list, i32 model_id, i32 texture, v3 position, v3 orientation) {
	RenderListModel* model = &list->models[list->models_len];
	model->id = model_id;
	model->texture = texture;
	model->position = position;
	model->orientation = orientation;
	list->models_len++;
}

void render_list_draw_glyph(RenderList* list, FontData* fonts, FontAssetHandle font_handle, char c, v2 position, v2 screen_anchor, v4 color) {
	FontData* font = &fonts[font_handle];
	FontGlyph* font_glyph = &font->glyphs[c];
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
