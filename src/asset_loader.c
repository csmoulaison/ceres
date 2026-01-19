AssetPack* asset_pack_load(Arena* arena) {
	FILE* file = fopen(ASSET_PACK_FILENAME, "r");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	u64 pack_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	AssetPack* pack = (AssetPack*)arena_alloc(arena, pack_size);
	fread(pack, pack_size, 1, file);
	return pack;
}
