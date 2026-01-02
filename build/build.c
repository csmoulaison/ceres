#include "stdlib.h"
#include "stdio.h"

void copy_directory(char* path) {
	char cmd[4096];
	sprintf(cmd, "cp -r %s ../bin/", path);
	system(cmd);
}

int main(int argc, char** argv) {
	system("mkdir ../bin");
	system("gcc ../src/xlib_main.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	return 0;
}
