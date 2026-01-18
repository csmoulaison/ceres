#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include "stb_image.h"

/*
This program loads .bmp files and outputs them in the following format:

	u32: w, u32: h, u32* pixels

The input is assumed to consist of 8 bit RGBA components, and the output is the
same.
*/

void bitmap_to_texture(char* in_filename, char* out_filename) {
}
