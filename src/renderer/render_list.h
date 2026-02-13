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

#define RENDER_LIST_MAX_INSTANCE_TYPES 32
#define RENDER_LIST_MAX_INSTANCES 16000
#define RENDER_LIST_MAX_LASERS 32
#define RENDER_LIST_MAX_GLYPHS_PER_FONT 512
#define RENDER_LIST_MAX_CAMERAS 2

typedef u8 RenderListInstanceArray;

typedef struct {
	v3 position;
	v3 target;
	v4 screen_rect;
} RenderListCamera;

typedef struct {
	f32 transform[16];
	v4 color;
} RenderListInstanceData;

typedef struct {
	u8 model;
	u8 texture;
	u16 instances_len;
	u16 instance_index_offset;
} RenderListInstanceType;

typedef struct {
	v2 screen_anchor;
	v2 offset;
	v2 size;
	v4 color;
	v4 src;
} RenderListGlyph;

typedef struct {
	v3 clear_color;

	RenderListCamera cameras[RENDER_LIST_MAX_CAMERAS];
	u32 cameras_len;

	RenderListInstanceData instances[RENDER_LIST_MAX_INSTANCES];
	u32 instances_len;
	u32 instance_index_offset;

	RenderListInstanceType instance_types[RENDER_LIST_MAX_INSTANCE_TYPES];
	u32 instance_types_len;

	RenderListGlyph glyph_lists[ASSET_NUM_FONTS][RENDER_LIST_MAX_GLYPHS_PER_FONT];
	u32 glyph_list_lens[ASSET_NUM_FONTS];
	u32 glyph_list_textures[ASSET_NUM_FONTS];
} RenderList;

#endif // render_list_h_INCLUDED
