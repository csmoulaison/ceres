// Issue8 - TODO: We are temporarily writing this into the engine to get it working, then
// moving it out into its own executable as part of the build step.

#define LOAD_VERTICES 500000
#define LOAD_INDICES 500000
#define TMP_LOAD_UVS 500000
#define TMP_LOAD_NORMALS 500000
#define TMP_LOAD_FACE_ELEMENTS 500000

typedef struct {
	union {
		struct {
			f32 position[3];
			f32 normal[3];
			f32 texture_uv[2];
		};
		f32 data[8];
	};
} LoadMeshVertex;

typedef struct
{
	u32 vertices_len;
	u32 indices_len;
	LoadMeshVertex vertices[LOAD_VERTICES];
	u32 indices[LOAD_INDICES];
	bool flat_shading;
} LoadMeshData;

typedef struct {
	u32 vertex_index;
	u32 texture_uv_index;
	u32 normal_index;
} MeshFaceElement;

typedef struct {
	f32 uv[2];
} TmpUv;

typedef struct {
	f32 normal[3];
} TmpNormal;

void load_mesh(LoadMeshData* data, char* mesh_filename, Arena* init_arena, bool flat_shading)
{
	data->flat_shading = flat_shading;

	FILE* file = fopen(mesh_filename, "r");
	if(file == NULL) { panic(); }

	TmpUv* tmp_uvs = (TmpUv*)arena_alloc(init_arena, sizeof(TmpUv) * TMP_LOAD_UVS);
	u32 tmp_uvs_len = 0;

	TmpNormal* tmp_normals = (TmpNormal*)arena_alloc(init_arena, sizeof(TmpNormal) * TMP_LOAD_NORMALS);
	u32 tmp_normals_len = 0;

	MeshFaceElement* tmp_face_elements = (MeshFaceElement*)arena_alloc(init_arena, sizeof(MeshFaceElement) * TMP_LOAD_FACE_ELEMENTS);
	u32 tmp_face_elements_len = 0;

	// tmp_vertices used in the flat shading case.
	f32 tmp_vertices[LOAD_VERTICES][3];
	u32 tmp_vertices_len = 0;

	data->vertices_len = 0;
	while(true) {
		char keyword[128];
		i32 res = fscanf(file, "%s", keyword);

		if(res == EOF)
			break;

		if(strcmp(keyword, "v") == 0) {
			f32* pos;
			if(flat_shading) {
				pos = tmp_vertices[tmp_vertices_len];
				tmp_vertices_len++;
			} else {
				pos = data->vertices[data->vertices_len].position;
				data->vertices_len++;
			}
			fscanf(file, "%f %f %f", &pos[0], &pos[1], &pos[2]);
		} else if(strcmp(keyword, "vt") == 0) {
			TmpUv* tmp_uv = &tmp_uvs[tmp_uvs_len];
			fscanf(file, "%f %f", &tmp_uv->uv[0], &tmp_uv->uv[1]);
			tmp_uv->uv[1] = 1 - tmp_uv->uv[1];
			tmp_uvs_len++;
		} else if(strcmp(keyword, "vn") == 0) {
			TmpNormal* tmp_normal = &tmp_normals[tmp_normals_len];
			fscanf(file, "%f %f %f", &tmp_normal->normal[0], &tmp_normal->normal[1], &tmp_normal->normal[2]);
			tmp_normals_len++;
		} else if(strcmp(keyword, "f") == 0) {
			i32 throwaways[3];
			i32 values_len = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
				&tmp_face_elements[tmp_face_elements_len + 0].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 0].texture_uv_index, 
				&tmp_face_elements[tmp_face_elements_len + 0].normal_index, 
				&tmp_face_elements[tmp_face_elements_len + 1].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 1].texture_uv_index, 
				&tmp_face_elements[tmp_face_elements_len + 1].normal_index, 
				&tmp_face_elements[tmp_face_elements_len + 2].vertex_index, 
				&tmp_face_elements[tmp_face_elements_len + 2].texture_uv_index, 
				&tmp_face_elements[tmp_face_elements_len + 2].normal_index);

			if(values_len != 9) { panic(); }
			tmp_face_elements_len += 3;
		}
	}
	fclose(file);

	if(data->flat_shading)
		data->vertices_len = 0; 
	data->indices_len = 0;

	for(u32 element_index = 0; element_index < tmp_face_elements_len; element_index++) {
		MeshFaceElement* elem = &tmp_face_elements[element_index];
		u32 index = elem->vertex_index - 1;
		data->indices[element_index] = index;

		LoadMeshVertex* vert;
		if(data->flat_shading) {
			vert = &data->vertices[element_index];
		} else {
			vert = &data->vertices[index];
		}

		if(data->flat_shading) {
			vert->position[0] = tmp_vertices[elem->vertex_index - 1][0]; // ONLY for the no index buffer case.
			vert->position[1] = tmp_vertices[elem->vertex_index - 1][1]; // ONLY for the no index buffer case.
			vert->position[2] = tmp_vertices[elem->vertex_index - 1][2]; // ONLY for the no index buffer case.
		}
		
		vert->texture_uv[0] = tmp_uvs[elem->texture_uv_index - 1].uv[0];
		vert->texture_uv[1] = tmp_uvs[elem->texture_uv_index - 1].uv[1];

		vert->normal[0] = tmp_normals[elem->normal_index - 1].normal[0];
		vert->normal[1] = tmp_normals[elem->normal_index - 1].normal[1];
		vert->normal[2] = tmp_normals[elem->normal_index - 1].normal[2];

		if(flat_shading)
			data->vertices_len++;
		data->indices_len++;
	}
}
