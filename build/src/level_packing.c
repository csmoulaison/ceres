typedef struct {
	char filename[256];
} LevelInfo;

void calculate_level_assets(AssetInfoList* list, char* handle, i32 args_len, ManifestArgument* args, StackAllocator* stack) {
	LevelInfo* info = (LevelInfo*)stack_alloc(stack, sizeof(LevelInfo));
	strcpy(info->filename, args[0].text);

	FILE* file = fopen(info->filename, "r");
	assert(file != NULL);
	fseek(file, 0L, SEEK_END);
	u64 size = ftell(file);
	fclose(file);

	push_asset_info(list, ASSET_TYPE_LEVEL, handle, size, info);
}

void pack_level_asset(void* p_info, void* p_asset) {
	LevelInfo* info = (LevelInfo*)p_info;
	LevelAsset* asset = (LevelAsset*)p_asset;

	FILE* file = fopen(info->filename, "r");
	assert(file != NULL);
	fread(&asset->side_length, sizeof(asset->side_length), 1, file);
	fread(asset->buffer, sizeof(u8) * asset->side_length * asset->side_length, 1, file);
	fclose(file);
}
