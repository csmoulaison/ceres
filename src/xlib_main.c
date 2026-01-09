/*

RENDERER
Everything specific to a game, besides core capabilities which abstract those of
the underlying APIs, should be put into a data format. There are certain places
where this interacts with the game layer, such as literally everywhere. 

Arbitrating how this works is going to be fairly key to a well functioning 
system. The quintessential example is how to structure the rendering of entities
which have arbitrary meshes/textures/programs. 

One easy way out of all of this trouble would be to have the host program be 
able to register to specific callback points where things like buffering data 
can be handled. This is pretty fucked, though, as it would necessitate, for 
instance, that each individual entity type be given a different callback to 
bind/buffer all of its particular resources in advance of the draw call.

The key point here is that binding programs, meshes, and textures is a task
which is common to all, or the great majority, of renderable entities, and as
such, the concept of a renderable entity ought to make its way into the data 
formats for the renderer.

Let's imagine now that we are designing a render graph for this spaceship game.
We will need to define:

	- A program for rendering textured meshes, along with lighting information
	- Ubos and other such buffers
	- A set of meshes/textures, and the association between them and the program
		(an entity type)

Really, the primal subject here is entities, which may or may not be game 
objects in the way we think of them, with a transform and such. Really, what we
are dealing with in the general case is the following:

INPUT  
Input system should just basically abstract the event loop. Pass the events
through if they've been registered and allow the game layer to process that
for gameplay. Game logic code should never just be asking the platform about
input mid loop.

*/

#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "config.c"
#include "platform/platform.h"
#include "renderer/renderer.c"
#include "renderer/opengl/opengl.c"
#include "game.c"

#include <GL/glx.h>
typedef GLXContext(*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

typedef struct {
	Display* display;
	Window window;
} XlibContext;

typedef struct {
	Arena global;
	Arena render;
	Arena render_init;
} MemoryArenas;

void xlib_init(Platform* platform, Renderer* renderer, MemoryArenas* arenas) {
}

i32 main(i32 argc, char** argv) {
	MemoryArenas arenas;
	arena_init(&arenas.global, GLOBAL_ARENA_SIZE, NULL, "Global");
	arena_init(&arenas.render, RENDER_ARENA_SIZE, &arenas.global, "Render");
	arena_init(&arenas.render_init, RENDER_INIT_ARENA_SIZE, NULL, "RenderInit");

	Platform* platform = (Platform*)arena_alloc(&arenas.global, sizeof(Platform));
	XlibContext* xlib = (XlibContext*)arena_alloc(&arenas.global, sizeof(XlibContext));
	platform->backend = xlib;

	xlib->display = XOpenDisplay("");
	if(!xlib->display) { panic(); }

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
	platform->input_buttons_len = 1;
	for(u32 i = 0; i < PLATFORM_INPUT_KEYCODE_TO_BUTTON_LOOKUP_LEN; i++) {
		platform->input_keycode_to_button_lookup[i] = PLATFORM_INPUT_KEYCODE_UNREGISTERED;
	}

	// Initialize open GL before getting window attributes.
	RenderInitData* init_data = render_load_init_data(&arenas.render_init);
	Renderer* renderer = gl_init(init_data, &arenas.render, &arenas.render_init);

	arena_destroy(&arenas.render_init);

	XWindowAttributes window_attributes;
	XGetWindowAttributes(xlib->display, xlib->window, &window_attributes);
	platform->window_width = window_attributes.width;
	platform->window_height = window_attributes.height;

	Arena game_arena;
	arena_init(&game_arena, GAME_ARENA_SIZE, &arenas.global, "Game");

	Game* game = game_init(&game_arena);

	platform->frames_since_init = 0;
	bool quit = false;

	
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	while(!quit) {
		while(XPending(xlib->display)) {
			XEvent event;
			XNextEvent(xlib->display, &event);

			switch(event.type) {
				case Expose:
					break;
				case ConfigureNotify: {
					XWindowAttributes attribs;
					XGetWindowAttributes(xlib->display, xlib->window, &attribs);
					platform->window_width = attribs.width;
					platform->window_height = attribs.height;
					platform->viewport_update_requested = true;
				} break;
				case KeyPress: {
					if(XLookupKeysym(&(event.xkey), 0) == XK_Escape) {
						quit = true;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Up) {
						up = true;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Down) {
						down = true;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Left) {
						left = true;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Right) {
						right = true;
					}
				} break;
				case KeyRelease: {
					if(XLookupKeysym(&(event.xkey), 0) == XK_Up) {
						up = false;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Down) {
						down = false;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Left) {
						left = false;
					} else if(XLookupKeysym(&(event.xkey), 0) == XK_Right) {
						right = false;
					}
				} break;
				default: break;
			}
		}

		game_update(game, up, down, left, right, 0.02f);
		render_prepare_frame_data(renderer, platform, 
			game->ship_position, 
			game->ship_direction, 
			clamp(game->ship_rotation_velocity, -8.0f, 8.0f), 
			game->camera_offset);
		gl_update(renderer, platform);
		arena_clear_to_zero(&renderer->frame_arena);
		glXSwapBuffers(xlib->display, xlib->window);
		platform->frames_since_init++;
	}

	return 0;
}
