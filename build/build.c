#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int system_call_result;
char cmd[4096];

#define system_call(call) { system_call_result = system(call); if(system_call_result != 0) { printf("bootstrapper error: sysystem_call failed on line %i\n", __LINE__); exit(1); } }
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
	printf("\n\033[1m\033[33m%s\033[0m\n", s);
}

void copy_manifest_asset_directly_to_bin(AssetManifestEntry* entry) {
	sprintf(cmd, "cp %s ../bin/%s", entry->src_relative, entry->dst_relative);
	system_call(cmd);
	printf("Copied %s to ../bin/%s.\n", entry->src_relative, entry->dst_relative);
}

int main(int argc, char** argv) {
	print_header("Creating input configuration file...");
	system_call_ignore_result("mkdir config");
	sprintf(cmd, "tools/input_builder/input_builder ../src/input.c InputButtonType config/def_input.conf");
	system_call(cmd);

	print_header("Creating bin and asset directories...");
	system_call_ignore_result("mkdir ../bin");
	system_call_ignore_result("mkdir ../bin/meshes");
	system_call_ignore_result("mkdir ../bin/textures");
	system_call_ignore_result("mkdir ../bin/config");

	print_header("Copying shaders from src...");
	system_call("cp ../src/shaders ../bin/shaders -r");

	print_header("Processing assets from assets/manifest...");
	FILE* manifest = fopen("assets.manifest", "r");
	if(manifest == NULL) {
		printf("Asset manifest could not be opened. Does it exist?\n");
		return 1;
	}

	char* mesh_id_str = "mesh";
	char* texture_id_str = "texture";
	char* config_id_str = "config";
	int mesh_id_str_len = strlen(mesh_id_str);
	int texture_id_str_len = strlen(texture_id_str);
	int config_id_str_len = strlen(config_id_str);

	char line[8196];
	while(fgets(line, sizeof(line), manifest)) {
		AssetManifestEntry entry;
		int fields_len = 3;
		char* fields[fields_len];
		fields[0] = entry.manifest_prefix;
		fields[1] = entry.src_file;
		fields[2] = entry.dst_file;
		int line_i = 0;
		for(int current_field = 0; current_field < fields_len; current_field++) {
			char* field = fields[current_field];
			int field_i = 0;
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
		sprintf(entry.src_relative, "%s/%s", entry.asset_prefix, entry.src_file);
		sprintf(entry.dst_relative, "%s/%s", entry.asset_prefix, entry.dst_file);

		FILE* test = fopen(entry.src_relative, "r");
		if(test == NULL) {
			printf("Manifest entry src file '%s' could not be opened. Does it exist?\n", entry.src_relative);
			return 1;
		}
		fclose(test);

		if(strncmp(line, mesh_id_str, mesh_id_str_len) == 0) {
			copy_manifest_asset_directly_to_bin(&entry);
		} else if(strncmp(line, texture_id_str, texture_id_str_len) == 0) {
			sprintf(cmd, "tools/bmpp/bmpp %s ../bin/%s", entry.src_relative, entry.dst_relative);
			printf("%s\n", cmd);
			system_call(cmd);
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
