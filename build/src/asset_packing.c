#include "asset_format.h"

#define MAX_MESHES 4096
#define MAX_TEXTURES 4096

#define FLAT_MESH_SHADING true

typedef enum {
	MANIFEST_MESH,
	MANIFEST_TEXTURE,
	MANIFEST_RENDER_PROGRAM,
	MANIFEST_INPUT_CONFIG
} AssetManifestItemType;

typedef struct {
	AssetManifestItemType type;
	char handle[64];
	char filenames[2][2048];
} AssetManifestItem;

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

// NOW: This code has been demonstrated to work when integrated into the main
// codebase, but there are a couple differences in data here that still need to
// be verified.
//
// Just as a reminder, what we are doing is packing all the data into an
// AssetPackData struct, which for now will contain textures and meshes, but
// should have input config and render programs added directly after that.
//
// The approach is to prepare all the different assets in an arena, keeping
// of the headers and data buffers. After they are all prepared, we do a second
// pass, writing the data directly into a file with the appropriate information
// encoded into the AssetPackData struct, which should probably be called
// AssetPackHeader, because that's what it is.
//
// Once the first pass of meshes and textures has been done on the packing side,
// we need to implement the game side loading of these assets as well. Do that
// before doing the other asset types. That will also give us a good test run
// regarding the friction of modifying this pipeline. That may in turn give us
// ideas for improvements.
void prepare_mesh_asset(char* filename, MeshAssetHeader* header, u8** data, Arena* arena) {
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
		u32 i = 0;
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

	rewind(file);
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
	header->indices_len = tmp_faces_len * 3;
	// NOW: This?
	*data = (u8*)arena_alloc(arena, sizeof(MeshVertexData) * header->vertices_len + sizeof(u32) * header->indices_len);
	MeshVertexData* vertices = (MeshVertexData*)*data;
	// NOW: This funkiness to verify?
	u32* indices = (u32*)((*data) + sizeof(MeshVertexData) * header->vertices_len);

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
			v3_copy(vert->position, &tmp_vertices[face_vert_idx * 3]);
		} else {
			vert = &vertices[face_vert_idx];
		}
		v2_copy(vert->texture_uv, &tmp_uvs[face_uv_idx * 2]);
		v3_copy(vert->normal, &tmp_normals[face_norm_idx * 3]);
	}

	arena_destroy(&tmp_arena);
}

void prepare_texture_asset(char* filename, TextureAssetHeader* header, u8** pixels, Arena* arena) {
	u32 w, h, channels;
	stbi_uc* stb_pixels = stbi_load(filename, &w, &h, &channels, STBI_rgb_alpha);
	assert(stb_pixels != NULL && channels == 4);
	assert(w == 256 && h == 256);
	header->width = w;
	header->height = h;

	u64 pixels_size = sizeof(u32) * w * h;
	*pixels = (u8*)arena_alloc(arena, pixels_size);
	memcpy(*pixels, stb_pixels, pixels_size);
	stbi_image_free(stb_pixels);
}

void pack_assets() {
	MeshAssetHeader mesh_headers[MAX_MESHES];
	u8* mesh_buffers[MAX_MESHES];
	u32 meshes_len = 0;

	TextureAssetHeader texture_headers[MAX_TEXTURES];
	u8* texture_buffers[MAX_TEXTURES];
	u32 textures_len = 0;

	Arena arena;
	arena_init(&arena, MEGABYTE * 64, NULL, "AssetPacker");

	// Load asset manifest file
	FILE* manifest = fopen("data/assets.manifest", "r");
	assert(manifest != NULL);

	char line[4096];
	while(fgets(line, sizeof(line), manifest)) {
		AssetManifestItem item;
		i32 line_i = 0;

		char type_str[64];
		consume_word(type_str, line, &line_i);
		consume_word(item.handle, line, &line_i);
		consume_word(item.filenames[0], line, &line_i);

		if(strncmp(type_str, "mesh", strlen("mesh")) == 0) { item.type = MANIFEST_MESH; }
		else if(strncmp(type_str, "texture", strlen("texture")) == 0) { item.type = MANIFEST_TEXTURE; }
		else if(strncmp(type_str, "texture", strlen("texture")) == 0) { item.type = MANIFEST_RENDER_PROGRAM; }
		else if(strncmp(type_str, "texture", strlen("texture")) == 0) { item.type = MANIFEST_INPUT_CONFIG; }
		else { printf("Entry type '%s' not recognized in asset manifest.", type_str); }

		switch(item.type) {
			case MANIFEST_MESH: {
				prepare_mesh_asset(item.filenames[0], &mesh_headers[meshes_len], &mesh_buffers[meshes_len], &arena);
				meshes_len++;
			} break;
			case MANIFEST_TEXTURE: {
				prepare_texture_asset(item.filenames[0], &texture_headers[textures_len], &texture_buffers[textures_len], &arena);
				textures_len++;
			} break;
			case MANIFEST_RENDER_PROGRAM: {
				consume_word(item.filenames[1], line, &line_i);
				// NOW: Implement render program preparation
			} break;
			case MANIFEST_INPUT_CONFIG: {
				// NOW: Implement input config preparation
			} break;


			default: break;
		}
	}

	AssetPackData pack;
	pack.meshes_len = 0;
	pack.textures_len = 0;
}
