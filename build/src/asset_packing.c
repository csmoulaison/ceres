// Some kind of file like asset_packing_config is requried to make
// asset_packing.c compile. Information about what is required can be found in
// asset_packing_config.c.
#include "asset_packing_config.c"

typedef struct {
	AssetType type;
	u64 size_in_buffer;
	char handle[64];
	void* data;
} AssetInfo;

typedef struct {
	AssetInfo infos_by_type[NUM_ASSET_TYPES][MAX_ASSETS];
	u32 counts_by_type[NUM_ASSET_TYPES];
} AssetInfoList;

typedef struct {
	char text[64];
} ManifestArgument;

typedef struct {
	void (*request_asset_list)(AssetInfoList* list, i32 args_len, ManifestArgument* args, Arena* arena);
	void (*pack_asset)(void* info, void* asset);
} AssetCallbacks;

AssetInfo* push_asset_info(AssetInfoList* list, AssetType type, char* manifest_name, u64 size_in_buffer, void* data) {
	AssetInfo* info = &list->infos_by_type[type][list->counts_by_type[type]];
	(list->counts_by_type[type])++;
	info->type = type;
	info->data = data;
	info->size_in_buffer = size_in_buffer;
	return info;
}

#include "asset_packing_callbacks.c"

void pack_assets() {
	// Process asset manifest into a list of assets along with their sizes in the
	// final asset pack data buffer. The list of assets in the manifest isn't 1:1
	// with the final list in the pack. For instance, manifest fonts create both a
	// "font" asset and a texture asset.
	Arena arena;
	arena_init(&arena, MEGABYTE * 32, NULL, "AssetInfo");
	AssetInfoList infos_list;
	for(i32 i = 0; i < NUM_ASSET_TYPES; i++) {
		infos_list.counts_by_type[i] = 0;
	}

	FILE* manifest = fopen(MANIFEST_FILENAME, "r");
	assert(manifest != NULL);
	u32 info_i = 0;
	u32 line_i = 0;
	char line[2048];
	while(fgets(line, sizeof(line), manifest)) {
		i32 char_i = 0;

		char manifest_key[64];
		consume_word(manifest_key, line, &char_i);

		char manifest_name[64];
		consume_word(manifest_name, line, &char_i);

		i32 args_len = 0;
		ManifestArgument args[32];
		while(consume_word(args[args_len].text, line, &char_i)) {
			args_len++;
		}

		AssetType type = -1;
		for(i32 i = 0; i < NUM_ASSET_TYPES; i++) {
			if(strcmp(manifest_key, string_to_asset_type[i]) == 0) {
				type = i;
				asset_callbacks_by_type[type].request_asset_list(&infos_list, args_len, args, &arena);
			}
		}
		if(type == -1) {
			printf("Error: manifest key '%s' not a recognized asset type.\n");
			panic();
		}

		line_i++;
	}

	u64 total_buffer_size = 0;
	for(i32 i = 0; i < NUM_ASSET_TYPES; i++) {
		for(i32 j = 0; j < infos_list.counts_by_type[i]; j++) {
			total_buffer_size += infos_list.infos_by_type[i][j].size_in_buffer;
		}
	}

	// Calculate the size of the asset pack buffer and store asset buffer offsets
	AssetPack* pack = (AssetPack*)arena_alloc(&arena, sizeof(AssetPack) + total_buffer_size);
	u64 buffer_pos = 0;

	u8* asset_counts[NUM_ASSET_TYPES];
	asset_counts[ASSET_TYPE_MESH] = &pack->meshes_len;
	asset_counts[ASSET_TYPE_TEXTURE] = &pack->textures_len;
	asset_counts[ASSET_TYPE_RENDER_PROGRAM] = &pack->render_programs_len;
	asset_counts[ASSET_TYPE_FONT] = &pack->fonts_len;

	u64* asset_offset_lists[NUM_ASSET_TYPES];
	asset_offset_lists[ASSET_TYPE_MESH] = pack->mesh_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_TEXTURE] = pack->texture_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_RENDER_PROGRAM] = pack->render_program_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_FONT] = pack->font_buffer_offsets;

	for(i32 i = 0; i < NUM_ASSET_TYPES; i++) {
		for(i32 j = 0; j < infos_list.counts_by_type[i]; j++) {
			AssetInfo* info = &infos_list.infos_by_type[i][j];
			printf("Packing %s %u...\n", string_to_asset_type[i], j);
			asset_callbacks_by_type[i].pack_asset(info->data, &pack->buffer[buffer_pos]);
			buffer_pos += info->size_in_buffer;
		}
	}

	FILE* out_file = fopen("../bin/data/assets.pack", "w");
	assert(out_file != NULL);

	fwrite(pack, sizeof(AssetPack) + total_buffer_size, 1, out_file);
	fclose(out_file);

	// TODO: Print out the assets!
	//char* asset_type_to_handle_prefix[NUM_ASSET_TYPES];
	//asset_type_to_handle_prefix[ASSET_TYPE_MESH] = "MESH";
	//asset_type_to_handle_prefix[ASSET_TYPE_TEXTURE] = "TEXTURE";
	//asset_type_to_handle_prefix[ASSET_TYPE_RENDER_PROGRAM] = "RENDER_PROGRAM";
	//asset_type_to_handle_prefix[ASSET_TYPE_FONT] = "FONT";
	//sprintf(info->handle, "ASSET_%s_%s", asset_type_to_handle_prefix[type], manifest_name);

	/* FOR DEBUG PURPOSES ONLY:
	for(i32 i = 0; i < pack->meshes_len; i++) {
		printf("mesh offset: %u\n", pack->mesh_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->textures_len; i++) {
		printf("texture offset: %u\n", pack->texture_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->render_programs_len; i++) {
		printf("render_program offset: %u\n", pack->render_program_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->fonts_len; i++) {
		printf("font offset: %u\n", pack->font_buffer_offsets[i]);
	}

	printf("meshes len: %u\n", pack->meshes_len);
	printf("textures len: %u\n", pack->textures_len);
	printf("render_programs len: %u\n", pack->render_programs_len);
	printf("fonts len: %u\n", pack->fonts_len);
	*/
}
