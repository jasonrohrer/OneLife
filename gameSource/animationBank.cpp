#include "animationBank.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "soundBank.h"

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
                int randomStartPhaseRead = 0;
                
                sscanf( lines[next], "type=%d,randStartPhase=%d", 
                        &( typeRead ), &randomStartPhaseRead );
                r->type = (AnimType)typeRead;
                r->randomStartPhase = randomStartPhaseRead;
                next++;
                
                
                // optional sounds
                r->numSounds = 0;
                if( strstr( lines[next], "numSounds=" ) != NULL ) {
                    r->numSounds = 0;
                    sscanf( lines[next], "numSounds=%d", 
                            &( r->numSounds ) );
                    next++;
                    }
                r->soundAnim = new SoundAnimationRecord[ r->numSounds ];

                if( r->numSounds > 0 ) {
                    
                    for( int j=0; j< r->numSounds && next < numLines; j++ ) {
                                        
                        r->soundAnim[j].sound = blankSoundUsage;
                        r->soundAnim[j].repeatPerSec = 0;
                        r->soundAnim[j].repeatPhase = 0;
                        r->soundAnim[j].ageStart = -1;
                        r->soundAnim[j].ageEnd = -1;
                        
                        sscanf( lines[next], 
                                "soundParam=%d %lf %lf %lf %lf %lf",
                                &( r->soundAnim[j].sound.id ),
                                &( r->soundAnim[j].sound.volume ),
                                &( r->soundAnim[j].repeatPerSec ),
                                &( r->soundAnim[j].repeatPhase ),
                                &( r->soundAnim[j].ageStart ),
                                &( r->soundAnim[j].ageEnd ) );
                        next++;
                        }
                    }


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
                    
                    
                    r->spriteAnim[j].fadeOscPerSec = 0;
                    r->spriteAnim[j].fadeHardness = 0;
                    
                    r->spriteAnim[j].fadeMin = 0;
                    r->spriteAnim[j].fadeMax = 1;

                    r->spriteAnim[j].fadePhase = 0;
                    
                    sscanf( lines[next], 
                            "animParam="
                            "%lf %lf %lf %lf %lf %lf (%lf,%lf) %lf %lf "
                            "%lf %lf %lf %lf %lf "
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
                            &( r->spriteAnim[j].pauseSec ),
                            
                            &( r->spriteAnim[j].fadeOscPerSec ),
                            &( r->spriteAnim[j].fadeHardness ),
                            &( r->spriteAnim[j].fadeMin ),
                            &( r->spriteAnim[j].fadeMax ),
                            &( r->spriteAnim[j].fadePhase ) );

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



static float *animLayerFades = NULL;

void setAnimLayerFades( float *inFades ) {
    if( animLayerFades != NULL ) {
        delete [] animLayerFades;
        animLayerFades = NULL;
        }
    animLayerFades = inFades;
    }



void freeAnimationBank() {
    if( animLayerFades != NULL ) {
        delete [] animLayerFades;
        animLayerFades = NULL;
        }
    
    for( int i=0; i<mapSize; i++ ) {
        for( int j=0; j<endAnimType; j++ ) {
            if( idMap[i][j] != NULL ) {

                delete [] idMap[i][j]->soundAnim;

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
    if( inID < mapSize && inType < endAnimType && inType != ground2 ) {
        return idMap[inID][inType];
        }
    return NULL;
    }




// record destroyed by caller
void addAnimation( AnimationRecord *inRecord ) {
    
    AnimationRecord *oldRecord = getAnimation( inRecord->objectID, 
                                               inRecord->type );
    
    SimpleVector<int> oldSoundIDs;
    if( oldRecord != NULL ) {
        for( int i=0; i<oldRecord->numSounds; i++ ) {
            if( oldRecord->soundAnim[i].sound.id != -1 ) {
                oldSoundIDs.push_back( oldRecord->soundAnim[i].sound.id );
                }
            }
        }
    

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
        lines.push_back( autoSprintf( "type=%d,randStartPhase=%d", 
                                      inRecord->type, 
                                      inRecord->randomStartPhase ) );

        lines.push_back( 
            autoSprintf( "numSounds=%d", inRecord->numSounds ) );

        for( int j=0; j<inRecord->numSounds; j++ ) {
            lines.push_back( autoSprintf( 
                                 "soundParam=%d %lf %lf %lf %lf %lf",
                                 inRecord->soundAnim[j].sound.id,
                                 inRecord->soundAnim[j].sound.volume,
                                 inRecord->soundAnim[j].repeatPerSec,
                                 inRecord->soundAnim[j].repeatPhase,
                                 inRecord->soundAnim[j].ageStart,
                                 inRecord->soundAnim[j].ageEnd ) );
            }
        
        lines.push_back( 
            autoSprintf( "numSprites=%d", inRecord->numSprites ) );
        
        lines.push_back( 
            autoSprintf( "numSlots=%d", inRecord->numSlots ) );
        
        for( int j=0; j<inRecord->numSprites; j++ ) {
            lines.push_back( 
                autoSprintf( 
                    "animParam="
                    "%lf %lf %lf %lf %lf %lf (%lf,%lf) %lf %lf "
                    "%lf %lf %lf %lf %lf "
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
                    inRecord->spriteAnim[j].pauseSec,

                    inRecord->spriteAnim[j].fadeOscPerSec,
                    inRecord->spriteAnim[j].fadeHardness,
                    inRecord->spriteAnim[j].fadeMin,
                    inRecord->spriteAnim[j].fadeMax,
                    inRecord->spriteAnim[j].fadePhase ) );
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

    
    // check if sounds still used (prevent orphan sounds)
    for( int i=0; i<oldSoundIDs.size(); i++ ) {
        checkIfSoundStillNeeded( oldSoundIDs.getElementDirect( i ) );
        }
    }




void clearAnimation( int inObjectID, AnimType inType ) {
    AnimationRecord *r = getAnimation( inObjectID, inType );
    
    if( r != NULL ) {
        
        delete [] r->soundAnim;
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

    if( ( inCurType != ground2 && inTargetType == ground2 )
        ||
        ( inCurType == ground2 && inTargetType != ground2 ) ) {
        
        return true;
        }
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
        

        if( inCurR->spriteAnim[i].fadeOscPerSec > 0 ) return true;

        if( inCurR->spriteAnim[i].fadeMax != 1 ||
            inCurR->spriteAnim[i].fadePhase != 0 ) return true;



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
        
        
        if( inTargetR->spriteAnim[i].fadeMax != 1 ||
            inTargetR->spriteAnim[i].fadePhase != 0 ) return true;
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




ObjectAnimPack drawObjectAnimPacked( 
    int inObjectID,
    AnimType inType, double inFrameTime, 
    double inAnimFade,
    AnimType inFadeTargetType,
    double inFadeTargetFrameTime,
    double inFrozenRotFrameTime,
    char *outFrozenRotFrameTimeUsed,
    // set endAnimType for no frozen arms
    AnimType inFrozenArmType,
    AnimType inFrozenArmFadeTargetType,
    doublePair inPos,
    double inRot,
    char inWorn,
    char inFlipH,
    double inAge,
    int inHideClosestArm,
    char inHideAllLimbs,
    char inHeldNotInPlaceYet,
    ClothingSet inClothing,
    SimpleVector<int> *inClothingContained,
    int inNumContained, int *inContainedIDs ) {
    
    ObjectAnimPack outPack = {
        inObjectID,
        inType,
        inFrameTime, 
        inAnimFade,
        inFadeTargetType,
        inFadeTargetFrameTime,
        inFrozenRotFrameTime,
        outFrozenRotFrameTimeUsed,
        inFrozenArmType,
        inFrozenArmFadeTargetType,
        inPos,
        inRot,
        inWorn,
        inFlipH,
        inAge,
        inHideClosestArm,
        inHideAllLimbs,
        inHeldNotInPlaceYet,
        inClothing,
        inClothingContained,
        inNumContained,
        inContainedIDs };
    
    return outPack;
    }

        


void drawObjectAnim( ObjectAnimPack inPack ) {
    if( inPack.inContainedIDs == NULL ) {
        drawObjectAnim( 
            inPack.inObjectID,
            2,
            inPack.inType,
            inPack.inFrameTime, 
            inPack.inAnimFade,
            inPack.inFadeTargetType,
            inPack.inFadeTargetFrameTime,
            inPack.inFrozenRotFrameTime,
            inPack.outFrozenRotFrameTimeUsed,
            inPack.inFrozenArmType,
            inPack.inFrozenArmFadeTargetType,
            inPack.inPos,
            inPack.inRot,
            inPack.inWorn,
            inPack.inFlipH,
            inPack.inAge,
            inPack.inHideClosestArm,
            inPack.inHideAllLimbs,
            inPack.inHeldNotInPlaceYet,
            inPack.inClothing,
            inPack.inClothingContained );
        }
    else {
        drawObjectAnim( 
            inPack.inObjectID,
            inPack.inType,
            inPack.inFrameTime, 
            inPack.inAnimFade,
            inPack.inFadeTargetType,
            inPack.inFadeTargetFrameTime,
            inPack.inFrozenRotFrameTime,
            inPack.outFrozenRotFrameTimeUsed,
            inPack.inFrozenArmType,
            inPack.inFrozenArmFadeTargetType,
            inPack.inPos,
            inPack.inRot,
            inPack.inWorn,
            inPack.inFlipH,
            inPack.inAge,
            inPack.inHideClosestArm,
            inPack.inHideAllLimbs,
            inPack.inHeldNotInPlaceYet,
            inPack.inClothing,
            inPack.inClothingContained,
            inPack.inNumContained,
            inPack.inContainedIDs );
        }
    }




HoldingPos drawObjectAnim( int inObjectID, int inDrawBehindSlots,
                           AnimType inType, double inFrameTime,
                           double inAnimFade,
                           AnimType inFadeTargetType,
                           double inFadeTargetFrameTime,
                           double inFrozenRotFrameTime,
                           char *outFrozenRotFrameTimeUsed,
                           AnimType inFrozenArmType,
                           AnimType inFrozenArmFadeTargetType,
                           doublePair inPos,
                           double inRot,
                           char inWorn,
                           char inFlipH,
                           double inAge,
                           int inHideClosestArm,
                           char inHideAllLimbs,
                           char inHeldNotInPlaceYet,
                           ClothingSet inClothing,
                           SimpleVector<int> *inClothingContained ) {
    
    if( inType == ground2 ) {
        inType = ground;
        }

    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        return drawObject( getObject( inObjectID ), inDrawBehindSlots,
                           inPos, inRot, inWorn, 
                           inFlipH, inAge, 
                           inHideClosestArm, inHideAllLimbs, 
                           inHeldNotInPlaceYet,
                           inClothing );
        }
    else {
        if( inFadeTargetType == ground2 ) {
            inFadeTargetType = ground;
            }

        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        AnimationRecord *rF = getAnimation( inObjectID, moving );
        
        AnimationRecord *rArm = getAnimation( inObjectID, inFrozenArmType );
        AnimationRecord *rArmFade = getAnimation( inObjectID, 
                                                  inFrozenArmFadeTargetType );
        
        if( rB == NULL ) {
            rB = getAnimation( inObjectID, ground );
            }
        if( rF == NULL ) {
            rF = getAnimation( inObjectID, ground );
            }
        
                
        return drawObjectAnim( inObjectID, inDrawBehindSlots, 
                               r, inFrameTime,
                               inAnimFade, rB, 
                               inFadeTargetFrameTime, 
                               inFrozenRotFrameTime,
                               outFrozenRotFrameTimeUsed,
                               rF,
                               rArm,
                               rArmFade,
                               inPos, inRot, inWorn, inFlipH, inAge, 
                               inHideClosestArm, inHideAllLimbs, 
                               inHeldNotInPlaceYet,
                               inClothing,
                               inClothingContained );
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
static double workingSpriteFade[1000];



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





HoldingPos drawObjectAnim( int inObjectID, int inDrawBehindSlots,
                           AnimationRecord *inAnim, 
                           double inFrameTime,
                           double inAnimFade,
                           AnimationRecord *inFadeTargetAnim,
                           double inFadeTargetFrameTime,
                           double inFrozenRotFrameTime,
                           char *outFrozenRotFrameTimeUsed,
                           AnimationRecord *inFrozenRotAnim,
                           AnimationRecord *inFrozenArmAnim,
                           AnimationRecord *inFrozenArmFadeTargetAnim,
                           doublePair inPos,
                           double inRot, 
                           char inWorn,
                           char inFlipH,
                           double inAge,
                           int inHideClosestArm,
                           char inHideAllLimbs,
                           char inHeldNotInPlaceYet,
                           ClothingSet inClothing,
                           SimpleVector<int> *inClothingContained ) {

    HoldingPos returnHoldingPos = { false, {0, 0}, 0 };



    *outFrozenRotFrameTimeUsed = false;



    ObjectRecord *obj = getObject( inObjectID );
    
    if( obj->numSprites > MAX_WORKING_SPRITES ) {
        // cannot animate objects with this many sprites
        drawObject( obj, inDrawBehindSlots,
                    inPos, inRot, inWorn, inFlipH, inAge, 
                    inHideClosestArm, inHideAllLimbs, inHeldNotInPlaceYet,
                    inClothing );
        return returnHoldingPos;
        }

    SimpleVector <int> frontArmIndices;
    getFrontArmIndices( obj, inAge, &frontArmIndices );

    SimpleVector <int> backArmIndices;
    getBackArmIndices( obj, inAge, &backArmIndices );

    SimpleVector <int> legIndices;
    getAllLegIndices( obj, inAge, &legIndices );

    


    int headIndex = getHeadIndex( obj, inAge );
    int bodyIndex = getBodyIndex( obj, inAge );
    int backFootIndex = getBackFootIndex( obj, inAge );
    int frontFootIndex = getFrontFootIndex( obj, inAge );


    int topBackArmIndex = -1;
    
    if( backArmIndices.size() > 0 ) {
        topBackArmIndex =
            backArmIndices.getElementDirect( backArmIndices.size() - 1 );
        }

    
    int backHandIndex = getBackHandIndex( obj, inAge );
    
    doublePair headPos = obj->spritePos[ headIndex ];

    doublePair frontFootPos = obj->spritePos[ frontFootIndex ];

    doublePair bodyPos = obj->spritePos[ bodyIndex ];
    

    doublePair animHeadPos = headPos;
    double animHeadRotDelta = 0;

    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        double spriteFrameTime = inFrameTime;
        
        double targetSpriteFrameTime = inFadeTargetFrameTime;
        
        AnimationRecord *spriteAnim = inAnim;
        AnimationRecord *spriteFadeTargetAnim = inFadeTargetAnim;
        
        if( frontArmIndices.getElementIndex( i ) != -1 ||
            backArmIndices.getElementIndex( i ) != -1 ) {
            
            if( inFrozenArmAnim != NULL ) {
                spriteAnim = inFrozenArmAnim;
                spriteFrameTime = 0;
                }
            if( inFrozenArmFadeTargetAnim != NULL ) {
                spriteFadeTargetAnim = inFrozenArmFadeTargetAnim;
                targetSpriteFrameTime = 0;
                }
            }
        

        spriteFrameTime = processFrameTimeWithPauses( spriteAnim,
                                                      i,
                                                      true,
                                                      spriteFrameTime );
        if( inAnimFade < 1 ) {
            targetSpriteFrameTime = 
                processFrameTimeWithPauses( spriteFadeTargetAnim,
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

        double rot = 0;
        
        workingSpriteFade[i] = 1.0f;
        
        if( i < spriteAnim->numSprites ) {

            double sinVal = getOscOffset( 
                spriteFrameTime,
                spriteAnim->spriteAnim[i].fadeOscPerSec,
                1.0,
                spriteAnim->spriteAnim[i].fadePhase + .25 );
            
            double hardVersion;
            
            // hardened sin formula found here:
            // https://thatsmaths.com/2015/12/31/
            //         squaring-the-circular-functions/
            double hardness = spriteAnim->spriteAnim[i].fadeHardness;

            if( hardness == 1 ) {
                
                if( sinVal > 0  ) {
                    hardVersion = 1;
                    }
                else {
                    hardVersion = -1;
                    }
                }
            else {
                double absSinVal = fabs( sinVal );
                
                if( absSinVal != 0 ) {
                    hardVersion = ( sinVal / absSinVal ) * 
                        pow( absSinVal, 
                             1.0 / ( hardness * 10 + 1 ) );
                    }
                else {
                    hardVersion = 0;
                    }
                }

            double fade =
                (spriteAnim->spriteAnim[i].fadeMax - 
                 spriteAnim->spriteAnim[i].fadeMin ) *
                ( 0.5 * hardVersion + 0.5 )
                + spriteAnim->spriteAnim[i].fadeMin;

            workingSpriteFade[i] = inAnimFade * fade;
            

            spritePos.x += 
                inAnimFade * 
                getOscOffset( 
                    spriteFrameTime,
                    spriteAnim->spriteAnim[i].xOscPerSec,
                    spriteAnim->spriteAnim[i].xAmp,
                    spriteAnim->spriteAnim[i].xPhase );
            
            spritePos.y += 
                inAnimFade *
                getOscOffset( 
                    spriteFrameTime,
                    spriteAnim->spriteAnim[i].yOscPerSec,
                    spriteAnim->spriteAnim[i].yAmp,
                    spriteAnim->spriteAnim[i].yPhase );
            
            double rock = inAnimFade * 
                getOscOffset( spriteFrameTime,
                              spriteAnim->spriteAnim[i].rockOscPerSec,
                              spriteAnim->spriteAnim[i].rockAmp,
                              spriteAnim->spriteAnim[i].rockPhase );
            

            
            doublePair rotCenterOffset = 
                mult( spriteAnim->spriteAnim[i].rotationCenterOffset,
                      inAnimFade );
            
            double targetWeight = 1 - inAnimFade;
            
            if( inAnimFade < 1 && i < spriteFadeTargetAnim->numSprites ) {
                
                
                double sinValB = getOscOffset( 
                    targetSpriteFrameTime,
                    spriteFadeTargetAnim->spriteAnim[i].fadeOscPerSec,
                    1.0,
                    spriteFadeTargetAnim->spriteAnim[i].fadePhase + .25 );
            
                double hardVersionB;
            
                // hardened sin formula found here:
                // https://thatsmaths.com/2015/12/31/
                //         squaring-the-circular-functions/
                double hardnessB = 
                    spriteFadeTargetAnim->spriteAnim[i].fadeHardness;

                if( hardnessB == 1 ) {
                
                    if( sinValB > 0  ) {
                        hardVersionB = 1;
                        }
                    else {
                        hardVersionB = -1;
                        }
                    }
                else {
                    double absSinValB = fabs( sinValB );
                
                    if( absSinValB != 0 ) {
                        hardVersionB = ( sinValB / absSinValB ) * 
                            pow( absSinValB, 
                             1.0 / ( hardnessB * 10 + 1 ) );
                        }
                    else {
                        hardVersionB = 0;
                        }
                    }

                double fadeB =
                    (spriteFadeTargetAnim->spriteAnim[i].fadeMax - 
                     spriteFadeTargetAnim->spriteAnim[i].fadeMin ) *
                    ( 0.5 * hardVersionB + 0.5 )
                    + spriteFadeTargetAnim->spriteAnim[i].fadeMin;

                workingSpriteFade[i] += targetWeight * fadeB;


                
                spritePos.x += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        spriteFadeTargetAnim->spriteAnim[i].xOscPerSec,
                        spriteFadeTargetAnim->spriteAnim[i].xAmp,
                        spriteFadeTargetAnim->spriteAnim[i].xPhase );
                
                spritePos.y += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        spriteFadeTargetAnim->spriteAnim[i].yOscPerSec,
                        spriteFadeTargetAnim->spriteAnim[i].yAmp,
                        spriteFadeTargetAnim->spriteAnim[i].yPhase );

                 rock += 
                     targetWeight *
                     getOscOffset( 
                         targetSpriteFrameTime,
                         spriteFadeTargetAnim->spriteAnim[i].rockOscPerSec,
                         spriteFadeTargetAnim->spriteAnim[i].rockAmp,
                         spriteFadeTargetAnim->spriteAnim[i].rockPhase );
                 
                 rotCenterOffset = 
                     add( rotCenterOffset,
                          mult( spriteFadeTargetAnim->
                                spriteAnim[i].rotationCenterOffset,
                                
                                targetWeight ) );
                }
            

            double totalRotOffset = 
                spriteAnim->spriteAnim[i].rotPerSec * 
                spriteFrameTime + 
                spriteAnim->spriteAnim[i].rotPhase;
            
            
            // use frozen rot instead if either current or
            // target satisfies 
            if( inAnim->type != moving 
                &&
                spriteAnim->spriteAnim[i].rotPerSec == 0 &&
                spriteAnim->spriteAnim[i].rotPhase == 0 &&
                spriteAnim->spriteAnim[i].rockOscPerSec == 0 &&
                spriteAnim->spriteAnim[i].rockPhase == 0 
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
            else if( inAnimFade < 1  && i < spriteFadeTargetAnim->numSprites
                     &&
                     spriteAnim->type == moving
                     && 
                     spriteFadeTargetAnim->type != moving
                     &&
                     spriteFadeTargetAnim->spriteAnim[i].rotPerSec == 0 &&
                     spriteFadeTargetAnim->spriteAnim[i].rotPhase == 0 &&
                     spriteFadeTargetAnim->spriteAnim[i].rockOscPerSec == 0 &&
                     spriteFadeTargetAnim->spriteAnim[i].rockPhase == 0 
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
            


            if( inAnimFade < 1  && i < spriteFadeTargetAnim->numSprites ) {
                double totalTargetRotOffset =
                    spriteFadeTargetAnim->spriteAnim[i].rotPerSec * 
                    targetSpriteFrameTime + 
                    spriteFadeTargetAnim->spriteAnim[i].rotPhase;
                

                if( spriteFadeTargetAnim->type != moving
                    &&
                    spriteFadeTargetAnim->spriteAnim[i].rotPerSec == 0 &&
                    spriteFadeTargetAnim->spriteAnim[i].rotPhase == 0 &&
                    spriteFadeTargetAnim->spriteAnim[i].rockOscPerSec == 0 &&
                    spriteFadeTargetAnim->spriteAnim[i].rockPhase == 0 
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
        
        
        rot += obj->spriteRot[i];

        
        workingSpritePos[i] = spritePos;
        workingRot[i] = rot;
        
        workingDeltaSpritePos[i] = sub( spritePos, obj->spritePos[i] );
        workingDeltaRot[i] = rot - obj->spriteRot[i];
        }


    doublePair tunicPos = { 0, 0 };
    double tunicRot = 0;

    doublePair bottomPos = { 0, 0 };
    double bottomRot = 0;

    doublePair backpackPos = { 0, 0 };
    double backpackRot = 0;

    doublePair backShoePos = { 0, 0 };
    double backShoeRot = 0;
    
    doublePair frontShoePos = { 0, 0 };
    double frontShoeRot = 0;

    
    // now that their individual animations have been computed
    // walk through and follow parent chains to compute compound animations
    // for each one before drawing
    for( int i=0; i<obj->numSprites; i++ ) {
        
        if( obj->person &&
            ! isSpriteVisibleAtAge( obj, i, inAge ) ) {    
            // skip drawing this aging layer entirely
            continue;
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
            
            if( inFlipH ) {    
                animHeadPos.x *= -1;
                animHeadRotDelta *= -1;
                }
            
            }


        if( inFlipH ) {
            spritePos.x *= -1;
            rot *= -1;
            }
        
        if( inRot != 0 ) {
            rot += inRot;
            spritePos = rotate( spritePos, -2 * M_PI * inRot );
            }
            

        
        doublePair pos = add( spritePos, inPos );


        char skipSprite = false;
        

        if( !inHeldNotInPlaceYet && 
            inHideClosestArm == 1 && 
            frontArmIndices.getElementIndex( i ) != -1 ) {
            skipSprite = true;
            }
        else if( !inHeldNotInPlaceYet && 
            inHideClosestArm == -1 && 
            backArmIndices.getElementIndex( i ) != -1 ) {
            skipSprite = true;
            }
        else if( !inHeldNotInPlaceYet && inHideAllLimbs ) {
            if( legIndices.getElementIndex( i ) != -1 ) {
             
                skipSprite = true;
                }
            }


        if( obj->clothing != 'n' &&
            obj->spriteInvisibleWhenWorn[i] != 0 ) {
            
            if( inWorn &&
                obj->spriteInvisibleWhenWorn[i] == 1 ) {
        
                // skip invisible layer in worn clothing
                skipSprite = true;
                }
            else if( ! inWorn &&
                     obj->spriteInvisibleWhenWorn[i] == 2 ) {
        
                // skip invisible layer in unworn clothing
                skipSprite = true;
                }
            }
        
        
        if( inDrawBehindSlots != 2 ) {    
            if( inDrawBehindSlots == 0 && 
                ! obj->spriteBehindSlots[i] ) {
                skipSprite = true;
                }
            else if( inDrawBehindSlots == 1 && 
                     obj->spriteBehindSlots[i] ) {
                skipSprite = true;
                }
            }


        if( ! skipSprite && 
            i == backFootIndex 
            && inClothing.backShoe != NULL ) {
            
            doublePair offset = inClothing.backShoe->clothingOffset;

            if( inFlipH ) {
                offset.x *= -1;
                backShoeRot = -rot - obj->spriteRot[i];
                }
            else {
                backShoeRot = rot - obj->spriteRot[i];
                }
            
            if( backShoeRot != 0 ) {
                if( inFlipH ) {      
                    offset = rotate( offset, 2 * M_PI * backShoeRot );
                    backShoeRot *= -1;
                    }
                else { 
                    offset = rotate( offset, -2 * M_PI * backShoeRot );
                    }
                }
            

            doublePair cPos = add( spritePos, offset );

            cPos = add( cPos, inPos );
            
            backShoePos = cPos;
            }

        if( i == bodyIndex ) {
            
            if( inClothing.tunic != NULL ) {

                doublePair offset = inClothing.tunic->clothingOffset;
            
                if( inFlipH ) {
                    offset.x *= -1;
                    tunicRot = -rot - obj->spriteRot[i];
                    }
                else {
                    tunicRot = rot - obj->spriteRot[i];
                    }
                
                if( tunicRot != 0 ) {
                    if( inFlipH ) {
                        offset = rotate( offset, 2 * M_PI * tunicRot );
                        tunicRot *= -1;
                        }
                    else {
                        offset = rotate( offset, -2 * M_PI * tunicRot );
                        }
                    }
                
            
                doublePair cPos = add( spritePos, offset );
                
                cPos = add( cPos, inPos );
                
                tunicPos = cPos;
                }
            if( inClothing.bottom != NULL ) {

                doublePair offset = inClothing.bottom->clothingOffset;
            
                if( inFlipH ) {
                    offset.x *= -1;
                    bottomRot = -rot - obj->spriteRot[i];
                    }
                else {
                    bottomRot = rot - obj->spriteRot[i];
                    }
                
                if( bottomRot != 0 ) {
                    if( inFlipH ) {
                        offset = rotate( offset, 2 * M_PI * bottomRot );
                        bottomRot *= -1;
                        }
                    else {
                        offset = rotate( offset, -2 * M_PI * bottomRot );
                        }
                    }
                
                
            
                doublePair cPos = add( spritePos, offset );
                
                cPos = add( cPos, inPos );
                
                bottomPos = cPos;
                }
            if( inClothing.backpack != NULL ) {

                doublePair offset = inClothing.backpack->clothingOffset;
            
                if( inFlipH ) {
                    offset.x *= -1;
                    backpackRot = -rot - obj->spriteRot[i];
                    }
                else {
                    backpackRot = rot - obj->spriteRot[i];
                    }
                
                if( backpackRot != 0 ) {
                    if( inFlipH ) {
                        offset = rotate( offset, 2 * M_PI * backpackRot );
                        backpackRot *= -1;
                        }
                    else {
                        offset = rotate( offset, -2 * M_PI * backpackRot );
                        }
                    }
                
            
                doublePair cPos = add( spritePos, offset );
                
                cPos = add( cPos, inPos );
                
                backpackPos = cPos;
                }
            
            }
        else if( i == topBackArmIndex ) {
            // draw under top of back arm
            
            if( inClothing.bottom != NULL ) {
                int numCont = 0;
                int *cont = NULL;
                if( inClothingContained != NULL ) {    
                    numCont = inClothingContained[4].size();
                    cont = inClothingContained[4].getElementArray();
                    }
                

                char used;
                drawObjectAnim( inClothing.bottom->id, 
                                inAnim->type, 
                                inFrameTime,
                                inAnimFade, 
                                inFadeTargetAnim->type,
                                inFadeTargetFrameTime,
                                inFrozenRotFrameTime,
                                &used,
                                endAnimType,
                                endAnimType,
                                bottomPos,
                                bottomRot,
                                true,
                                inFlipH,
                                -1,
                                0,
                                false,
                                false,
                                emptyClothing,
                                NULL,
                                numCont, cont );
                
                if( cont != NULL ) {
                    delete [] cont;
                    }
                }
            if( inClothing.tunic != NULL ) {
                int numCont = 0;
                int *cont = NULL;
                if( inClothingContained != NULL ) {    
                    numCont = inClothingContained[1].size();
                    cont = inClothingContained[1].getElementArray();
                    }
                
                char used;
                drawObjectAnim( inClothing.tunic->id, 
                                inAnim->type, 
                                inFrameTime,
                                inAnimFade, 
                                inFadeTargetAnim->type,
                                inFadeTargetFrameTime,
                                inFrozenRotFrameTime,
                                &used,
                                endAnimType,
                                endAnimType,
                                tunicPos,
                                tunicRot,
                                true,
                                inFlipH,
                                -1,
                                0,
                                false,
                                false,
                                emptyClothing,
                                NULL,
                                numCont, cont );

                if( cont != NULL ) {
                    delete [] cont;
                    }
                }
            if( inClothing.backpack != NULL ) {
                int numCont = 0;
                int *cont = NULL;
                if( inClothingContained != NULL ) {    
                    numCont = inClothingContained[5].size();
                    cont = inClothingContained[5].getElementArray();
                    }

                char used;
                drawObjectAnim( inClothing.backpack->id, 
                                inAnim->type, 
                                inFrameTime,
                                inAnimFade, 
                                inFadeTargetAnim->type,
                                inFadeTargetFrameTime,
                                inFrozenRotFrameTime,
                                &used,
                                endAnimType,
                                endAnimType,
                                backpackPos,
                                backpackRot,
                                true,
                                inFlipH,
                                -1,
                                0,
                                false,
                                false,
                                emptyClothing,
                                NULL,
                                numCont, cont );

                if( cont != NULL ) {
                    delete [] cont;
                    }
                }
            }
        

        if( ! skipSprite && 
            i == frontFootIndex 
            && inClothing.frontShoe != NULL ) {
        
            doublePair offset = inClothing.frontShoe->clothingOffset;
                
            if( inFlipH ) {
                offset.x *= -1;
                frontShoeRot = -rot - obj->spriteRot[i];
                }
            else {
                frontShoeRot = rot - obj->spriteRot[i];
                }
            
            if( frontShoeRot != 0 ) {
                if( inFlipH ) {
                    offset = rotate( offset, 2 * M_PI * frontShoeRot );
                    frontShoeRot *= -1;
                    }
                else {
                    offset = rotate( offset, -2 * M_PI * frontShoeRot );
                    }
                }

            doublePair cPos = add( spritePos, offset );

            cPos = add( cPos, inPos );
            
            frontShoePos = cPos;
            }
        


        if( !skipSprite ) {
            setDrawColor( obj->spriteColor[i] );
            
            if( animLayerFades != NULL ) {
                setDrawFade( animLayerFades[i] );
                }

            char multiplicative = 
                getUsesMultiplicativeBlending( obj->sprites[i] );
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( true );
                
                if( workingSpriteFade[i] < 1 ||
                    getTotalGlobalFade() < 1 ) {
                    
                    toggleAdditiveTextureColoring( true );
                    
                    float invFade = 1.0f - workingSpriteFade[i];
                    setDrawColor( invFade, invFade, invFade, 1.0f );
                    }
                }
            else if( workingSpriteFade[i] < 1 ) {
                setDrawFade( workingSpriteFade[i] );
                }
            
            drawSprite( getSprite( obj->sprites[i] ), pos, 1.0, rot, 
                        logicalXOR( inFlipH, obj->spriteHFlip[i] ) );

            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                toggleAdditiveTextureColoring( false );
                }


            // this is front-most drawn hand
            // in unanimated, unflipped object
            if( i == backHandIndex && inHideClosestArm == 0 
                && !inHideAllLimbs ) {
                returnHoldingPos.valid = true;
                // return screen pos for hand, which may be flipped, etc.
                returnHoldingPos.pos = pos;
                
                if( inFlipH ) {
                    returnHoldingPos.rot = -rot - obj->spriteRot[i];
                    }
                else {
                    returnHoldingPos.rot = rot - obj->spriteRot[i];
                    }
                }
            else if( i == bodyIndex && inHideClosestArm != 0 ) {
                returnHoldingPos.valid = true;
                // return screen pos for body, which may be flipped, etc.
                returnHoldingPos.pos = pos;
                
                if( inFlipH ) {
                    returnHoldingPos.rot = -rot - obj->spriteRot[i];
                    }
                else {
                    returnHoldingPos.rot = rot - obj->spriteRot[i];
                    }
                }
            
            }

        // shoes on top of feet
        if( inClothing.backShoe != NULL && i == backFootIndex ) {
            int numCont = 0;
            int *cont = NULL;
            if( inClothingContained != NULL ) {    
                numCont = inClothingContained[3].size();
                cont = inClothingContained[3].getElementArray();
                }

            char used;
            drawObjectAnim( inClothing.backShoe->id, 
                            inAnim->type, 
                            inFrameTime,
                            inAnimFade, 
                            inFadeTargetAnim->type,
                            inFadeTargetFrameTime,
                            inFrozenRotFrameTime,
                            &used,
                            endAnimType,
                            endAnimType,
                            backShoePos,
                            backShoeRot,
                            true,
                            inFlipH,
                            -1,
                            0,
                            false,
                            false,
                            emptyClothing,
                            NULL,
                            numCont, cont );

            if( cont != NULL ) {
                delete [] cont;
                }
            }
        else if( inClothing.frontShoe != NULL && i == frontFootIndex ) {
            int numCont = 0;
            int *cont = NULL;
            if( inClothingContained != NULL ) {    
                numCont = inClothingContained[2].size();
                cont = inClothingContained[2].getElementArray();
                }
            
            char used;
            drawObjectAnim( inClothing.frontShoe->id, 
                            inAnim->type, 
                            inFrameTime,
                            inAnimFade, 
                            inFadeTargetAnim->type,
                            inFadeTargetFrameTime,
                            inFrozenRotFrameTime,
                            &used,
                            endAnimType,
                            endAnimType,
                            frontShoePos,
                            frontShoeRot,
                            true,
                            inFlipH,
                            -1,
                            0,
                            false,
                            false,
                            emptyClothing,
                            NULL,
                            numCont, cont );

            if( cont != NULL ) {
                delete [] cont;
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
        
        int numCont = 0;
        int *cont = NULL;
        if( inClothingContained != NULL ) {    
            numCont = inClothingContained[0].size();
            cont = inClothingContained[0].getElementArray();
            }
        
        char used;
        drawObjectAnim( inClothing.hat->id, 
                        inAnim->type, 
                        inFrameTime,
                        inAnimFade, 
                        inFadeTargetAnim->type,
                        inFadeTargetFrameTime,
                        inFrozenRotFrameTime,
                        &used,
                        endAnimType,
                        endAnimType,
                        cPos,
                        animHeadRotDelta,
                        true,
                        inFlipH,
                        -1,
                        0,
                        false,
                        false,
                        emptyClothing,
                        NULL,
                        numCont, cont );
        
        if( cont != NULL ) {
            delete [] cont;
            }
        }
    
    if( animLayerFades != NULL ) {
        delete [] animLayerFades;
        animLayerFades = NULL;
        }
    
    return returnHoldingPos;
    }



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inAnimFade, 
                     AnimType inFadeTargetType,
                     double inFadeTargetFrameTime,
                     double inFrozenRotFrameTime,
                     char *outFrozenRotFrameTimeUsed,
                     AnimType inFrozenArmType,
                     AnimType inFrozenArmFadeTargetType,
                     doublePair inPos,
                     double inRot,
                     char inWorn,
                     char inFlipH,
                     double inAge,
                     int inHideClosestArm,
                     char inHideAllLimbs,
                     char inHeldNotInPlaceYet,
                     ClothingSet inClothing,
                     SimpleVector<int> *inClothingContained,
                     int inNumContained, int *inContainedIDs ) {
    
    if( inType == ground2 ) {
        inType = ground;
        }
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, inRot, inWorn, 
                    inFlipH, inAge, inHideClosestArm, inHideAllLimbs, 
                    inHeldNotInPlaceYet, inClothing,
                    inNumContained, inContainedIDs );
        }
    else {
        if( inFadeTargetType == ground2 ) {
            inFadeTargetType = ground;
            }
    
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        AnimationRecord *rF = getAnimation( inObjectID, moving );
        
        AnimationRecord *rArm = getAnimation( inObjectID, inFrozenArmType );
        AnimationRecord *rArmFade = getAnimation( inObjectID, 
                                                  inFrozenArmFadeTargetType );
        
        if( rB == NULL ) {
            rB = getAnimation( inObjectID, ground );
            }
        if( rF == NULL ) {
            rF = getAnimation( inObjectID, ground );
            }


        drawObjectAnim( inObjectID, r, inFrameTime,
                        inAnimFade, rB, inFadeTargetFrameTime,
                        inFrozenRotFrameTime,
                        outFrozenRotFrameTimeUsed,
                        rF,
                        rArm,
                        rArmFade,
                        inPos, inRot, inWorn, inFlipH, inAge,
                        inHideClosestArm, inHideAllLimbs, inHeldNotInPlaceYet,
                        inClothing,
                        inClothingContained,
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
                     AnimationRecord *inFrozenArmAnim,
                     AnimationRecord *inFrozenArmFadeTargetAnim,
                     doublePair inPos,
                     double inRot,
                     char inWorn,
                     char inFlipH,
                     double inAge,
                     int inHideClosestArm,
                     char inHideAllLimbs,
                     char inHeldNotInPlaceYet,
                     ClothingSet inClothing,
                     SimpleVector<int> *inClothingContained,
                     int inNumContained, int *inContainedIDs ) {
    
    ClothingSet emptyClothing = getEmptyClothingSet();


    // draw portion of animating object behind slots
    drawObjectAnim( inObjectID, 0, inAnim, inFrameTime,
                    inAnimFade, inFadeTargetAnim, inFadeTargetFrameTime,
                    inFrozenRotFrameTime,
                    outFrozenRotFrameTimeUsed,
                    inFrozenRotAnim,
                    inFrozenArmAnim,
                    inFrozenArmFadeTargetAnim,
                    inPos, inRot, inWorn, inFlipH,
                    inAge, inHideClosestArm, inHideAllLimbs, 
                    inHeldNotInPlaceYet,
                    inClothing,
                    inClothingContained );

    
    // next, draw jiggling (never rotating) objects in slots
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
            
            doublePair centerOffset = getObjectCenterOffset( contained );
            
            double rot = inRot;
            
            if( obj->slotVert[i] ) {
                double rotOffset = 
                    0.25 + contained->vertContainRotationOffset;
                
                if( inFlipH ) {
                    centerOffset = rotate( centerOffset, 
                                           - rotOffset * 2 * M_PI );
                    rot -= rotOffset;
                    }
                else {
                    centerOffset = rotate( centerOffset, 
                                           - rotOffset * 2 * M_PI );
                    rot += rotOffset;
                    }
                }


            pos = sub( pos, centerOffset );
            
        
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
  
            if( inRot != 0 ) {
                pos = rotate( pos, -2 * M_PI * inRot );
                }
            

            pos = add( pos, inPos );

            
            drawObject( contained, 2, pos, rot, false, inFlipH,
                        inAge, 0, false, false, emptyClothing );
            }
        
        } 

    // draw portion of animating object on top of contained slots
    drawObjectAnim( inObjectID, 1, inAnim, inFrameTime,
                    inAnimFade, inFadeTargetAnim, inFadeTargetFrameTime,
                    inFrozenRotFrameTime,
                    outFrozenRotFrameTimeUsed,
                    inFrozenRotAnim,
                    inFrozenArmAnim,
                    inFrozenArmFadeTargetAnim,
                    inPos, inRot, inWorn, inFlipH,
                    inAge, inHideClosestArm, inHideAllLimbs, 
                    inHeldNotInPlaceYet,
                    inClothing,
                    inClothingContained );
    }



AnimationRecord *copyRecord( AnimationRecord *inRecord ) {
    AnimationRecord *newRecord = new AnimationRecord;
    
    newRecord->objectID = inRecord->objectID;
    newRecord->type = inRecord->type;
    newRecord->randomStartPhase = inRecord->randomStartPhase;
    
    newRecord->numSounds = inRecord->numSounds;
    newRecord->soundAnim = new SoundAnimationRecord[ newRecord->numSounds ];
    
    memcpy( newRecord->soundAnim, inRecord->soundAnim,
            sizeof( SoundAnimationRecord ) * newRecord->numSounds );

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
    
    inRecord->fadeOscPerSec = 0;
    inRecord->fadeHardness = 0;
    inRecord->fadeMin = 0;
    inRecord->fadeMax = 1;
    inRecord->fadePhase = 0;
    }



void zeroRecord( SoundAnimationRecord *inRecord ) {
    inRecord->sound = blankSoundUsage;
    inRecord->repeatPerSec = 0;
    inRecord->repeatPhase = 0;
    inRecord->ageStart = -1;
    inRecord->ageEnd = -1;
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



char isSoundUsedByAnim( int inSoundID ) {
    for( int i=0; i<mapSize; i++ ) {
        for( int j=0; j<endAnimType; j++ ) {
            AnimationRecord *r = idMap[i][j];
            
            if( r != NULL ) {
                
                if( r->numSounds > 0 ) {
                    
                    for( int k=0; k < r->numSounds; k++ ) {
                        if( r->soundAnim[k].sound.id == inSoundID ) {
                            return true;
                            }
                        }
                    }
                }
            }
        }

    return false;
    }
