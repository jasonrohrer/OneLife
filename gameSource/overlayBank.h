#ifndef OVERLAY_BANK_INCLUDED
#define OVERLAY_BANK_INCLUDED


#include "minorGems/game/gameGraphics.h"


typedef struct OverlayRecord {
        int id;

        SpriteHandle thumbnailSprite;
        
        Image *image;

        char *tag;
        
    } OverlayRecord;


void initOverlayBank();


void freeOverlayBank();



OverlayRecord *getOverlay( int inID );


// return array destroyed by caller, NULL if none found
OverlayRecord **searchOverlays( const char *inSearch, 
                                int inNumToSkip, 
                                int inNumToGet, 
                                int *outNumResults, int *outNumRemaining );


// returns new ID, or -1 if adding failed
int addOverlay( const char *inTag, Image *inSourceImage );


void deleteOverlayFromBank( int inID );



#endif
