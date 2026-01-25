#ifndef font_h_INCLUDED
#define font_h_INCLUDED

#define FONT_GLYPHS_COUNT 128

typedef struct {
	u32 position[2];
	u32 size[2];
	i32 bearing[2];
	u32 advance;
} FontGlyph;

typedef struct {
	u32 texture_id;
	u32 texture_width;
	u32 texture_height;
	u32 size;
	FontGlyph glyphs[FONT_GLYPHS_COUNT];
} FontData;

#endif // font_h_INCLUDED
