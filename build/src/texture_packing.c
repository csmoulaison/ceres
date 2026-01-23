#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include "stb_image.h"

typedef enum {
	TEXTURE_SOURCE_BMP,
	TEXTURE_SOURCE_BUFFER
} TextureSourceType;

typedef struct {
	TextureSourceType source_type;
	char filename[256];
	u8 channel_count;

	// only used if source type is buffer
	u8* buffer; 
	u32 buffer_width;
	u32 buffer_height;
} TextureInfo;

void calculate_texture_assets(AssetInfoList* list, char* handle, i32 args_len, ManifestArgument* args, Arena* arena) {
	TextureInfo* info = (TextureInfo*)arena_alloc(arena, sizeof(TextureInfo));
	info->source_type = TEXTURE_SOURCE_BMP;
	assert(args_len == 1);
	strcpy(info->filename, args[0].text);

	u32 w, h, channels;
	stbi_uc* stb_pixels = stbi_load(info->filename, &w, &h, &channels, STBI_rgb_alpha);
	assert(stb_pixels != NULL);
	assert(channels == 4);

	u64 size = sizeof(TextureAsset) + sizeof(u32) * w * h;
	push_asset_info(list, ASSET_TYPE_TEXTURE, handle, size, info);
}

void pack_texture_asset(void* p_info, void* p_asset) {
	// NOW: handle the font case with bmp, buffer switch
	TextureInfo* info = (TextureInfo*)p_info;
	TextureAsset* asset = (TextureAsset*)p_asset;

	switch(info->source_type) {
		case TEXTURE_SOURCE_BMP: {
			u32 channels;
			stbi_uc* stb_pixels = stbi_load(info->filename, &asset->width, &asset->height, &channels, STBI_rgb_alpha);
			asset->channel_count = channels;

			assert(stb_pixels != NULL);
			assert(asset->channel_count == 4 || asset->channel_count == 1);
			assert(asset->width == 256 && asset->height == 256);

			u64 buffer_size = sizeof(u32) * asset->width * asset->height;
			memcpy(asset->buffer, stb_pixels, buffer_size);
			stbi_image_free(stb_pixels);
		} break;
		case TEXTURE_SOURCE_BUFFER: {
			asset->width = info->buffer_width;
			asset->height = info->buffer_height;
			asset->channel_count = info->channel_count;
			memcpy(asset->buffer, info->buffer, sizeof(u8) * info->buffer_width * info->buffer_height);
		} break;
		default: break;
	}
}
