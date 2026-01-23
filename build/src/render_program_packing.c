typedef struct {
	char vert_filename[256];
	char frag_filename[256];
} RenderProgramInfo;

void calculate_render_program_assets(AssetInfoList* list, i32 args_len, ManifestArgument* args, Arena* arena) {
	RenderProgramInfo* rp_info = (RenderProgramInfo*)arena_alloc(arena, sizeof(RenderProgramInfo));
	assert(args_len == 2);
	strcpy(rp_info->vert_filename, args[0].text);
	strcpy(rp_info->frag_filename, args[1].text);

	FILE* vert_file = fopen(rp_info->vert_filename, "r");
	assert(vert_file != NULL);
	u64 vert_len = 1;
	while((fgetc(vert_file)) != EOF) {
		vert_len++;
	}
	fseek(vert_file, 0, SEEK_SET);

	FILE* frag_file = fopen(rp_info->frag_filename, "r");
	assert(frag_file != NULL);
	u64 frag_len = 1;
	while((fgetc(frag_file)) != EOF) {
		frag_len++;
	}
	fseek(frag_file, 0, SEEK_SET);

	// NOW: pusyh the render program

	//return sizeof(RenderProgramAsset) + sizeof(char) * vert_len + sizeof(char) * frag_len;
}

void pack_render_program_asset(void* p_info, void* p_asset) {
	RenderProgramInfo* info = (RenderProgramInfo*)p_info;
	RenderProgramAsset* asset = (RenderProgramAsset*)p_asset;
	
	FILE* vert_file = fopen(info->vert_filename, "r");
	assert(vert_file != NULL);
	asset->vertex_shader_src_len = 1;
	while((fgetc(vert_file)) != EOF) {
		asset->vertex_shader_src_len++;
	}
	fseek(vert_file, 0, SEEK_SET);

	FILE* frag_file = fopen(info->frag_filename, "r");
	assert(frag_file != NULL);
	asset->fragment_shader_src_len = 1;
	while((fgetc(frag_file)) != EOF) {
		asset->fragment_shader_src_len++;
	}
	fseek(frag_file, 0, SEEK_SET);

	int i = 0;
	while((asset->buffer[i] = fgetc(vert_file)) != EOF) {
		i++;
	}
	asset->buffer[i] = '\0';
	fclose(vert_file);

	i = 0;
	while((asset->buffer[asset->vertex_shader_src_len + i] = fgetc(frag_file)) != EOF) {
		i++;
	}
	asset->buffer[asset->vertex_shader_src_len + i] = '\0';
	//while((*buffer[asset->vertex_shader_src_len + i] = 'c') != EOF) {}
	fclose(frag_file);
}
