#ifndef SPRITE_BANK_INCLUDED
#define SPRITE_BANK_INCLUDED


#include "FloatRGB.h"


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
        
        // total width of image
        int w, h;
        
        // center of non-transparent area of sprite
        // offset relative to w/2, h/2
        int centerXOffset, centerYOffset;


        // size of visible a>= 0.25 area of sprite
        int visibleW, visibleH;

        // center around which sprite is rotated and drawn
        // note that is is loaded into SpriteHandle and is used 
        // automatically by drawSprite
        int centerAnchorXOffset, centerAnchorYOffset;
        

        // 0 where alpha <0.25, for registering mouse clicks on sprite
        char *hitMap;

        char loading;

        int numStepsUnused;

        
        char remappable;
        char remapTarget;

    } SpriteRecord;




// enable index for string-based sprite searching
// call before init to index sprites on load
// defaults to off
void enableSpriteSearch( char inEnable );


// returns number of sprite metadata files that need to be loaded
int initSpriteBankStart( char *outRebuildingCache );


// returns progress... ready for Finish when progress == 1.0
float initSpriteBankStep();
void initSpriteBankFinish();


char isSpriteBankLoaded();


// can only be called after bank init is complete
int getMaxSpriteID();



void freeSpriteBank();


// processing for dynamic, asynchronous sprite loading
void stepSpriteBank();


// returns NULL if asynchronous loading process hasn't failed
// returns internally-allocated string (destroyed internally) if
//    loading process fails.  String is name of file that failed to load
char *getSpriteBankLoadFailure();



SpriteRecord *getSpriteRecord( int inID );


char getUsesMultiplicativeBlending( int inID );

// not destroyed by caller
char *getSpriteTag( int inID );


SpriteHandle getSprite( int inID );


// returns true if sprite is already loaded
char markSpriteLive( int inID );



// return array destroyed by caller, NULL if none found
SpriteRecord **searchSprites( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


// returns new ID, or -1 if adding failed
int addSprite( const char *inTag, SpriteHandle inSprite, 
               Image *inSourceImage,
               char inMultiplicativeBlending,
               int inCenterAnchorXOffset = 0,
               int inCenterAnchorYOffset = 0 );



// bakes multiple sprite layers into a single sprite and saves it in the bank
// returns the new ID
// does NOT work for multiplicative blending sprites
// does NOT work for sprite rotations that aren't in increments of 90 degrees
// (0.25, 0.5, 0.75, 1.0 radians)
int bakeSprite( const char *inTag,
                int inNumSprites,
                int *inSpriteIDs,
                // offset of each sprite center relative to center point
                doublePair *inSpritePos,
                double *inSpriteRot,
                char *inSpriteHFlips,
                FloatRGB *inSpriteColors );


                
void deleteSpriteFromBank( int inID );



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset );



// for randomly remapping sprites to other sprites
void setRemapSeed( int inSeed );

void setRemapFraction( double inFraction );


void countLoadedSprites( int *outLoaded, int *outTotal );



#endif
