typedef struct {
	char vert_filename[256];
	char frag_filename[256];
	i32 vert_src_len;
	i32 frag_src_len;
} RenderProgramInfo;

void calculate_render_program_assets(AssetInfoList* list, char* handle, i32 args_len, ManifestArgument* args, StackAllocator* stack) {
	RenderProgramInfo* info = (RenderProgramInfo*)stack_alloc(stack, sizeof(RenderProgramInfo));
	assert(args_len == 2);
	strcpy(info->vert_filename, args[0].text);
	strcpy(info->frag_filename, args[1].text);

	FILE* vert_file = fopen(info->vert_filename, "r");
	assert(vert_file != NULL);
	info->vert_src_len = 0;
	while((fgetc(vert_file)) != EOF) {
		info->vert_src_len++;
	}
	fclose(vert_file);

	FILE* frag_file = fopen(info->frag_filename, "r");
	assert(frag_file != NULL);
	info->frag_src_len = 0;
	while((fgetc(frag_file)) != EOF) {
		info->frag_src_len++;
	}
	fclose(frag_file);

	u64 size = sizeof(RenderProgramAsset) + sizeof(char) * info->vert_src_len + sizeof(char) * info->frag_src_len;
	push_asset_info(list, ASSET_TYPE_RENDER_PROGRAM, handle, size, info);
}

void pack_render_program_asset(void* p_info, void* p_asset) {
	RenderProgramInfo* info = (RenderProgramInfo*)p_info;
	RenderProgramAsset* asset = (RenderProgramAsset*)p_asset;
	asset->vertex_shader_src_len = info->vert_src_len;
	asset->fragment_shader_src_len = info->frag_src_len;

	struct { char* filename; u64 len; } files[2] = { 
		{ .filename = info->vert_filename, .len = info->vert_src_len },
		{ .filename = info->frag_filename, .len = info->frag_src_len }
	};

	u64 buffer_pos = 0;
	for(i32 i = 0; i < 2; i++) {
		FILE* file = fopen(files[i].filename, "r");
		assert(file != NULL);

		fread(&asset->buffer[buffer_pos], files[i].len, 1, file);
		buffer_pos += files[i].len;
		fclose(file);
	}
}
