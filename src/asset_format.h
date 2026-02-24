#ifndef asset_format_h_INCLUDED
#define asset_format_h_INCLUDED

// NOW: This just ended up making dirty memory because there are no bounds
// checks in place anywhere. Do some checking in the packing code.
#define MAX_MESH_ASSETS 16
#define MAX_TEXTURE_ASSETS 16
#define MAX_RENDER_PROGRAM_ASSETS 8
#define MAX_FONT_ASSETS 12
#define MAX_LEVEL_ASSETS 8

#define MAX_LEVEL_SPAWNS 32

#include "font.h"

typedef struct {
	// NOW: counts not needed eh if we are putting those in the thing.
	u64 mesh_buffer_offsets[MAX_MESH_ASSETS];
	u64 texture_buffer_offsets[MAX_TEXTURE_ASSETS];
	u64 render_program_buffer_offsets[MAX_RENDER_PROGRAM_ASSETS];
	u64 font_buffer_offsets[MAX_FONT_ASSETS];
	u64 level_buffer_offsets[MAX_LEVEL_ASSETS];

	u8 meshes_len;
	u8 textures_len;
	u8 render_programs_len;
	u8 fonts_len;
	u8 levels_len;

	u8 buffer[];
} AssetMemory;

typedef struct {
	union {
		struct {
			f32 position[3];
			f32 normal[3];
			f32 texture_uv[2];
		};
		f32 data[8];
	};
} MeshVertexData;

typedef struct {
	u32 vertices_len;
	u32 indices_len;
	// MeshVertexData * vertices_len, u32 * indices_len
	u8 buffer[];
} MeshAsset;

typedef struct {
	u32 width;
	u32 height;
	u8 channel_count;
	// u8 * channel_count * width * height
	u8 buffer[];
} TextureAsset;

typedef struct {
	i32 vertex_shader_src_len;
	i32 fragment_shader_src_len;
	// vertex_src, fragment_src
	char buffer[];
} RenderProgramAsset;

typedef struct {
	u32 texture_id;
	u32 glyphs_len;
	// FontGlyph * glyphs_len
	FontGlyph buffer[];
} FontAsset;

typedef struct {
	u16 x;
	u16 y;
	u8 team;
} LevelSpawn;

typedef struct {
	u16 side_length;
	LevelSpawn spawns[MAX_LEVEL_SPAWNS];
	u8 spawns_len;
	u8 buffer[];
} LevelAsset;

/*
#define ASSET_BUFFER_MEMSIZE sizeof(MeshAsset) * MAX_MESH_ASSETS \
	+ sizeof(TextureAsset) * MAX_TEXTURE_ASSETS \
	+ sizeof(RenderProgramAsset) * MAX_RENDER_PROGRAM_ASSETS \
	+ sizeof(FontAsset) * MAX_FONT_ASSETS
*/

// NOW: This didn't allocate enough memory with the above memsize. The second
// loaded mesh was junk data during render initialization, so it wasn't really
// even close.
#define ASSET_BUFFER_MEMSIZE MEGABYTE * 64

#endif // asset_format_h_INCLUDED
