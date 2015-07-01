#ifndef SPRITE_BANK_INCLUDED
#define SPRITE_BANK_INCLUDED


#include "minorGems/game/gameGraphics.h"


typedef struct SpriteRecord {
        int id;
        SpriteHandle sprite;
        
        char *tag;
        
    } SpriteRecord;



// loads from sprites folder
void initSpriteBank();


void freeSpriteBank();



SpriteHandle getSprite( int inID );


// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


void addSprite( int inID, const char *inTag, SpriteHandle inSprite );


#endif
