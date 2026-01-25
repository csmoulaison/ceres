// TODO: Maybe this should be our first module? The metric we should be looking
// at for splitting code into separate files is the ability to copy a number of
// files into a new project and have different bits of them working with minimal
// code change, within reason!

#ifndef primitive_vbo_data_h_INCLUDED
#define primitive_vbo_data_h_INCLUDED

f32 render_primitive_quad[] = {
	 1.0f,  1.0f,
	 1.0f, -1.0f,
	-1.0f,  1.0f,

	 1.0f, -1.0f,
	-1.0f, -1.0f,
	-1.0f,  1.0f
};

u32 render_primitive_quad_indices[] = {
	0, 1, 2, 3, 4, 5, 6
};

// WARNING: The entries here must match the primitives array.
typedef enum {
	RENDER_PRIMITIVE_QUAD,
	NUM_RENDER_PRIMITIVES
} RenderPrimitiveType;

f32* render_primitives[NUM_RENDER_PRIMITIVES] = {
	render_primitive_quad
};

u32* render_primitives_indices[NUM_RENDER_PRIMITIVES] = {
	render_primitive_quad_indices
};

#endif // primitive_vbo_data_h_INCLUDED
