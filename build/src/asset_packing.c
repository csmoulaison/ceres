#include "asset_format.h"

#define FLAT_MESH_SHADING true

typedef enum {
	MANIFEST_MESH,
	MANIFEST_TEXTURE,
	MANIFEST_RENDER_PROGRAM
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

void prepare_mesh_asset(char* filename, MeshAsset* asset, u8** data, Arena* arena) {
	printf("Preparing mesh asset '%s'...\n", filename);
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
		asset->vertices_len = tmp_faces_len * 3;
	} else {
		asset->vertices_len = tmp_vertices_len;
	}
	asset->indices_len = tmp_faces_len * 3;
	*data = (u8*)arena_alloc(arena, sizeof(MeshVertexData) * asset->vertices_len + sizeof(u32) * asset->indices_len);
	MeshVertexData* vertices = (MeshVertexData*)*data;
	u32* indices = (u32*)((*data) + sizeof(MeshVertexData) * asset->vertices_len);

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

void prepare_texture_asset(char* filename, TextureAsset* asset, u8** buffer, Arena* arena) {
	printf("Preparing texture asset '%s'...\n", filename);
	u32 w, h, channels;
	stbi_uc* stb_pixels = stbi_load(filename, &w, &h, &channels, STBI_rgb_alpha);
	assert(stb_pixels != NULL);
	assert(channels == 4);
	assert(w == 256 && h == 256);
	asset->width = w;
	asset->height = h;

	u64 buffer_size = sizeof(u32) * w * h;
	*buffer = (u8*)arena_alloc(arena, buffer_size);
	memcpy(*buffer, stb_pixels, buffer_size);
	stbi_image_free(stb_pixels);
}

void prepare_render_program_asset(char* vert_filename, char* frag_filename, RenderProgramAsset* asset, char** buffer, Arena* arena) {
	printf("Preparing render program asset (vert: '%s', frag: '%s')...\n", vert_filename, frag_filename);

	FILE* vert_file = fopen(vert_filename, "r");
	assert(vert_file != NULL);
	asset->vertex_shader_src_len = 1;
	while((fgetc(vert_file)) != EOF) {
		asset->vertex_shader_src_len++;
	}
	fseek(vert_file, 0, SEEK_SET);

	FILE* frag_file = fopen(frag_filename, "r");
	assert(frag_file != NULL);
	asset->fragment_shader_src_len = 1;
	while((fgetc(frag_file)) != EOF) {
		asset->fragment_shader_src_len++;
	}
	fseek(frag_file, 0, SEEK_SET);

	*buffer = (char*)arena_alloc(arena, sizeof(char) * asset->vertex_shader_src_len + sizeof(char) * asset->fragment_shader_src_len);

	int i = 0;
	while(((*buffer)[i] = fgetc(vert_file)) != EOF) {
		i++;
	}
	(*buffer)[i] = '\0';
	fclose(vert_file);

	i = 0;
	while(((*buffer)[asset->vertex_shader_src_len + i] = fgetc(frag_file)) != EOF) {
		i++;
	}
	(*buffer)[asset->vertex_shader_src_len + i] = '\0';
	//while((*buffer[asset->vertex_shader_src_len + i] = 'c') != EOF) {}
	fclose(frag_file);
}

// TODO: We could avoid this multi-pass approach by having dedicated functions
// which iterate each asset type and only do the processing needed to get the
// required buffer size, then processing it into the final asset pack format
// live.
//
// This feels to me like it would be easier to eventually encode into a generic
// process as well, which will probably be useful as there is a lot of similar
// code used throughout this pipeline.
//
// Of course, get this working first before we move onto any of that stuff.
void pack_assets(char* manifest_filename, char* out_filename, char* generated_asset_handles_filename) {
	// We first process the assets and store the output data into temporary buffers
	// allocated by the arena. This is then written into the actual asset pack and
	// saved to disk.
	Arena arena;
	arena_init(&arena, MEGABYTE * 128, NULL, "AssetPacker");

	MeshAsset mesh_assets[MAX_MESH_ASSETS];
	u8* mesh_buffers[MAX_MESH_ASSETS];
	u32 meshes_len = 0;

	TextureAsset texture_assets[MAX_TEXTURE_ASSETS];
	u8* texture_buffers[MAX_TEXTURE_ASSETS];
	u32 textures_len = 0;

	RenderProgramAsset render_program_assets[MAX_RENDER_PROGRAM_ASSETS];
	char* render_program_buffers[MAX_RENDER_PROGRAM_ASSETS];
	u32 render_programs_len = 0;

	// Load asset manifest file
	FILE* manifest = fopen(manifest_filename, "r");
	assert(manifest != NULL);

	char mesh_handles[MAX_MESH_ASSETS][256];
	char texture_handles[MAX_TEXTURE_ASSETS][256];
	char render_program_handles[MAX_RENDER_PROGRAM_ASSETS][256];

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
		else if(strncmp(type_str, "render_program", strlen("render_program")) == 0) { item.type = MANIFEST_RENDER_PROGRAM; }
		else { printf("Entry type '%s' not recognized in asset manifest.", type_str); }

		switch(item.type) {
			case MANIFEST_MESH: {
				strcpy(mesh_handles[meshes_len], item.handle);
				assert(meshes_len < MAX_MESH_ASSETS);
				prepare_mesh_asset(item.filenames[0], &mesh_assets[meshes_len], &mesh_buffers[meshes_len], &arena);
				meshes_len++;
			} break;
			case MANIFEST_TEXTURE: {
				strcpy(texture_handles[textures_len], item.handle);
				assert(textures_len < MAX_TEXTURE_ASSETS);
				prepare_texture_asset(item.filenames[0], &texture_assets[textures_len], &texture_buffers[textures_len], &arena);
				textures_len++;
			} break;
			case MANIFEST_RENDER_PROGRAM: {
				strcpy(render_program_handles[render_programs_len], item.handle);
				assert(render_programs_len < MAX_RENDER_PROGRAM_ASSETS);
				consume_word(item.filenames[1], line, &line_i);
				prepare_render_program_asset(item.filenames[0], item.filenames[1], &render_program_assets[render_programs_len], &render_program_buffers[render_programs_len], &arena);
				render_programs_len++;
			} break;
			default: break;
		}
	}

	// Calculate total asset pack buffer size and per-asset buffer offsets
	u64 buffer_size = 0;
	u64 mesh_offsets[MAX_MESH_ASSETS];
	u64 texture_offsets[MAX_TEXTURE_ASSETS];
	u64 render_program_offsets[MAX_RENDER_PROGRAM_ASSETS];

	for(i32 i = 0; i < meshes_len; i++) {
		mesh_offsets[i] = buffer_size;
		MeshAsset* asset = &mesh_assets[i];
		buffer_size += sizeof(MeshAsset) + sizeof(MeshVertexData) * asset->vertices_len + sizeof(u32) * asset->indices_len;
	}
	for(i32 i = 0; i < textures_len; i++) {
		texture_offsets[i] = buffer_size;
		TextureAsset* asset = &texture_assets[i];
		buffer_size += sizeof(TextureAsset) + TEXTURE_PIXEL_STRIDE_BYTES * asset->width * asset->height;
	}
	for(i32 i = 0; i < render_programs_len; i++) {
		render_program_offsets[i] = buffer_size;
		RenderProgramAsset* asset = &render_program_assets[i];
		buffer_size += sizeof(RenderProgramAsset) + sizeof(char) * asset->vertex_shader_src_len + sizeof(char) * asset->fragment_shader_src_len;
	}

	// Build asset pack, copying the previously prepared data
	u64 asset_pack_size = sizeof(AssetPack) + buffer_size;
	AssetPack* pack = (AssetPack*)arena_alloc(&arena, asset_pack_size);
	pack->meshes_len = meshes_len;
	pack->textures_len = textures_len;
	pack->render_programs_len = render_programs_len;
	memcpy(pack->mesh_buffer_offsets, mesh_offsets, sizeof(pack->mesh_buffer_offsets));
	memcpy(pack->texture_buffer_offsets, texture_offsets, sizeof(pack->texture_buffer_offsets));
	memcpy(pack->render_program_buffer_offsets, render_program_offsets, sizeof(pack->render_program_buffer_offsets));

	for(i32 i = 0; i < meshes_len; i++) {
		MeshAsset* tmp_asset = &mesh_assets[i];
		u8* tmp_buffer = mesh_buffers[i];

		MeshAsset* pack_asset = (MeshAsset*)&pack->buffer[pack->mesh_buffer_offsets[i]];
		u8* pack_buffer = &pack->buffer[pack->mesh_buffer_offsets[i] + sizeof(MeshAsset)];

		memcpy(pack_asset, tmp_asset, sizeof(MeshAsset));
		memcpy(pack_buffer, tmp_buffer, sizeof(MeshVertexData) * tmp_asset->vertices_len + sizeof(u32) * tmp_asset->indices_len);
	}
	for(i32 i = 0; i < textures_len; i++) {
		TextureAsset* tmp_asset = &texture_assets[i];
		u8* tmp_buffer = texture_buffers[i];

		TextureAsset* pack_asset = (TextureAsset*)&pack->buffer[pack->texture_buffer_offsets[i]];
		u8* pack_buffer = &pack->buffer[pack->texture_buffer_offsets[i] + sizeof(TextureAsset)];

		memcpy(pack_asset, tmp_asset, sizeof(TextureAsset));
		memcpy(pack_buffer, tmp_buffer, TEXTURE_PIXEL_STRIDE_BYTES * tmp_asset->width * tmp_asset->height);
	}
	for(i32 i = 0; i < render_programs_len; i++) {
		RenderProgramAsset* tmp_asset = &render_program_assets[i];
		u8* tmp_buffer = render_program_buffers[i];

		RenderProgramAsset* pack_asset = (RenderProgramAsset*)&pack->buffer[pack->render_program_buffer_offsets[i]];
		char* pack_buffer = pack_asset->buffer;

		memcpy(pack_asset, tmp_asset, sizeof(RenderProgramAsset));
		memcpy(pack_buffer, tmp_buffer, sizeof(char) * tmp_asset->vertex_shader_src_len + sizeof(char) * tmp_asset->fragment_shader_src_len);
	}

	FILE* out_file = fopen(out_filename, "w");
	assert(out_file != NULL);

	fwrite(pack, asset_pack_size, 1, out_file);
	fclose(out_file);


	FILE* test_file = fopen(out_filename, "r");
	assert(test_file != NULL);

	AssetPack* test = (AssetPack*)arena_alloc(&arena, asset_pack_size);
	fread(test, asset_pack_size, 1, test_file);

	FILE* generated_file = fopen(generated_asset_handles_filename, "w");
	assert(generated_file != NULL);

	fprintf(generated_file, "\
// WARNING: This file (will be in the future I promise) was generated by the\n\
// build system. Any modifications made to it will be overwritten.\n\
#ifndef GEN_asset_handles_h_INCLUDED\n\
#define GEN_asset_handles_h_INCLUDED\n\
\n\
#define ASSET_PACK_FILENAME \"%s\"\n\n", "data/assets.pack");

	for(i32 i = 0; i < meshes_len; i++) {
		fprintf(generated_file, "#define ASSET_MESH_%s %i\n", mesh_handles[i], i);
	}
	fprintf(generated_file, "\n");
	for(i32 i = 0; i < textures_len; i++) {
		fprintf(generated_file, "#define ASSET_TEXTURE_%s %i\n", texture_handles[i], i);
	}
	fprintf(generated_file, "\n");
	for(i32 i = 0; i < render_programs_len; i++) {
		fprintf(generated_file, "#define ASSET_RENDER_PROGRAM_%s %i\n", render_program_handles[i], i);
	}
	fprintf(generated_file, "\n#endif // GEN_asset_handles_h_INCLUDED");

	fclose(generated_file);
}
