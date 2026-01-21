#include "asset_format.h"

#define MANIFEST_FILENAME "data/assets.manifest"
#define MAX_ASSETS 256
#define FLAT_MESH_SHADING true

typedef enum {
	ASSET_TYPE_MESH,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_RENDER_PROGRAM,
	ASSET_TYPE_FONT,
	NUM_ASSET_TYPES
} AssetType;

typedef struct {
	AssetType type;
	u64 size_in_buffer;
	char handle[256];
	void* data; // i.e. MeshInfo, TextureInfo
} AssetInfo;

typedef struct {
	AssetInfo infos[MAX_ASSETS];
	u32 len;
	u32 total_buffer_size;
} AssetInfoList;

typedef struct {
	char filename[256];
	u32 vertices_len;
	u32 indices_len;
} MeshInfo;

typedef enum {
	TEXTURE_SOURCE_IMAGE,
	TEXTURE_SOURCE_FONT
} TextureSourceType;

typedef struct {
	char filename[256];
	TextureSourceType source_type;
	u32 font_size; // only used if source_type = TEXTURE_SOURCE_FONT
} TextureInfo;

typedef struct {
	char vert_filename[256];
	char frag_filename[256];
} RenderProgramInfo;

typedef struct {
	char filename[256];
	u32 font_size;
	u32 texture_id;
} FontInfo;

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

// NOW: Reduce redundancy here from the size calculator logic for all these
// assets, storing all of it in the info struct so it doesn't need to be
// recalculated in the actual preparation functions.
//
// Also rename them. Prepare doesn't make as uch sense anymore.
void prepare_mesh_asset(MeshInfo* info, MeshAsset* asset) {
	printf("Preparing mesh asset '%s'...\n", info->filename);
	FILE* file = fopen(info->filename, "r");
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
			v3_copy(vert->position, &tmp_vertices[face_vert_idx * 3]);
		} else {
			vert = &vertices[face_vert_idx];
		}
		v2_copy(vert->texture_uv, &tmp_uvs[face_uv_idx * 2]);
		v3_copy(vert->normal, &tmp_normals[face_norm_idx * 3]);
	}

	arena_destroy(&tmp_arena);
}

void prepare_texture_asset(TextureInfo* info, TextureAsset* asset) {
	printf("Preparing texture asset '%s'...\n", info->filename);
	u32 w, h, channels;
	stbi_uc* stb_pixels = stbi_load(info->filename, &w, &h, &channels, STBI_rgb_alpha);
	assert(stb_pixels != NULL);
	assert(channels == 4);
	assert(w == 256 && h == 256);
	asset->width = w;
	asset->height = h;

	u64 buffer_size = sizeof(u32) * w * h;
	memcpy(asset->buffer, stb_pixels, buffer_size);
	stbi_image_free(stb_pixels);
}

void prepare_render_program_asset(RenderProgramInfo* info, RenderProgramAsset* asset) {
	printf("Preparing render program asset (vert: '%s', frag: '%s')...\n", info->vert_filename, info->frag_filename);

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

void push_asset_info(AssetInfoList* list, AssetType type, char* manifest_name, void* data) {
	char* asset_type_to_handle_prefix[NUM_ASSET_TYPES];
	asset_type_to_handle_prefix[ASSET_TYPE_MESH] = "MESH";
	asset_type_to_handle_prefix[ASSET_TYPE_TEXTURE] = "TEXTURE";
	asset_type_to_handle_prefix[ASSET_TYPE_RENDER_PROGRAM] = "RENDER_PROGRAM";
	asset_type_to_handle_prefix[ASSET_TYPE_FONT] = "FONT";

	AssetInfo* info = &list->infos[list->len];
	list->len++;
	info->type = type;
	info->data = data;
	sprintf(info->handle, "ASSET_%s_%s", asset_type_to_handle_prefix[type], manifest_name);

	// Getting the asset size in the buffer is specific to each asset type
	switch(type) {
		case ASSET_TYPE_MESH: {
			MeshInfo* mesh_data = (MeshInfo*)data;
			FILE* mesh_file = fopen(mesh_data->filename, "r");
			printf("%s\n", mesh_data->filename);
			assert(mesh_file != NULL);

			u32 vertex_lines = 0;
			u32 uv_lines = 0;
			u32 normal_lines = 0;
			u32 face_lines = 0;

			char line[512];
			while(fgets(line, sizeof(line), mesh_file)) {
				char key[32];
				u32 char_i = 0;
				consume_word(key, line, &char_i);
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
			// Three vertices per face. This is in the flat shading case without an index
			// buffer. With an index buffer, the number of vertices would probably be
			// equal to vertex_lines.
			// 
			// TODO: Just don't even have an index buffer in the flat shading case,
			// obviously. Wait to make this change until after we get this reimplemented.
			mesh_data->vertices_len = face_lines * 3;
			mesh_data->indices_len = face_lines * 3;
			info->size_in_buffer = sizeof(MeshAsset) + sizeof(MeshVertexData) * mesh_data->vertices_len + sizeof(u32) * mesh_data->indices_len;
		} break;
		case ASSET_TYPE_TEXTURE: {
			TextureInfo* tex_data = (TextureInfo*)data;
			switch(tex_data->source_type) {
				case TEXTURE_SOURCE_IMAGE: {
					u32 w, h, channels;
					stbi_uc* stb_pixels = stbi_load(tex_data->filename, &w, &h, &channels, STBI_rgb_alpha);
					assert(stb_pixels != NULL);
					assert(channels == 4);
					info->size_in_buffer = sizeof(TextureAsset) + TEXTURE_PIXEL_STRIDE_BYTES * w * h;
				} break;
				case TEXTURE_SOURCE_FONT: {
					// NOW: implement font size calculation.
					info->size_in_buffer = 0;
					panic();
				} break;
				default: break;
			}
		} break;
		case ASSET_TYPE_RENDER_PROGRAM: {
			RenderProgramInfo* rp_info = (RenderProgramInfo*)data;

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

			info->size_in_buffer = sizeof(RenderProgramAsset) + sizeof(char) * vert_len + sizeof(char) * frag_len;
		} break;
		case ASSET_TYPE_FONT: {
			FontInfo* font_info = (FontInfo*)data;
			// NOW: implement font asset size caclulation.
		} break;
		default: break;
	}

	list->total_buffer_size += info->size_in_buffer;
}

void pack_assets() {
	// Process asset manifest into a list of assets along with their sizes in the
	// final asset pack data buffer. The list of assets in the manifest isn't 1:1
	// with the final list in the pack. For instance, manifest fonts create both a
	// "font" asset and a texture asset.
	Arena arena;
	arena_init(&arena, MEGABYTE * 32, NULL, "AssetInfo");
	AssetInfoList infos_list;
	infos_list.len = 0;

	FILE* manifest = fopen(MANIFEST_FILENAME, "r");
	assert(manifest != NULL);
	u32 info_i = 0;
	u32 line_i = 0;
	char line[2048];
	while(fgets(line, sizeof(line), manifest)) {
		i32 char_i = 0;

		char manifest_key[64];
		consume_word(manifest_key, line, &char_i);

		char manifest_name[64];
		consume_word(manifest_name, line, &char_i);

		if(strcmp(manifest_key, "mesh") == 0) {
			MeshInfo* info = (MeshInfo*)arena_alloc(&arena, sizeof(MeshInfo));
			consume_word(info->filename, line, &char_i);
			printf("%s\n", info->filename);
			push_asset_info(&infos_list, ASSET_TYPE_MESH, manifest_name, info);
		} else if(strcmp(manifest_key, "texture") == 0) {
			TextureInfo* info = (TextureInfo*)arena_alloc(&arena, sizeof(TextureInfo));
			info->source_type = TEXTURE_SOURCE_IMAGE;
			consume_word(info->filename, line, &char_i);
			push_asset_info(&infos_list, ASSET_TYPE_TEXTURE, manifest_name, info);
		} else if(strcmp(manifest_key, "render_program") == 0) {
			RenderProgramInfo* info = (RenderProgramInfo*)arena_alloc(&arena, sizeof(RenderProgramInfo));
			consume_word(info->vert_filename, line, &char_i);
			consume_word(info->frag_filename, line, &char_i);
			push_asset_info(&infos_list, ASSET_TYPE_RENDER_PROGRAM, manifest_name, info);
		} else if(strcmp(manifest_key, "font") == 0) {
			char filename[256];
			consume_word(filename, line, &char_i);

			char font_size_str[64];
			consume_word(font_size_str, line, &char_i);
			u32 font_size = atoi(font_size_str);
			
			TextureInfo* tex_info = (TextureInfo*)arena_alloc(&arena, sizeof(TextureInfo));
			tex_info->source_type = TEXTURE_SOURCE_FONT;
			tex_info->font_size = font_size;
			strcpy(tex_info->filename, filename);
			push_asset_info(&infos_list, ASSET_TYPE_TEXTURE, manifest_name, tex_info);

			FontInfo* font_info = (FontInfo*)arena_alloc(&arena, sizeof(FontInfo));
			font_info->font_size = font_size;
			strcpy(font_info->filename, filename);
			push_asset_info(&infos_list, ASSET_TYPE_FONT, manifest_name, font_info);
		} else {
			printf("Asset manifest key '%s' at line %u not recognized!\n", line_i);
		}
		line_i++;
	}

	// Calculate the size of the asset pack buffer and store asset buffer offsets
	AssetPack* pack = (AssetPack*)arena_alloc(&arena, sizeof(AssetPack) + infos_list.total_buffer_size);
	u64 buffer_pos = 0;

	u8* asset_counts[NUM_ASSET_TYPES];
	asset_counts[ASSET_TYPE_MESH] = &pack->meshes_len;
	asset_counts[ASSET_TYPE_TEXTURE] = &pack->textures_len;
	asset_counts[ASSET_TYPE_RENDER_PROGRAM] = &pack->render_programs_len;
	asset_counts[ASSET_TYPE_FONT] = &pack->fonts_len;

	u64* asset_offset_lists[NUM_ASSET_TYPES];
	asset_offset_lists[ASSET_TYPE_MESH] = pack->mesh_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_TEXTURE] = pack->texture_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_RENDER_PROGRAM] = pack->render_program_buffer_offsets;
	asset_offset_lists[ASSET_TYPE_FONT] = pack->font_buffer_offsets;

	for(i32 i = 0; i < infos_list.len; i++) {
		AssetInfo* info = &infos_list.infos[i];
		(asset_offset_lists[info->type])[*(asset_counts[info->type])] = buffer_pos;
		*asset_counts[info->type] += 1;

		switch(info->type) {
			case ASSET_TYPE_MESH: {
				prepare_mesh_asset((MeshInfo*)info->data, (MeshAsset*)&pack->buffer[buffer_pos]);
			} break;
			case ASSET_TYPE_TEXTURE: {
				prepare_texture_asset((TextureInfo*)info->data, (TextureAsset*)&pack->buffer[buffer_pos]);
			} break;
			case ASSET_TYPE_RENDER_PROGRAM: {
				prepare_render_program_asset((RenderProgramInfo*)info->data, (RenderProgramAsset*)&pack->buffer[buffer_pos]);
			} break;
			case ASSET_TYPE_FONT: {
				panic();
				// TODO: implement font loading
				//prepare_font_asset((FontInfo*)info->data, (FontAsset*)&pack->buffer[buffer_pos]);
			} break;
			default: break;
		}

		buffer_pos += info->size_in_buffer;
	}

	FILE* out_file = fopen("../bin/data/assets.pack", "w");
	assert(out_file != NULL);

	fwrite(pack, sizeof(AssetPack) + infos_list.total_buffer_size, 1, out_file);
	fclose(out_file);

	/* FOR DEBUG PURPOSES ONLY:
	for(i32 i = 0; i < pack->meshes_len; i++) {
		printf("mesh offset: %u\n", pack->mesh_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->textures_len; i++) {
		printf("texture offset: %u\n", pack->texture_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->render_programs_len; i++) {
		printf("render_program offset: %u\n", pack->render_program_buffer_offsets[i]);
	}
	for(i32 i = 0; i < pack->fonts_len; i++) {
		printf("font offset: %u\n", pack->font_buffer_offsets[i]);
	}

	printf("meshes len: %u\n", pack->meshes_len);
	printf("textures len: %u\n", pack->textures_len);
	printf("render_programs len: %u\n", pack->render_programs_len);
	printf("fonts len: %u\n", pack->fonts_len);
	*/
}
