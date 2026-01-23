#define TEXTURE_PIXEL_STRIDE_BYTES 4
// NOW: This just ended up making dirty memory because there are no bounds
// checks in place anywhere. Do some checking in the packing code.
#define MAX_MESH_ASSETS 4
#define MAX_TEXTURE_ASSETS 4
#define MAX_RENDER_PROGRAM_ASSETS 2
#define MAX_FONT_ASSETS 1

typedef struct {
	u8 meshes_len;
	u64 mesh_buffer_offsets[MAX_MESH_ASSETS];

	u8 textures_len;
	u64 texture_buffer_offsets[MAX_TEXTURE_ASSETS];

	u8 render_programs_len;
	u64 render_program_buffer_offsets[MAX_RENDER_PROGRAM_ASSETS];

	u8 fonts_len;
	u64 font_buffer_offsets[MAX_FONT_ASSETS];

	u8 buffer[];
} AssetPack;

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
	u32 position[2];
	u32 size[2];
	i32 bearing[2];
	u32 advance;
} FontGlyph;

typedef struct {
	u32 texture_id;
	u32 glyphs_len;
	// FontGlyph * glyphs_len
	FontGlyph buffer[];
} FontAsset;
