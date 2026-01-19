#define CSM_CORE_IMPLEMENTATION
#include "core/core.h"

#include "input_builder.c"
#include "bitmap_processor.c"
#include "asset_packing.c"

i32 system_call_result;
char cmd[4096];

#define system_call(call) { system_call_result = system(call); if(system_call_result != 0) { printf("build error: system_call failed on line %i\n", __LINE__); exit(1); } }
#define system_call_save_result(call) { system_call_result = system(call); }
#define system_call_ignore_result(call) { system(call); }

typedef struct {
	char manifest_prefix[32];
	char asset_prefix[32];
	char src_file[4096];
	char dst_file[4096];
	char src_relative[4096];
	char dst_relative[4096];
} AssetManifestEntry;

void print_header(const char* s) {
	printf("\033[1m\033[33m%s\033[0m\n", s);
}

void copy_manifest_asset_directly_to_bin(AssetManifestEntry* entry) {
	sprintf(cmd, "cp %s ../bin/%s", entry->src_relative, entry->dst_relative);
	system_call(cmd);
	printf("Copied %s to ../bin/%s.\n", entry->src_relative, entry->dst_relative);
}

i32 main(i32 argc, char** argv) {
	print_header("Creating bin and asset directories...");
	system_call_ignore_result("mkdir ../bin");
	system_call_ignore_result("mkdir ../bin/data");

	print_header("Creating input configuration file...");
	system_call_ignore_result("mkdir config");
	build_default_input_config_file("../src/input.c", "InputButtonType", "data/config/def_input.conf", false);
	system_call("cp data/config/def_input.conf ../bin/data/def_input.conf");

	print_header("Building asset pack from manifest...");
	pack_assets("data/assets.manifest", "../bin/data/assets.pack", "../src/generated/asset_handles.h");

	print_header("Compiling program...");
	system_call("gcc -g ../src/xlib.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../extern/ -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	if(system_call_result == 0) {
		print_header("\033[32mProgram compilation succeeded!");
	}
	return system_call_result;
}
