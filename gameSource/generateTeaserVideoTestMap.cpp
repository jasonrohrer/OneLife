
#include "spriteBank.h"
#include "objectBank.h"
#include "animationBank.h"
#include "transitionBank.h"
#include "categoryBank.h"

#include "soundBank.h"


#include "minorGems/io/file/File.h"
#include "minorGems/system/Thread.h"
#include "minorGems/game/game.h"
#include "minorGems/util/MinPriorityQueue.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


JenkinsRandomSource randSource( 100 );


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
    printf(
    "\nUsage:\n\n generateTeaserVideoTestMap ""outMap.txt [baseMap.txt]\n\n" );
    
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
    
    if( inNumArgs != 2 && inNumArgs != 3 ) {
        usage();
        }
    
    FILE *outFile = fopen( inArgs[1], "w" );
    
    if( outFile == NULL ) {
        usage();
        }
    
    FILE *baseFile = NULL;
    
    if( inNumArgs == 3 ) {
        baseFile = fopen( inArgs[2], "r" );
        
        if( baseFile == NULL ) {
            usage();
            }
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


    num = initCategoryBankStart( &rebuilding );
    
    runSteps( "categories", num, &initCategoryBankStep );
    
    initCategoryBankFinish();

    printf( "\n" );



    num = initAnimationBankStart( &rebuilding );

    runSteps( "animations", num, &initAnimationBankStep );
    
    
    initAnimationBankFinish();

    printf( "\n" );



    // auto-gen category-based transitions so that we can detect
    // human-made objects properly.
    //
    // Generic use transitions will work without being filled in, I think
    num = initTransBankStart( &rebuilding, true );

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

    MinPriorityQueue<int> queue;
    
    SimpleVector<int> treeList;

    int berryBush = -1;
    
    int maxQueuedID = 0;
    
    int numObjects;
    ObjectRecord **objects = getAllObjects( &numObjects );

    char *objectAllowed = new char[ getMaxObjectID() + 1 ];
    memset( objectAllowed, false, getMaxObjectID() + 1 );

    for( int i=0; i<numObjects; i++ ) {

        ObjectRecord *o = objects[i];
        
        
        if( o->mapChance == 0 &&
            ! o->person ) {

            // find out how it comes to exist
            
            int numTrans, numRemain;
            
            TransRecord **trans =
                searchProduces( o->id, 
                                0,
                                1000,
                                &numTrans, &numRemain );
            
            char humanMade = true;
            
            if( trans != NULL ) {    
                for( int t=0; t<numTrans; t++ ) {
                    if( trans[t]->actor == -1 &&
                        trans[t]->autoDecaySeconds != 0 ) {
                        humanMade = false;
                        break;
                        }
                    if( trans[t]->actor == -2 ) {
                        // default transition
                        // doesn't count as making something
                        humanMade = false;
                        break;
                        }
                    }
                delete [] trans;
                }
            
            if( humanMade && o->permanent ) {
                // make sure it won't auto-decay back to natural
                
                TransRecord **trans =
                    searchUses( o->id, 
                                0,
                                1000,
                                &numTrans, &numRemain );
                
                if( trans != NULL ) {    
                    for( int t=0; t<numTrans; t++ ) {
                        if( trans[t]->actor == -1 &&
                            trans[t]->autoDecaySeconds != 0 &&
                            trans[t]->newTarget > 0 ) {

                            if( getObjectDepth( trans[t]->newTarget ) 
                                == 0 ) {
                                // one step of decay back to natural
                                humanMade = false;
                                }
                            break;
                            }
                        }
                    delete [] trans;
                    }
                }
            

            if( humanMade ) {
                
                        
                int d = getObjectDepth( o->id );
                if( d != UNREACHABLE ) {
                    queue.insert( o->id, d );
                    objectAllowed[ o->id ] = true;
                    
                    if( o->id > maxQueuedID ) {
                        maxQueuedID = o->id;
                        }
                    }
                }
            }
        else if( o->mapChance > 0 && getObjectDepth( o->id ) == 0 ) {
            if( strstr( o->description, "Tree" ) != NULL  ) {
                
                if( strstr( o->description, "Branch" ) != NULL ) {

                    // see if bare hand applies to remove branch
                    TransRecord *t = getTrans( 0, o->id );
                    
                    
                    if( t != NULL && t->newTarget != o->id &&
                        t->newTarget > 0 ) {
                        
                        o = getObject( t->newTarget );
                        }
                    }

                treeList.push_back( o->id );
                }
            if( strstr( o->description, "Gooseberry Bush" ) != NULL  ) {
                berryBush = o->id;
                }
            }
        }
        
    delete [] objects;
    
    SimpleVector<int> orderedObjects;

    char *objectIncluded = new char[ maxQueuedID + 1 ];
    memset( objectIncluded, false, maxQueuedID + 1 );
    

    double lastP = queue.checkMinPriority();
    
    JenkinsRandomSource sortRandom( 200 );
    
    while( queue.checkMinPriority() > 0 ) {
        
        // build queue for all objects with same depth, or next depth, 
        // sorted by random number
        MinPriorityQueue<int> strataQueue;
        while( queue.checkMinPriority() == lastP && 
               queue.checkMinPriority() > 0 ) {
            
            int id = queue.removeMin();
            strataQueue.insert( id, sortRandom.getRandomBoundedInt(1,20000) );
            }

        // end of strata

        while( strataQueue.checkMinPriority() > 0 ) {
            int id = strataQueue.removeMin();

            if( ! objectIncluded[ id ] ) {
                // only if not yet included
                objectIncluded[ id ] = true;
                orderedObjects.push_back( id );
                }
            // but walk tree again from this spot regardless
            // to look for stuff that can be made now

            printf( "Walking tree from:  %s\n", 
                    getObject( id )->description );
            
            // walk transition tree from this object and include
            // other things that it makes using already-included objects
            // (skip things that require not-yet-included ingredients)
            
            // depth first search.  Next node to explore at end of list
            SimpleVector<TransRecord*> exploreList;
            
            exploreList.push_back_other( getAllUses( id ) );
            
            
            while( exploreList.size() > 0 ) {
                TransRecord *r = exploreList.getLastElementDirect();
                exploreList.deleteLastElement();
                
                if( ( r->actor == 0 || r->actor == -1 || r->actor == -2 || 
                      objectIncluded[ r->actor ] )
                    &&
                    ( r->target == 0 || r->target == -1 ||
                      objectIncluded[ r->target ] ) ) {
                    
                    if( r->newActor > 0 && 
                        ! objectIncluded[ r->newActor ] ) {
                        
                        if( objectAllowed[ r->newActor ] ) {    
                            objectIncluded[ r->newActor ] = true;
                            orderedObjects.push_back( r->newActor );
                            printf( "  Adding %s\n",
                                    getObject( r->newActor )->
                                    description );
                            }
                        // explore further into tree
                        exploreList.push_back_other( 
                            getAllUses( r->newActor ) );
                        }
                    if( r->newTarget > 0 && 
                        ! objectIncluded[ r->newTarget ] ) {
                        
                        if( objectAllowed[ r->newTarget ] ) {
                            objectIncluded[ r->newTarget ] = true;
                            orderedObjects.push_back( r->newTarget );
                            printf( "  Adding %s\n",
                                    getObject( r->newTarget )->
                                    description );
                            }
                        
                        exploreList.push_back_other( 
                            getAllUses( r->newTarget ) );
                        }
                    }
                }
            }
    
        // start next strata
        lastP = queue.checkMinPriority();
        }
    
    printf( "\n\n%d objects will be displayed\n", orderedObjects.size() );
    
    for( int i=0; i<orderedObjects.size(); i++ ) {
        printf( "%d. %s\n", i, 
                getObject( orderedObjects.getElementDirect( i ) )->
                description );
        }

    int spacing = 1;
    int xMax = ( orderedObjects.size() - 1 ) * spacing + 10;

    // for testing
    //int xMax = 25 * spacing + 10;
    
    for( int y=-5; y<=5; y++ ) {
        int biome = 2;
        if( y < 0 || y > 1 ) {
            biome = 0;
            }
        for( int x=-20; x<=xMax+20; x++ ) {
            int id = 0;
            int rowBiome = biome;
            if( x <= -2 ) {
                rowBiome = 0;
                }
            /*
            if( y < 2 && x >= -2 && x < 0 ) {
                rowBiome = 2;
                }
            if( y >=-10 && y<=-9 && x < 0 ) {
                rowBiome = 2;
                }
            
            */

            if( y < 2 && y >= -5 && x >= -2 && x < 0 ) {
                rowBiome = 2;
                }

            if( y == 1 && x == -2 ) {
                rowBiome = 0;
                }
            
            if( y == 1 && x > 10 && 
                x % spacing == 0 ) {
                
                // note that this skips first item by accident
                // BUT, we want to be consistent with original draft of
                // video that had 260 items shown
                int objectIndex = x / spacing - 10;
                
                if( objectIndex >=0 && objectIndex < orderedObjects.size() ) {
                    id = orderedObjects.getElementDirect( objectIndex );
                    }
                }

            char addTree = false;
            

            if( y == 2  ||
                y == 3  ||
                ( ( x >= 0 || x < -2 ) 
                  && 
                  ( y == -3 ||
                    y == -4 ) ) ) {
                addTree = true;
                }
            else if( x < -2 && y >= -4 && y <=3 ) {
                addTree = true;
                }
            else if( rowBiome == 0 && y <= -5 && y != -11 &&
                     x != -10 && ( x >= 0 || x < -2 ) ) {
                // grassy
                addTree = ( randSource.getRandomBoundedInt( 0, 20 ) < 10 );
                }

            if( y == -3 && x < 7 ) {
                addTree = false;
                }

            if( y == -3 && x >= xMax + 10 ) {
                addTree = false;
                }
            
            
            if( addTree  ) {
                id = treeList.getElementDirect( 
                    randSource.getRandomBoundedInt( 0, treeList.size() - 1 ) );
                }
            else if( y == -12 && x == -10 ) {
                id = berryBush;
                }
            else if( y == 1 && x == xMax + 10 ) {
                id = berryBush;
                }

            fprintf( outFile, "%d %d %d 0 %d\n",
                     x, y, rowBiome, id );
            }
        }        
        
    if( baseFile != NULL ) {
        unsigned char buffer[512];
        int numRead = fread( buffer, 1, 512, baseFile );
        
        while( numRead > 0 ) {
            fwrite( buffer, 1, numRead, outFile );
            numRead = fread( buffer, 1, 512, baseFile );
            }
        fclose( baseFile );
        }
    
    fclose( outFile );
    
    
    freeSpriteBank();
    freeObjectBank();
    freeAnimationBank();
    freeTransBank();
    freeSoundBank();
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

float getTotalGlobalFade() {
    return 1.0f;
    }


void toggleAdditiveTextureColoring( char ) {
    }


void startOutputAllFrames() {
    }


void stopOutputAllFrames() {
    }
