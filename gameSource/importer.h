


// Mod-loader functions assume that sprite, sound, object, and animation
// banks have already been loaded.

// Mods are loaded into RAM without impacting the disk

// returns number of mod blocks to load
int initModLoaderStart();

// returns progress... ready for Finish when progress == 1.0
float initModLoaderStep();

void initModLoaderFinish();



// Import functions (both add and replace) assume that sprite and sound banks
// have been loaded
// but object and animation banks have NOT been loaded.

// New sprites and sounds are saved to disk AND loaded into the live
// banks.

// New/replaced objects and animations are saved to disk
// They will (hopefully) be loaded with the object and animation banks
// load later.

int initImportAddStart();

float initImportAddStep();

void initImportAddFinish();



int initImportReplaceStart();

float initImportReplaceStep();

void initImportReplaceFinish();









void freeImporter();
