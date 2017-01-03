
#include "spriteBank.h"
#include "objectBank.h"
#include "animationBank.h"
#include "transitionBank.h"

#include "soundBank.h"


#include "minorGems/io/file/File.h"
#include "minorGems/system/Thread.h"
#include "minorGems/game/game.h"


#include <stdlib.h>



static int targetNumSteps = 78;


void runSteps( const char *inBankName,
                 int inNumExpectedSteps,
                 float (*inStepFunction)() ) {
    printf( "Loading bank of %d %s\n", inNumExpectedSteps, inBankName );
        
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



static void usage() {
    printf( "\nUsage:\n\n printReportHTML out.html\n\n" );
    exit( 0 );
    }


char *getLatestString( int inID ) {
    
    char *latestString = stringDuplicate( "" );
    
    if( inID != 0 ) {
        delete [] latestString;

        char *des = stringDuplicate( getObject( inID )->description );
        
        // cut off stuff after #
        char *poundPos = strstr( des, "#" );
        if( poundPos != NULL ) {
            poundPos[0] = '\0';
            }
        

        latestString = 
            autoSprintf( "(latest: %s)", des );

        delete [] des;
        }
    return latestString;
    }


int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 2  ) {
        }
    
    FILE *outFile = fopen( inArgs[1], "w" );
    
    if( outFile == NULL ) {
        usage();
        }
    

    char rebuilding;

    int num = initSpriteBankStart( &rebuilding );

    // .tga files not cached
    num /= 2;

    runSteps( "sprites", num, &initSpriteBankStep );
    
    initSpriteBankFinish();

    printf( "\n" );


    num = initObjectBankStart( &rebuilding );
    
    runSteps( "objects", num, &initObjectBankStep );
    
    initObjectBankFinish();

    printf( "\n" );



    num = initAnimationBankStart( &rebuilding );

    runSteps( "animations", num, &initAnimationBankStep );
    
    
    initAnimationBankFinish();

    printf( "\n" );



    num = initTransBankStart( &rebuilding );

    runSteps( "transitions", num, &initTransBankStep );
    
    initTransBankFinish();

    
    printf( "\n" );


    num = initSoundBankStart();

    if( num > 0 ) {
        runSteps( "sounds", num, &initSoundBankStep );
        }
    else {
        printf( "No reverbs need to be generated\n" );
        }
    initSoundBankFinish();

    printf( "\n" );

    int numNatural = 0;
    int numHumanMade = 0;
    int numPeople = 0;
    
    int highestNaturalID = 0;
    int highestHumanMadeID = 0;
    

    int numObjects;
    ObjectRecord **objects = getAllObjects( &numObjects );

    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = objects[i];
        
        if( o->mapChance > 0 ) {
            numNatural ++;

            if( o->id > highestNaturalID ) {
                highestNaturalID = o->id;
                }
            }
        else if( o->person ) {
            numPeople++;
            }
        else {
            // find out how it comes to exist
            
            int numTrans, numRemain;
            
            TransRecord **trans =
                searchProduces( o->id, 
                                0,
                                1000,
                                &numTrans, &numRemain );
            
            char humanMade = true;
            
            for( int t=0; t<numTrans; t++ ) {
                if( trans[t]->actor == -1 &&
                    trans[t]->autoDecaySeconds != 0 ) {
                    humanMade = false;
                    break;
                    }
                }
            delete [] trans;

            if( humanMade ) {
                numHumanMade ++;
                
                if( o->id > highestHumanMadeID ) {
                    highestHumanMadeID = o->id;
                    }
                }
            
            }    
        }
        
    delete [] objects;
    

    char *latestNaturalString = getLatestString( highestNaturalID );
    char *latestHumanMadeString = getLatestString( highestHumanMadeID );
    
    fprintf( outFile, 
             "<table width=100%% border=0><tr>"
             "<td width=33%% valign=top align=center>"
               "%d natural objects<br>%s</td>"
             "<td> </td>"
             "<td width=33%% valign=top align=center>"
               "%d playable characters</td>"
             "<td> </td>"
             "<td width=33%% valign=top align=center>"
               "%d human-makeable objects<br>%s</td>"
             "</tr></table>",
             numNatural, latestNaturalString, 
             numPeople, 
             numHumanMade, latestHumanMadeString );
    
    delete [] latestHumanMadeString;
    delete [] latestNaturalString;
        

    fclose( outFile );
    
    
    freeSpriteBank();
    freeObjectBank();
    freeAnimationBank();
    freeTransBank();
    freeSoundBank();

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
