#define TEXTURE_PIXEL_STRIDE_BYTES 4

typedef struct {
	u8 meshes_len;
	u8 textures_len;
	u64* mesh_buffer_offsets;
	u64* texture_buffer_offsets;
	u8* buffer;
} AssetPackData;

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
} MeshAssetHeader;

typedef struct {
	u32 width;
	u32 height;
} TextureAssetHeader;
