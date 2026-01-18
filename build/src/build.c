#define CSM_BASE_IMPLEMENTATION
#include "core/core.h"

#include "input_builder.c"
#include "bitmap_processor.c"

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
	print_header("Creating input configuration file...");
	system_call_ignore_result("mkdir config");
	build_default_input_config_file("../src/input.c", "InputButtonType", "data/config/def_input.conf", false);

	print_header("Creating bin and asset directories...");
	system_call_ignore_result("mkdir ../bin");
	system_call_ignore_result("mkdir ../bin/data");
	system_call_ignore_result("mkdir ../bin/data/meshes");
	system_call_ignore_result("mkdir ../bin/data/textures");
	system_call_ignore_result("mkdir ../bin/data/config");
	system_call_ignore_result("mkdir ../bin/data/shaders");

	print_header("Copying shaders from src...");
	system_call("cp ../src/shaders ../bin/data/ -r");

	print_header("Processing assets from assets/manifest...");
	FILE* manifest = fopen("data/assets.manifest", "r");
	if(manifest == NULL) {
		printf("Asset manifest could not be opened. Does it exist?\n");
		return 1;
	}

	char* mesh_id_str = "mesh";
	char* texture_id_str = "texture";
	char* config_id_str = "config";
	int mesh_id_str_len = strlen(mesh_id_str);
	i32 texture_id_str_len = strlen(texture_id_str);
	i32 config_id_str_len = strlen(config_id_str);

	char line[8196];
	while(fgets(line, sizeof(line), manifest)) {
		AssetManifestEntry entry;
		i32 fields_len = 3;
		char* fields[fields_len];
		fields[0] = entry.manifest_prefix;
		fields[1] = entry.src_file;
		fields[2] = entry.dst_file;
		i32 line_i = 0;
		for(i32 current_field = 0; current_field < fields_len; current_field++) {
			char* field = fields[current_field];
			i32 field_i = 0;
			while(line[line_i] != ' ' && line[line_i] != '\0' && line[line_i] != '\n') {
				field[field_i] = line[line_i];
				field_i++;
				line_i++;
			}
			if(current_field != fields_len - 1) {
				if(line[line_i] == '\0' || line[line_i] == '\n') {
					printf("Unexpected end to entry in asset manifest.\n");
					return 1;
				}
			}
			field[field_i] = '\0';
			line_i++;
		}
		if(strncmp(line, mesh_id_str, mesh_id_str_len) == 0) { strcpy(entry.asset_prefix, "meshes"); }
		else if(strncmp(line, texture_id_str, texture_id_str_len) == 0) { strcpy(entry.asset_prefix, "textures"); }
		else if(strncmp(line, config_id_str, config_id_str_len) == 0) { strcpy(entry.asset_prefix, "config"); }
		else { printf("Entry type not recognized in asset manifest.\n"); }
		sprintf(entry.src_relative, "data/%s/%s", entry.asset_prefix, entry.src_file);
		sprintf(entry.dst_relative, "data/%s/%s", entry.asset_prefix, entry.dst_file);

		FILE* test = fopen(entry.src_relative, "r");
		if(test == NULL) {
			printf("Manifest entry src file '%s' could not be opened. Does it exist?\n", entry.src_relative);
			return 1;
		}
		fclose(test);

		if(strncmp(line, mesh_id_str, mesh_id_str_len) == 0) {
			copy_manifest_asset_directly_to_bin(&entry);
		} else if(strncmp(line, texture_id_str, texture_id_str_len) == 0) {
			char dst_texture[4096];
			sprintf(dst_texture, "../bin/%s", entry.dst_relative);
			bitmap_to_texture(entry.src_relative, dst_texture);
		} else if(strncmp(line, config_id_str, config_id_str_len) == 0) {
			copy_manifest_asset_directly_to_bin(&entry);
		}
	}
	fclose(manifest);

	print_header("Compiling program...");
	system_call("gcc -g ../src/xlib.c ../extern/GL/gl3w.c -o ../bin/shiptastic -I ../extern/ -I ../src/ -lX11 -lX11-xcb -lGL -lm -lxcb -lXfixes");
	if(system_call_result == 0) {
		print_header("\033[32mProgram compilation succeeded!");
	}
	return system_call_result;
}
