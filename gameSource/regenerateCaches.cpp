
#include "spriteBank.h"
#include "objectBank.h"
#include "animationBank.h"
#include "transitionBank.h"

#include "soundBank.h"


#include "minorGems/io/file/File.h"
#include "minorGems/system/Thread.h"
#include "minorGems/game/game.h"



static void deleteCache( const char *inFolderName ) {
    File folder( NULL, inFolderName );
    
    if( folder.exists() && folder.isDirectory() ) {
        
        File *cacheFile = folder.getChildFile( "cache.fcz" );
        
        if( cacheFile != NULL ) {
            
            cacheFile->remove();
            
            delete cacheFile;
            }
        }
    }


static int targetNumSteps = 78;


void runRebuild( const char *inBankName,
                 int inNumExpectedSteps,
                 float (*inStepFunction)() ) {
    printf( "Rebuilding cache of %d %s\n", inNumExpectedSteps, inBankName );
        
    int batchSize = inNumExpectedSteps / targetNumSteps;
        
    if( batchSize < 1 ) {
        batchSize = 1;
        }

    float progress = 0;
    int lastStepsPrinted = 0;
        
    printf( "[" );
    
    for( int i=0; i<targetNumSteps; i++ ) {
        printf( "-" );
        }
    printf( "]" );

    while( progress < 1 ) {
        for( int i=0; i<batchSize && progress < 1; i++ ) {
            progress = (*inStepFunction)();
            }
            
        // erase old progress steps
        for( int p=0; p<targetNumSteps; p++ ) {
            printf( "\b" );
            }
        printf( "\b" );

        lastStepsPrinted = lrint( floor( progress * targetNumSteps ) );
        
        for( int p=0; p<lastStepsPrinted; p++ ) {
            printf( "#" );
            }
        for( int p=0; p<targetNumSteps - lastStepsPrinted; p++ ) {
            printf( "-" );
            }
        printf( "]" );
        
        fflush( stdout );
        }
    
    printf( "\n" );
    }




int main() {

    
    int batchSize;
    
    // first, delete old ones
    deleteCache( "sprites" );
    deleteCache( "objects" );
    deleteCache( "animations" );
    deleteCache( "transitions" );

    
    char rebuilding;

    int num = initSpriteBankStart( &rebuilding );

    // .tga files not cached
    num /= 2;

    if( rebuilding ) {
        runRebuild( "sprites", num, &initSpriteBankStep );
        }
    
    initSpriteBankFinish();

    freeSpriteBank();
    printf( "\n" );


    num = initObjectBankStart( &rebuilding );
    
    if( rebuilding ) {
        runRebuild( "objects", num, &initObjectBankStep );
        }
    
    initObjectBankFinish();

    freeObjectBank();
    printf( "\n" );



    num = initAnimationBankStart( &rebuilding );

    if( rebuilding ) {
        runRebuild( "animations", num, &initAnimationBankStep );
        }
    
    initAnimationBankFinish();

    freeAnimationBank();
    printf( "\n" );



    num = initTransBankStart( &rebuilding );

    if( rebuilding ) {
        runRebuild( "transitions", num, &initTransBankStep );
        }
    
    initTransBankFinish();

    freeTransBank();
    printf( "\n" );


    num = initSoundBankStart();

    if( num > 0 ) {
        runRebuild( "sounds", num, &initSoundBankStep );
        }
    else {
        printf( "No reverbs need to be generated\n" );
        }
    initSoundBankFinish();

    freeSoundBank();
    printf( "\n" );

    }




// implement dummy versions of these functions
// they are needed for compiling, but never called when we are regenerating 
// caches
int startAsyncFileRead( const char *inFilePath ) {
    return -1;
    }

char checkAsyncFileReadDone( int inHandle ) {
    return false;
    }

unsigned char *getAsyncFileData( int inHandle, int *outDataLength ) {
    return NULL;
    }

Image *readTGAFileBase( const char *inTGAFileName ) {
    return NULL;
    }

RawRGBAImage *readTGAFileRawFromBuffer( unsigned char *inBuffer, 
                                        int inLength ) {
    return NULL;
    }

char startRecording16BitMonoSound( int inSampleRate ) {
    return false;
    }

int16_t *stopRecording16BitMonoSound( int *outNumSamples ) {
    return NULL;
    }

SoundSpriteHandle setSoundSprite( int16_t *inSamples, int inNumSamples ) {
    return NULL;
    }

void setMaxTotalSoundSpriteVolume( double inMaxTotal, 
                                   double inCompressionFraction ) {
    }

void setMaxSimultaneousSoundSprites( int inMaxCount ) {
    }

void playSoundSprite( SoundSpriteHandle inHandle, double inVolumeTweak,
                      double inStereoPosition ) {
    }

void playSoundSprite( int inNumSprites, SoundSpriteHandle *inHandles, 
                      double *inVolumeTweaks,
                      double *inStereoPositions ) {
    }


void freeSoundSprite( SoundSpriteHandle inHandle ) {
    }



void freeSprite( SpriteHandle ) {
    }

SpriteHandle fillSprite( unsigned char*, unsigned int, unsigned int ) {
    return NULL;
    }

void setSpriteCenterOffset( void*, doublePair ) {
    }

SpriteHandle fillSprite( Image*, char ) {
    return NULL;
    }

void drawSprite( SpriteHandle, doublePair, double, double, char ) {
    }


void setDrawColor( float, float, float, float ) {
    }

void toggleMultiplicativeBlend( char ) {
    }

void setDrawFade( float ) {
    }

void toggleAdditiveTextureColoring( char ) {
    }


