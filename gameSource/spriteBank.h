#ifndef SPRITE_BANK_INCLUDED
#define SPRITE_BANK_INCLUDED


#include "minorGems/game/gameGraphics.h"


typedef struct SpriteRecord {
        int id;

        // NULL if image not loaded
        SpriteHandle sprite;
        
        char *tag;

        char multiplicativeBlend;

        // maximum pixel dimension
        // (used for sizing in pickers)
        int maxD;
        
        int w, h;
        
        // 0 where alpha <0.25, for registering mouse clicks on sprite
        char *hitMap;

        char loading;

    } SpriteRecord;



// returns number of sprite metadata files that need to be loaded
int initSpriteBankStart( char *outRebuildingCache );


// returns progress... ready for Finish when progress == 1.0
float initSpriteBankStep();
void initSpriteBankFinish();


void freeSpriteBank();


// processing for dynamic, asynchronous sprite loading
void stepSpriteBank();



SpriteRecord *getSpriteRecord( int inID );


char getUsesMultiplicativeBlending( int inID );


SpriteHandle getSprite( int inID );


// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


// returns new ID, or -1 if adding failed
int addSprite( const char *inTag, SpriteHandle inSprite, 
               Image *inSourceImage,
               char inMultiplicativeBlending );


void deleteSpriteFromBank( int inID );



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset );


#endif
