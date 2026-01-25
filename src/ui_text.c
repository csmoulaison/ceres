#include "font.h"

// TODO: Make screen anchor part of this.
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

void ui_text_line(
	RenderList* list,
	char* text,
	FontData* font,
	u32 font_handle,
	f32* color,
	float x,
	float y,
	float anchor_x,
	float anchor_y)
{
	u32 str_len = strlen(text);
	float x_placements[str_len];
	float y_placements[str_len];
	ui_text_line_placements(font, text, x_placements, y_placements,
		x, y, anchor_x, anchor_y);

	for(i32 j = 0; j < str_len; j++) {
		f32 position[2] = {x_placements[j], y_placements[j]};
		render_list_draw_glyph(list, font, font_handle, text[j], position, color);
	}
}
