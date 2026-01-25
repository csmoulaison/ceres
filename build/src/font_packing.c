#include <ft2build.h>
#include FT_FREETYPE_H

#define FONT_CHARS_LEN 128

typedef struct {
	u32 src_position[2];
	u32 size[2];
	i32 bearing[2];
	u32 advance;
	u8* bitmap_pixels;
} GlyphInfo;

typedef struct {
	char filename[256];
	u32 font_size;
	u32 texture_id;
	u32 atlas_width;

	u32 pack_order[FONT_CHARS_LEN];
	GlyphInfo glyphs[FONT_CHARS_LEN];
} FontInfo;

void calculate_font_assets(AssetInfoList* list, char* handle, i32 args_len, ManifestArgument* args, Arena* arena) {
	FontInfo* info = (FontInfo*)arena_alloc(arena, sizeof(FontInfo));
	assert(args_len == 2);
	strcpy(info->filename, args[0].text);
	info->font_size = atoi(args[1].text);

	FT_Library ft;
	if(FT_Init_FreeType(&ft)) { panic(); }
	FT_Face ft_face;
	if(FT_New_Face(ft, info->filename, 0, &ft_face)) { panic(); }
	FT_Set_Pixel_Sizes(ft_face, 0, info->font_size);

	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		if(FT_Load_Char(ft_face, (unsigned char)i, FT_LOAD_RENDER)) { panic(); }
		info->pack_order[i] = i;

		GlyphInfo* g = &info->glyphs[i];
		g->size[0] = ft_face->glyph->bitmap.width;
		g->size[1] = ft_face->glyph->bitmap.rows;
		g->bearing[0] = ft_face->glyph->bitmap_left;
		g->bearing[1] = ft_face->glyph->bitmap_top;
		g->advance = (u32)ft_face->glyph->advance.x;

		u32 bm_size = sizeof(u8) * g->size[0] * g->size[1];
		g->bitmap_pixels = (u8*)arena_alloc(arena, bm_size);
		memcpy(g->bitmap_pixels, ft_face->glyph->bitmap.buffer, bm_size);

	}
	FT_Done_Face(ft_face);
	FT_Done_FreeType(ft);

	// Sort the packing order by height, tallest glyphs first
	for(u32 i = 0; i < FONT_CHARS_LEN; i++) {
		for(i32 j = 0; j < FONT_CHARS_LEN - 1; j++) {
			if(info->glyphs[info->pack_order[j]].size[1] < info->glyphs[info->pack_order[j + 1]].size[1]) {
				u32 tmp = info->pack_order[j];
				info->pack_order[j] = info->pack_order[j + 1];
				info->pack_order[j + 1] = tmp;
			}
		}
	}

	// Pack rects using shelf algorithm.
	info->atlas_width = 16;

// This is essentially a while loop that goes until a large enough atlas size
// is found. It seems nicer this way to me.
try_pack_again:

	info->atlas_width *= 2;
	u32 atlas_area = info->atlas_width * info->atlas_width;

	i32 curx = 0;
	i32 cury = 0;
	i32 cur_shelf_size = 0;
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		GlyphInfo* g = &info->glyphs[info->pack_order[i]];
		if(cur_shelf_size == 0) {
			cur_shelf_size = g->size[1];
		}

		if(curx + g->size[0] > info->atlas_width) {
			if(cur_shelf_size == 0) {
				goto try_pack_again;
			}

			cury += cur_shelf_size;
			cur_shelf_size = g->size[1];
			curx = 0;

			//printf("New shelf! x,y,h:%i,%i,%i\n", curx, cury, cur_shelf_size);
		}

		g->src_position[0] = curx;
		g->src_position[1] = cury;
		//printf("Rect packed (x,y,w,h): %.3u,%.3u,%.3u,%.3u\n", rect->x, rect->y, rect->w, rect->h);
		curx += g->size[0];

		if(cury + cur_shelf_size >= info->atlas_width) {
			goto try_pack_again;
		}
	}

	// If we made it here, we found a good atlas size, so it's time to allocate
	// space and render glyphs to the buffer
	TextureInfo* t_info = (TextureInfo*)arena_alloc(arena, sizeof(TextureInfo));
	t_info->source_type = TEXTURE_SOURCE_BUFFER;
	t_info->buffer = (u8*)arena_alloc(arena, sizeof(u8) * info->atlas_width * info->atlas_width);
	t_info->buffer_width = info->atlas_width;
	t_info->buffer_height = info->atlas_width;
	t_info->channel_count = 1;

	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		GlyphInfo* g = &info->glyphs[info->pack_order[i]];
		for(i32 y = 0; y < g->size[1]; y++) {
			for(i32 x = 0; x < g->size[0]; x++) {
				i32 dst_x = g->src_position[0] + x;
				i32 dst_y = g->src_position[1] + y;
				u8 pixel = g->bitmap_pixels[y * g->size[0] + x];
				t_info->buffer[dst_y * info->atlas_width + dst_x] = pixel;
			
				/* This, and the comment below, print out each character in the terminal.
				if(g->bitmap_pixels[y * g->size[0] + x] > 128) {
					printf("#");
				} else {
					printf(" ");
				}
				*/
			}
			//printf("\n");
		}
	}

	u64 t_size = sizeof(TextureAsset) + sizeof(u8) * t_info->buffer_width * t_info->buffer_height;
	u32 t_id = push_asset_info(list, ASSET_TYPE_TEXTURE, handle, t_size, t_info);

	info->texture_id = t_id;
	u64 f_size = sizeof(FontAsset) * sizeof(FontGlyph) * FONT_CHARS_LEN;
	push_asset_info(list, ASSET_TYPE_FONT, handle, f_size, info);
}

void pack_font_asset(void* p_info, void* p_asset) {
	FontAsset* asset = (FontAsset*)p_asset;
	FontInfo* info = (FontInfo*)p_info;

	asset->glyphs_len = FONT_CHARS_LEN;
	asset->texture_id = info->texture_id;
	
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		GlyphInfo* g_info = &info->glyphs[i];
		FontGlyph* g_asset = &asset->buffer[i];

		g_asset->position[0] = g_info->src_position[0];
		g_asset->position[1] = g_info->src_position[1];
		g_asset->size[0] = g_info->size[0];
		g_asset->size[1] = g_info->size[1];
		g_asset->bearing[0] = g_info->bearing[0];
		g_asset->bearing[1] = g_info->bearing[1];
		g_asset->advance = g_info->advance;
	}
}
