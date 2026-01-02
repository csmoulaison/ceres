#include "stdlib.h"

int main(int argc, char** argv) {
	system("mkdir ../bin");
	system("gcc ../src/xlib_main.c -o ../bin/shiptastic");
	return 0;
}
