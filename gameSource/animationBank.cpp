#include "animationBank.h"

#include "objectBank.h"
#include "spriteBank.h"

#include "stdlib.h"
#include "math.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"

#include "minorGems/game/gameGraphics.h"


#include "ageControl.h"


#include "folderCache.h"


static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static AnimationRecord * **idMap;



static ClothingSet emptyClothing = getEmptyClothingSet();



static FolderCache cache;

static int currentFile;


static SimpleVector<AnimationRecord*> records;
static int maxID;


int initAnimationBankStart( char *outRebuildingCache ) {

    maxID = 0;

    currentFile = 0;

    
    cache = initFolderCache( "animations", outRebuildingCache );

    return cache.numFiles;
    }



float initAnimationBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;


    char *txtFileName = getFileName( cache, i );
                        
    if( strstr( txtFileName, ".txt" ) != NULL ) {
        
        // every .txt file is an animation file

        char *animText = getFileContents( cache, i );
        if( animText != NULL ) {
            AnimationRecord *r = new AnimationRecord;
                        
            int numLines;
                        
            char **lines = split( animText, "\n", &numLines );
                        
            delete [] animText;

            if( numLines > 4 ) {
                int next = 0;
                            
                r->objectID = 0;
                sscanf( lines[next], "id=%d", 
                        &( r->objectID ) );
                            
                if( r->objectID > maxID ) {
                    maxID = r->objectID;
                    }
                            
                next++;

                            
                int typeRead = 0;
                sscanf( lines[next], "type=%d", 
                        &( typeRead ) );
                r->type = (AnimType)typeRead;
                next++;

                r->numSprites = 0;
                sscanf( lines[next], "numSprites=%d", 
                        &( r->numSprites ) );
                next++;

                r->numSlots = 0;
                sscanf( lines[next], "numSlots=%d", 
                        &( r->numSlots ) );
                next++;
                            
                r->spriteAnim = 
                    new SpriteAnimationRecord[ r->numSprites ];
                            
                r->slotAnim = 
                    new SpriteAnimationRecord[ r->numSlots ];
                            
                for( int j=0; j< r->numSprites && next < numLines;
                     j++ ) {
                                
                    sscanf( lines[next], 
                            "animParam="
                            "%lf %lf %lf %lf %lf %lf (%lf,%lf) %lf %lf "
                            "%lf %lf %lf %lf %lf",
                            &( r->spriteAnim[j].xOscPerSec ),
                            &( r->spriteAnim[j].xAmp ),
                            &( r->spriteAnim[j].xPhase ),
                                        
                            &( r->spriteAnim[j].yOscPerSec ),
                            &( r->spriteAnim[j].yAmp ),
                            &( r->spriteAnim[j].yPhase ),
                                        
                            &( r->spriteAnim[j].rotationCenterOffset.x ),
                            &( r->spriteAnim[j].rotationCenterOffset.y ),

                            &( r->spriteAnim[j].rotPerSec ),
                            &( r->spriteAnim[j].rotPhase ),
                                        
                            &( r->spriteAnim[j].rockOscPerSec ),
                            &( r->spriteAnim[j].rockAmp ),
                            &( r->spriteAnim[j].rockPhase ),

                            &( r->spriteAnim[j].durationSec ),
                            &( r->spriteAnim[j].pauseSec ) );
                        
                    next++;
                    }

                for( int j=0; j< r->numSlots && next < numLines;
                     j++ ) {
                                
                    sscanf( lines[next], 
                            "animParam="
                            "%lf %lf %lf %lf %lf %lf %lf %lf",
                            &( r->slotAnim[j].xOscPerSec ),
                            &( r->slotAnim[j].xAmp ),
                            &( r->slotAnim[j].xPhase ),
                                        
                            &( r->slotAnim[j].yOscPerSec ),
                            &( r->slotAnim[j].yAmp ),
                            &( r->slotAnim[j].yPhase ) ,

                            &( r->slotAnim[j].durationSec ),
                            &( r->slotAnim[j].pauseSec ) );
                    next++;
                    }
                            

                records.push_back( r );
                }
            for( int j=0; j<numLines; j++ ) {
                delete [] lines[j];
                }
            delete [] lines;
            }
        }
    delete [] txtFileName;


    currentFile ++;
    return (float)( currentFile ) / (float)( cache.numFiles );
    }



void initAnimationBankFinish() {
    
    freeFolderCache( cache );
    
    mapSize = maxID + 1;
    
    idMap = new AnimationRecord**[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = new AnimationRecord*[ endAnimType ];
        for( int j=0; j<endAnimType; j++ ) {    
            idMap[i][j] = NULL;
            }
        
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        AnimationRecord *r = records.getElementDirect(i);
        
        idMap[ r->objectID ][ r->type ] = r;
        }

    printf( "Loaded %d animations from animations folder\n", numRecords );
    }




void freeAnimationBank() {
    for( int i=0; i<mapSize; i++ ) {
        for( int j=0; j<endAnimType; j++ ) {
            if( idMap[i][j] != NULL ) {
            

                delete [] idMap[i][j]->spriteAnim;
                delete [] idMap[i][j]->slotAnim;

                delete idMap[i][j];
                }
            }
        delete [] idMap[i];
        }

    delete [] idMap;
    }




static File *getFile( int inID, AnimType inType ) {
    File animationsDir( NULL, "animations" );
            
    if( !animationsDir.exists() ) {
        animationsDir.makeDirectory();
        }
    
    if( animationsDir.exists() && animationsDir.isDirectory() ) {
        
        char *fileName = autoSprintf( "%d_%d.txt", inID, inType );


        File *animationFile = animationsDir.getChildFile( fileName );

        delete [] fileName;
        
        return animationFile;
        }
    
    return NULL;
    }



AnimationRecord *getAnimation( int inID, AnimType inType ) {
    if( inID < mapSize ) {
        return idMap[inID][inType];
        }
    return NULL;
    }




// record destroyed by caller
void addAnimation( AnimationRecord *inRecord ) {
    clearAnimation( inRecord->objectID,
                    inRecord->type );
    
    int newID = inRecord->objectID;
    
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;        

        
        AnimationRecord ***newMap = new AnimationRecord**[newMapSize];

        memcpy( newMap, idMap, sizeof(AnimationRecord*) * mapSize );

        delete [] idMap;
                
        for( int i=mapSize; i<newMapSize; i++ ) {
            newMap[i] = new AnimationRecord*[ endAnimType ];
            for( int j=0; j<endAnimType; j++ ) {    
                newMap[i][j] = NULL;
                }
            }

        idMap = newMap;
        mapSize = newMapSize;
        }

    
    // copy to add it to memory bank
    
    

    idMap[newID][inRecord->type] = copyRecord( inRecord );
    
    

    // store on disk
    File *animationFile = getFile( newID, inRecord->type );
    
    if( animationFile != NULL ) {
        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "id=%d", newID ) );
        lines.push_back( autoSprintf( "type=%d", inRecord->type ) );
        
        lines.push_back( 
            autoSprintf( "numSprites=%d", inRecord->numSprites ) );
        
        lines.push_back( 
            autoSprintf( "numSlots=%d", inRecord->numSlots ) );
        
        for( int j=0; j<inRecord->numSprites; j++ ) {
            lines.push_back( 
                autoSprintf( 
                    "animParam="
                    "%lf %lf %lf %lf %lf %lf (%lf,%lf) %lf %lf "
                    "%lf %lf %lf %lf %lf",
                    inRecord->spriteAnim[j].xOscPerSec,
                    inRecord->spriteAnim[j].xAmp,
                    inRecord->spriteAnim[j].xPhase,
                    
                    inRecord->spriteAnim[j].yOscPerSec,
                    inRecord->spriteAnim[j].yAmp,
                    inRecord->spriteAnim[j].yPhase,
                    
                    inRecord->spriteAnim[j].rotationCenterOffset.x,
                    inRecord->spriteAnim[j].rotationCenterOffset.y,

                    inRecord->spriteAnim[j].rotPerSec,
                    inRecord->spriteAnim[j].rotPhase,
                    
                    inRecord->spriteAnim[j].rockOscPerSec,
                    inRecord->spriteAnim[j].rockAmp,
                    inRecord->spriteAnim[j].rockPhase,
                    
                    inRecord->spriteAnim[j].durationSec,
                    inRecord->spriteAnim[j].pauseSec ) );
            }
        for( int j=0; j<inRecord->numSlots; j++ ) {
            lines.push_back( 
                autoSprintf( 
                    "animParam="
                    "%lf %lf %lf %lf %lf %lf %lf %lf",
                    inRecord->slotAnim[j].xOscPerSec,
                    inRecord->slotAnim[j].xAmp,
                    inRecord->slotAnim[j].xPhase,
                    
                    inRecord->slotAnim[j].yOscPerSec,
                    inRecord->slotAnim[j].yAmp,
                    inRecord->slotAnim[j].yPhase,
                    
                    inRecord->slotAnim[j].durationSec,
                    inRecord->slotAnim[j].pauseSec ) );
            }



        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );

        delete [] linesArray;
        lines.deallocateStringElements();
        
        
        File animationsDir( NULL, "animations" );
        
        File *cacheFile = animationsDir.getChildFile( "cache.fcz" );

        cacheFile->remove();
        
        delete cacheFile;


        animationFile->writeToFile( contents );
        
        delete [] contents;
        
        delete animationFile;
        }
    }




void clearAnimation( int inObjectID, AnimType inType ) {
    AnimationRecord *r = getAnimation( inObjectID, inType );
    
    if( r != NULL ) {
        
        delete [] r->spriteAnim;
        delete [] r->slotAnim;
        
        delete r;
        
        idMap[inObjectID][inType] = NULL;
        }

    
    File animationsDir( NULL, "animations" );


    File *cacheFile = animationsDir.getChildFile( "cache.fcz" );

    cacheFile->remove();
    
    delete cacheFile;


    File *animationFile = getFile( inObjectID, inType );
    animationFile->remove();
    
    delete animationFile;
    }



static double getOscOffset( double inFrameTime,
                            double inOscPerSec,
                            double inAmp,
                            double inPhase ) {
    return inAmp * sin( ( inFrameTime * inOscPerSec + inPhase ) * 2 * M_PI );
    }




char isAnimFadeNeeded( int inObjectID, AnimType inCurType, 
                       AnimType inTargetType ) {
    AnimationRecord *curR = getAnimation( inObjectID, inCurType );
    AnimationRecord *targetR = getAnimation( inObjectID, inTargetType );

    return isAnimFadeNeeded( inObjectID, curR, targetR );
    }


char isAnimFadeNeeded( int inObjectID,
                       AnimationRecord *inCurR, AnimationRecord *inTargetR ) {
        
    
    if( inCurR == NULL || inTargetR == NULL ) {
        return false;
        }

    ObjectRecord *obj = getObject( inObjectID );
    
    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        // if current is moving at all, must fade

        if( inCurR->spriteAnim[i].xOscPerSec > 0 ) return true;
        
        if( inCurR->spriteAnim[i].xAmp != 0 &&
            inCurR->spriteAnim[i].xPhase != 0 &&
            inCurR->spriteAnim[i].xPhase != 0.5 ) return true;
        
        if( inCurR->spriteAnim[i].yOscPerSec > 0 ) return true;
        
        if( inCurR->spriteAnim[i].yAmp != 0 &&
            inCurR->spriteAnim[i].yPhase != 0 &&
            inCurR->spriteAnim[i].yPhase != 0.5 ) return true;
        
        
        if( inCurR->spriteAnim[i].rockOscPerSec > 0 ) return true;
        
        if( inCurR->spriteAnim[i].rockAmp != 0 &&
            inCurR->spriteAnim[i].rockPhase != 0 &&
            inCurR->spriteAnim[i].rockPhase != 0.5 ) return true;
        
        
        if( inCurR->spriteAnim[i].rotPerSec != 0 ) return true;
        
        if( inCurR->spriteAnim[i].rotPhase != 0 ) return true;
        
        

        // if target starts out of phase, must fade
        
        if( inTargetR->spriteAnim[i].xAmp != 0 &&
            inTargetR->spriteAnim[i].xPhase != 0 &&
            inTargetR->spriteAnim[i].xPhase != 0.5 ) return true;
        
        if( inTargetR->spriteAnim[i].yAmp != 0 &&
            inTargetR->spriteAnim[i].yPhase != 0 &&
            inTargetR->spriteAnim[i].yPhase != 0.5 ) return true;
        
        if( inTargetR->spriteAnim[i].rockAmp != 0 &&
            inTargetR->spriteAnim[i].rockPhase != 0 &&
            inTargetR->spriteAnim[i].rockPhase != 0.5 ) return true;
        
        if( inTargetR->spriteAnim[i].rotPerSec != 0 || 
            inTargetR->spriteAnim[i].rotPhase != 0 ) return true;
        }
    
    
    for( int i=0; i<obj->numSlots; i++ ) {
        // if current is moving at all, must fade

        if( inCurR->slotAnim[i].xOscPerSec > 0 ) return true;
        
        if( inCurR->slotAnim[i].xAmp != 0 &&
            inCurR->slotAnim[i].xPhase != 0 &&
            inCurR->slotAnim[i].xPhase != 0.5 ) return true;
        
        if( inCurR->slotAnim[i].yOscPerSec > 0 ) return true;
        
        if( inCurR->slotAnim[i].yAmp != 0 &&
            inCurR->slotAnim[i].yPhase != 0 &&
            inCurR->slotAnim[i].yPhase != 0.5 ) return true;        
        

        // if target starts out of phase, must fade
        
        if( inTargetR->slotAnim[i].xAmp != 0 &&
            inTargetR->slotAnim[i].xPhase != 0 &&
            inTargetR->slotAnim[i].xPhase != 0.5 ) return true;
        
        if( inTargetR->slotAnim[i].yAmp != 0 &&
            inTargetR->slotAnim[i].yPhase != 0 &&
            inTargetR->slotAnim[i].yPhase != 0.5 ) return true;
        }
    
    // current animation lines up with start of target
    // no fade
    return false;
    }




char isAnimEmpty( int inObjectID, AnimType inType ) {
    AnimationRecord *r = getAnimation( inObjectID, inType );
    
    if( r == NULL ) {
        return true;
        }
    

    ObjectRecord *obj = getObject( inObjectID );
    
    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        if( r->spriteAnim[i].xOscPerSec > 0 ) return false;
        
        if( r->spriteAnim[i].xAmp != 0 &&
            r->spriteAnim[i].xPhase != 0 &&
            r->spriteAnim[i].xPhase != 0.5 ) return false;
        
        if( r->spriteAnim[i].yOscPerSec > 0 ) return false;
        
        if( r->spriteAnim[i].yAmp != 0 &&
            r->spriteAnim[i].yPhase != 0 &&
            r->spriteAnim[i].yPhase != 0.5 ) return false;
        
        
        if( r->spriteAnim[i].rockOscPerSec > 0 ) return false;
        
        if( r->spriteAnim[i].rockAmp != 0 &&
            r->spriteAnim[i].rockPhase != 0 &&
            r->spriteAnim[i].rockPhase != 0.5 ) return false;
        
        
        if( r->spriteAnim[i].rotPerSec != 0 ) return false;
        
        if( r->spriteAnim[i].rotPhase != 0 ) return false;

        }
    
    
    for( int i=0; i<obj->numSlots; i++ ) {
        // if current is moving at all, must fade

        if( r->slotAnim[i].xOscPerSec > 0 ) return false;
        
        if( r->slotAnim[i].xAmp != 0 &&
            r->slotAnim[i].xPhase != 0 &&
            r->slotAnim[i].xPhase != 0.5 ) return false;
        
        if( r->slotAnim[i].yOscPerSec > 0 ) return false;
        
        if( r->slotAnim[i].yAmp != 0 &&
            r->slotAnim[i].yPhase != 0 &&
            r->slotAnim[i].yPhase != 0.5 ) return false;        

        }
    
    return true;
    }




static char logicalXOR( char inA, char inB ) {
    return !inA != !inB;
    }




HandPos drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                        double inAnimFade,
                        AnimType inFadeTargetType,
                        double inFadeTargetFrameTime,
                        double inFrozenRotFrameTime,
                        char *outFrozenRotFrameTimeUsed,
                        doublePair inPos,
                        char inFlipH,
                        double inAge,
                        char inHoldingSomething,
                        ClothingSet inClothing ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        return drawObject( getObject( inObjectID ), inPos, 0, inFlipH, inAge, 
                           inHoldingSomething, inClothing );
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        AnimationRecord *rF = getAnimation( inObjectID, moving );
        
        return drawObjectAnim( inObjectID, r, inFrameTime,
                               inAnimFade, rB, 
                               inFadeTargetFrameTime, 
                               inFrozenRotFrameTime,
                               outFrozenRotFrameTimeUsed,
                               rF,
                               inPos, inFlipH, inAge, 
                               inHoldingSomething, inClothing );
        }
    }




// compute base pos and rot of each object
// THEN go back through list and compute compound effects by walking
// through tree of parent relationships.

// we need a place to store all of these intermediary pos and rots

// avoid mallocs every draw call by statically allocating this
// here, we assume no object has more than 1000 sprites
#define MAX_WORKING_SPRITES 1000
static doublePair workingSpritePos[1000];
static doublePair workingDeltaSpritePos[1000];
static double workingRot[1000];
static double workingDeltaRot[1000];




static double processFrameTimeWithPauses( AnimationRecord *inAnim,
                                          int inLayerIndex,
                                          // true if sprite, false if slot
                                          char inSpriteOrSlot,
                                          double inFrameTime ) {
    SpriteAnimationRecord *layerAnimation = NULL;
    
    int i = inLayerIndex;

    if( inSpriteOrSlot ) {
        if( i < inAnim->numSprites ) {
            layerAnimation = &( inAnim->spriteAnim[i] );
            }
        }
    else {
        if( i < inAnim->numSlots ) {
            layerAnimation = &( inAnim->slotAnim[i] );
            }
        }
    
    if( layerAnimation == NULL ) {
        return inFrameTime;
        }
    

    if( layerAnimation->pauseSec == 0 ) {
        return inFrameTime;
        }
    

    double dur = layerAnimation->durationSec;
    double pause = layerAnimation->pauseSec;
            

    double blockTime = dur + pause;
    
    double blockFraction = inFrameTime / blockTime;
            
    double numFullBlocksPassed = floor( blockFraction );

    double thisBlockFraction = blockFraction - numFullBlocksPassed;

    double thisBlockTime = thisBlockFraction * blockTime;
            
    if( thisBlockTime > dur ) {
        // in pause, freeze time at end of last dur
        return ( numFullBlocksPassed + 1 ) * dur;
        }
    else {
        // in a dur block
        return numFullBlocksPassed * dur + thisBlockTime;
        }
    }





HandPos drawObjectAnim( int inObjectID, AnimationRecord *inAnim, 
                        double inFrameTime,
                        double inAnimFade,
                        AnimationRecord *inFadeTargetAnim,
                        double inFadeTargetFrameTime,
                        double inFrozenRotFrameTime,
                        char *outFrozenRotFrameTimeUsed,
                        AnimationRecord *inFrozenRotAnim,
                        doublePair inPos,
                        char inFlipH,
                        double inAge,
                        char inHoldingSomething,
                        ClothingSet inClothing ) {

    HandPos returnHandPos = { false, {0, 0} };


    *outFrozenRotFrameTimeUsed = false;


    double frontHandPosX = -999999999;


    ObjectRecord *obj = getObject( inObjectID );
    
    if( obj->numSprites > MAX_WORKING_SPRITES ) {
        // cannot animate objects with this many sprites
        drawObject( obj, inPos, 0, inFlipH, inAge, 
                    inHoldingSomething, inClothing );
        }
    


    int headIndex = getHeadIndex( obj, inAge );
    int bodyIndex = getBodyIndex( obj, inAge );
    int backFootIndex = getBackFootIndex( obj, inAge );
    int frontFootIndex = getFrontFootIndex( obj, inAge );
    

    doublePair headPos = obj->spritePos[ headIndex ];

    doublePair frontFootPos = obj->spritePos[ frontFootIndex ];

    doublePair bodyPos = obj->spritePos[ bodyIndex ];
    

    doublePair animHeadPos = headPos;
    double animHeadRotDelta = 0;

    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        double spriteFrameTime = inFrameTime;
        
        double targetSpriteFrameTime = inFadeTargetFrameTime;
        

        spriteFrameTime = processFrameTimeWithPauses( inAnim,
                                                      i,
                                                      true,
                                                      spriteFrameTime );
        if( inAnimFade < 1 ) {
            targetSpriteFrameTime = 
                processFrameTimeWithPauses( inFadeTargetAnim,
                                            i,
                                            true,
                                            targetSpriteFrameTime );
            }
        


        doublePair spritePos = obj->spritePos[i];
        
        if( obj->person && i == headIndex ) {
            spritePos = add( spritePos, getAgeHeadOffset( inAge, headPos,
                                                          bodyPos,
                                                          frontFootPos ) );
            }
        else if( obj->person && i == bodyIndex ) {
            spritePos = add( spritePos, getAgeBodyOffset( inAge, bodyPos ) );
            }

        double rot = obj->spriteRot[i];
        

        if( i < inAnim->numSprites ) {
            

            spritePos.x += 
                inAnimFade * 
                getOscOffset( 
                    spriteFrameTime,
                    inAnim->spriteAnim[i].xOscPerSec,
                    inAnim->spriteAnim[i].xAmp,
                    inAnim->spriteAnim[i].xPhase );
            
            spritePos.y += 
                inAnimFade *
                getOscOffset( 
                    spriteFrameTime,
                    inAnim->spriteAnim[i].yOscPerSec,
                    inAnim->spriteAnim[i].yAmp,
                    inAnim->spriteAnim[i].yPhase );
            
            double rock = inAnimFade * 
                getOscOffset( spriteFrameTime,
                              inAnim->spriteAnim[i].rockOscPerSec,
                              inAnim->spriteAnim[i].rockAmp,
                              inAnim->spriteAnim[i].rockPhase );
            

            
            doublePair rotCenterOffset = 
                mult( inAnim->spriteAnim[i].rotationCenterOffset,
                      inAnimFade );
            
            double targetWeight = 1 - inAnimFade;
            
            if( inAnimFade < 1 && i < inFadeTargetAnim->numSprites ) {
                
                spritePos.x += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        inFadeTargetAnim->spriteAnim[i].xOscPerSec,
                        inFadeTargetAnim->spriteAnim[i].xAmp,
                        inFadeTargetAnim->spriteAnim[i].xPhase );
                
                spritePos.y += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        inFadeTargetAnim->spriteAnim[i].yOscPerSec,
                        inFadeTargetAnim->spriteAnim[i].yAmp,
                        inFadeTargetAnim->spriteAnim[i].yPhase );

                 rock += 
                     targetWeight *
                     getOscOffset( 
                         targetSpriteFrameTime,
                         inFadeTargetAnim->spriteAnim[i].rockOscPerSec,
                         inFadeTargetAnim->spriteAnim[i].rockAmp,
                         inFadeTargetAnim->spriteAnim[i].rockPhase );
                 
                 rotCenterOffset = 
                     add( rotCenterOffset,
                          mult( inFadeTargetAnim->
                                spriteAnim[i].rotationCenterOffset,
                                
                                targetWeight ) );
                }
            

            double totalRotOffset = 
                inAnim->spriteAnim[i].rotPerSec * 
                spriteFrameTime + 
                inAnim->spriteAnim[i].rotPhase;
            
            
            // use frozen rot instead if either current or
            // target satisfies 
            if( inAnim->type == held 
                &&
                inAnim->spriteAnim[i].rotPerSec == 0 &&
                inAnim->spriteAnim[i].rotPhase == 0 &&
                inAnim->spriteAnim[i].rockOscPerSec == 0 &&
                inAnim->spriteAnim[i].rockPhase == 0 
                &&
                inFrozenRotAnim->spriteAnim[i].rotPerSec != 0 ) {
                
                // use frozen instead
                totalRotOffset = 
                    inFrozenRotAnim->spriteAnim[i].rotPerSec * 
                    inFrozenRotFrameTime + 
                    inFrozenRotAnim->spriteAnim[i].rotPhase;
                
                *outFrozenRotFrameTimeUsed = 
                    *outFrozenRotFrameTimeUsed || true;
                }
            else if( inAnimFade < 1  && i < inFadeTargetAnim->numSprites
                     &&
                     inFadeTargetAnim->type == held 
                     &&
                     inFadeTargetAnim->spriteAnim[i].rotPerSec == 0 &&
                     inFadeTargetAnim->spriteAnim[i].rotPhase == 0 &&
                     inFadeTargetAnim->spriteAnim[i].rockOscPerSec == 0 &&
                     inFadeTargetAnim->spriteAnim[i].rockPhase == 0 
                     &&
                     inFrozenRotAnim->spriteAnim[i].rotPerSec != 0 ) {
                
                // use frozen instead
                totalRotOffset = 
                    inFrozenRotAnim->spriteAnim[i].rotPerSec * 
                    inFrozenRotFrameTime + 
                    inFrozenRotAnim->spriteAnim[i].rotPhase;

                *outFrozenRotFrameTimeUsed = 
                    *outFrozenRotFrameTimeUsed || true;
                }
            

            // relative to 0 on circle
            double relativeRotOffset = 
                totalRotOffset - floor( totalRotOffset );
            
            // make positive
            if( relativeRotOffset < 0 ) {
                relativeRotOffset += 1;
                }
            
            // to take average of two rotations
            // rotate them both so that one is at 0.5,
            // take average, and then rotate them back
            // This ensures that we always move through closest average
            // point on circle (shortest path along circle).

            double offset = 0.5 - relativeRotOffset;
            


            if( inAnimFade < 1  && i < inFadeTargetAnim->numSprites ) {
                double totalTargetRotOffset =
                    inFadeTargetAnim->spriteAnim[i].rotPerSec * 
                    targetSpriteFrameTime + 
                    inFadeTargetAnim->spriteAnim[i].rotPhase;
                

                if( inFadeTargetAnim->type == held 
                    &&
                    inFadeTargetAnim->spriteAnim[i].rotPerSec == 0 &&
                    inFadeTargetAnim->spriteAnim[i].rotPhase == 0 &&
                    inFadeTargetAnim->spriteAnim[i].rockOscPerSec == 0 &&
                    inFadeTargetAnim->spriteAnim[i].rockPhase == 0 
                    &&
                    inFrozenRotAnim->spriteAnim[i].rotPerSec != 0 ) {
                
                    // use frozen instead
                    totalTargetRotOffset = 
                        inFrozenRotAnim->spriteAnim[i].rotPerSec * 
                        inFrozenRotFrameTime + 
                        inFrozenRotAnim->spriteAnim[i].rotPhase;
                    
                    *outFrozenRotFrameTimeUsed = 
                        *outFrozenRotFrameTimeUsed || true;
                    }


                // relative to 0 on circle
                double relativeTargetRotOffset = 
                    totalTargetRotOffset - floor( totalTargetRotOffset );
            
                // make positive
                if( relativeTargetRotOffset < 0 ) {
                    relativeTargetRotOffset += 1;
                    }

                
                double centeredOffset = offset + relativeRotOffset;
                double centeredTargetOffset = offset + relativeTargetRotOffset;
                
                if( centeredTargetOffset < 0 ) {
                    centeredTargetOffset += 1;
                    }
                else if( centeredTargetOffset > 1 ) {
                    centeredTargetOffset -= 1;
                    }
                

                double aveCenteredOffset = 
                    inAnimFade * centeredOffset +
                    targetWeight * centeredTargetOffset;
                
                // remove to-0.5 offset
                double aveOffset = aveCenteredOffset - offset;

                rot += aveOffset;
                }
            else {
                rot += relativeRotOffset;
                }
            

            rot += rock;


            if( rotCenterOffset.x != 0 ||
                rotCenterOffset.y != 0 ) {
                
                // move spritePos as if it were applying rot based
                // on this offset
                // the rot itself will be applied later

                doublePair newCenter = rotate( rotCenterOffset,
                                               - 2 * M_PI * rot );
                
                doublePair delta = sub( newCenter, rotCenterOffset );
                spritePos = sub( spritePos, delta );
                }
                
            }
        

        
        workingSpritePos[i] = spritePos;
        workingRot[i] = rot;
        
        workingDeltaSpritePos[i] = sub( spritePos, obj->spritePos[i] );
        workingDeltaRot[i] = rot - obj->spriteRot[i];
        }


    // if drawing a tunic, skip drawing body and ALL
    // layers above body (except feet) until
    // a holder (hand) or head has been drawn above the body
    char holderOrHeadDrawnAboveBody = true;
    
    
    // now that their individual animations have been computed
    // walk through and follow parent chains to compute compound animations
    // for each one before drawing
    for( int i=0; i<obj->numSprites; i++ ) {
        
        if( obj->person &&
            ( obj->spriteAgeStart[i] != -1 ||
              obj->spriteAgeEnd[i] != -1 ) ) {
            
            if( inAge < obj->spriteAgeStart[i] ||
                inAge >= obj->spriteAgeEnd[i] ) {
                
                // skip drawing this aging layer entirely
                continue;
                }
            }

        
        doublePair spritePos = workingSpritePos[i];
        double rot = workingRot[i];
        

        int nextParent = obj->spriteParent[i];
        
        while( nextParent != -1 ) {
            
            
            
            if( workingDeltaRot[nextParent] != 0 ) {
                rot += workingDeltaRot[ nextParent ];
            
                double angle = - 2 * M_PI * workingDeltaRot[ nextParent ];

                spritePos = add( spritePos, 
                                 rotate( workingDeltaSpritePos[ nextParent ],
                                         -angle ) );

                // add in positional change based on arm's length rotation
                // around parent
            
                doublePair childOffset = 
                    sub( spritePos, 
                         obj->spritePos[nextParent] );
                
                doublePair newChildOffset =  rotate( childOffset, angle );
                
                spritePos = 
                    add( spritePos, sub( newChildOffset, childOffset ) );
                }
            else {
                spritePos = add( spritePos, 
                                 workingDeltaSpritePos[ nextParent ] );
                }
                    
	    nextParent = obj->spriteParent[nextParent];
            }


        if( i == headIndex ) {
            // this is the head
            animHeadPos = spritePos;
            animHeadRotDelta = rot - obj->spriteRot[i];
            }


        if( inFlipH ) {
            spritePos.x *= -1;
            rot *= -1;

            animHeadPos.x *= -1;
            animHeadRotDelta *= -1;
            }
        
        

        
        doublePair pos = add( spritePos, inPos );


        char skipSprite = false;
        
        if( obj->spriteInvisibleWhenHolding[i] ) {
            holderOrHeadDrawnAboveBody = true;
            
            if( inHoldingSomething ) {
                skipSprite = true;
                }
            }
        if( i == headIndex ) {
            holderOrHeadDrawnAboveBody = true;
            }


        if( i == backFootIndex 
            && inClothing.backShoe != NULL ) {
            
            skipSprite = true;

            doublePair offset = inClothing.backShoe->clothingOffset;

            if( inFlipH ) {
                offset.x *= -1;
                }
            
            if( rot != 0 ) {
                offset = rotate( offset, -2 * M_PI * rot );
                }

            doublePair cPos = add( spritePos, offset );

            cPos = add( cPos, inPos );
            
            drawObject( inClothing.backShoe, cPos, rot,
                        inFlipH, -1, false, emptyClothing );
            }

        if( i == bodyIndex 
            && inClothing.tunic != NULL ) {
            skipSprite = true;

            doublePair offset = inClothing.tunic->clothingOffset;
            
            if( inFlipH ) {
                offset.x *= -1;
                }
            
            if( rot != 0 ) {
                offset = rotate( offset, -2 * M_PI * rot );
                }
            
            
            doublePair cPos = add( spritePos, offset );
            
            cPos = add( cPos, inPos );
            
            drawObject( inClothing.tunic, cPos, rot,
                        inFlipH, -1, false, emptyClothing );
            
            // now skip all non-foot layers drawn above body
            // until holder or head drawn
            holderOrHeadDrawnAboveBody = false;
            }
        else if( inClothing.tunic != NULL && ! holderOrHeadDrawnAboveBody &&
                 i != frontFootIndex  &&
                 i != backFootIndex &&
                 i != headIndex ) {
            // skip it, it's under tunic
            skipSprite = true;
            }
        

        if( i == frontFootIndex 
            && inClothing.frontShoe != NULL ) {
        
            skipSprite = true;
            
            doublePair offset = inClothing.frontShoe->clothingOffset;
                
            if( inFlipH ) {
                offset.x *= -1;
                }

            if( rot != 0 ) {
                offset = rotate( offset, -2 * M_PI * rot );
                }
                

            doublePair cPos = add( spritePos, offset );

            cPos = add( cPos, inPos );
                
            drawObject( inClothing.frontShoe, cPos, rot,
                        inFlipH, -1, false, emptyClothing );
            }
        


        if( !skipSprite ) {
            setDrawColor( obj->spriteColor[i] );
            
            char multiplicative = 
                getUsesMultiplicativeBlending( obj->sprites[i] );
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( true );
                }

            drawSprite( getSprite( obj->sprites[i] ), pos, 1.0, rot, 
                        logicalXOR( inFlipH, obj->spriteHFlip[i] ) );

            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                }


            // compare raw sprite pos x to find front-most drawn hand
            // in unanimated, unflipped object
            if( obj->spriteInvisibleWhenHolding[i] &&
                obj->spritePos[i].x > frontHandPosX ) {
                
                frontHandPosX = obj->spritePos[i].x;

                returnHandPos.valid = true;
                // return screen pos for hand, which may be flipped, etc.
                returnHandPos.pos = pos;
                }
            }
        
        } 


    if( inClothing.hat != NULL ) {

        // hat on top of everything
            
        // relative to head

        doublePair offset = inClothing.hat->clothingOffset;

        
        if( inFlipH ) {
            offset.x *= -1;
            }                
        
        if( animHeadRotDelta != 0 ) {
            offset = rotate( offset, -2 * M_PI * animHeadRotDelta );
            }
        
        
        doublePair cPos = add( animHeadPos, offset );

        cPos = add( cPos, inPos );
        
        drawObject( inClothing.hat, cPos, animHeadRotDelta,
                    inFlipH, -1, false, emptyClothing );
        }

    return returnHandPos;
    }



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inAnimFade, 
                     AnimType inFadeTargetType,
                     double inFadeTargetFrameTime,
                     double inFrozenRotFrameTime,
                     char *outFrozenRotFrameTimeUsed,
                     doublePair inPos,
                     char inFlipH,
                     double inAge,
                     char inHoldingSomething,
                     ClothingSet inClothing,
                     int inNumContained, int *inContainedIDs ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, 0, 
                    inFlipH, inAge, inHoldingSomething, inClothing,
                    inNumContained, inContainedIDs );
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        AnimationRecord *rF = getAnimation( inObjectID, moving );
        
        drawObjectAnim( inObjectID, r, inFrameTime,
                        inAnimFade, rB, inFadeTargetFrameTime,
                        inFrozenRotFrameTime,
                        outFrozenRotFrameTimeUsed,
                        rF,
                        inPos, inFlipH, inAge,
                        inHoldingSomething, inClothing,
                        inNumContained, inContainedIDs );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim,
                     double inFrameTime, 
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     double inFadeTargetFrameTime,
                     double inFrozenRotFrameTime,
                     char *outFrozenRotFrameTimeUsed,
                     AnimationRecord *inFrozenRotAnim,
                     doublePair inPos,
                     char inFlipH,
                     double inAge,
                     char inHoldingSomething,
                     ClothingSet inClothing,
                     int inNumContained, int *inContainedIDs ) {
    
    ClothingSet emptyClothing = getEmptyClothingSet();
    
    // first, draw jiggling (never rotating) objects in slots
    // can't safely rotate them, because they may be compound objects
    
    ObjectRecord *obj = getObject( inObjectID );

    for( int i=0; i<obj->numSlots; i++ ) {
        if( i < inNumContained ) {


            double slotFrameTime = inFrameTime;
            double targetSlotFrameTime = inFadeTargetFrameTime;
            
            slotFrameTime = processFrameTimeWithPauses( inAnim,
                                                        i,
                                                        false,
                                                        slotFrameTime );
            
            if( inAnimFade < 1 ) {
                targetSlotFrameTime = 
                    processFrameTimeWithPauses( inFadeTargetAnim,
                                                i,
                                                false,
                                                targetSlotFrameTime );
                }

            ObjectRecord *contained = getObject( inContainedIDs[i] );

            doublePair pos = obj->slotPos[i];

            pos = sub( pos, getObjectCenterOffset( contained ) );
            
        
            if( i < inAnim->numSlots ) {
                
                pos.x += 
                    inAnimFade *
                    getOscOffset( 
                        slotFrameTime,
                        inAnim->slotAnim[i].xOscPerSec,
                        inAnim->slotAnim[i].xAmp,
                        inAnim->slotAnim[i].xPhase );
                
                pos.y += 
                    inAnimFade * 
                    getOscOffset( 
                        slotFrameTime,
                        inAnim->slotAnim[i].yOscPerSec,
                        inAnim->slotAnim[i].yAmp,
                        inAnim->slotAnim[i].yPhase );

                if( inAnimFade < 1 ) {
                    double targetWeight = 1 - inAnimFade;
                
                    pos.x += 
                        targetWeight *
                        getOscOffset( 
                            targetSlotFrameTime,
                            inFadeTargetAnim->slotAnim[i].xOscPerSec,
                            inFadeTargetAnim->slotAnim[i].xAmp,
                            inFadeTargetAnim->slotAnim[i].xPhase );
                    
                    pos.y += 
                        targetWeight *
                        getOscOffset( 
                            targetSlotFrameTime,
                            inFadeTargetAnim->slotAnim[i].yOscPerSec,
                            inFadeTargetAnim->slotAnim[i].yAmp,
                            inFadeTargetAnim->slotAnim[i].yPhase );
                    }
                }
                  
            if( inFlipH ) {
                pos.x *= -1;
                }
  
            pos = add( pos, inPos );
            drawObject( contained, pos, 0, inFlipH,
                        inAge, false, emptyClothing );
            }
        
        } 

    // draw animating object on top of contained slots
    drawObjectAnim( inObjectID, inAnim, inFrameTime,
                    inAnimFade, inFadeTargetAnim, inFadeTargetFrameTime,
                    inFrozenRotFrameTime,
                    outFrozenRotFrameTimeUsed,
                    inFrozenRotAnim,
                    inPos, inFlipH,
                    inAge, inHoldingSomething, inClothing );
    }



AnimationRecord *copyRecord( AnimationRecord *inRecord ) {
    AnimationRecord *newRecord = new AnimationRecord;
    
    newRecord->objectID = inRecord->objectID;
    newRecord->type = inRecord->type;
    
    newRecord->numSprites = inRecord->numSprites;
    newRecord->numSlots = inRecord->numSlots;
    
    newRecord->spriteAnim = new SpriteAnimationRecord[ newRecord->numSprites ];
    newRecord->slotAnim = new SpriteAnimationRecord[ newRecord->numSlots ];
    
    memcpy( newRecord->spriteAnim, inRecord->spriteAnim,
            sizeof( SpriteAnimationRecord ) * newRecord->numSprites );
    
    memcpy( newRecord->slotAnim, inRecord->slotAnim,
            sizeof( SpriteAnimationRecord ) * newRecord->numSlots );
    
    return newRecord;
    }



void zeroRecord( SpriteAnimationRecord *inRecord ) {
    inRecord->xOscPerSec = 0;
    inRecord->xAmp = 0;
    inRecord->xPhase = 0;
    
    inRecord->yOscPerSec = 0;
    inRecord->yAmp = 0;
    inRecord->yPhase = 0;
    
    inRecord->rotPerSec = 0;
    inRecord->rotPhase = 0;

    inRecord->rockOscPerSec = 0;
    inRecord->rockAmp = 0;
    inRecord->rockPhase = 0;

    inRecord->durationSec = 1;
    inRecord->pauseSec = 0;
    
    inRecord->rotationCenterOffset.x = 0;
    inRecord->rotationCenterOffset.y = 0;
    }




void performLayerSwaps( int inObjectID, 
                        SimpleVector<LayerSwapRecord> *inSwapList,
                        int inNewNumSprites ) {

    int maxSwapIndex = inNewNumSprites;
    for( int i=0; i<inSwapList->size(); i++ ) {
        LayerSwapRecord *r = inSwapList->getElement(i);
        
        if( r->indexA > maxSwapIndex  ) {
            maxSwapIndex = r->indexA;
            }
        if( r->indexB > maxSwapIndex  ) {
            maxSwapIndex = r->indexB;
            }
        }
    
        

    for( int i=0; i<endAnimType; i++ ) {
        AnimationRecord *oldAnim = getAnimation( inObjectID, (AnimType)i );
        
        if( oldAnim == NULL ) {
            continue;
            }
        
        AnimationRecord *r = copyRecord( oldAnim );
        
        

        int numTempSprites = maxSwapIndex + 1;
        
        SpriteAnimationRecord *tempSpriteAnim = 
            new SpriteAnimationRecord[ numTempSprites ];
        

        for( int j=0; j<numTempSprites; j++ ) {
            zeroRecord( &tempSpriteAnim[j] );
            }
        
        // populate with old anim records (any extra are zeroed above)
        for( int j=0; j<r->numSprites && j<numTempSprites; j++ ) {
            tempSpriteAnim[j] = r->spriteAnim[j];
            }
        
        // now apply swaps
        for( int j=0; j<inSwapList->size(); j++ ) {
            LayerSwapRecord *swap = inSwapList->getElement(j);

            SpriteAnimationRecord temp = tempSpriteAnim[ swap->indexA ];
            
            tempSpriteAnim[ swap->indexA ] = tempSpriteAnim[ swap->indexB ];
            
            tempSpriteAnim[ swap->indexB ] = temp;
            }
        

        delete [] r->spriteAnim;
        
        r->numSprites = inNewNumSprites;
        
        r->spriteAnim = new SpriteAnimationRecord[ r->numSprites ];
        
        for( int j=0; j<r->numSprites; j++ ) {
            r->spriteAnim[j] = tempSpriteAnim[j];
            }
        
        delete [] tempSpriteAnim;

        addAnimation( r );
        
        delete [] r->spriteAnim;
        delete [] r->slotAnim;
        delete r;
        }
    

    }

        

