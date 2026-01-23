/* NOW: We need to implement SSBOs in order to do text rendering.
  
Source from Netpong here:


opengl.cpp:init

	// Quad rendering
	gl->quad_program = gl_create_program("shaders/quad.vert", "shaders/quad.frag");

	f32 quad_vertices[] = {
		 1.0f,  1.0f,
		 1.0f, -1.0f,
		-1.0f,  1.0f,

		 1.0f, -1.0f,
		-1.0f, -1.0f,
		-1.0f,  1.0f
	};

	glGenVertexArrays(1, &gl->quad_vao);
	glBindVertexArray(gl->quad_vao);

	u32 quad_vbo;
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);

	gl->quad_ubo = gl_create_ubo(sizeof(QuadUbo), nullptr);

	// Text rendering
	gl->text_program = gl_create_program("shaders/text.vert", "shaders/text.frag");

	glGenBuffers(1, &gl->text_buffer_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Render::Character) * MAX_RENDER_CHARS, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, gl->text_buffer_ssbo);


opengl.cpp:update

	// Text rendering
	glUseProgram(gl->text_program);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(gl->quad_vao);

	for(u8 i = 0; i < NUM_FONTS; i++) {
		Render::CharacterList* list = &render_state->character_lists[i];
		Render::Font* font = &renderer->fonts[i];

		for(u32 j = 0; j < list->characters_len; j++) {
			Render::Character* character = &list->characters[j];

			character->dst[0] /= window->window_width;
			character->dst[1] /= window->window_height;
			character->dst[0] *= 2.0f;
			character->dst[1] *= 2.0f;
			character->dst[0] -= 1.0f;
			character->dst[1] -= 1.0f;

			character->dst[2] /= window->window_width;
			character->dst[3] /= window->window_height;
			character->dst[2] *= 2.0f;
			character->dst[3] *= 2.0f;
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl->text_buffer_ssbo);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(Render::Character) * list->characters_len, list->characters);
		glBindTexture(GL_TEXTURE_2D, font->texture_id);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, list->characters_len);
	}


renderer.cpp:helpers

	void character(Context* context, char c, float x, float y, float r, float g, float b, float a, FontFace face)
	{
		State* state = &context->current_state;
		CharacterList* list = &state->character_lists[face];
		Character* character = &list->characters[list->characters_len];

		Font* font = &context->fonts[face];
		FontGlyph* glyph = &font->glyphs[c];

		list->characters_len++;

		u32 tex_w = font->texture_width;
		character->src[0] = ((float)glyph->x) / tex_w;
		character->src[1] = ((float)glyph->y) / tex_w;
		character->src[2] = ((float)glyph->w) / tex_w;
		character->src[3] = ((float)glyph->h) / tex_w;

		character->dst[0] = x;
		character->dst[1] = y;
		character->dst[2] = glyph->w;
		character->dst[3] = glyph->h;

		character->color[0] = r;
		character->color[1] = g;
		character->color[2] = b;
		character->color[3] = a;
	}

	// Placements must be preallocated float * string length.
	void text_line_placements(
		Context* context,
		const char* string,
		float* x_placements,
		float* y_placements,
		float x,
		float y,
		float anchor_x,
		float anchor_y,
		FontFace face)
	{
		Font* font = &context->fonts[face];
		
		float line_width = 0;
		float cur_x = x;

		i32 len = strlen(string);
		for(i32 i = 0; i < len; i++) {
			char c = string[i];
			FontGlyph* glyph = &font->glyphs[c];

			x_placements[i] = cur_x + glyph->bearing[0];
			y_placements[i] = y - (glyph->h - glyph->bearing[1]);
	        cur_x += (glyph->advance >> 6);
		}

		float off_x = (cur_x - x) * anchor_x;
		float off_y = font->size * anchor_y;
		for(i32 i = 0; i < len; i++) {
			x_placements[i] -= off_x;
			y_placements[i] -= off_y;

			x_placements[i] = floor(x_placements[i]);
			y_placements[i] = floor(y_placements[i]);
		}
	}

	void text_line(
		Context* context, 
		const char* string, 
		float x, float y, 
		float anchor_x, float anchor_y, 
		float r, float g, float b, float a, 
		FontFace face)
	{
		State* state = &context->current_state;

		i32 len = strlen(string);
		float x_placements[len];
		float y_placements[len];
		text_line_placements(context, string, x_placements, y_placements, x, y, anchor_x, anchor_y, face);
		
		for(i32 i = 0; i < len; i++) {
			char c = string[i];
			FontGlyph* glyph = &context->fonts[face].glyphs[c];
			Render::character(context, c, x_placements[i], y_placements[i], r, g, b, a, face);
		}
	}
*/

#ifndef renderer_h_INCLUDED
#define renderer_h_INCLUDED

#define RENDER_MAX_VERTEX_ATTRIBUTES 16
#define RENDER_MAX_HOST_BUFFERS 32

#define RENDER_PROGRAM_MODEL 0

#define RENDER_UBO_WORLD 0
#define RENDER_UBO_INSTANCE 1

#define RENDER_HOST_BUFFER_WORLD 0
#define RENDER_HOST_BUFFER_INSTANCE 1

typedef enum {
	GRAPHICS_API_OPENGL
} GraphicsApi;

// Render commands are populated by the host and read in sequential order by the
// renderer implementation. Later, we might want to change this to a render
// graph with dependencies and all of that.
typedef enum {
	RENDER_COMMAND_NULL,
	RENDER_COMMAND_CLEAR,
	RENDER_COMMAND_USE_PROGRAM,
	RENDER_COMMAND_USE_UBO,
	RENDER_COMMAND_USE_TEXTURE,
	RENDER_COMMAND_BUFFER_UBO_DATA,
	RENDER_COMMAND_DRAW_MESH
} RenderCommandType;

typedef struct RenderCommand {
	struct RenderCommand* next;
	RenderCommandType type;
	void* data;
} RenderCommand;

typedef struct {
	RenderCommand* root;
	RenderCommand* tail;
} RenderGraph;

// Render command types
typedef struct {
	float color[4];
} RenderCommandClear;

typedef struct {
	RenderProgram program;
} RenderCommandUseProgram;

typedef struct {
	RenderUbo ubo;
} RenderCommandUseUbo;

typedef struct {
	RenderTexture texture;
} RenderCommandUseTexture;

typedef struct {
	RenderUbo ubo;
	u32 host_buffer_index;
	u64 host_buffer_offset;
} RenderCommandBufferUboData;

typedef struct {
	RenderMesh mesh;
} RenderCommandDrawMesh;

// Key here is the data oriented approach. The association between the index of
// a resource is known by the host of the renderer, either by hardcoded enums or
// a loaded data format that keeps track of the linkages.
typedef struct {
	void* backend;
	GraphicsApi graphics_api;
	RenderGraph* graph;
	i32 frames_since_init;

	Arena persistent_arena;
	Arena viewport_arena;
	Arena frame_arena;

	RenderProgram* programs;
	u32 programs_len;

	RenderMesh* meshes;
	u32 meshes_len;

	RenderTexture* textures;
	u32 textures_len;

	RenderUbo* ubos;
	u32 ubos_len;

	RenderHostBuffer* host_buffers;
	u32 host_buffers_len;
} Renderer;

// Initialization data which is only used during renderer startup. Right now
// these are being populated manually, but we want to move towards loading these
// from a file at startup.
typedef struct RenderProgramInitData {
	struct RenderProgramInitData* next;
	char* vertex_shader_src;
	char* fragment_shader_src;
} RenderProgramInitData;

typedef struct RenderMeshInitData {
	struct RenderMeshInitData* next;

	f32* vertex_data;
	u32 vertex_attribute_sizes[RENDER_MAX_VERTEX_ATTRIBUTES];
	u32 vertex_attributes_len;
	u32 vertices_len;

	u32* indices;
	u32 indices_len;

	bool flat_shading;
} RenderMeshInitData;

typedef struct RenderTextureInitData {
	struct RenderTextureInitData* next;
	u16 width;
	u16 height;
	u8 channel_count;;
	u8* pixel_data;
} RenderTextureInitData;

typedef struct RenderUboInitData {
	struct RenderUboInitData* next;
	u64 size;
	u64 binding;
} RenderUboInitData;

typedef struct RenderHostBufferInitData {
	struct RenderHostBufferInitData* next;
} RenderHostBufferInitData;

typedef struct {
	RenderProgramInitData* programs;
	u32 programs_len;

	RenderMeshInitData* meshes;
	u32 meshes_len;

	RenderTextureInitData* textures;
	u32 textures_len;

	RenderUboInitData* ubos;
	u32 ubos_len;

	RenderHostBufferInitData* host_buffers;
	u32 host_buffers_len;
} RenderInitData;

#endif // renderer_h_INCLUDED
