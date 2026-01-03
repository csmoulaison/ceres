#include "GL/gl3w.h"
#include "renderer/renderer.h"

typedef struct {

} OpenGl;

u32 gl_compile_shader(const char* filename, GLenum type) {
	// Read file
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

	// Compile shader
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
	arena_init(&renderer->persistent_arena, RENDER_PERSISTENT_ARENA_SIZE, render_arena, "RenderPersistent");
	arena_init(&renderer->viewport_arena, RENDER_VIEWPORT_ARENA_SIZE, render_arena, "RenderViewport");
	arena_init(&renderer->frame_arena, RENDER_FRAME_ARENA_SIZE, render_arena, "RenderFrame");

	renderer->backend = arena_alloc(&renderer->persistent_arena, sizeof(OpenGl));
	OpenGl* gl = (OpenGl*)renderer->backend;

	if(gl3wInit() != 0) { panic(); }

	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	renderer->programs = (RenderProgram*)arena_alloc(&renderer->persistent_arena, sizeof(RenderProgram) * data->programs_len);
	RenderProgramInitData* program_data = data->programs;
	while(program_data != NULL) {
		u32 vert_shader = gl_compile_shader(program_data->vertex_shader_filename, GL_VERTEX_SHADER);
		u32 frag_shader = gl_compile_shader(program_data->fragment_shader_filename, GL_FRAGMENT_SHADER);

		RenderProgram* program = &renderer->programs[renderer->programs_len];
		*program = glCreateProgram();
		glAttachShader(*program, vert_shader);
		glAttachShader(*program, frag_shader);
		glLinkProgram(*program);
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);

		renderer->programs_len++;
		program_data = program_data->next;
	}
	assert(renderer->programs_len == data->programs_len);

	renderer->meshes = (RenderMesh*)arena_alloc(&renderer->persistent_arena, sizeof(RenderMesh) * data->meshes_len);
	RenderMeshInitData* mesh_data = data->meshes;
	while(mesh_data != NULL) {
		RenderMesh* mesh = &renderer->meshes[renderer->meshes_len];
		glGenVertexArrays(1, (u32*)mesh);
		glBindVertexArray(*mesh);

		u32 vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, mesh_data->vertices_len * mesh_data->vertex_size, mesh_data->vertex_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		// NOW: NULL instead of '(void*)0' works just as well here?
		glVertexAttribPointer(0, mesh_data->vertex_size, GL_FLOAT, GL_FALSE, sizeof(f32) * mesh_data->vertex_size, (void*)0);
		
		renderer->meshes_len++;
		mesh_data = mesh_data->next;
	}
	assert(renderer->meshes_len == data->meshes_len);

	renderer->textures = (RenderTexture*)arena_alloc(&renderer->persistent_arena, sizeof(RenderTexture) * data->textures_len);
	RenderTextureInitData* texture_data = data->textures;
	while(texture_data != NULL) {
		RenderTexture* texture = &renderer->textures[renderer->textures_len];
		glGenTextures(1, (u32*)texture);
		glBindTexture(GL_TEXTURE_2D, *texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texture_data->width, texture_data->height, 0, GL_RED, GL_UNSIGNED_BYTE, texture_data->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		renderer->textures_len++;
		texture_data = texture_data->next;
	}
	assert(renderer->textures_len == data->textures_len);

	renderer->ubos = (RenderUbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderUbo) * data->ubos_len);
	RenderUboInitData* ubo_data = data->ubos;
	while(ubo_data != NULL) {
		RenderUbo* ubo = &renderer->ubos[renderer->ubos_len];
		glGenBuffers(1, (u32*)ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, *ubo);
		glBufferData(GL_UNIFORM_BUFFER, ubo_data->size, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		renderer->ubos_len++;
		ubo_data = ubo_data->next;
	}
	assert(renderer->ubos_len == data->ubos_len);

	renderer->ssbos = (RenderSsbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderSsbo) * data->ssbos_len);
	RenderSsboInitData* ssbo_data = data->ssbos;
	while(ssbo_data != NULL) {
		RenderSsbo* ssbo = &renderer->ssbos[renderer->ssbos_len];
		glGenBuffers(1, (u32*)ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, ssbo_data->size, NULL, GL_DYNAMIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, *ssbo);

		renderer->ssbos_len++;
		ssbo_data = ssbo_data->next;
	}
	assert(renderer->ssbos_len == data->ssbos_len);

	glViewport(0, 0, 1, 1);
	return renderer;
}

void gl_update(Renderer* renderer, Platform* platform) {
	if(platform->viewport_update_requested) {
		glViewport(0, 0, platform->window_width, platform->window_height);
		platform->viewport_update_requested = false;
	}

	RenderCommand* cmd = renderer->graph.root;
	while(cmd != NULL) {
		switch(cmd->type) {
			case RENDER_COMMAND_NULL:
				break;
			case RENDER_COMMAND_CLEAR: {
				assert(cmd->data != NULL);
				f32* comps = (f32*)cmd->data;
				glClearColor(comps[0], comps[1], comps[2], comps[3]);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			} break;
			default: break;
		}
		cmd = cmd->next;
	}
}
