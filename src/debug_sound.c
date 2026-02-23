void debug_draw_sound_channels(Audio* audio, RenderList* list, FontData* fonts, StackAllocator* ui_stack) {
	for(i32 channel_index = 0; channel_index < SOUND_MAX_CHANNELS; channel_index++) {
		SoundChannel* channel = &audio->channels[channel_index];

		v4 color = v4_new(0.0f, 1.0f, 0.0f, 1.0f);
		if(channel_index >= audio->active_channels_len) color = v4_new(1.0f, 0.0f, 0.0f, 1.0f);
		render_list_draw_box(list, v4_new(64.0f + 96.0f * channel_index, -128.0f, 64.0f, 64.0f), v2_new(0.0f, 1.0f), 2.0f, color);
	}

	for(i32 sound_index = 0; sound_index < SOUND_SPARSE_LEN; sound_index++) {
		SoundData* sound = &audio->sounds[sound_index];
		v4 color = v4_new(0.0f, 1.0f, 0.0f, 1.0f);
		if(!sound->channel_assigned) color = v4_new(1.0f, 1.0f, 0.0f, 1.0f);
		if(!sound->active) color = v4_new(1.0f, 0.0f, 0.0f, 1.0f);
		render_list_draw_box(list, v4_new(64.0f, -164.0f - 16.0f * sound_index, 12.0f, 12.0f), v2_new(0.0f, 1.0f), 2.0f, color);

		v2 position = v2_new(96.0f, -152.0f -16.0f * sound_index);
		v2 inner_anchor = v2_new(0.0f, 1.0f);
		v2 screen_anchor = v2_new(0.0f, 1.0f);
		color = v4_new(1.0f, 1.0f, 1.0f, 1.0f);
		char buffer[256];
		if(sound->active) {
			sprintf(buffer, "channel/%u priority/%u amplitude/%.4f", sound->channel, sound->priority, sound->settings.amplitude / 32000.0f);
			ui_draw_text_line(list, fonts, ASSET_FONT_MONO_SMALL, buffer,
				position, inner_anchor, screen_anchor, color, ui_stack);
		}
	}
}
