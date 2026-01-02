void platform_render_init(Renderer* renderer, RenderInitData* data, Arena* init_arena) {
	renderer->graphics_api = GRAPHICS_API_OPENGL;
	opengl_init(renderer, data, init_arena);
}

void platform_render_update(Renderer* renderer) {
	opengl_update(renderer, platform);
}

void platform_render_resize_viewport(Renderer* renderer, i32 window_width, i32 window_height) {
	opengl_resize_viewport(renderer, window_width, window_height);
}


