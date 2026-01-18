// Issue8 - TODO: We are temporarily writing this into the engine to get it working, then
// moving it out into its own executable as part of the build step.

#define LOAD_VERTICES 500000
#define LOAD_INDICES 500000
#define TMP_LOAD_UVS 500000
#define TMP_LOAD_NORMALS 500000
#define TMP_LOAD_FACE_ELEMENTS 500000

#define FLAT_MESH_SHADING true

typedef struct {
	union {
		struct {
			f32 position[3];
			f32 normal[3];
			f32 texture_uv[2];
		};
		f32 data[8];
	};
} MeshVertex;

typedef struct
{
	u32 vertices_len;
	u32 indices_len;
	bool flat_shading;
	MeshVertex* vertices;
	u32* indices;
} MeshData;

typedef struct
{
	u32 vertices_len;
	u32 indices_len;
	MeshVertex vertices[LOAD_VERTICES];
	u32 indices[LOAD_INDICES];
	bool flat_shading;
} oldMeshData;

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

void remove_slashes_from_line(char* line) {
	i32 i = 0;
	while(line[i] != '\0' && line[i] != '\n') {
		if(line[i] == '/') line[i] = ' ';
		i++;
	}
}

void consume_word(char* word, char* line, i32* line_i) {
	i32 word_i = 0;
	while(line[*line_i] != ' ' && line[*line_i] != '\0' && line[*line_i] != '\n') {
		word[word_i] = line[*line_i];
		*line_i += 1;
		word_i++;
	}
	while(line[*line_i] == ' ' && line[*line_i] != '\0' && line[*line_i] != '\n') {
		*line_i += 1;
	}
	word[word_i] = '\0';
}

void consume_f32(f32* f, char* line, i32* line_i) {
	char word[64];
	consume_word(word, line, line_i);
	*f = (f32)atof(word);
}

void consume_i32(i32* v, char* line, i32* line_i) {
	char word[64];
	consume_word(word, line, line_i);
	*v = (i32)atoi(word);
}

void load_mesh(MeshData* data, char* filename, Arena* arena, bool flat_shading) {
	FILE* file = fopen(filename, "r");
	assert(file != NULL);

	// Count line types (vertices, uvs, normals, faces)
	u32 vertex_lines = 0;
	u32 uv_lines = 0;
	u32 normal_lines = 0;
	u32 face_lines = 0;
	char line[512];
	while(fgets(line, sizeof(line), file)) {
		char key[32];
		i32 i = 0;
		consume_word(key, line, &i);
		if(strcmp(key, "v") == 0) {
			vertex_lines++;
		} else if(strcmp(key, "vt") == 0) {
			uv_lines++;
		} else if(strcmp(key, "vn") == 0) {
			normal_lines++;
		} else if(strcmp(key, "f") == 0) {
			face_lines++;
		}
	}

	// Allocate temporary buffers and populate them with file data.
	Arena tmp_arena;
	arena_init(&tmp_arena, MEGABYTE * 64, NULL, "TmpMeshLoad");

	// x, y, z | x, y, z | ...
	f32* tmp_vertices = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 3 * vertex_lines);
	u32 tmp_vertices_len = 0;
	// u, v | u, v | ...
	f32* tmp_uvs = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 2 * uv_lines);
	u32 tmp_uvs_len = 0;
	// x, y, z | x, y, z | ...
	f32* tmp_normals = (f32*)arena_alloc(&tmp_arena, sizeof(f32) * 3 * normal_lines);
	u32 tmp_normals_len = 0;
	// v1: vert, uv, norm, v2: vert, uv, norm, v3: vert, uv, norm | v1: vert, uv ...
	i32* tmp_faces = (i32*)arena_alloc(&tmp_arena, sizeof(i32) * 9 * face_lines);
	u32 tmp_faces_len = 0;

	//rewind(file);
	fseek(file, 0, SEEK_SET);
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
		data->vertices_len = tmp_faces_len * 3;
	} else {
		data->vertices_len = tmp_vertices_len;
	}
	data->indices_len = tmp_faces_len * 3;
	data->vertices = (MeshVertex*)arena_alloc(arena, sizeof(MeshVertex) * data->vertices_len);
	data->indices = (u32*)arena_alloc(arena, sizeof(u32) * data->indices_len);
	data->flat_shading = true;
	MeshVertex* vertices = data->vertices;
	u32* indices = data->indices;

	// Iterate faces, filling index buffer with face data and associating
	// verts/uvs/normals together at the same time.
	for(i32 i = 0; i < tmp_faces_len * 3; i++) {
		i32* face_element = &tmp_faces[i * 3];
		i32 face_vert_idx = face_element[0] - 1;
		i32 face_uv_idx = face_element[1] - 1;
		i32 face_norm_idx = face_element[2] - 1;
		indices[i] = face_vert_idx;

		MeshVertex* vert;
		if(FLAT_MESH_SHADING) {
			vert = &vertices[i];
			v3_copy(vert->position, &tmp_vertices[face_vert_idx * 3]);
		} else {
			vert = &vertices[face_vert_idx];
		}
		v2_copy(vert->texture_uv, &tmp_uvs[face_uv_idx * 2]);
		v3_copy(vert->normal, &tmp_normals[face_norm_idx * 3]);
	}

	arena_destroy(&tmp_arena);
}
