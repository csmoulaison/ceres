/* NOW: Use this to drive the render graph creation by the renderer frontend.

The general thinking is this: the render list is the API from the game layer to
the renderer frontend, and the render graph is the API from the renderer 
frontend to a renderer backend.

As a rule of thumb, the render list encodes information about specific instances
of a particular type of draw, with information about specific mesh/textures and
the like.

The render list references assets and entities, the renderer frontend is 
agnostic to particular instances, it only knows how to encode data into 
commands, which is still specific to a set of capabilities provided by the game.

It's hard to strictly delineate this split in my head, but here's the pragmatic
explanation: the render list is the API which allows us the programmer to work 
on the game layer whenever we want, in a way that isn't brittle to inevitable 
changes made to the render layer. This is ALWAYS the point of an API boundary.
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
	memcpy(world->clear_color, clear_color, sizeof(f32) * 3);
	memcpy(world->camera_position, camera_position, sizeof(f32) * 3);
	memcpy(world->camera_target, camera_target, sizeof(f32) * 3);
}

void render_list_draw_model(RenderList* list, RenderMesh mesh, RenderTexture texture, f32* position, f32* orientation) {
	RenderListModel* model = &list->models[list->models_len];
	model->mesh = mesh;
	model->texture = texture;
	memcpy(model->position, position, sizeof(f32) * 3);
	memcpy(model->orientation, orientation, sizeof(f32) * 3);
	list->models_len++;
}
