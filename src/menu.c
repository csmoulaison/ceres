// Returns -1 if no option was selected. Returns the selection index otherwise
i32 menu_list(u8* selection, u8 num_selections, bool up, bool down, bool select) {
	if(up) {
		if(*selection <= 0) {
			*selection = num_selections - 1;
		} else {
			*selection -= 1;
		}
	}
	if(down) {
		if(*selection >= num_selections - 1) {
			*selection = 0;
		} else {
			*selection += 1;
		}
	}
	if(select) {
		return *selection;
	} else {
		return -1;
	}
}

void draw_menu_list(u8 selection, u8 num_selections, char** strings, RenderList* list, FontData* fonts, StackAllocator* draw_stack) {
	f32 line_h = 64.0f;
	f32 root_y = ((f32)num_selections* line_h) / 2.0f;
	for(i32 selection_index = 0; selection_index < num_selections; selection_index++) {
		v4 color = v4_new(0.0f, 1.0f, 0.0f, 0.5f);
		if(selection_index == selection) color = v4_new(1.0f, 1.0f, 0.0f, 0.5f);
		ui_draw_text_line(list, fonts, ASSET_FONT_QUANTICO_LARGE, strings[selection_index],
			v2_new(0.0f, root_y - line_h * selection_index), v2_new(0.5f, 0.5f), v2_new(0.5f, 0.5f), color, draw_stack);
	}
}
