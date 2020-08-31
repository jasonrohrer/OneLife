#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../gameSource/objectBank.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/categoryBank.h"
#include "../gameSource/animationBank.h"

void usage() {
    printf( "Usage:\n\n"
            "printObjectName id\n\n" );
    exit( 1 );
    }


int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 2 ) {
        usage();
        }

    char rebuilding;
    
    initAnimationBankStart( &rebuilding );
    while( initAnimationBankStep() < 1.0 );
    initAnimationBankFinish();

    initObjectBankStart( &rebuilding, true, true );
    while( initObjectBankStep() < 1.0 );
    initObjectBankFinish();

    
    initCategoryBankStart( &rebuilding );
    while( initCategoryBankStep() < 1.0 );
    initCategoryBankFinish();


    // auto-generate category-based transitions
    initTransBankStart( &rebuilding, true, true, true, true );
    while( initTransBankStep() < 1.0 );
    initTransBankFinish();
    
    int id = 0;
    sscanf( inArgs[1], "%d", &id );

    ObjectRecord *o = getObject( id );
    
    if( o != NULL ) {
        printf( "%s\n", o->description );
        }
    else {
        printf( "Empty space\n" );
        }
    
    

    freeTransBank();
    freeCategoryBank();
    freeObjectBank();
    freeAnimationBank();
    

    return 1;
    }




void *getSprite( int ) {
    return NULL;
    }

char *getSpriteTag( int ) {
    return NULL;
    }

char isSpriteBankLoaded() {
    return false;
    }

char markSpriteLive( int ) {
    return false;
    }

void stepSpriteBank() {
    }

void drawSprite( void*, doublePair, double, double, char ) {
    }

void setDrawColor( float inR, float inG, float inB, float inA ) {
    }

void setDrawFade( float ) {
    }

float getTotalGlobalFade() {
    return 1.0f;
    }

void toggleAdditiveTextureColoring( char inAdditive ) {
    }

void toggleAdditiveBlend( char ) {
    }

void drawSquare( doublePair, double ) {
    }

void startAddingToStencil( char, char, float ) {
    }

void startDrawingThroughStencil( char ) {
    }

void stopStencil() {
    }





// dummy implementations of these functions, which are used in editor
// and client, but not server
#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {
    return NULL;
    }

#include "../gameSource/soundBank.h"
void checkIfSoundStillNeeded( int inID ) {
    }



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    return false;
    }


char getUsesMultiplicativeBlending( int inID ) {
    return false;
    }


void toggleMultiplicativeBlend( char inMultiplicative ) {
    }


void countLiveUse( SoundUsage inUsage ) {
    }

void unCountLiveUse( SoundUsage inUsage ) {
    }



// animation bank calls these only if lip sync hack is enabled, which
// it never is for server
void *loadSpriteBase( const char*, char ) {
    return NULL;
    }

void freeSprite( void* ) {
    }

void startOutputAllFrames() {
    }

void stopOutputAllFrames() {
    }

