#ifndef OVERLAY_BANK_INCLUDED
#define OVERLAY_BANK_INCLUDED


#include "minorGems/game/gameGraphics.h"


typedef struct OverlayRecord {
        int id;

        SpriteHandle thumbnailSprite;
        
        Image *image;

        char *tag;
        
    } OverlayRecord;


void initOverlayBankStart();
// returns progress... ready for Finish when progress == 1.0
float initOverlayBankStep();
void initOverlayBankFinish();


void freeOverlayBank();



OverlayRecord *getOverlay( int inID );


// return array destroyed by caller, NULL if none found
OverlayRecord **searchOverlays( const char *inSearch, 
                                int inNumToSkip, 
                                int inNumToGet, 
                                int *outNumResults, int *outNumRemaining );


// added image is stored internally
// should not be destroyed by caller unless adding failed
// returns new ID, or -1 if adding failed
int addOverlay( const char *inTag, Image *inSourceImage );


void deleteOverlayFromBank( int inID );



#endif
