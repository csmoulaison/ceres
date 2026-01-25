typedef struct {
	char filename[256];
	u32 vertices_len;
	u32 indices_len;

	u32 vertex_lines;
	u32 uv_lines;
	u32 normal_lines;
	u32 face_lines;
} MeshInfo;

void calculate_mesh_assets(AssetInfoList* list, char* handle, i32 args_len, ManifestArgument* args, Arena* arena) {
	MeshInfo* info = (MeshInfo*)arena_alloc(arena, sizeof(MeshInfo));
	assert(args_len == 1);
	strcpy(info->filename, args[0].text);

	FILE* file = fopen(info->filename, "r");
	assert(file != NULL);

	char line[512];
	while(fgets(line, sizeof(line), file)) {
		char key[32];
		u32 char_i = 0;
		consume_word(key, line, &char_i);
		if(strcmp(key, "v") == 0) {
			info->vertex_lines++;
		} else if(strcmp(key, "vt") == 0) {
			info->uv_lines++;
		} else if(strcmp(key, "vn") == 0) {
			info->normal_lines++;
		} else if(strcmp(key, "f") == 0) {
			info->face_lines++;
		}
	}
	// Three vertices per face. This is in the flat shading case without an index
	// buffer. With an index buffer, the number of vertices would probably be
	// equal to vertex_lines.
	// 
	// TODO: Just don't even have an index buffer in the flat shading case,
	// obviously. Wait to make this change until after we get this reimplemented.
	info->vertices_len = info->face_lines * 3;
	info->indices_len = info->face_lines * 3;

	u64 size = sizeof(MeshAsset) + sizeof(MeshVertexData) * info->vertices_len + sizeof(u32) * info->indices_len;
	push_asset_info(list, ASSET_TYPE_MESH, handle, size, info);
}

void pack_mesh_asset(void* p_info, void* p_asset) {
	MeshInfo* info = (MeshInfo*)p_info;
	MeshAsset* asset = (MeshAsset*)p_asset;

	// Allocate temporary buffers and populate them with file data.
	Arena tmp_arena;
	arena_init(&tmp_arena, MEGABYTE * 64, NULL, "TmpMeshLoad");

	// x, y, z | x, y, z | ...
	f32* tmp_vertices = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 3 * info->vertex_lines);
	u32 tmp_vertices_len = 0;
	// u, v | u, v | ...
	f32* tmp_uvs = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 2 * info->uv_lines);
	u32 tmp_uvs_len = 0;
	// x, y, z | x, y, z | ...
	f32* tmp_normals = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 3 * info->normal_lines);
	u32 tmp_normals_len = 0;
	// v1: vert, uv, norm, v2: vert, uv, norm, v3: vert, uv, norm | v1: vert, uv ...
	i32* tmp_faces = (i32*)arena_alloc(&tmp_arena, sizeof(i32) * 9 * info->face_lines);
	u32 tmp_faces_len = 0;

	FILE* file = fopen(info->filename, "r");
	assert(file != NULL);

	char line[512];
	while(fgets(line, sizeof(line), file)) {
		char key[32];
		u32 line_i = 0;
		consume_word(key, line, &line_i);
		if(strcmp(key, "v") == 0) {
			f32* v = &tmp_vertices[tmp_vertices_len * 3];
			for(i32 comp = 0; comp < 3; comp++) {
				consume_f32(&v[comp], line, &line_i);
			}
			tmp_vertices_len++;
		} else if(strcmp(key, "vt") == 0) {
			f32* u = &tmp_uvs[tmp_uvs_len * 2];
			for(i32 comp = 0; comp < 2; comp++) {
				consume_f32(&u[comp], line, &line_i);
			}
			u[1] = 1.0f - u[1];
			tmp_uvs_len++;
		} else if(strcmp(key, "vn") == 0) {
			f32* n = &tmp_normals[tmp_normals_len * 3];
			for(i32 comp = 0; comp < 3; comp++) {
				consume_f32(&n[comp], line, &line_i);
			}
			tmp_normals_len++;
		} else if(strcmp(key, "f") == 0) {
			remove_slashes_from_line(line);
			i32* f = &tmp_faces[tmp_faces_len * 9];
			for(i32 comp = 0; comp < 9; comp++) {
				consume_i32(&f[comp], line, &line_i);
			}
			tmp_faces_len++;
		}
	}
	fclose(file);

	// Allocate final vertex and index data
	if(FLAT_MESH_SHADING) {
		asset->vertices_len = tmp_faces_len * 3;
	} else {
		asset->vertices_len = tmp_vertices_len;
	}
	asset->indices_len = tmp_faces_len * 3;
	MeshVertexData* vertices = (MeshVertexData*)asset->buffer;
	u32* indices = (u32*)((asset->buffer) + sizeof(MeshVertexData) * asset->vertices_len);

	// Iterate faces, filling index buffer with face data and associating
	// verts/uvs/normals together at the same time.
	for(i32 i = 0; i < tmp_faces_len * 3; i++) {
		i32* face_element = &tmp_faces[i * 3];
		i32 face_vert_idx = face_element[0] - 1;
		i32 face_uv_idx = face_element[1] - 1;
		i32 face_norm_idx = face_element[2] - 1;
		indices[i] = face_vert_idx;

		MeshVertexData* vert;
		if(FLAT_MESH_SHADING) {
			vert = &vertices[i];
			//v3_copy(vert->position, &tmp_vertices[face_vert_idx * 3]);
			memcpy(vert->position, &tmp_vertices[face_vert_idx * 3], sizeof(f32) * 3);
		} else {
			vert = &vertices[face_vert_idx];
		}
		//v2_copy(vert->texture_uv, &tmp_uvs[face_uv_idx * 2]);
		memcpy(vert->texture_uv, &tmp_uvs[face_uv_idx * 2], sizeof(f32) * 2);
		//v3_copy(vert->normal, &tmp_normals[face_norm_idx * 3]);
		memcpy(vert->normal, &tmp_normals[face_norm_idx * 3], sizeof(f32) * 3);
	}

	arena_destroy(&tmp_arena);
}
