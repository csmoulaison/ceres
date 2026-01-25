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

void render_list_update_world(RenderList* list, f32* clear_color, f32* camera_position, f32* camera_target) {
	RenderListWorld* world = &list->world;
	v3_copy(world->clear_color, clear_color);
	v3_copy(world->camera_position, camera_position);
	v3_copy(world->camera_target, camera_target);
}

void render_list_draw_model(RenderList* list, i32 model_id, i32 texture, f32* position, f32* orientation) {
	RenderListModel* model = &list->models[list->models_len];
	model->id = model_id;
	model->texture = texture;
	v3_copy(model->position, position);
	v3_copy(model->orientation, orientation);
	list->models_len++;
}

void render_list_draw_glyph(RenderList* list, FontData* font, char c, f32* position, f32* color) {
	FontGlyph* font_glyph = &font->glyphs[c];
	RenderListGlyph* list_glyph = &list->glyphs[list->glyphs_len];
	list->glyphs_len++;

	list_glyph->src[0] = ((f32)font_glyph->position[0]) / font->texture_width;
	list_glyph->src[1] = ((f32)font_glyph->position[1]) / font->texture_height;
	list_glyph->src[2] = ((f32)font_glyph->size[0]) / font->texture_width;
	list_glyph->src[3] = ((f32)font_glyph->size[1]) / font->texture_height;

	list_glyph->dst[0] = position[0];
	list_glyph->dst[1] = position[1];
	list_glyph->dst[2] = font_glyph->size[0];
	list_glyph->dst[3] = font_glyph->size[1];

	list_glyph->color[0] = color[0];
	list_glyph->color[1] = color[1];
	list_glyph->color[2] = color[2];
	list_glyph->color[3] = color[3];
}
