#include "minorGems/game/doublePair.h"


void initPhotos();
void freePhotos();


void stepPhotos();


// takes image to the north of the camera
// facing = -1 for left, +1 for right
void takePhoto( doublePair inCamerLocation, int inCameraFacing );
