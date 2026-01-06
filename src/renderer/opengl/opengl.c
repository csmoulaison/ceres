#include "GL/gl3w.h"
#include "renderer/renderer.h"

typedef struct {
	u32 id;
} GlProgram;

typedef struct {
	u32 vao;
	u32 vertices_len;
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
	GlProgram* programs;
	GlMesh* meshes;
	GlTexture* textures;
	GlUbo* ubos;
} OpenGl;

u32 gl_compile_shader(const char* filename, GLenum type) {
	FILE* file = fopen(filename, "r");
	if(file == NULL) { panic(); }
	fseek(file, 0, SEEK_END);
	u32 fsize = ftell(file);
	fseek(file, 0, SEEK_SET);
	char src[fsize];
	char c;
	u32 i = 0;
	while((c = fgetc(file)) != EOF) {
		src[i] = c;
		i++;
	}
	src[i] = '\0';
	fclose(file);

	u32 shader = glCreateShader(type);
	const char* src_ptr = src;
	glShaderSource(shader, 1, &src_ptr, 0);
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

Renderer* gl_init(RenderInitData* data, Arena* render_arena, Arena* init_arena) {
	Renderer* renderer = (Renderer*)arena_alloc(render_arena, sizeof(Renderer));
	renderer->frames_since_init = 0;

	arena_init(&renderer->persistent_arena, RENDER_PERSISTENT_ARENA_SIZE, render_arena, "RenderPersistent");
	arena_init(&renderer->viewport_arena, RENDER_VIEWPORT_ARENA_SIZE, render_arena, "RenderViewport");
	arena_init(&renderer->frame_arena, RENDER_FRAME_ARENA_SIZE, render_arena, "RenderFrame");

	printf("allocating opengl\n");
	renderer->backend = arena_alloc(&renderer->persistent_arena, sizeof(OpenGl));
	OpenGl* gl = (OpenGl*)renderer->backend;

	if(gl3wInit() != 0) { panic(); }

	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	printf("allocating renderer resources\n");
	if(data->programs_len > 0)
		renderer->programs = (RenderProgram*)arena_alloc(&renderer->persistent_arena, sizeof(RenderProgram) * data->programs_len);
	if(data->meshes_len > 0)
		renderer->meshes = (RenderMesh*)arena_alloc(&renderer->persistent_arena, sizeof(RenderMesh) * data->meshes_len);
	if(data->textures_len > 0)
		renderer->textures = (RenderTexture*)arena_alloc(&renderer->persistent_arena, sizeof(RenderTexture) * data->textures_len);
	if(data->ubos_len > 0)
		renderer->ubos = (RenderUbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderUbo) * data->ubos_len);
	if(data->host_buffers_len > 0)
		renderer->host_buffers = (RenderHostBuffer*)arena_alloc(&renderer->persistent_arena, sizeof(RenderHostBuffer) * data->host_buffers_len);

	printf("allocating gl resources\n");
	if(data->programs_len > 0)
		gl->programs = (GlProgram*)arena_alloc(&renderer->persistent_arena, sizeof(GlProgram) * data->programs_len);
	if(data->meshes_len > 0)
		gl->meshes = (GlMesh*)arena_alloc(&renderer->persistent_arena, sizeof(GlMesh) * data->meshes_len);
	if(data->textures_len > 0)
		gl->textures = (GlTexture*)arena_alloc(&renderer->persistent_arena, sizeof(GlTexture) * data->textures_len);
	if(data->ubos_len > 0)
		gl->ubos = (GlUbo*)arena_alloc(&renderer->persistent_arena, sizeof(GlUbo) * data->ubos_len);

	printf("resources allocated\n");

	RenderProgramInitData* program_data = data->programs;
	while(program_data != NULL) {
		u32 vert_shader = gl_compile_shader(program_data->vertex_shader_filename, GL_VERTEX_SHADER);
		u32 frag_shader = gl_compile_shader(program_data->fragment_shader_filename, GL_FRAGMENT_SHADER);

		GlProgram* program = &gl->programs[renderer->programs_len];
		program->id = glCreateProgram();
		glAttachShader(program->id, vert_shader);
		glAttachShader(program->id, frag_shader);
		glLinkProgram(program->id);
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);

		renderer->programs[renderer->programs_len] = renderer->programs_len;
		renderer->programs_len++;
		program_data = program_data->next;
	}
	assert(renderer->programs_len == data->programs_len);

	RenderMeshInitData* mesh_data = data->meshes;
	while(mesh_data != NULL) {
		GlMesh* mesh = &gl->meshes[renderer->meshes_len];
		glGenVertexArrays(1, &mesh->vao);
		glBindVertexArray(mesh->vao);
		mesh->vertices_len = mesh_data->vertices_len;

		u32 vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * mesh_data->vertices_len * mesh_data->vertex_size, mesh_data->vertex_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		// NOW: NULL instead of '(void*)0' works just as well here?
		glVertexAttribPointer(0, mesh_data->vertex_size, GL_FLOAT, GL_FALSE, sizeof(f32) * mesh_data->vertex_size, (void*)0);
		
		renderer->meshes[renderer->meshes_len] = renderer->meshes_len;
		renderer->meshes_len++;
		mesh_data = mesh_data->next;
	}
	assert(renderer->meshes_len == data->meshes_len);

	RenderTextureInitData* texture_data = data->textures;
	while(texture_data != NULL) {
		GlTexture* texture = &gl->textures[renderer->textures_len];
		glGenTextures(1, &texture->id);
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texture_data->width, texture_data->height, 0, GL_RED, GL_UNSIGNED_BYTE, texture_data->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		renderer->textures[renderer->textures_len] = renderer->textures_len;
		renderer->textures_len++;
		texture_data = texture_data->next;
	}
	assert(renderer->textures_len == data->textures_len);

	RenderUboInitData* ubo_data = data->ubos;
	while(ubo_data != NULL) {
		GlUbo* ubo = &gl->ubos[renderer->ubos_len];
		ubo->binding = ubo_data->binding;
		ubo->size = ubo_data->size;

		glGenBuffers(1, &ubo->id);
		glBindBuffer(GL_UNIFORM_BUFFER, ubo->id);
		glBufferData(GL_UNIFORM_BUFFER, ubo_data->size, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		renderer->ubos[renderer->ubos_len] = renderer->ubos_len;
		renderer->ubos_len++;
		ubo_data = ubo_data->next;
	}
	assert(renderer->ubos_len == data->ubos_len);

	RenderHostBufferInitData* host_buffer_data = data->host_buffers;
	while(host_buffer_data != NULL) {
		RenderHostBuffer* host_buffer = &renderer->host_buffers[renderer->host_buffers_len];
		host_buffer->id = renderer->host_buffers_len;
		host_buffer->data = NULL;

		renderer->host_buffers_len++;
		host_buffer_data = host_buffer_data->next;
	}
	assert(renderer->host_buffers_len == data->host_buffers_len);

	glViewport(0, 0, 1, 1);
	return renderer;
}

void gl_update(Renderer* renderer, Platform* platform) {
	OpenGl* gl = (OpenGl*)renderer->backend;
	
	if(platform->viewport_update_requested) {
		glViewport(0, 0, platform->window_width, platform->window_height);
		platform->viewport_update_requested = false;
	}

	RenderCommand* cmd = renderer->graph->root;
	while(cmd != NULL) {
		assert(cmd->data != NULL);
		switch(cmd->type) {
			case RENDER_COMMAND_CLEAR: {
				RenderCommandClear* data = (RenderCommandClear*)cmd->data;
				glClearColor(data->color[0], data->color[1], data->color[2], data->color[3]);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
			case RENDER_COMMAND_BUFFER_UBO_DATA: {
				RenderCommandBufferUboData* data = (RenderCommandBufferUboData*)cmd->data;
				GlUbo* ubo = &gl->ubos[data->ubo];
				RenderHostBuffer* host_buffer = &renderer->host_buffers[data->host_buffer_index];
				assert(host_buffer->data != NULL);

				glBindBuffer(GL_UNIFORM_BUFFER, ubo->id);
				void* mapped_buffer = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
				memcpy(mapped_buffer, host_buffer->data, ubo->size);
				glUnmapBuffer(GL_UNIFORM_BUFFER);
			} break;
			case RENDER_COMMAND_DRAW_MESH: {
				RenderCommandDrawMesh* data = (RenderCommandDrawMesh*)cmd->data;
				GlMesh* mesh = &gl->meshes[data->mesh];
				glBindVertexArray(mesh->vao);
				glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_len);
			} break;
			default: break;
		}
		cmd = cmd->next;
	}
	renderer->frames_since_init++;
}
