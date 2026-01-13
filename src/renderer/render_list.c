/*
The render list is the API from the game layer to the renderer frontend, and the 
render graph is the API from the renderer frontend to a renderer backend.

As a rule of thumb, the render list encodes information about specific instances
of a particular type of draw, with information about specific mesh/textures and
the like.

The renderer frontend is agnostic to particular instances, it only knows how to 
encode data into commands, which is still specific to a set of capabilities 
provided by the game.

It's hard to strictly delineate this split in my head, but here's the pragmatic
heuristic: the render list is the API which allows us the programmer to work on
the game layer whenever we want, in a way that isn't brittle to inevitable
changes made to the render layer.
*/

#define RENDER_LIST_MAX_MODEL_INSTANCES 4096

typedef struct {
	f32 clear_color[3];
	f32 camera_position[3];
	f32 camera_target[3];
} RenderListWorld;

typedef struct {
	f32 position[3];
	f32 orientation[3];

	RenderMesh mesh;
	RenderTexture texture;
} RenderListModel;

typedef struct {
	RenderListWorld world;

	RenderListModel models[RENDER_LIST_MAX_MODEL_INSTANCES];
	u32 models_len;
} RenderList;

void render_list_init(RenderList* list) {
	*list = (RenderList){};
}

void render_list_update_world(RenderList* list, f32* clear_color, f32* camera_position, f32* camera_target) {
	RenderListWorld* world = &list->world;
	v3_copy(world->clear_color, clear_color);
	v3_copy(world->camera_position, camera_position);
	v3_copy(world->camera_target, camera_target);
}

// TODO: RenderTexture becomes RenderMaterial.
void render_list_draw_model(RenderList* list, RenderMesh mesh, RenderTexture texture, f32* position, f32* orientation) {
	RenderListModel* model = &list->models[list->models_len];
	model->mesh = mesh;
	model->texture = texture;
	v3_copy(model->position, position);
	v3_copy(model->orientation, orientation);
	list->models_len++;
}
