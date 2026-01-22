// NOW: Integrate this into the asset packing code. This is really going to
// stretch the limits of the whole "get the size in buffer before actually
// packing the asset" paradigm. For now, just be maximally redundant and pray
// for forgiveness.
// 
// Something to optimize if the build step gets slow, I guess.

#include <ft2build.h>
#include FT_FREETYPE_H

#define FONT_CHARS_LEN 128
#define MAX_PACK_LEAVES 1024

struct FontChar {
	u8* pixels;
	u32 size[2];
	i32 bearing[2];
	u32 advance;
};

struct PackRect {
	u32 x, y, w, h;
	i32 char_index;
};

// Represents an empty space in the atlas being packed.
struct PackNode {
	i32 x, y, w, h;
};

// { (u32) x, y, w, h,
//   (i32) bearing,
//   (u32) advance
// } Chars[]
struct SerialChar {
	u32 x, y, w, h;
	i32 bearing[2];
	u32 advance;
};

bool try_to_pack_font(u32 font_size, i32 atlas_length, i32 atlas_area, 
PackRect* rects, FontChar* chars, const char* out_fname) {
	// Pack rects using shelf algorithm.
	i32 curx = 0;
	i32 cury = 0;
	i32 cur_shelf_size = 0;
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		PackRect* rect = &rects[i];
		if(cur_shelf_size == 0) {
			cur_shelf_size = rect->h;
		}

		if(curx + rect->w > atlas_length) {
			if(cur_shelf_size == 0) {
				return false;
			}

			cury += cur_shelf_size;
			cur_shelf_size = rect->h;
			curx = 0;

			//printf("New shelf! x,y,h:%i,%i,%i\n", curx, cury, cur_shelf_size);
		}

		rect->x = curx;
		rect->y = cury;
		//printf("Rect packed (x,y,w,h): %.3u,%.3u,%.3u,%.3u\n", rect->x, rect->y, rect->w, rect->h);
		curx += rect->w;

		if(cury + cur_shelf_size >= atlas_length) {
			return false;
		}
	}

	// Render to atlas texture.
	u8 atlas[atlas_area] = {};
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		PackRect* rect = &rects[i];
		FontChar* ch = &chars[rect->char_index];
		for(i32 y = 0; y < ch->size[1]; y++) {
			for(i32 x = 0; x < ch->size[0]; x++) {
				i32 dst_x = rect->x + x;
				i32 dst_y = rect->y + y;
				u8 pixel = ch->pixels[y * ch->size[0] + x];
				atlas[dst_y * atlas_length + dst_x] = pixel;
			}
		}
	}

	SerialChar serial_chars[FONT_CHARS_LEN];
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		PackRect* rect = &rects[i];
		FontChar* ch = &chars[rect->char_index];

		SerialChar* schar = &serial_chars[rect->char_index];
		schar->x = rect->x;
		schar->y = rect->y;
		schar->w = rect->w;
		schar->h = rect->h;
		schar->bearing[0] = ch->bearing[0];
		schar->bearing[1] = ch->bearing[1];
		schar->advance = ch->advance;
	}

	// Store in binary format.
	// 
	// FORMAT SPECIFICATION:
	// (u32) AtlasLength
	// (u32) NumChars
	// { (u32) x, y, w, h,
	//   (i32) bearing[2],
	//   (u32) advance
	// } Chars[]
	// (u8) Pixels[]

	Arena file_arena;
	arena_init(&file_arena, GIGABYTE);

	*((u32*)arena_alloc(&file_arena, sizeof(u32))) = atlas_length;
	*((u32*)arena_alloc(&file_arena, sizeof(u32))) = FONT_CHARS_LEN;
	for(i32 i = 0; i < FONT_CHARS_LEN; i++) {
		SerialChar* ch = &serial_chars[i];
		
		*((u32*)arena_alloc(&file_arena, sizeof(u32))) = ch->x;
		*((u32*)arena_alloc(&file_arena, sizeof(u32))) = ch->y;
		*((u32*)arena_alloc(&file_arena, sizeof(u32))) = ch->w;
		*((u32*)arena_alloc(&file_arena, sizeof(u32))) = ch->h;

		*((i32*)arena_alloc(&file_arena, sizeof(i32))) = ch->bearing[0];
		*((i32*)arena_alloc(&file_arena, sizeof(i32))) = ch->bearing[1];
		*((u32*)arena_alloc(&file_arena, sizeof(u32))) = ch->advance;;
	}
	for(u32 i = 0; i < atlas_area; i++) {
		*((u8*)arena_alloc(&file_arena, sizeof(u8))) = atlas[i];
	}

	// Write to file
	FILE* f = fopen(out_fname, "w");
	size_t s = fwrite(file_arena.region, 1, file_arena.index, f);
	printf("Wrote %u bytes (%u)\n", s, file_arena.index);
	printf("Image length: %u\n", atlas_length);
	fclose(f);

	return true;
}

void pack_font_asset(FontInfo* info, FontAsset* asset) {
	const char* in_filename = argv[1];
	const char* out_filename = argv[2];

	i32 font_size = atoi(argv[3]);
	if(font_size == 0) {
		printf("Invalid font size argument.\n  (Usage: atlas font.tff 12)\n");
		return 1;
	}

	// Load glyphs with freetype
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) { panic(); }

	FT_Face face;
	if(FT_New_Face(ft, in_filename, 0, &face)) { panic(); }

	FT_Set_Pixel_Sizes(face, 0, font_size);

	Arena pixel_arena;
	arena_init(&pixel_arena, GIGABYTE);

	FontChar chars[FONT_CHARS_LEN];

	for(unsigned char c = 0; c < FONT_CHARS_LEN; c++) {
		if(FT_Load_Char(face, c, FT_LOAD_RENDER)) { panic(); }

		FontChar* ch = &chars[c];
		ch->size[0] = face->glyph->bitmap.width;
		ch->size[1] = face->glyph->bitmap.rows;
		ch->bearing[0] = face->glyph->bitmap_left;
		ch->bearing[1] = face->glyph->bitmap_top;
		ch->advance = (u32)face->glyph->advance.x;

		u32 tex_size = sizeof(u8) * ch->size[0] * ch->size[1];
		ch->pixels = (u8*)arena_alloc(&pixel_arena, tex_size);
		memcpy(ch->pixels, face->glyph->bitmap.buffer, tex_size);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// RECT PACKING:
	PackRect rects[FONT_CHARS_LEN];
	i32 rects_len = 0;

	// Populate rects and sort by height. Just bubble sort, don't @ me.
	for(u32 i = 0; i < FONT_CHARS_LEN; i++) {
		PackRect* rect = &rects[i];
		rect->x = 0;
		rect->y = 0;
		rect->w = chars[i].size[0];
		rect->h = chars[i].size[1];
		rect->char_index = i;
	}
	for(u32 i = 0; i < FONT_CHARS_LEN; i++) {
		for(i32 j = 0; j < FONT_CHARS_LEN - 1; j++) {
			if(rects[j].h < rects[j + 1].h) {
				PackRect tmp = rects[j];
				rects[j] = rects[j + 1];
				rects[j + 1] = tmp;
			}
		}
	}

	// TODO: Just keep trying to pack and double on each attempt.
	i32 atlas_length = 2;
	i32 atlas_area = 0;
	while(pack(font_size, atlas_length, atlas_area, rects, chars, out_filename) == false) {
		atlas_length *= 2;
		atlas_area = atlas_length * atlas_length;

		assert(atlas_length <= 4096);
	}
}
