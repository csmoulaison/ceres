#include "GL/gl3w.h"
#include "renderer/renderer.h"

typedef struct {

} OpenGl;

u32 opengl_compile_shader(const char* filename, GLenum type) {
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

Renderer* opengl_init(RenderInitData* data, Arena* render_arena, Arena* init_arena) {
	Renderer* renderer = (Renderer*)arena_alloc(render_arena, sizeof(Renderer));
	arena_init(&renderer->persistent_arena, RENDER_PERSISTENT_ARENA_SIZE, render_arena);
	arena_init(&renderer->viewport_arena, RENDER_VIEWPORT_ARENA_SIZE, render_arena);
	arena_init(&renderer->frame_arena, RENDER_FRAME_ARENA_SIZE, render_arena);

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
		u32 vert_shader = opengl_compile_shader(program_data->vertex_shader_filename, GL_VERTEX_SHADER);
		u32 frag_shader = opengl_compile_shader(program_data->fragment_shader_filename, GL_FRAGMENT_SHADER);

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
		//renderer->meshes[renderer->meshes_len] = platform_render_create_mesh(renderer, mesh_data);
		renderer->meshes_len++;
		mesh_data = mesh_data->next;
	}
	assert(renderer->meshes_len == data->meshes_len);

	renderer->textures = (RenderTexture*)arena_alloc(&renderer->persistent_arena, sizeof(RenderTexture) * data->textures_len);
	RenderTextureInitData* texture_data = data->textures;
	while(texture_data != NULL) {
		//renderer->textures[renderer->textures_len] = platform_render_create_texture(renderer, texture_data);
		renderer->textures_len++;
		texture_data = texture_data->next;
	}
	assert(renderer->textures_len == data->textures_len);

	renderer->ubos = (RenderUbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderUbo) * data->ubos_len);
	RenderUboInitData* ubo_data = data->ubos;
	while(ubo_data != NULL) {
		//renderer->ubos[renderer->ubos_len] = platform_render_create_ubo(renderer, ubo_data);
		renderer->ubos_len++;
		ubo_data = ubo_data->next;
	}
	assert(renderer->ubos_len == data->ubos_len);

	renderer->ssbos = (RenderSsbo*)arena_alloc(&renderer->persistent_arena, sizeof(RenderSsbo) * data->ssbos_len);
	RenderSsboInitData* ssbo_data = data->ssbos;
	while(ssbo_data != NULL) {
		//renderer->ssbos[renderer->ssbos_len] = platform_render_create_ssbo(renderer, ssbo_data);
		renderer->ssbos_len++;
		ssbo_data = ssbo_data->next;
	}
	assert(renderer->ssbos_len == data->ssbos_len);

	return renderer;
}

void opengl_update(Renderer* renderer, Platform* platform) {
	if(platform->viewport_update_requested) {
		glViewport(0, 0, platform->window_width, platform->window_height);
		platform->viewport_update_requested = false;
	}
}
