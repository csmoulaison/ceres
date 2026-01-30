#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

// TODO #27: What does the Xlib layer do? (as of January 1, 2026)
// - Allocates memory blocks
// - Opens a window, along with some glx (wgl in the win32 case) initialization
// - Loads game assets
// - Initializes renderer and OpenGL
// - Initializes the game
// - Then, every frame:
//   - Polls x11 for key and window resize events, ignoring repeated keypresses
//     which are automatically generated when key is held down in x11
//   - Updates the game
//   - Updates the renderer
//   - Swap buffer
//   - Clears transient arena memory
