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

#ifndef render_list_h_INCLUDED
#define render_list_h_INCLUDED

#include "renderer/renderer.h"

#define RENDER_LIST_MAX_MODEL_INSTANCES 4096
#define RENDER_LIST_MAX_GLYPHS 1024

typedef struct {
	f32 clear_color[3];
	f32 camera_position[3];
	f32 camera_target[3];
} RenderListWorld;

typedef struct {
	f32 position[3];
	f32 orientation[3];

	i32 id;
	i32 texture;
} RenderListModel;

typedef struct {
	f32 src[4];
	f32 dst[4];
	f32 color[4];
} RenderListGlyph;

typedef struct {
	RenderListWorld world;

	RenderListModel models[RENDER_LIST_MAX_MODEL_INSTANCES];
	u32 models_len;

	RenderListGlyph glyph_lists[ASSET_NUM_FONTS][RENDER_LIST_MAX_GLYPHS];
	u32 glyph_list_lens[ASSET_NUM_FONTS];
	u32 glyph_list_textures[ASSET_NUM_FONTS];
} RenderList;

#endif // render_list_h_INCLUDED
