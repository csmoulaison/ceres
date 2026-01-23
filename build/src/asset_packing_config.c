// All the data that is essential to the compilation and functioning of
// asset_packing.c is marked 'REQUIRED' here.
#include "asset_format.h"

#define MANIFEST_FILENAME "data/assets.manifest"
#define GENERATED_ASSET_HANDLES_FILENAME "../src/generated/asset_handles.h"
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
char* asset_type_to_manifest_key[NUM_ASSET_TYPES] = {
	"mesh",
	"texture",
	"render_program",
	"font"
};

char* asset_type_to_macro_prefix[NUM_ASSET_TYPES] = {
	"MESH",
	"TEXTURE",
	"RENDER_PROGRAM",
	"FONT"
};
