#include "font.h"

// Placements must be preallocated float * string length.
void ui_text_line_placements(
	FontData* font,
	const char* string,
	float* x_placements,
	float* y_placements,
	float x,
	float y,
	float anchor_x,
	float anchor_y)
{
	float line_width = 0;
	float cur_x = x;

	i32 len = strlen(string);
	for(i32 i = 0; i < len; i++) {
		char c = string[i];
		FontGlyph* glyph = &font->glyphs[c];

		x_placements[i] = cur_x + glyph->bearing[0];
		y_placements[i] = y - (glyph->size[1] - glyph->bearing[1]);
        cur_x += (glyph->advance >> 6);
	}

	float off_x = (cur_x - x) * anchor_x;
	float off_y = font->size * anchor_y;
	for(i32 i = 0; i < len; i++) {
		x_placements[i] -= off_x;
		y_placements[i] -= off_y;

		x_placements[i] = floor(x_placements[i]);
		y_placements[i] = floor(y_placements[i]);
	}
}
