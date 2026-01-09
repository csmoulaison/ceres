#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_BMP
#include "stb_image.h"

/*
This program loads .bmp files and outputs them in the following format:

	u32: w, u32: h, u32* pixels

The input is assumed to consist of 8 bit RGBA components, and the output is the
same.
*/

i32 main(i32 argc, char** argv) {
	if(argc != 3) {
		printf("usage: bmpp in.bmp out.bmp\n");
		return 1;
	}

	u32 w;
	u32 h;
	u32 channels;
	stbi_uc* stb_pixels = stbi_load(argv[1], &w, &h, &channels, STBI_rgb_alpha);
	assert(stb_pixels != NULL && channels == 4);

	assert(w == 256 && h == 256);

	u64 pixels_len = w * h;
	u64 buf_size = sizeof(u32) * (2 + pixels_len);
	u8 buf[buf_size];
	u32* buf32 = (u32*)buf;
	buf32[0] = w;
	buf32[1] = h;
	memcpy(&buf32[2], stb_pixels, sizeof(u32) * pixels_len);

	/*
	for(i32 i = 0; i < w * h; i++) {
		if(i % w == 0) {
			printf("\n");
		}
		if((u8)(buf32[2 + i]) >= 128) {
			printf("#");
		} else {
			printf(".");
		}
	}
	*/

	FILE* f = fopen(argv[2], "w");
	assert(f != NULL);

	assert(fwrite(buf, buf_size, 1, f) == 1);
	fclose(f);

	return 0;
}
