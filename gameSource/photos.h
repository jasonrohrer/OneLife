#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"


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
                char *inServerSig,
                int inAuthorID,
                // these are copied internally, destroyed by caller
                char *inAuthorName,
                SimpleVector<int> *inSubjectIDs,
                SimpleVector<char*> *inSubjectNames );


// after takePhoto, once photo is successfully posted
// returns NULL if there's no new ID
// Can return empty string "" on error posting photo
// result destroyed by caller
char *getPostedPhotoID();
