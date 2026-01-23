// All the data that is essential to the compilation and functioning of
// asset_packing.c is marked 'REQUIRED' here.
#include "asset_format.h"

#define MANIFEST_FILENAME "data/assets.manifest"
#define MAX_ASSETS 256
#define FLAT_MESH_SHADING true

// REQUIRED: Asset types are defined here. The final entry must be
// NUM_ASSET_TYPES.
// 
// WARNING: This must match the count of items in 'asset_callbacks_by_type'
typedef enum {
	ASSET_TYPE_MESH,
	ASSET_TYPE_TEXTURE,
	ASSET_TYPE_RENDER_PROGRAM,
	ASSET_TYPE_FONT,
	NUM_ASSET_TYPES
} AssetType;

// REQUIRED: This array maps strings to asset types for reading the asset
// manifest.
// 
// WARNING: Array length must match NUM_ASSET_TYPES.
char* string_to_asset_type[NUM_ASSET_TYPES] = {
	"mesh",
	"texture",
	"render_program",
	"font"
};
