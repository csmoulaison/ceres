#include "mesh_packing.c"
#include "texture_packing.c"
#include "render_program_packing.c"
#include "font_packing.c"

// REQUIRED: Callback functions which define the packing logic of the different
// asset types.
// - request_asset_list: This is where assets are prepared and added to the
//   list for being packed. Multiple assets can be added with push_asset_info.
// - pack_asset: The asset is packed into the void* asset pointer. void* info
//   comes from the void* info passed into the AssetInfo struct when pushing
//   with push_asset_info during the request_asset_list stage.
//
// WARNING: The number of items in this list must match NUM_ASSET_TYPES.
AssetCallbacks asset_callbacks_by_type[NUM_ASSET_TYPES] = {
	{ .request_asset_list = calculate_mesh_assets, .pack_asset = pack_mesh_asset },
	{ .request_asset_list = calculate_texture_assets, .pack_asset = pack_texture_asset },
	{ .request_asset_list = calculate_render_program_assets, .pack_asset = pack_render_program_asset },
	{ .request_asset_list = calculate_font_assets, .pack_asset = pack_font_asset }
};
