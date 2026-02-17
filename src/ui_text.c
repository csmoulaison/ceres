#include "font.h"

typedef struct {
	f32* x;
	f32* y;
	u32 len;
} TextLinePlacements;

// TODO: Make screen anchor part of this.
TextLinePlacements ui_text_line_placements(FontData* fonts, FontAssetHandle font_handle, const char* text, v2 position, v2 inner_anchor, StackAllocator* stack) {
	TextLinePlacements result;
	result.len = strlen(text);
	result.x = (f32*)stack_alloc(stack, sizeof(f32) * result.len);
	result.y = (f32*)stack_alloc(stack, sizeof(f32) * result.len);

	FontData* font = &fonts[font_handle];
	f32 cur_x = position.x;
	for(i32 ci = 0; ci < result.len; ci++) {
		char c = text[ci];
		FontGlyph* glyph = &font->glyphs[(u8)c];

		result.x[ci] = cur_x + glyph->bearing[0];
		result.y[ci] = position.y - (glyph->size[1] - glyph->bearing[1]);
        cur_x += (glyph->advance >> 6);
	}

	f32 off_x = (cur_x - position.x) * inner_anchor.x;
	f32 off_y = font->size * inner_anchor.y;
	for(i32 pl = 0; pl < result.len; pl++) {
		result.x[pl] -= off_x;
		result.y[pl] -= off_y;

		result.x[pl] = floor(result.x[pl]);
		result.y[pl] = floor(result.y[pl]);
	}

	return result;
}

void ui_draw_text_line(RenderList* list, FontData* fonts, FontAssetHandle font_handle, char* text, v2 position, v2 inner_anchor, v2 screen_anchor, v4 color, StackAllocator* stack) {
	TextLinePlacements placements = ui_text_line_placements(fonts, font_handle, text, position, inner_anchor, stack);
	for(i32 pl = 0; pl < placements.len; pl++) {
		v2 position = v2_new(placements.x[pl], placements.y[pl]);
		render_list_draw_glyph(list, fonts, font_handle, text[pl], position, screen_anchor, color);
	}
}
