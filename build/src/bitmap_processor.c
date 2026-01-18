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
	u32 w;
	u32 h;
	u32 channels;
	stbi_uc* stb_pixels = stbi_load(in_filename, &w, &h, &channels, STBI_rgb_alpha);

	assert(stb_pixels != NULL && channels == 4);
	assert(w == 256 && h == 256);

	u64 pixels_len = w * h;
	u64 buf_size = sizeof(u32) * (2 + pixels_len);
	u8 buf[buf_size];
	u32* buf32 = (u32*)buf;
	buf32[0] = w;
	buf32[1] = h;
	memcpy(&buf32[2], stb_pixels, sizeof(u32) * pixels_len);

	FILE* f = fopen(out_filename, "w");
	assert(f != NULL);

	assert(fwrite(buf, buf_size, 1, f) == 1);
	fclose(f);
}
