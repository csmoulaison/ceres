#include "font.h"

// NOW: time for vector structs dude. semantics, i'm sorry. i'm sorry bro.

typedef struct {
	f32* x;
	f32* y;
	u32 len;
} TextLinePlacements;

// TODO: Make screen anchor part of this.
TextLinePlacements ui_text_line_placements(FontData* fonts, FontAssetHandle font_handle, const char* text, float x, float y, float anchor_x, float anchor_y, Arena* arena) {
	TextLinePlacements result;
	result.len = strlen(text);
	result.x = (f32*)arena_alloc(arena, sizeof(f32) * result.len);
	result.y = (f32*)arena_alloc(arena, sizeof(f32) * result.len);

	FontData* font = &fonts[font_handle];
	float line_width = 0;
	float cur_x = x;
	for(i32 i = 0; i < result.len; i++) {
		char c = text[i];
		FontGlyph* glyph = &font->glyphs[c];

		result.x[i] = cur_x + glyph->bearing[0];
		result.y[i] = y - (glyph->size[1] - glyph->bearing[1]);
        cur_x += (glyph->advance >> 6);
	}

	float off_x = (cur_x - x) * anchor_x;
	float off_y = font->size * anchor_y;
	for(i32 i = 0; i < result.len; i++) {
		result.x[i] -= off_x;
		result.y[i] -= off_y;

		result.x[i] = floor(result.x[i]);
		result.y[i] = floor(result.y[i]);
	}

	return result;
}

void ui_draw_text_line(RenderList* list, FontData* fonts, FontAssetHandle font_handle, char* text, float x, float y, float anchor_x, float anchor_y, f32* color, Arena* arena) {
	TextLinePlacements placements = ui_text_line_placements(fonts, font_handle, text, x, y, anchor_x, anchor_y, arena);

	FontData* font = &fonts[font_handle];
	for(i32 j = 0; j < placements.len; j++) {
		f32 position[2] = {placements.x[j], placements.y[j]};
		render_list_draw_glyph(list, fonts, font_handle, text[j], position, color);
	}
}
