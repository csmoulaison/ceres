#include "stdlib.h"
#include "stdio.h"

int scall_result;
#define scall(call) { scall_result = system(call); if(scall_result != 0) { printf("bootstrapper error: syscall failed on line %i\n", __LINE__); exit(1); } }
#define scall_ignore(call) { system(call); }

void copy_directory(char* path) {
	char cmd[4096];
	sprintf(cmd, "cp -r %s ../bin/", path);
	scall(cmd);
}

int main(int argc, char** argv) {
	scall_ignore("mkdir ../bin");
	copy_directory("../src/shaders");
	copy_directory("meshes");

	// build textures
	scall_ignore("mkdir ../bin/textures");
	scall("bmpp/bmpp textures/ship.bmp ../bin/textures/ship.tex");
	scall("bmpp/bmpp textures/metal.bmp ../bin/textures/metal.tex");
	
	scall("gcc -g ../src/xlib_main.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../extern/ -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	return scall_result;
}
