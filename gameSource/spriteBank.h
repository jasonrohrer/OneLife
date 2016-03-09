#ifndef SPRITE_BANK_INCLUDED
#define SPRITE_BANK_INCLUDED


#include "minorGems/game/gameGraphics.h"


typedef struct SpriteRecord {
        int id;
        SpriteHandle sprite;
        
        char *tag;

        // maximum pixel dimension
        // (used for sizing in pickers)
        int maxD;
        
        int w, h;
        
        // 0 where alpha <0.25, for registering mouse clicks on sprite
        char *hitMap;

    } SpriteRecord;



// loads from sprites folder
void initSpriteBankStart();
// returns progress... ready for Finish when progress == 1.0
float initSpriteBankStep();
void initSpriteBankFinish();


void freeSpriteBank();



SpriteRecord *getSpriteRecord( int inID );

SpriteHandle getSprite( int inID );


// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


// returns new ID, or -1 if adding failed
int addSprite( const char *inTag, SpriteHandle inSprite, 
               Image *inSourceImage );


void deleteSpriteFromBank( int inID );


#endif
