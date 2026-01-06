#include "stdlib.h"
#include "stdio.h"

void copy_directory(char* path) {
	char cmd[4096];
	sprintf(cmd, "cp -r %s ../bin/", path);
	system(cmd);
}

int main(int argc, char** argv) {
	system("mkdir ../bin");
	copy_directory("../src/shaders");
	copy_directory("meshes");
	int res = system("gcc -g ../src/xlib_main.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../extern/ -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	if(res != 0) {
		return 1;
	}
	return res;
}
