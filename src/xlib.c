// NOW: < LIST: I think our order is as follows:
// - Fiedler frames
// - Designed competitive level
// - Post death respawn animation (camera moves to spawn location, ship flies
//   in from tunnnel?)
// - Menu, so we can have two buttons, one for local and one for ...
// - ... Networking
// 
// - Controller input in here somewhere
// - Menu and session flow
// 
// - Whenever we start working on assets more in earnest, we may think about
//   whether a more iterable asset pipeline is in order with regards to .blend
//   files and similar things.

// This is to get ctim in the stat struct while still compiling with c99
#define _POSIX_C_SOURCE 200809L

#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include <dlfcn.h>
#include <sys/stat.h>

#include "config.c"
#include "platform/platform.h"
#include "alsa.c"

#include "generated/asset_handles.h"
#include "asset_format.h"
#include "asset_loader.c"
#include "renderer/renderer.c"
#include "renderer/opengl/opengl.c"
#include "game.h"

#include <GL/glx.h>
typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

typedef struct {
	Display* display;
	Window window;

	void* game_lib_handle;
	u64 game_lib_last_modified;
	GameInitFunction* game_init;
	GameUpdateFunction* game_update;
	GameGenerateSoundSamplesFunction* game_generate_sound_samples;
} XlibMemory;

typedef struct {
	RenderInitMemory render;
} InitMemory;

typedef struct {
	Platform platform;
	XlibMemory xlib;
	GameMemory game;
	RenderMemory renderer;
	AlsaMemory alsa;
	// NOTE: For now, AssetMemory must be kept at the end of the struct, as we
	// allocate the asset buffer memory size along with the GlobalMemory size.
	AssetMemory assets;
} GlobalMemory;

void push_game_event(Platform* platform, GameEventType type, void* data, u64 data_bytes, StackAllocator* stack) {
	GameEvent* event = (GameEvent*)stack_alloc(stack, sizeof(GameEvent));
	event->next = NULL;
	event->type = type;
	event->data = stack_alloc(stack, sizeof(data_bytes));
	memcpy(event->data, data, data_bytes);

	if(platform->head_event == NULL) {
		platform->head_event = event;
	} else {
		platform->current_event->next = event;
	}
	platform->current_event = event;
}

void xlib_reload_game_code(XlibMemory* xlib) {
	struct stat game_lib_stat;
	stat("shiptastic.so", &game_lib_stat);

	u64 actual_last_modified = game_lib_stat.st_mtim.tv_sec;
	if(actual_last_modified > xlib->game_lib_last_modified) {
		if(xlib->game_lib_last_modified != 0) {
			dlclose(xlib->game_lib_handle);
		}
		xlib->game_lib_last_modified = actual_last_modified;

		char copy_fname[256];
		time_t t;
		time(&t);
		sprintf(copy_fname, "./shiptastic_%li.so", t);
		//struct tm* tm;
		//tm = localtime(&t);
		//strftime(cpy_fname, sizeof(cpy_fname), "", tm);

		char cmd[2048];
		sprintf(cmd, "cp shiptastic.so %s", copy_fname);
		system(cmd);

		xlib->game_lib_handle = dlopen(copy_fname, RTLD_NOW);
		char* err;
		if((err = dlerror()) != NULL) {
			fprintf(stderr, "ERROR! %s\n", err);
		}
		assert(xlib->game_lib_handle != NULL);

		xlib->game_init = dlsym(xlib->game_lib_handle, "game_init");
		assert(dlerror() == NULL);
		xlib->game_update = dlsym(xlib->game_lib_handle, "game_update");
		assert(dlerror() == NULL);
		xlib->game_generate_sound_samples = dlsym(xlib->game_lib_handle, "game_generate_sound_samples");
		assert(dlerror() == NULL);
	}
}

i32 main(i32 argc, char** argv) {
	InitMemory* init_memory = (InitMemory*)calloc(1, sizeof(InitMemory));
	RenderInitMemory* render_init_memory = &init_memory->render;

	// NOW: after we are running again, put all this stuff into init functions for
	// each memory domain.
	GlobalMemory* global_memory = (GlobalMemory*)calloc(1, sizeof(GlobalMemory) + ASSET_BUFFER_MEMSIZE);
	Platform* platform = &global_memory->platform;
	XlibMemory* xlib = &global_memory->xlib;
	GameMemory* game = &global_memory->game;
	RenderMemory* renderer = &global_memory->renderer;
	AlsaMemory* alsa = &global_memory->alsa;
	AssetMemory* assets = &global_memory->assets;

	platform->backend = xlib;
	renderer->backend = calloc(1, sizeof(GlMemory));

	xlib->display = XOpenDisplay("");
	if(!xlib->display) { panic(); }

	srand(time(NULL));

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
	GLXFBConfig* framebuffer_configs = glXChooseFBConfig(xlib->display, DefaultScreen(xlib->display), desired_framebuffer_attributes, &framebuffer_configs_len);
	if(!framebuffer_configs) { panic(); }

	i32 best_framebuffer_config = -1;
	i32 best_sample_count = -1;
	for(i32 fc = 0; fc < framebuffer_configs_len; fc++) {
		XVisualInfo* tmp_visual_info = glXGetVisualFromFBConfig(xlib->display, framebuffer_configs[fc]);
		if(tmp_visual_info != NULL) {
			i32 sample_buffers;
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[fc], GLX_SAMPLE_BUFFERS, &sample_buffers);

			i32 samples;
			glXGetFBConfigAttrib(xlib->display, framebuffer_configs[fc], GLX_SAMPLES, &samples);

			if(best_framebuffer_config == -1 || (sample_buffers && samples > best_sample_count)) {
				best_framebuffer_config = fc;
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
	Window root_window = RootWindow(xlib->display, glx_visual_info->screen);
	XSetWindowAttributes set_window_attributes = {};
	set_window_attributes.colormap = XCreateColormap(xlib->display, root_window, glx_visual_info->visual, AllocNone);
	set_window_attributes.background_pixmap = None;
	set_window_attributes.border_pixel = 0;
	set_window_attributes.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

	// Create our actual Xlib window.
	platform->window_width = 1;
	platform->window_height = 1;
	xlib->window = XCreateWindow(xlib->display, root_window, 0, 0, platform->window_width, platform->window_height, 0, glx_visual_info->depth, InputOutput, glx_visual_info->visual, CWBorderPixel | CWColormap | CWEventMask, &set_window_attributes);
	if(xlib->window == 0) { panic(); }

	XFree(glx_visual_info);
	XStoreName(xlib->display, xlib->window, PROGRAM_NAME);
	XMapWindow(xlib->display, xlib->window);

	// Validate existence of required GL extensions
	glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
	char* gl_extensions = (char*)glXQueryExtensionsString(xlib->display, DefaultScreen(xlib->display));
	glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
	const char* extension = "GLX_ARB_create_context";
	char* start;
	char* where;
	char* terminator;

	// Extension names shouldn't have spaces
	where = strchr((char*)extension, ' ');
	if (where || *extension == '\0') { panic(); }

	bool found_extension = true;
	for (start = gl_extensions;;) {
		where = strstr(start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ') {
			if (*terminator == ' ' || *terminator == '\0')
				found_extension = true;
			start = terminator;
		}
	}
	if(found_extension == false) { panic(); }

	// Create GLX context and window
	int32_t glx_attributes[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 6,
		None
	};

	GLXContext glx = glXCreateContextAttribsARB(xlib->display, glx_framebuffer_config, 0, 1, glx_attributes);
	if(glXIsDirect(xlib->display, glx) == false) { panic(); }

	// Bind GLX to window
	glXMakeCurrent(xlib->display, xlib->window, glx);
	platform->viewport_update_requested = true;

	// Load asset pack file
	asset_pack_load(assets);

	// Initialize open GL before getting window attributes.
	render_init(renderer, render_init_memory, assets);
	gl_init(renderer, render_init_memory);

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	platform->window_width = window_attributes.width;
	platform->window_height = window_attributes.height;

	// Initialize ALSA for sound
	alsa_init(alsa);

	// Initialize game, loading the dynamic libraary and initializing it with some
	// memory we allocate here.
	xlib->game_lib_last_modified = 0;
	xlib_reload_game_code(xlib);

	xlib->game_init(game, assets);
	GameOutput game_output = {};

	free(init_memory);

	platform->frames_since_init = 0;
	while(!game_output.close_requested) {
		xlib_reload_game_code(xlib);
		
		platform->head_event = NULL;
		platform->current_event = NULL;
		StackAllocator event_stack = stack_init(platform->event_memory, GAME_EVENTS_MEMSIZE, "GameEvents");

		while(XPending(xlib->display)) {
			XEvent event;
			XNextEvent(xlib->display, &event);

			switch(event.type) {
				case Expose:
					break;
				case ConfigureNotify: {
					XWindowAttributes attributes;
					XGetWindowAttributes(xlib->display, xlib->window, &attributes);
					platform->window_width = attributes.width;
					platform->window_height = attributes.height;
					platform->viewport_update_requested = true;
				} break;
				case KeyPress: {
					u64 keysym = XLookupKeysym(&(event.xkey), 0);
					push_game_event(platform, GAME_EVENT_KEY_DOWN, &keysym, sizeof(u64), &event_stack);
				} break;
				case KeyRelease: {
					// X11 natively repeats key events when the key is held down. We could turn
					// that off, but it turns it off globally for the user's X11 session, which
					// is unacceptable of course. Here we are ignoring them manually.
					bool is_repeat_key = false;
		            if (XPending(xlib->display)) {
						XEvent next_event;
		                XPeekEvent(xlib->display, &next_event);
		                if (next_event.type == KeyPress && next_event.xkey.time == event.xkey.time 
		                && next_event.xkey.keycode == event.xkey.keycode) {
							XNextEvent(xlib->display, &next_event);
							is_repeat_key = true;
		                }
		            }

					if(!is_repeat_key) {
						u64 keysym = XLookupKeysym(&(event.xkey), 0);
						push_game_event(platform, GAME_EVENT_KEY_UP, &keysym, sizeof(u64), &event_stack);
					}
				} break;
				default: break;
			}
		}
		platform->current_event = platform->head_event;

		xlib->game_update(game, platform->current_event, &game_output, 0.016f);

		i32 sound_samples_count = alsa_write_samples_count(alsa);
		if(sound_samples_count > 0) {
			// TODO: Fix glitchy audio on quit
			xlib->game_generate_sound_samples(game, alsa->write_buffer, sound_samples_count);
			//alsa_write_samples(alsa, alsa->write_buffer, sound_samples_count);
		}

		render_prepare_frame_data(renderer, platform, &game_output.render_list);
		gl_update(renderer, platform);

		glXSwapBuffers(xlib->display, xlib->window);
		platform->frames_since_init++;
	}

	//free(global_memory);
	//free(renderer->backend);
	return 0;
}
