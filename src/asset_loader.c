AssetPack* asset_pack_load(Arena* arena) {
	FILE* file = fopen(ASSET_PACK_FILENAME, "r");
	assert(file != NULL);

	fseek(file, 0, SEEK_END);
	u64 pack_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	AssetPack* pack = (AssetPack*)arena_alloc(arena, pack_size);
	fread(pack, pack_size, 1, file);

	if(pack->meshes_len > 255) {
		printf("\033[31mThere are more than 255 meshes in the asset pack! Ensure they aren't being indexed with 8 bits.\033[0m\n");
		panic();
	}
	if(pack->textures_len > 255) {
		printf("\033[31mThere are more than 255 textures in the asset pack! Ensure they aren't being indexed with 8 bits.\033[0m\n");
		panic();
	}
	if(pack->render_programs_len > 255) {
		printf("\033[31mThere are more than 255 render_programs in the asset pack! Ensure they aren't being indexed with 8 bits.\033[0m\n");
		panic();
	}
	if(pack->fonts_len > 255) {
		printf("\033[31mThere are more than 255 fonts in the asset pack! Ensure they aren't being indexed with 8 bits.\033[0m\n");
		panic();
	}
	return pack;
}
