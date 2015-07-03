#ifndef OBJECT_BANK_INCLUDED
#define OBJECT_BANK_INCLUDED


#include "minorGems/game/doublePair.h"


typedef struct ObjectRecord {
        int id;
        
        char *description;
        

        int numSprites;
        
        int *sprites;
        
        doublePair *spritePos;
        
    } ObjectRecord;





// loads from objects folder
void initObjectBank();


void freeObjectBank();



ObjectRecord *getObject( int inID );


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


int addObject( const char *inDescription,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               int inReplaceID = -1 );




void drawObject( ObjectRecord *inObject, doublePair inPos );




#endif
