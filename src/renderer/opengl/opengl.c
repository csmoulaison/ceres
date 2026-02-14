#include "GL/gl3w.h"
#include "renderer/renderer.h"

#define GL_BUFFER_MEMSIZE MEGABYTE * 2

typedef struct {
	u32 id;
} GlProgram;

typedef struct {
	u32 vao;
	u32 ebo;
	u32 vertices_len;
	u32 indices_len;
} GlMesh;

typedef struct {
	u32 id;
} GlTexture;

typedef struct {
	u32 id;
	u32 size;
	u32 binding;
} GlUbo;

typedef struct {
	u32 id;
	u32 size;
	u32 binding;
} GlSsbo;

typedef struct {
	GlProgram* programs;
	GlMesh* meshes;
	GlTexture* textures;
	GlUbo* ubos;
	GlSsbo* ssbos;
	u8 buffer[GL_BUFFER_MEMSIZE];
} GlMemory;

u32 gl_compile_shader(const char* src, i32 src_len,  GLenum type) {
	u32 shader = glCreateShader(type);
	const char* src_ptr = src;
	glShaderSource(shader, 1, &src_ptr, &src_len);
	glCompileShader(shader);

	i32 success;
	char info[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(success == false) {
		glGetShaderInfoLog(shader, 512, NULL, info);
		printf(info);
		panic();
	}
	return shader;
}

void gl_init_buffer(u32* id, u64 size, GLenum target, GLenum usage) {
	glGenBuffers(1, id);
	glBindBuffer(target, *id);
	glBufferData(target, size, NULL, usage);
	glBindBuffer(target, 0);
}

void gl_init(RenderMemory* renderer, RenderInitMemory* init) {
	GlMemory* gl = (GlMemory*)renderer->backend;

	if(gl3wInit() != 0) { panic(); }
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//glDisable(GL_CULL_FACE);
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	StackAllocator buffer_stack = stack_init(gl->buffer, GL_BUFFER_MEMSIZE, "GlBuffer");
	gl->programs = (GlProgram*)stack_alloc(&buffer_stack, sizeof(GlProgram) * init->programs_len);
	gl->meshes = (GlMesh*)stack_alloc(&buffer_stack, sizeof(GlMesh) * init->meshes_len);
	gl->textures = (GlTexture*)stack_alloc(&buffer_stack, sizeof(GlTexture) * init->textures_len);
	gl->ubos = (GlUbo*)stack_alloc(&buffer_stack, sizeof(GlUbo) * init->ubos_len);
	gl->ssbos = (GlSsbo*)stack_alloc(&buffer_stack, sizeof(GlSsbo) * init->ssbos_len);

	RenderProgramInitData* program_data = init->programs;
	i32 program_index = 0;
	while(program_data != NULL) {
		u32 vert_shader = gl_compile_shader(program_data->vertex_shader_src, program_data->vertex_shader_src_len, GL_VERTEX_SHADER);
		u32 frag_shader = gl_compile_shader(program_data->fragment_shader_src, program_data->fragment_shader_src_len, GL_FRAGMENT_SHADER);

		GlProgram* program = &gl->programs[program_index];
		program->id = glCreateProgram();
		glAttachShader(program->id, vert_shader);
		glAttachShader(program->id, frag_shader);
		glLinkProgram(program->id);
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);

		program_data = program_data->next;
		program_index++;
	}

	RenderMeshInitData* mesh_data = init->meshes;
	i32 mesh_index = 0;
	while(mesh_data != NULL) {
		GlMesh* mesh = &gl->meshes[mesh_index];
		glGenVertexArrays(1, &mesh->vao);
		glBindVertexArray(mesh->vao);

		u32 vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		u32 vertex_size = 0;
		for(i32 i = 0; i < mesh_data->vertex_attributes_len; i++) {
			vertex_size += mesh_data->vertex_attribute_sizes[i];
		}
		u32 cur_offset = 0;
		for(i32 i = 0; i < mesh_data->vertex_attributes_len; i++) {
			u32 attribute_size = mesh_data->vertex_attribute_sizes[i];
			glVertexAttribPointer(i, attribute_size, GL_FLOAT, GL_FALSE, sizeof(f32) * vertex_size, (void*)(cur_offset * sizeof(f32))); 
			glEnableVertexAttribArray(i);
			cur_offset += attribute_size;
		}
		glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * mesh_data->vertices_len * vertex_size, mesh_data->vertex_data, GL_STATIC_DRAW);

		glGenBuffers(1, &mesh->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * mesh_data->indices_len, mesh_data->indices, GL_STATIC_DRAW);
		mesh->vertices_len = mesh_data->vertices_len;
		mesh->indices_len = mesh_data->indices_len;
		//mesh->flat_shading = mesh_data->flat_shading;

		mesh_data = mesh_data->next;
		mesh_index++;
	}

	RenderTextureInitData* texture_data = init->textures;
	i32 texture_index = 0;
	while(texture_data != NULL) {
		GlTexture* texture = &gl->textures[texture_index];
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);

		GLint format;
		if(texture_data->channel_count == 1) {
			format = GL_RED;
		} else if(texture_data->channel_count == 4) {
			format = GL_RGBA;
		} else {
			printf("Channel count %u in texture data not supported.\n", texture_data->channel_count);
			panic();
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, format, texture_data->width, texture_data->height, 0, format, GL_UNSIGNED_BYTE, texture_data->pixel_data);
		glGenerateMipmap(GL_TEXTURE_2D);

		texture_data = texture_data->next;
		texture_index++;
	}

	RenderUboInitData* ubo_data = init->ubos;
	u32 ubo_index = 0;
	while(ubo_data != NULL) {
		GlUbo* ubo = &gl->ubos[ubo_index];
		ubo->binding = ubo_data->binding;
		ubo->size = ubo_data->size;
		gl_init_buffer(&ubo->id, ubo_data->size, GL_UNIFORM_BUFFER, GL_DYNAMIC_DRAW);
		ubo_data = ubo_data->next;
		ubo_index++;
	}

	RenderSsboInitData* ssbo_data = init->ssbos;
	u32 ssbo_index = 0;
	while(ssbo_data != NULL) {
		GlSsbo* ssbo = &gl->ssbos[ssbo_index];
		ssbo->binding = ssbo_data->binding;
		ssbo->size = ssbo_data->size;
		gl_init_buffer(&ssbo->id, ssbo_data->size, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW);
		ssbo_data = ssbo_data->next;
		ssbo_index++;
	}

	glViewport(0, 0, 1, 1);
}

void gl_update(RenderMemory* renderer, Platform* platform) {
	GlMemory* gl = (GlMemory*)renderer->backend;
	
	if(platform->viewport_update_requested) {
		glViewport(0, 0, platform->window_width, platform->window_height);
		platform->viewport_update_requested = false;
	}

	RenderCommand* cmd = renderer->commands.root;
	printf("here!\n");
	i32 count = 0;
	while(cmd != NULL) {
		printf("  count %i\n", count);
		assert(cmd->data != NULL);
		printf("    good\n");
		count++;
		switch(cmd->type) {
			case RENDER_COMMAND_CLEAR: {
				RenderCommandClear* data = (RenderCommandClear*)cmd->data;
				glClearColor(data->color[0], data->color[1], data->color[2], data->color[3]);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			} break;
			case RENDER_COMMAND_SET_VIEWPORT: {
				RenderCommandSetViewport* data = (RenderCommandSetViewport*)cmd->data;
				glViewport(data->rect.x, data->rect.y, data->rect.z, data->rect.w);
			} break;
			case RENDER_COMMAND_USE_PROGRAM: {
				RenderCommandUseProgram* data = (RenderCommandUseProgram*)cmd->data;
				glUseProgram(gl->programs[data->program].id);
			} break;
			case RENDER_COMMAND_USE_UBO: {
				RenderCommandUseUbo* data = (RenderCommandUseUbo*)cmd->data;
				GlUbo* ubo = &gl->ubos[data->ubo];
				glBindBufferBase(GL_UNIFORM_BUFFER, ubo->binding, ubo->id);
			} break;
			case RENDER_COMMAND_USE_SSBO: {
				RenderCommandUseSsbo* data = (RenderCommandUseSsbo*)cmd->data;
				GlSsbo* ssbo = &gl->ssbos[data->ssbo];
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo->binding, ssbo->id);
			} break;
			case RENDER_COMMAND_USE_TEXTURE: {
				RenderCommandUseTexture* data = (RenderCommandUseTexture*)cmd->data;
				glBindTexture(GL_TEXTURE_2D, gl->textures[data->texture].id);
			} break;
			case RENDER_COMMAND_BUFFER_UBO_DATA: {
				RenderCommandBufferUboData* data = (RenderCommandBufferUboData*)cmd->data;
				GlUbo* ubo = &gl->ubos[data->ubo];
				u8* host_buffer = renderer->host_buffers[data->host_buffer_index];
				assert(host_buffer != NULL);

				glBindBuffer(GL_UNIFORM_BUFFER, ubo->id);
				void* mapped_buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
				memcpy(mapped_buffer, host_buffer + data->host_buffer_offset, ubo->size);
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			} break;
			case RENDER_COMMAND_BUFFER_SSBO_DATA: {
				RenderCommandBufferSsboData* data = (RenderCommandBufferSsboData*)cmd->data;
				GlSsbo* ssbo = &gl->ssbos[data->ssbo];
				u8* host_buffer = renderer->host_buffers[data->host_buffer_index];
				assert(host_buffer != NULL);

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo->id);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data->size, host_buffer + data->host_buffer_offset);
			} break;
			case RENDER_COMMAND_DRAW_MESH: {
				RenderCommandDrawMesh* data = (RenderCommandDrawMesh*)cmd->data;
				GlMesh* mesh = &gl->meshes[data->mesh];
				glBindVertexArray(mesh->vao);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
				glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_len);
				//glDrawElements(GL_TRIANGLES, mesh->indices_len, GL_UNSIGNED_INT, 0);
			} break;
			case RENDER_COMMAND_DRAW_MESH_INSTANCED: {
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				
				RenderCommandDrawMeshInstanced* data = (RenderCommandDrawMeshInstanced*)cmd->data;
				GlMesh* mesh = &gl->meshes[data->mesh];
				glBindVertexArray(mesh->vao);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
				// TODO: take out magic number of 6. I believe its the number of vertex attributes * lens
				glDrawArraysInstanced(GL_TRIANGLES, 0, mesh->vertices_len, data->count);
				glDisable(GL_BLEND);
			} break;
			default: break;
		}
		cmd = cmd->next;
	}
	renderer->frames_since_init++;
}
