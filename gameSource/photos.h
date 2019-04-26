#include "minorGems/game/doublePair.h"


void initPhotos();
void freePhotos();


void stepPhotos();


// returns -1 until next sequence number available
int getNextPhotoSequenceNumber();


// takes image to the north of the camera
// facing = -1 for left, +1 for right
// server sig copied internally, destroyed by caller
void takePhoto( doublePair inCamerLocation, int inCameraFacing,
                int inSequenceNumber,
                char *inServerSig );
