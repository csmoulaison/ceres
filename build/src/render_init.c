#include "renderer/renderer.h"
#include "mesh_loader.c"

void build_render_init_data() {
	data->meshes_len = 2;
	data->meshes = (RenderMeshInitData*)arena_alloc(&arena, sizeof(RenderMeshInitData) * data->meshes_len);
	char* mesh_filenames[2] = { "data/meshes/ship.obj", "data/meshes/floor.obj" };
	for(i32 i = 0; i < data->meshes_len; i++) {
		MeshData* mesh_data = (MeshData*)arena_alloc(&arena, sizeof(MeshData));
		load_mesh(mesh_data, mesh_filenames[i], &arena, true);

		RenderMeshInitData* mesh = &data->meshes[i];
		mesh->flat_shading = mesh_data->flat_shading;

		mesh->vertex_attributes_len = 3;
		mesh->vertex_attribute_sizes[0] = 3;
		mesh->vertex_attribute_sizes[1] = 3;
		mesh->vertex_attribute_sizes[2] = 2;
		u32 total_vertex_size = 8;

		mesh->vertices_len = mesh_data->vertices_len;
		mesh->vertex_data = (f32*)arena_alloc(&arena, sizeof(f32) * total_vertex_size * mesh->vertices_len);
		for(i32 i = 0; i < total_vertex_size * mesh->vertices_len; i++) {
			mesh->vertex_data[i] = mesh_data->vertices[i / total_vertex_size].data[i % total_vertex_size];
		}

		mesh->indices_len = mesh_data->indices_len;
		mesh->indices = (u32*)arena_alloc(&arena, sizeof(u32) * mesh->indices_len);
		for(i32 i = 0; i < mesh->indices_len; i++) {
			mesh->indices[i] = mesh_data->indices[i];
		}

		if(i == data->meshes_len - 1) {
			mesh->next = NULL;
		} else {
			mesh->next = &data->meshes[i + 1];
		}
	}

	data->textures_len = 2;
	data->textures = (RenderTextureInitData*)arena_alloc(&arena, sizeof(RenderTextureInitData) * data->textures_len);
	char* texture_filenames[2] = { "data/textures/ship.tex", "data/textures/metal.tex" };
	for(i32 i = 0; i < data->textures_len; i++) {
		FILE* file = fopen(texture_filenames[i], "r");
		assert(file != NULL);
		u32 dimensions[2];
		fread(dimensions, sizeof(u32), 2, file);

		RenderTextureInitData* texture = &data->textures[i];
		texture->width = dimensions[0];
		texture->height = dimensions[1];

		u64 pixel_data_size = sizeof(32) * texture->width * texture->height;
		texture->pixel_data = (u8*)arena_alloc(&arena, pixel_data_size);
		assert(fread(texture->pixel_data, pixel_data_size, 1, file) == 1);

		fclose(file);

		if(i == data->textures_len - 1) {
			texture->next = NULL;
		} else {
			texture->next = &data->textures[i + 1];
		}
	}

	data->ubos_len = 2;
	data->ubos = (RenderUboInitData*)arena_alloc(&arena, sizeof(RenderUboInitData) * data->ubos_len);
	for(i32 i = 0; i < data->ubos_len; i++) {
		RenderUboInitData* ubo = &data->ubos[i];

		switch(i) {
			case RENDER_UBO_WORLD: {
				ubo->size = sizeof(f32) * 20;
				ubo->binding = 0;
			} break;
			case RENDER_UBO_INSTANCE: {
				ubo->size = sizeof(f32) * 16;
				ubo->binding = 1;
			} break;
			default: break;
		}

		if(i == data->ubos_len - 1) {
			ubo->next = NULL;
		} else {
			ubo->next = &data->ubos[i + 1];
		}
	}

	data->host_buffers_len = 2;
	data->host_buffers = (RenderHostBufferInitData*)arena_alloc(&arena, sizeof(RenderHostBufferInitData) * data->host_buffers_len);
	data->host_buffers[0].next = &data->host_buffers[1];
	data->host_buffers[1].next = NULL;

	arena_destroy(&arena);
	return data;
}
