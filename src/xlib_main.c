/*
  
Here we are, back in C.  

We want to make a few changes relative to the previous project:
- The platform layer becomes primal once more. It controls the top-level
  control flow. Anything which is shared between the platforms is called by both.
	- The platform layer knows which rendering API's it has available, allowing it
      to fall back if one fails or even expose this information/control to the game
      layer so the player can select these things.
    - This time, try to actually put loads of code in the actual renderer layer.
    - Create a single platform struct that has all the things we need. If there
      are things that might have swappable implementations (if even?) then hide
      them behind a pointer.

- Set up everything we can think of to be data driven. Finally create yourself
  an engine here. This means data driven rendering layer stuff, data driven input
  config, DLL for game logic layer.
  - If we don't have serialization/deserialization for these yet, just hardcode
    data into a struct that will eventually be populated by a data file.

- Write subsystems in small chunks and do them well. First do a quick run, then
  make them clean and good.
  - Platform layer:
	- Windowing
	- Renderer
	- Audio
	- Network
	- Time
	- Input

- Input system should just basically abstract the event loop. Pass the events
  through if they've been registered and allow the game layer to process that
  for gameplay. Game logic code should never just be asking the platform about
  input mid loop.

- Build system from executable file bootstrapped with nothing more than a call 
  to "gcc build.c -o build"

*/

#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"
#include "platform/platform.h"

// NOW: Put this in its own file somewhere.
#define PROGRAM_NAME "Shiptastic"

typedef struct XlibContext {
	Display* display;
	Window window;
};

i32 main(i32 argc, char** argv) {
	Arena global_arena;
	arena_init(&global_arena, MEGABYTE);

	Platform* platform = (Platform*)arena_alloc(arena, sizeof(Platform));
	XlibContext* xlib = (XlibContext*)arena_alloc(arena, sizeof(XlibContext));
	platform->backend = xlib;

	xlib->display = XOpenDisplay(0);
	if(xlib->display == nullptr) { panic(); }

	// GLX (OpenGL+Xlib) specific stuff. The control flow for this can't really be
	// abstracted now, only refactored to specifics if/when we get another API going.

	// Validate GLX version
	i32 glx_version_major;
	i32 glx_version_minor;
	if(glXQueryVersion(xlib->display, &glx_version_major, &glx_version_minor) == 0
	|| (glx_version_major == 1 && glx_version_minor < 3) || glx_version_major < 1) {
		panic();
	}

	// Define framebuffer attributes. If we want these to be different between
	// projects, make it data driven.
	i32 desired_framebuffer_attributes[] = {
		GLX_X_RENDERABLE, True,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_ALPHA_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_STENCIL_SIZE, 8,
		GLX_DOUBLEBUFFER, True,
		GLX_SAMPLE_BUFFERS, 1,
		GLX_SAMPLES, 4,
		None
	};

	// Find the best framebuffer configuration from those available. Our only
	// criteria at the moment is sample count.
	i32 framebuffer_configs_len;
	GLXFBConfig* framebuffer_configs = GlXChooseGBConfig(xlib->display, DefaultScreen(xlib->display), desired_framebuffer_attributes, &framebuffer_configs_len);
	if(!framebuffer_configs) { panic(); }

	i32 best_framebuffer_config = -1;
	i32 best_sample_count = -1;
	for(i32 i = 0; i < framebuffer_configs_len; i++) {
		XVisualInfo* tmp_visual_info = glXGetVisualFromFBConfig(xlib->display, framebuffer_configs[i]);
		if(tmp_visual_info != NULL) {
			i32 sample_buffers;
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[i], GLX_SAMPLE_BUFFERS, &sample_buffers);

			i32 samples;
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[i], GLX_SAMPLES, &samples);

			if(best_framebuffer_config == -1 || (sample_buffers && samples > best_sample_count)) {
				best_framebuffer_config = i;
				best_sample_count = samples;
			}
		}
		XFree(tmp_visual_info);
	}
	GLXFBConfig glx_framebuffer_config = framebuffer_configs[best_framebuffer_config];
	XVisualInfo* glx_visual_info = glXGetVisualFromFBConfig(xlib->display, glx_framebuffer_config);
	XFree(framebuffer_configs);

	// Set up root window. This is somewhat mixed up with GLX stuff still, though
	// it should survive the generic case with some things factored into variables.
	Window root_window = RootWindow(xlib->display, glx_visual_info);
	XSetWindowAttributes set_window_attributes = {};
	set_window_attributes.colormap = XCreateColormap(xlib->display, root_window, glx_visual_info->visual, AllocNone);
	set_window_attributes.background_pixmap = None;
	set_window_attributes.border_pixel = 0;
	set_window_attributes.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

	// Create our actual Xlib window.
	// NOW: Does this need to start at a non zero value?
	platform->window_width = 1;
	platform->window_height = 1;
	xlib->window = XCreateWindow(xlib->display, root_window, 0, 0, platform->window_width, platform->window_height, 0, glx_visual_info->depth, InputOutput, glx_visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &set_window_attributes);
	if(xlib->window == 0) { panic(); }

	XFree(glx_visual_info);
	XStoreName(xlib->display, xlib->window, PROGRAM_NAME);
	XMapWindow(xlib->display, xlib->window);

	/* NOW: Redo this from netpong
	glx_init_post_window(xlib, glx_framebuffer_config);

	context->viewport_update_requested = true;
	context->backend = xlib;

	context->input_buttons_len = 1;
	for(u32 i = 0; i < INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN; i++) {
		context->input_keycode_to_button_lookup[i] = INPUT_KEYCODE_UNREGISTERED;
	}
	*/

	return 0;
}
