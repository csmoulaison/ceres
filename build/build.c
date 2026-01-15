#include "stdlib.h"
#include "stdio.h"

#include "../src/environment.h"

int system_call_result;
char cmd[4096];

#define system_call(call) { system_call_result = system(call); if(system_call_result != 0) { printf("bootstrapper error: sysystem_call failed on line %i\n", __LINE__); exit(1); } }
#define system_call_save_result(call) { system_call_result = system(call); }
#define system_call_ignore_result(call) { system(call); }

void copy_directory_to_bin(char* path) {
	sprintf(cmd, "cp -r %s ../bin/", path);
	system_call(cmd);
}

void print_header(const char* s) {
	printf("\n\033[1m\033[33m%s\033[0m\n", s);
}

int main(int argc, char** argv) {
	print_header("Creating bin directory...");
	system_call_ignore_result("mkdir ../bin");

	print_header("Creating input configuration file...");
	system_call_ignore_result("mkdir config");
	sprintf(cmd, "test -f %s", CONFIG_DEFAULT_INPUT_FILENAME);
	system_call_save_result(cmd);
	if(system_call_result != 0) {
		sprintf(cmd, "tools/input_serializer/input_serializer %s", CONFIG_DEFAULT_INPUT_FILENAME);
		system_call(cmd);
	}

	print_header("Copying asset directories to bin...");
	copy_directory_to_bin("config");
	copy_directory_to_bin("../src/shaders");
	copy_directory_to_bin("meshes");

	print_header("Baking texture data...");
	system_call_ignore_result("mkdir ../bin/textures");
	system_call("tools/bmpp/bmpp textures/ship.bmp ../bin/textures/ship.tex");
	system_call("tools/bmpp/bmpp textures/metal.bmp ../bin/textures/metal.tex");
	
	print_header("Compiling program...");
	system_call("gcc -g ../src/xlib.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../extern/ -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	if(system_call_result == 0) {
		print_header("\033[32mProgram compilation succeeded!");
	}
	return system_call_result;
}
