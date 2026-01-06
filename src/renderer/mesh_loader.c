// NOW: We are temporarily writing this into the engine to get it working, then
// moving it out into its own executable as part of the build step.

#define LOAD_VERTICES 4096
#define LOAD_INDICES 4096
#define LOAD_TEXTURE_UVS 4096

typedef struct {
	f32 position[3];
	f32 texture_uv[2];
} MeshVertex;

typedef struct
{
	MeshVertex vertices[LOAD_VERTICES];
	u32 vertices_len;
	u32 indices[LOAD_INDICES];
	u32 indices_len;
} MeshData;

void load_mesh(MeshData* data, char* mesh_filename)
{
	FILE* file = fopen(mesh_filename, "r");
	if(file == NULL) { panic(); }

	// The tricky thing about .obj is texture UVs being defined per index buffer vertex, as opposted
	// to being defined per vertex buffer vertex, you see.
	// 
	// To fix this, we'll apply the proper UVs and whatnot to the vertex buffer vertices retroactively,
	// as we are iterating our way through the faces.
	f32 tmp_texture_uvs[LOAD_TEXTURE_UVS][2];
	u32 tmp_texture_uvs_len = 0;

	// Note that tmp_face_elements do not correspond with "f" records, but rather with one of the
	// elements in those records.
	struct {
		u32 vertex_index;
		u32 texture_uv_index;
	} tmp_face_elements[32000];
	u32 tmp_face_elements_len = 0;

	data->vertices_len = 0;
	while(true) {
		char keyword[128];
		i32 res = fscanf(file, "%s", keyword);

		if(res == EOF)
			break;

		if(strcmp(keyword, "v") == 0) {
			f32* pos = data->vertices[data->vertices_len].position;
			fscanf(file, "%f %f %f", &pos[0], &pos[1], &pos[2]);
			data->vertices_len++;
		} else if(strcmp(keyword, "vt") == 0) {
			f32* tmp_texture_uv = tmp_texture_uvs[tmp_texture_uvs_len];
			fscanf(file, "%f %f", &tmp_texture_uv[0], &tmp_texture_uv[1]);
			tmp_texture_uv[1] = 1 - tmp_texture_uv[1];
			tmp_texture_uvs_len++;
		} else if(strcmp(keyword, "f") == 0) {
			i32 throwaways[3];
			i32 values_len = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
				&tmp_face_elements[tmp_face_elements_len + 0].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 0].texture_uv_index, 
				&throwaways[0], 
				&tmp_face_elements[tmp_face_elements_len + 1].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 1].texture_uv_index, 
				&throwaways[1], 
				&tmp_face_elements[tmp_face_elements_len + 2].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 2].texture_uv_index, 
				&throwaways[2]);

			if(values_len != 9) { panic(); }
			tmp_face_elements_len += 3;
		}
	}
	fclose(file);

	data->indices_len  = 0;
	for(u32 element_index = 0; element_index < tmp_face_elements_len; element_index++) {
		u32 index = tmp_face_elements[element_index].vertex_index - 1;
		data->indices[element_index] = index;
		data->vertices[index].texture_uv[0] = tmp_texture_uvs[tmp_face_elements[element_index].texture_uv_index - 1][0];
		data->vertices[index].texture_uv[1] = tmp_texture_uvs[tmp_face_elements[element_index].texture_uv_index - 1][1];
		data->indices_len++;
	}
}
