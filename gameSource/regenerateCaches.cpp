
#include "spriteBank.h"
#include "objectBank.h"
#include "animationBank.h"
#include "transitionBank.h"
#include "categoryBank.h"

#include "soundBank.h"

#include "groundSprites.h"


#include "minorGems/io/file/File.h"
#include "minorGems/system/Thread.h"
#include "minorGems/game/game.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"



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
    
    fflush( stdout );

    
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
    deleteCache( "categories" );
    deleteCache( "animations" );
    deleteCache( "transitions" );
    
    File groundTileCacheFolder( NULL, "groundTileCache" );
    
    if( groundTileCacheFolder.exists() && 
        groundTileCacheFolder.isDirectory() ) {
        
        int numChildFiles;
        File **childFiles = 
            groundTileCacheFolder.getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
            childFiles[i]->remove();
            delete childFiles[i];
            }
        delete [] childFiles;
        }


    
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
    printf( "\n" );
    

    num = initCategoryBankStart( &rebuilding );
    
    if( rebuilding ) {
        runRebuild( "categories", num, &initCategoryBankStep );
        }
    
    initCategoryBankFinish();
    printf( "\n" );


    num = initAnimationBankStart( &rebuilding );

    if( rebuilding ) {
        runRebuild( "animations", num, &initAnimationBankStep );
        }
    
    initAnimationBankFinish();

    freeAnimationBank();
    printf( "\n" );



    num = initTransBankStart( &rebuilding, false );

    if( rebuilding ) {
        runRebuild( "transitions", num, &initTransBankStep );
        }
    
    initTransBankFinish();

    freeTransBank();
    printf( "\n" );


    num = initSoundBankStart( false );

    if( num > 0 ) {
        runRebuild( "sounds", num, &initSoundBankStep );
        }
    else {
        printf( "No reverbs need to be generated\n" );
        }
    initSoundBankFinish();

    freeSoundBank();
    printf( "\n" );


    num = initGroundSpritesStart( false );

    if( num > 0 ) {
        runRebuild( "groundTiles", num, &initGroundSpritesStep );
        }
    else {
        printf( "No ground tiles need to be generated\n" );
        }
    initGroundSpritesFinish();

    freeGroundSprites();
    printf( "\n" );


    // ground tiles need this, so free last
    freeObjectBank();

    // trans bank needs these, so free last
    freeCategoryBank();

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


SpriteHandle fillSprite( RawRGBAImage *inRawImage ) {
    return NULL;
    }

SpriteHandle loadSpriteBase( const char *inTGAFileName, 
                             char inTransparentLowerLeftCorner ) {
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

float getTotalGlobalFade() {
    }



void startOutputAllFrames() {
    }


void stopOutputAllFrames() {
    }





// these implementations copied from gameSDL.cpp



static Image *readTGAFile( File *inFile ) {
    
    if( !inFile->exists() ) {
        char *fileName = inFile->getFullFileName();
        
        printf( 
            "CRITICAL ERROR:  TGA file %s does not exist",
            fileName );
        delete [] fileName;
        
        return NULL;
        }    


    FileInputStream tgaStream( inFile );
    
    TGAImageConverter converter;
    
    Image *result = converter.deformatImage( &tgaStream );

    if( result == NULL ) {        
        char *fileName = inFile->getFullFileName();
        
        printf( 
            "CRITICAL ERROR:  could not read TGA file %s, wrong format?",
            fileName );
        delete [] fileName;
        }
    
    return result;
    }



Image *readTGAFile( const char *inTGAFileName ) {

    File tgaFile( new Path( "graphics" ), inTGAFileName );
    
    return readTGAFile( &tgaFile );
    }



Image *readTGAFileBase( const char *inTGAFileName ) {

    File tgaFile( NULL, inTGAFileName );
    
    return readTGAFile( &tgaFile );
    }




static RawRGBAImage *readTGAFileRaw( InputStream *inStream ) {
    TGAImageConverter converter;
    
    RawRGBAImage *result = converter.deformatImageRaw( inStream );

    
    return result;
    }




static RawRGBAImage *readTGAFileRaw( File *inFile ) {
    
    if( !inFile->exists() ) {
        char *fileName = inFile->getFullFileName();
        
        printf( 
            "CRITICAL ERROR:  TGA file %s does not exist",
            fileName );
        delete [] fileName;
       
        return NULL;
        }    


    FileInputStream tgaStream( inFile );
    

    RawRGBAImage *result = readTGAFileRaw( &tgaStream );

    if( result == NULL ) {        
        char *fileName = inFile->getFullFileName();
        
        printf( 
            "CRITICAL ERROR:  could not read TGA file %s, wrong format?",
            fileName );
        delete [] fileName;
        
        }
    
    return result;
    }



RawRGBAImage *readTGAFileRaw( const char *inTGAFileName ) {

    File tgaFile( new Path( "graphics" ), inTGAFileName );
    
    return readTGAFileRaw( &tgaFile );
    }



RawRGBAImage *readTGAFileRawBase( const char *inTGAFileName ) {

    File tgaFile( NULL, inTGAFileName );
    
    return readTGAFileRaw( &tgaFile );
    }





void writeTGAFile( const char *inTGAFileName, Image *inImage ) {
    File tgaFile( NULL, inTGAFileName );
    FileOutputStream tgaStream( &tgaFile );
    
    TGAImageConverter converter;
    
    return converter.formatImage( inImage, &tgaStream );
    }


