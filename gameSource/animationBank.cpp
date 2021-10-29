#include "animationBank.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "soundBank.h"

#include "stdlib.h"
#include "math.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"

#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/drawUtils.h"


#include "ageControl.h"


#include "folderCache.h"

#include "spriteDrawColorOverride.h"



static int mapSize;
// maps IDs and AnimTyps to anim records
// sparse, so some entries are NULL
static AnimationRecord * **idMap;


// maps IDs to arbitrary number of extra animation records 
// non-sparse
// entries here that correspond to NULL entries above are just empty vectors
static SimpleVector<AnimationRecord*> *idExtraMap;


static ClothingSet emptyClothing = getEmptyClothingSet();



static FolderCache cache;

static int currentFile;


static SimpleVector<AnimationRecord*> records;
static int maxID;



static const char *animTypeNames[9] = { "ground", "held", "moving", "ground2",
                                        "eating", "doing", "endAnimType",
                                        "extra", "extraB" };


const char *typeToName( AnimType inAnimType ) {
    return animTypeNames[ inAnimType ];
    }


static char drawMouthShapes = false;
static int mouthAnchorID = 776;
static int numMouthShapes = 0;
static SpriteHandle *mouthShapes = NULL;
static int numMouthShapeFrames = 0;
static SpriteHandle *mouthShapeFrameList = NULL;
static int mouthShapeFrame = 0;

static char outputMouthFrames = false;
static char mouthFrameOutputStarted = false;



static char shouldFileBeCached( char *inFileName ) {
    if( strstr( inFileName, ".txt" ) != NULL ) {
        return true;
        }
    return false;
    }




int initAnimationBankStart( char *outRebuildingCache ) {

    if( drawMouthShapes ) {
        
        // load mouth shape sprites from a folder
        
        File mouthShapeFolder( NULL, "mouthShapes" );
        
        if( mouthShapeFolder.exists() && mouthShapeFolder.isDirectory() ) {
         
            File **mouthShapeFiles = 
                mouthShapeFolder.getChildFilesSorted( &numMouthShapes );
            
            mouthShapes = new SpriteHandle[ numMouthShapes ];
            

            for( int i=0; i<numMouthShapes; i++ ) {
                char *fileName = mouthShapeFiles[i]->getFullFileName();
                
                mouthShapes[i] = loadSpriteBase( fileName, false );
                
                delete [] fileName;
                delete mouthShapeFiles[i];
                }
            
            delete [] mouthShapeFiles;
            
            printf( "Loaded %d mouth shapes\n", numMouthShapes );
            
            if( numMouthShapes > 0 ) {
                
                // load mouth shape list, parse it, 
                // and generate mouthShapeFrameList
                
                File mouthFrameFile( NULL, "mouthFrameList.txt" );

                if( mouthFrameFile.exists() && 
                    ! mouthFrameFile.isDirectory() ) {
                    
                    char *cont = mouthFrameFile.readFileContents();
                    
                    if( cont != NULL ) {
                        SimpleVector<char *> *tokens =
                            tokenizeString( cont );
                        
                        delete [] cont;
                        
                        numMouthShapeFrames = tokens->size();
                        
                        SimpleVector<SpriteHandle> frameList;
                        
                        for( int i=0; i<numMouthShapeFrames; i++ ) {
                            int index = -1;
                            sscanf( tokens->getElementDirect( i ),
                                    "%d", &index );

                            if( index >= 0 &&
                                index < numMouthShapes ) {
                                frameList.push_back( mouthShapes[ index ] );
                                }
                            }

                        numMouthShapeFrames = frameList.size();
                        mouthShapeFrameList = frameList.getElementArray();

                        printf( "Loaded %d mouth frames\n", 
                                numMouthShapeFrames );
                        
                        tokens->deallocateStringElements();
                        delete tokens;
                        }
                    }
                }
            
            }
        
        }
    


    maxID = 0;

    currentFile = 0;

    
    cache = initFolderCache( "animations", outRebuildingCache,
                             shouldFileBeCached );

    return cache.numFiles;
    }



float initAnimationBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;


    char *txtFileName = getFileName( cache, i );
                        
    if( shouldFileBeCached( txtFileName ) ) {
        
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
                int extraIndexRead = -1;
                int randomStartPhaseRead = 0;
                
                if( strchr( lines[next], ':' ) != NULL ) {

                    sscanf( lines[next], "type=%d:%d,randStartPhase=%d", 
                            &( typeRead ), &extraIndexRead, 
                            &randomStartPhaseRead );
                    }
                else {
                    sscanf( lines[next], "type=%d,randStartPhase=%d", 
                            &( typeRead ), &randomStartPhaseRead );
                    }
                
                r->type = (AnimType)typeRead;
                r->extraIndex = extraIndexRead;
                r->randomStartPhase = randomStartPhaseRead;
                next++;

                
                // optional force zero start
                r->forceZeroStart = false;
                if( strstr( lines[next], "forceZeroStart=" ) != NULL ) {
                    int forceZeroStartRead = 0;
                    sscanf( lines[next], "forceZeroStart=%d", 
                            &forceZeroStartRead );
                    r->forceZeroStart = forceZeroStartRead;
                    next++;
                    }
                
                
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
                        
                        

                        
                        char *start = &( lines[next][11] );

                        char *end = strstr( start, " " );
                        
                        if( end != NULL ) {
                            // terminate at end of first token
                            // this should be a SoundUsage string
                            end[0] = '\0';
                            
                            r->soundAnim[j].sound = scanSoundUsage( start );
                            

                            // restore and scan remaining parameters
                            end[0] = ' ';
                            
                            
                            int footstepValue = 0;
                            sscanf( &( end[1] ), 
                                    "%lf %lf %lf %lf %d",
                                    &( r->soundAnim[j].repeatPerSec ),
                                    &( r->soundAnim[j].repeatPhase ),
                                    &( r->soundAnim[j].ageStart ),
                                    &( r->soundAnim[j].ageEnd ),
                                    &footstepValue );
                            
                            r->soundAnim[j].footstep = footstepValue;
                            }
                        
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

                    r->spriteAnim[j].offset.x = 0;
                    r->spriteAnim[j].offset.y = 0;

                    r->spriteAnim[j].startPauseSec = 0;


                    if( strstr( lines[next], "offset" ) != NULL ) {
                        sscanf( lines[next], 
                                "offset=(%lf,%lf)",
                                &( r->spriteAnim[j].offset.x ),
                                &( r->spriteAnim[j].offset.y ) );
                        next++;
                        if( next >= numLines ) {
                            break;
                            }
                        }
                    
                    if( strstr( lines[next], "startPause" ) != NULL ) {
                        sscanf( lines[next], 
                                "startPause=%lf",
                                &( r->spriteAnim[j].startPauseSec ) );
                        next++;
                        if( next >= numLines ) {
                            break;
                            }
                        }
                    

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
                    
                    r->slotAnim[j].offset.x = 0;
                    r->slotAnim[j].offset.y = 0;
                    
                    r->slotAnim[j].startPauseSec = 0;
                    

                    if( strstr( lines[next], "offset" ) != NULL ) {
                        sscanf( lines[next], 
                                "offset=(%lf,%lf)",
                                &( r->slotAnim[j].offset.x ),
                                &( r->slotAnim[j].offset.y ) );
                        next++;
                    
                        if( next >= numLines ) {
                            break;
                            }
                        }
                    
                    if( strstr( lines[next], "startPause" ) != NULL ) {
                        sscanf( lines[next], 
                                "startPause=%lf",
                                &( r->slotAnim[j].startPauseSec ) );
                        next++;
                        if( next >= numLines ) {
                            break;
                            }
                        }

                    
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
    
    idExtraMap = new SimpleVector<AnimationRecord*>[ mapSize ];

    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = new AnimationRecord*[ endAnimType ];
        for( int j=0; j<endAnimType; j++ ) {    
            idMap[i][j] = NULL;
            }
        
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        AnimationRecord *r = records.getElementDirect(i);
        
        if( r->type < endAnimType ) {
            idMap[ r->objectID ][ r->type ] = r;
            }
        else {
            idExtraMap[ r->objectID ].push_back( r );
            }
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



static int animLayerCutoff = -1;

void setAnimLayerCutoff( int inCutoff ) {
    animLayerCutoff = inCutoff;
    }




void freeAnimationBank() {

    if( mouthShapes != NULL ) {
        for( int i=0; i<numMouthShapes; i++ ) {
            freeSprite( mouthShapes[i] );
            }
        delete [] mouthShapes;
        mouthShapes = NULL;
        }
    
    if( mouthShapeFrameList != NULL ) {
        delete [] mouthShapeFrameList;
        mouthShapeFrameList = NULL;
        }


    if( animLayerFades != NULL ) {
        delete [] animLayerFades;
        animLayerFades = NULL;
        }
    
    for( int i=0; i<mapSize; i++ ) {
        for( int j=0; j<endAnimType; j++ ) {
            if( idMap[i][j] != NULL ) {
                AnimationRecord *r = idMap[i][j];
                
                for( int s=0; s< r->numSounds; s++ ) {
                    clearSoundUsage( & ( r->soundAnim[s].sound ) );
                    }
                
                delete [] r->soundAnim;

                delete [] r->spriteAnim;
                delete [] r->slotAnim;

                delete r;
                }
            }
        delete [] idMap[i];

        for( int j=0; j<idExtraMap[i].size(); j++ ) {
            AnimationRecord *r = idExtraMap[i].getElementDirect( j );
            
            for( int s=0; s< r->numSounds; s++ ) {
                clearSoundUsage( & ( r->soundAnim[s].sound ) );
                }

            delete [] r->soundAnim;

            delete [] r->spriteAnim;
            delete [] r->slotAnim;
            
            delete r;
            }
        }

    delete [] idMap;
    delete [] idExtraMap;
    }



static int extraIndex = 0;
static int extraIndexB = 0;


void setExtraIndex( int inIndex ) {
    extraIndex = inIndex;
    }


void setExtraIndexB( int inIndex ) {
    extraIndexB = inIndex;
    }




static File *getFile( int inID, AnimType inType ) {
    File animationsDir( NULL, "animations" );
            
    if( !animationsDir.exists() ) {
        animationsDir.makeDirectory();
        }
    
    if( animationsDir.exists() && animationsDir.isDirectory() ) {
        
        char *fileName;
        
        if( inType > endAnimType ) {
            fileName = autoSprintf( "%d_%dx%d.txt", 
                                    inID, extra, extraIndex );
            }
        else {
            fileName = autoSprintf( "%d_%d.txt", inID, inType );
            }
        

        File *animationFile = animationsDir.getChildFile( fileName );

        delete [] fileName;
        
        return animationFile;
        }
    
    return NULL;
    }



int getNumExtraAnim( int inID ) {
    if( inID < mapSize ) {
        return idExtraMap[inID].size();
        }

    return 0;
    }




AnimationRecord *getAnimation( int inID, AnimType inType ) {
    if( inID < mapSize ) {
        if( inType < endAnimType && inType != ground2 ) {
            return idMap[inID][inType];
            }
        else if( inType > endAnimType ) {
            
            int numExtra = idExtraMap[inID].size();
            
            int extraI = extraIndex;
            
            if( inType == extraB ) {
                extraI = extraIndexB;
                }

            for( int i=0; i<numExtra; i++ ) {
                AnimationRecord *r = 
                    idExtraMap[inID].getElementDirect( i );
                
                if( r->extraIndex == extraI ) {
                    return r;
                    }
                }
            }
        }
    
    return NULL;
    }




// record destroyed by caller
void addAnimation( AnimationRecord *inRecord, char inNoWriteToFile ) {
    
    if( inRecord->type == ground2 ) {
        // never add ground2
        return;
        }
    

    AnimationRecord *oldRecord = getAnimation( inRecord->objectID, 
                                               inRecord->type );
    
    SimpleVector<int> oldSoundIDs;
    if( oldRecord != NULL ) {
        for( int i=0; i<oldRecord->numSounds; i++ ) {
            for( int j=0; j< oldRecord->soundAnim[i].sound.numSubSounds; j++ ) {
                oldSoundIDs.push_back( oldRecord->soundAnim[i].sound.ids[j] );
                }
            }
        }
    

    clearAnimation( inRecord->objectID,
                    inRecord->type );
    
    int newID = inRecord->objectID;
    
    if( newID >= mapSize ) {
        // expand map

        // when expanding by one new record, expect that more are coming soon
        // new objects/animations are added in batches during auto-generation
        // phase of startup.
        
        // would be standard-practice to double newMapSize here, but
        // will waste a lot of space (each spot in map is 6 32-bit pointers,
        // or 24 bytes).

        // Found map expansion to be a hot spot with profiler
        // can speed it up by a factor of X by expanding it by X
        // For now, just speed it up by a factor of 100
        
        // every time it needs to grow, grow it by 100.
        // Worst case, this will waste 99 * 24 = 2376 bytes.
        int newMapSize = newID + 100;        

        
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

        SimpleVector<AnimationRecord*> *newExtraMap = 
            new SimpleVector<AnimationRecord*>[ newMapSize ];
        
        // can't memcpy, because we need copy constructors to be called
        // because destructors are called when we delete idMap
        for( int i=0; i<mapSize; i++ ) {
            newExtraMap[i] = idExtraMap[i];
            }
        
        delete [] idExtraMap;
        
        idExtraMap = newExtraMap;
        

        mapSize = newMapSize;
        }

    
    // copy to add it to memory bank, but do NOT count live sound uses
    
    if( inRecord->type < endAnimType ) {
        idMap[newID][inRecord->type] = copyRecord( inRecord, false );
        }
    else {
        // extra

        AnimationRecord *r = copyRecord( inRecord, false );
        
        // make sure index is set correctly
        r->extraIndex = extraIndex;
        
        idExtraMap[newID].push_back( r );
        }
    
    
    // unCountLiveUse no longer needed here, because we're telling copyRecord
    // to not cound these as live uses above
    
    
    
    if( ! inNoWriteToFile ) {
        
        // store on disk
        File *animationFile = getFile( newID, inRecord->type );
    
        if( animationFile != NULL ) {
            SimpleVector<char*> lines;
        
            lines.push_back( autoSprintf( "id=%d", newID ) );
            
            if( inRecord->type < endAnimType ) {    
                lines.push_back( autoSprintf( "type=%d,randStartPhase=%d", 
                                              inRecord->type, 
                                              inRecord->randomStartPhase ) );
                }
            else {
                lines.push_back( autoSprintf( "type=%d:%d,randStartPhase=%d", 
                                              extra,
                                              extraIndex,
                                              inRecord->randomStartPhase ) );
                }

            lines.push_back( autoSprintf( "forceZeroStart=%d", 
                                          inRecord->forceZeroStart ) );
            
            lines.push_back( 
                autoSprintf( "numSounds=%d", inRecord->numSounds ) );

            for( int j=0; j<inRecord->numSounds; j++ ) {
                lines.push_back( autoSprintf( 
                                     "soundParam=%s %lf %lf %lf %lf %d",
                                     printSoundUsage( 
                                         inRecord->soundAnim[j].sound ),
                                     inRecord->soundAnim[j].repeatPerSec,
                                     inRecord->soundAnim[j].repeatPhase,
                                     inRecord->soundAnim[j].ageStart,
                                     inRecord->soundAnim[j].ageEnd,
                                     inRecord->soundAnim[j].footstep ) );
                }
        
            lines.push_back( 
                autoSprintf( "numSprites=%d", inRecord->numSprites ) );
        
            lines.push_back( 
                autoSprintf( "numSlots=%d", inRecord->numSlots ) );
        
            for( int j=0; j<inRecord->numSprites; j++ ) {
                lines.push_back( 
                    autoSprintf( 
                        "offset=(%lf,%lf)",
                        inRecord->spriteAnim[j].offset.x,
                        inRecord->spriteAnim[j].offset.y ) );
                
                lines.push_back( 
                    autoSprintf( 
                        "startPause=%lf",
                        inRecord->spriteAnim[j].startPauseSec ) );
                
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
                        "offset=(%lf,%lf)",
                        inRecord->slotAnim[j].offset.x,
                        inRecord->slotAnim[j].offset.y ) );

                lines.push_back( 
                    autoSprintf( 
                        "startPause=%lf",
                        inRecord->slotAnim[j].startPauseSec ) );
                
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
    
    
    // check if sounds still used (prevent orphan sounds)
    for( int i=0; i<oldSoundIDs.size(); i++ ) {
        checkIfSoundStillNeeded( oldSoundIDs.getElementDirect( i ) );
        }
    }




void clearAnimation( int inObjectID, AnimType inType ) {
    AnimationRecord *r = getAnimation( inObjectID, inType );
    
    if( r != NULL ) {
        

        if( inType < endAnimType ) {    
            idMap[inObjectID][inType] = NULL;
            }
        else {
            // extra
            for( int i=0; i<idExtraMap[inObjectID].size(); i++ ) {
                if( idExtraMap[inObjectID].getElementDirect( i )->extraIndex ==
                    extraIndex ) {
                    idExtraMap[inObjectID].deleteElement( i );
                    break;
                    }
                }
            }
        
        for( int i=0; i<r->numSounds; i++ ) {
            clearSoundUsage( &( r->soundAnim[i].sound ) );
            }
        
        delete [] r->soundAnim;
        delete [] r->spriteAnim;
        delete [] r->slotAnim;
        
        delete r;
        
        File animationsDir( NULL, "animations" );
            

        File *cacheFile = animationsDir.getChildFile( "cache.fcz" );
        
        cacheFile->remove();
        
        delete cacheFile;
        

        File *animationFile = getFile( inObjectID, inType );
        animationFile->remove();
        
        delete animationFile;
        }
    }



static double getOscOffset( double inFrameTime,
                            double inOffset,
                            double inOscPerSec,
                            double inAmp,
                            double inPhase ) {
    return inOffset + 
        inAmp * sin( ( inFrameTime * inOscPerSec + inPhase ) * 2 * M_PI );
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
        
        // offset at all, must fade
        if( inCurR->spriteAnim[i].offset.x != 0 ) return true;
        if( inCurR->spriteAnim[i].offset.y != 0 ) return true;
        

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
        // offset at all, must fade
        if( inCurR->slotAnim[i].offset.x != 0 ) return true;
        if( inCurR->slotAnim[i].offset.y != 0 ) return true;

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
        
        if( r->spriteAnim[i].offset.x != 0 ) return false;
        if( r->spriteAnim[i].offset.y != 0 ) return false;
        
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
                
        if( r->slotAnim[i].offset.x != 0 ) return false;
        if( r->slotAnim[i].offset.y != 0 ) return false;
        
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


static SimpleVector<Emotion *> drawWithEmots;

void setAnimationEmotion( Emotion *inEmotion ) {
    drawWithEmots.deleteAll();
    if( inEmotion != NULL ) {
        drawWithEmots.push_back( inEmotion );
        }
    }


void addExtraAnimationEmotions( SimpleVector<Emotion*> *inList ) {
    drawWithEmots.push_back_other( inList );
    }


static int drawWithBadge = -1;
static char bareBadge = false;
static FloatColor drawWithBadgeColor = { 1, 1, 1, 1 };


void setAnimationBadge( int inBadgeID, char inBareBadge ) {
    drawWithBadge = inBadgeID;
    bareBadge = inBareBadge;
    }


void setAnimationBadgeColor( FloatColor inBadgeColor ) {
    drawWithBadgeColor = inBadgeColor;
    }




static float clothingHighlightFades[ NUM_CLOTHING_PIECES ];

void setClothingHighlightFades( float *inFades ) {
    if( inFades == NULL ) {
        for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
            clothingHighlightFades[i] = 0;
            }
        }
    else {
        for( int i=0; i<NUM_CLOTHING_PIECES; i++ ) {
            clothingHighlightFades[i] = inFades[i];
            }
        }
    }



static char shouldHidePersonShadows = false;

void hidePersonShadows( char inHide ) {
    shouldHidePersonShadows = inHide;
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
    int inNumContained, int *inContainedIDs,
    SimpleVector<int> *inSubContained ) {
    
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
        inContainedIDs,
        inSubContained,
        drawWithEmots,
        0 };
    
    return outPack;
    }

        


void drawObjectAnim( ObjectAnimPack inPack ) {
    HoldingPos p;
    p.valid = false;

    // set based on what's in pack, but restore main value afterward
    SimpleVector<Emotion*> oldEmots = drawWithEmots;
    
    drawWithEmots = inPack.setEmots;
    
    if( inPack.inContainedIDs == NULL ) {
        p = drawObjectAnim( 
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
            inPack.inContainedIDs,
            inPack.inSubContained );
        }
    
    drawWithEmots = oldEmots;
    

    if( inPack.additionalHeldID > 0 && p.valid ) {
        doublePair holdPos;
        
        double holdRot = 0;
        
        ObjectRecord *heldObject = getObject( inPack.additionalHeldID );
        
        computeHeldDrawPos( p, inPack.inPos,
                            heldObject,
                            inPack.inFlipH,
                            &holdPos, &holdRot );

        drawObjectAnim( 
            inPack.additionalHeldID,
            2,
            inPack.inType,
            inPack.inFrameTime, 
            inPack.inAnimFade,
            inPack.inFadeTargetType,
            inPack.inFadeTargetFrameTime,
            inPack.inFrozenRotFrameTime,
            inPack.outFrozenRotFrameTimeUsed,
            endAnimType,
            endAnimType,
            holdPos,
            holdRot,
            false,
            inPack.inFlipH,
            -1,
            0,
            false,
            false,
            emptyClothing,
            NULL );
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
                           SimpleVector<int> *inClothingContained,
                           double *outSlotRots,
                           doublePair *outSlotOffsets ) {
    
    if( inType == ground2 ) {
        inType = ground;
        }

    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        setObjectDrawLayerCutoff( animLayerCutoff );
        animLayerCutoff = -1;
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
                               inClothingContained,
                               outSlotRots, outSlotOffsets );
        }
    }




void drawObjectAnimHighlighted(
    float inHighlightFade,
    int inObjectID, AnimType inType, double inFrameTime,
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
    int inNumContained, int *inContainedIDs,
    SimpleVector<int> *inSubContained ) {

    
    // draw object normally
    drawObjectAnim( 
        inObjectID, inType, inFrameTime,
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
        inNumContained, inContainedIDs,
        inSubContained );

    if( inHighlightFade > 0 ) {
        // draw highlight over top
        int numPasses = 6;
        int startPass = 1;
        
        for( int i=startPass; i<numPasses; i++ ) {
            switch( i ) {
                case 0:
                    // normal color draw
                    break;
                case 1:
                    // opaque portion
                    startAddingToStencil( false, true, .99f );
                    break;
                case 2:
                    // first fringe
                    startAddingToStencil( false, true, .07f );
                    break;
                case 3:
                    // subtract opaque from fringe to get just first fringe
                    startAddingToStencil( false, false, .99f );
                    break;
                case 4:
                    // second fringe
                    // ignore totally transparent stuff
                    // like invisible animation layers
                    startAddingToStencil( false, true, 0.01f );
                    break;
                case 5:
                    // subtract first fringe from fringe to get 
                    // just secon fringe
                    startAddingToStencil( false, false, .07f );
                    break;
                default:
                    break;
                }

            drawObjectAnim( 
                inObjectID, inType, inFrameTime,
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
                inNumContained, inContainedIDs,
                inSubContained );
            
            
            float mainFade = .35f;
            
            toggleAdditiveBlend( true );
            
            doublePair squarePos = inPos;
            
            int squareRad = 128;
            
            switch( i ) {
                case 0:
                    // normal color draw
                    break;
                case 1:
                    // opaque portion
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, inHighlightFade * mainFade );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                case 2:
                    // first fringe
                    // wait until next pass to isolate fringe
                    break;
                case 3:
                    // now first fringe is isolated in stencil
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, inHighlightFade * mainFade * .5 );

                    drawSquare( squarePos, squareRad );

                    stopStencil();                    
                    break;
                case 4:
                    // second fringe
                    // wait until next pass to isolate fringe
                    break;
                case 5:
                    // now second fringe is isolated in stencil
                    startDrawingThroughStencil( false );
                    
                    setDrawColor( 1, 1, 1, inHighlightFade * mainFade *.25 );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                default:
                    break;
                }
            toggleAdditiveBlend( false );
            }
        }
    }




// compute base pos and rot of each object
// THEN go back through list and compute compound effects by walking
// through tree of parent relationships.

// we need a place to store all of these intermediary pos and rots

// avoid mallocs every draw call by statically allocating this
// here, we assume no object has more than 1000 sprites
#define MAX_WORKING_SPRITES 1000
static doublePair workingSpritePos[MAX_WORKING_SPRITES];
static doublePair workingDeltaSpritePos[MAX_WORKING_SPRITES];
static double workingRot[MAX_WORKING_SPRITES];
static double workingDeltaRot[MAX_WORKING_SPRITES];
static double workingSpriteFade[MAX_WORKING_SPRITES];

// also assume not more than 1000 slots
static double workingSlotRots[MAX_WORKING_SPRITES];
static doublePair workingSlotOffsets[MAX_WORKING_SPRITES];


static char headlessSkipFlags[MAX_WORKING_SPRITES];


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
    

    if( layerAnimation->pauseSec == 0 && layerAnimation->startPauseSec == 0 ) {
        return inFrameTime;
        }
    

    double dur = layerAnimation->durationSec;
    double pause = layerAnimation->pauseSec;
    double startPause = layerAnimation->startPauseSec;
    
    
    if( inFrameTime < startPause ) {
        // freeze time at 0, animation still in first pause
        return 0;
        }


    double blockTime = dur + pause;
    
    double blockFraction = ( inFrameTime - startPause ) / blockTime;
            
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




doublePair closestObjectDrawAnchorPos = { 0, 0 };

doublePair closestObjectDrawPos[2] = { { 0, 0 }, { 0, 0 } };

double closestObjectDrawDistance[2] = { DBL_MAX, DBL_MAX };

int closestObjectDrawID[2] = { -1, -1 };

char useFixedWatchedDrawPos = false;
doublePair fixedWatchedDrawPos;

char ignoreWatchedObjectDrawOn = false;


void fixWatchedObjectDrawPos( doublePair inPos ) {
    useFixedWatchedDrawPos = true;
    fixedWatchedDrawPos = inPos;
    }



void unfixWatchedObjectDrawPos() {
    useFixedWatchedDrawPos = false;
    }


void ignoreWatchedObjectDraw( char inIgnore ) {
    ignoreWatchedObjectDrawOn = inIgnore;
    }




void startWatchForClosestObjectDraw( int inObjecID[2], doublePair inPos ) {
    closestObjectDrawDistance[0] = DBL_MAX;
    closestObjectDrawDistance[1] = DBL_MAX;
    
    closestObjectDrawAnchorPos = inPos;
    closestObjectDrawID[0] = inObjecID[0];
    closestObjectDrawID[1] = inObjecID[1];
    }



doublePair getClosestObjectDraw( char *inDrawn, int inIndex ) {
    if( closestObjectDrawDistance[ inIndex ] < DBL_MAX ) {
        *inDrawn = true;
        }
    else {
        *inDrawn = false;
        }
    return closestObjectDrawPos[ inIndex ];
    }



void checkDrawPos( int inObjectID, doublePair inPos ) {
    if( ignoreWatchedObjectDrawOn ) return;
    
    if( inObjectID != closestObjectDrawID[0] &&
        inObjectID != closestObjectDrawID[1] ) {
        
        inObjectID = getObjectParent( inObjectID );
        
        if( inObjectID != closestObjectDrawID[0] &&
            inObjectID != closestObjectDrawID[1] ) return;
        }

    int index = 0;
    if( inObjectID == closestObjectDrawID[1] ) {
        index = 1;
        }
    
    doublePair posToUse = inPos;
    
    if( useFixedWatchedDrawPos ) {
        posToUse = fixedWatchedDrawPos;
        }

    double d = distance( posToUse, closestObjectDrawAnchorPos );
    
    if( d < closestObjectDrawDistance[index] ) {
        closestObjectDrawDistance[index] = d;
        closestObjectDrawPos[index] = posToUse;
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
                           SimpleVector<int> *inClothingContained,
                           double *outSlotRots,
                           doublePair *outSlotOffsets ) {

    HoldingPos returnHoldingPos = { false, {0, 0}, 0 };



    *outFrozenRotFrameTimeUsed = false;



    ObjectRecord *obj = getObject( inObjectID );
    
    if( obj->noFlip ) {
        inFlipH = false;
        }

    if( obj->numSprites > MAX_WORKING_SPRITES ) {
        // cannot animate objects with this many sprites
        
        setObjectDrawLayerCutoff( animLayerCutoff );
        animLayerCutoff = -1;

        drawObject( obj, inDrawBehindSlots,
                    inPos, inRot, inWorn, inFlipH, inAge, 
                    inHideClosestArm, inHideAllLimbs, inHeldNotInPlaceYet,
                    inClothing );
        return returnHoldingPos;
        }


    checkDrawPos( inObjectID, inPos );
    

    SimpleVector <int> frontArmIndices;
    getFrontArmIndices( obj, inAge, &frontArmIndices );

    SimpleVector <int> backArmIndices;
    getBackArmIndices( obj, inAge, &backArmIndices );

    SimpleVector <int> legIndices;
    getAllLegIndices( obj, inAge, &legIndices );


    // worn clothing never goes to ground animation
    // switches between moving, when the wearer is moving, and held,
    // when they're not moving
    AnimType clothingAnimType = inAnim->type;
    AnimType clothingFadeTargetAnimType = inFadeTargetAnim->type;

    if( inAnim->type != moving ) {
        clothingAnimType = held;
        }
    if( inFadeTargetAnim->type != moving ) {
        clothingFadeTargetAnimType = held;
        }


    int headIndex = getHeadIndex( obj, inAge );
    int bodyIndex = getBodyIndex( obj, inAge );
    int backFootIndex = getBackFootIndex( obj, inAge );
    int frontFootIndex = getFrontFootIndex( obj, inAge );

    int eyesIndex = -1;
    int mouthIndex = -1;
    
    if( drawWithEmots.size() > 0 ) {
        eyesIndex = getEyesIndex( obj, inAge );
        mouthIndex = getMouthIndex( obj, inAge );
        
        // these are never bottom layer
        // mark as non-existing instead
        if( eyesIndex == 0 ) {
            eyesIndex = -1;
            }
        if( mouthIndex == 0 ) {
            mouthIndex = -1;
            }
        }


    // otherEmote (over face) has power to hide head entirely
    char headless = false;
    if( headIndex != -1 &&
        drawWithEmots.size() > 0 &&
        obj->numSprites < MAX_WORKING_SPRITES ) {
        
        for( int e=0; e<drawWithEmots.size(); e++ ) {
            if( drawWithEmots.getElementDirect(e)->otherEmot != 0 ) {
                ObjectRecord *o = getObject( 
                    drawWithEmots.getElementDirect(e)->otherEmot );
                
                if( o->hideHead ) {
                    headless = true;
                    }
                }
            }

        if( headless ) {
            
            // look for a part that's even lower than head
            // but has head as a parent
            for( int i=0; i<obj->numSprites; i++ ) {
                // flag all objects that have head as parent
                headlessSkipFlags[i] = false;
                
                int p = obj->spriteParent[i];

                // walk up parent chain until we reach head or fall off
                while( p != -1 && p != headIndex ) {
                    p = obj->spriteParent[p];
                    }

                if( p == headIndex ) {
                    headlessSkipFlags[i] = true;
                    } 
                }
            headlessSkipFlags[headIndex] = true;
            }
        }
    
    

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

    doublePair animBodyPos = bodyPos;
    double animBodyRotDelta = 0;

    
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
                0,
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
            
            if( hardness == 1 ) {
                // don't apply cross-fade to fades
                workingSpriteFade[i] = fade;
                }
            else {
                // crossfade the fades
                workingSpriteFade[i] = inAnimFade * fade;
                }
            

            spritePos.x += 
                inAnimFade * 
                getOscOffset( 
                    spriteFrameTime,
                    spriteAnim->spriteAnim[i].offset.x,
                    spriteAnim->spriteAnim[i].xOscPerSec,
                    spriteAnim->spriteAnim[i].xAmp,
                    spriteAnim->spriteAnim[i].xPhase );
            
            spritePos.y += 
                inAnimFade *
                getOscOffset( 
                    spriteFrameTime,
                    spriteAnim->spriteAnim[i].offset.y,
                    spriteAnim->spriteAnim[i].yOscPerSec,
                    spriteAnim->spriteAnim[i].yAmp,
                    spriteAnim->spriteAnim[i].yPhase );
            
            double rock = inAnimFade * 
                getOscOffset( spriteFrameTime,
                              0,
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
                    0,
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

                // if target has fade that is fully hard, go
                // right there with no smooth cross-fade transtion
                if( hardnessB == 1 ) {
                    // wait until we're half-way through fade to
                    // execute the snap transition
                    if( targetWeight > 0.5 ) {
                        workingSpriteFade[i] = fadeB;
                        }
                    }
                else {
                    // crossfade the fades
                    workingSpriteFade[i] += targetWeight * fadeB;
                    }
                

                
                spritePos.x += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        spriteFadeTargetAnim->spriteAnim[i].offset.x,
                        spriteFadeTargetAnim->spriteAnim[i].xOscPerSec,
                        spriteFadeTargetAnim->spriteAnim[i].xAmp,
                        spriteFadeTargetAnim->spriteAnim[i].xPhase );
                
                spritePos.y += 
                    targetWeight *
                    getOscOffset( 
                        targetSpriteFrameTime,
                        spriteFadeTargetAnim->spriteAnim[i].offset.y,
                        spriteFadeTargetAnim->spriteAnim[i].yOscPerSec,
                        spriteFadeTargetAnim->spriteAnim[i].yAmp,
                        spriteFadeTargetAnim->spriteAnim[i].yPhase );

                 rock += 
                     targetWeight *
                     getOscOffset( 
                         targetSpriteFrameTime,
                         0,
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

    doublePair badgePos = { 0, 0 };
    double badgeRot = 0;

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

    int limit = obj->numSprites;
    
    if( animLayerCutoff > -1 && animLayerCutoff < limit ) {
        limit = animLayerCutoff;
        }
    animLayerCutoff = -1;
    

    for( int i=0; i<limit; i++ ) {
        
        if( obj->spriteSkipDrawing[i] ) {
            continue;
            }
        
        if( obj->person &&
            ! isSpriteVisibleAtAge( obj, i, inAge ) ) {    
            // skip drawing this aging layer entirely
            continue;
            }
        
        
        if( obj->person && shouldHidePersonShadows &&
            strstr( getSpriteRecord( obj->sprites[i] )->tag, 
                    "Shadow" ) != NULL ) {
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
            if( inRot != 0 ) {
                animHeadPos = rotate( animHeadPos, -2 * M_PI * inRot );
                animHeadRotDelta += inRot;
                }            
            }

        
        if( i == bodyIndex ) {
            // this is the body
            animBodyPos = spritePos;
            animBodyRotDelta = rot - obj->spriteRot[i];
            
            if( inFlipH ) {    
                animBodyPos.x *= -1;
                animBodyRotDelta *= -1;
                }
            
            if( inRot != 0 ) {
                animBodyPos = rotate( animBodyPos, -2 * M_PI * inRot );
                animBodyRotDelta += inRot;
                }
            }


        char spriteNoFlip = false;
        
        if( inFlipH && getNoFlip( obj->sprites[i] ) ) {
            spriteNoFlip = true;
            }

        if( inFlipH ) {
            
            if( spriteNoFlip && obj->spriteNoFlipXPos != NULL ) {
                spritePos.x = obj->spriteNoFlipXPos[i];
                }

            spritePos.x *= -1;
            rot *= -1;
            }
        
        if( inRot != 0 ) {
            rot += inRot;
            spritePos = rotate( spritePos, -2 * M_PI * inRot );
            }
            

        
        doublePair pos = add( spritePos, inPos );


        char skipSprite = false;
        

        if( headless &&
            headlessSkipFlags[i] ) {
            // skip drawing head and everything that is a child of it
            // this should still draw hat floating above
            skipSprite = true;
            }
        

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
        
        if( i == mouthIndex && drawWithEmots.size() > 0 ) {
            for( int e=0; e<drawWithEmots.size(); e++ ) {
                if( drawWithEmots.getElementDirect(e)->mouthEmot != 0 ) {
                    skipSprite = true;
                    break;
                    }
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
            
            if( drawWithBadge != -1 &&
                ( inClothing.tunic != NULL || bareBadge ) ) {
                ObjectRecord *badgeO = getObject( drawWithBadge );
                doublePair offset = badgeO->clothingOffset;
            
                if( inFlipH ) {
                    offset.x *= -1;
                    badgeRot = -rot - obj->spriteRot[i];
                    }
                else {
                    badgeRot = rot - obj->spriteRot[i];
                    }
                    
                if( badgeRot != 0 ) {
                    if( inFlipH ) {
                        offset = rotate( offset, 2 * M_PI * badgeRot );
                        badgeRot *= -1;
                        }
                    else {
                        offset = rotate( offset, -2 * M_PI * badgeRot );
                        }
                    }
                    
                    
                doublePair cPos = add( spritePos, offset );
                    
                cPos = add( cPos, inPos );
                    
                badgePos = cPos;               
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
            

            // body emote above all body parts
            for( int e=0; e<drawWithEmots.size(); e++ )
            if( drawWithEmots.getElementDirect( e )->bodyEmot != 0 &&
                obj->person ) {
            
                char used;
                drawObjectAnim( drawWithEmots.getElementDirect( e )->bodyEmot, 
                                clothingAnimType, 
                                inFrameTime,
                                inAnimFade, 
                                clothingFadeTargetAnimType,
                                inFadeTargetFrameTime,
                                inFrozenRotFrameTime,
                                &used,
                                endAnimType,
                                endAnimType,
                                add( animBodyPos, inPos ),
                                animBodyRotDelta,
                                true,
                                inFlipH,
                                -1,
                                0,
                                false,
                                false,
                                emptyClothing,
                                NULL,
                                0, NULL,
                                NULL );
                }

            if( inClothing.bottom != NULL ) {
                int numCont = 0;
                int *cont = NULL;
                if( inClothingContained != NULL ) {    
                    numCont = inClothingContained[4].size();
                    cont = inClothingContained[4].getElementArray();
                    }
                

                char used;
                drawObjectAnimHighlighted( clothingHighlightFades[4],
                                inClothing.bottom->id, 
                                clothingAnimType, 
                                inFrameTime,
                                inAnimFade, 
                                clothingFadeTargetAnimType,
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
                                numCont, cont,
                                NULL );
                
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
                drawObjectAnimHighlighted( clothingHighlightFades[1],
                                inClothing.tunic->id, 
                                clothingAnimType, 
                                inFrameTime,
                                inAnimFade, 
                                clothingFadeTargetAnimType,
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
                                numCont, cont,
                                NULL );

                if( cont != NULL ) {
                    delete [] cont;
                    cont = NULL;
                    }
                numCont = 0;
                }
            if( inClothing.backpack != NULL ) {
                int numCont = 0;
                int *cont = NULL;
                if( inClothingContained != NULL ) {    
                    numCont = inClothingContained[5].size();
                    cont = inClothingContained[5].getElementArray();
                    }

                char used;
                drawObjectAnimHighlighted( clothingHighlightFades[5],
                                inClothing.backpack->id, 
                                clothingAnimType, 
                                inFrameTime,
                                inAnimFade, 
                                clothingFadeTargetAnimType,
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
                                numCont, cont,
                                NULL );

                if( cont != NULL ) {
                    delete [] cont;
                    }
                }            
            if( drawWithBadge != -1 &&
                ( inClothing.tunic != NULL || bareBadge ) ) {
                spriteColorOverrideOn = true;
                spriteColorOverride = drawWithBadgeColor;
                
                char used;
                drawObjectAnimHighlighted( clothingHighlightFades[1],
                                           drawWithBadge, 
                                           clothingAnimType, 
                                           inFrameTime,
                                           inAnimFade, 
                                           clothingFadeTargetAnimType,
                                           inFadeTargetFrameTime,
                                           inFrozenRotFrameTime,
                                           &used,
                                           endAnimType,
                                           endAnimType,
                                           badgePos,
                                           badgeRot,
                                           true,
                                           inFlipH,
                                           -1,
                                           0,
                                           false,
                                           false,
                                           emptyClothing,
                                           NULL,
                                           0, NULL,
                                           NULL );
                spriteColorOverrideOn = false;
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
            if( spriteColorOverrideOn ) {
                setDrawColor( spriteColorOverride );
                }
            else {
                setDrawColor( obj->spriteColor[i] );
                }
            
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
                    // alpha ignored for multiplicative blend
                    // but leave 0 there so that they won't add to stencil
                    setDrawColor( invFade, invFade, invFade, 0.0f );
                    }
                else {
                    // set 0 so translucent layers never add to stencil
                    setDrawFade( 0.0f );
                    }
                }
            else if( workingSpriteFade[i] < 1 ) {
                setDrawFade( workingSpriteFade[i] );
                }

            
            char additive = false;
            if( obj->spriteAdditiveBlend != NULL ) {
                additive = obj->spriteAdditiveBlend[i];
                }
            if( additive ) {
                toggleAdditiveBlend( true );
                }
            
            int spriteID = obj->sprites[i];
            
            if( drawMouthShapes && spriteID == mouthAnchorID &&
                mouthShapeFrame < numMouthShapeFrames ) {
                drawSprite( mouthShapeFrameList[ mouthShapeFrame ], 
                            pos, 1.0, rot, 
                            logicalXOR( inFlipH, obj->spriteHFlip[i] ) );
                mouthShapeFrame ++;
                
                if( outputMouthFrames ) {
                    
                    if( mouthShapeFrame < numMouthShapeFrames ) {
                        
                        if( !mouthFrameOutputStarted ) {
                            startOutputAllFrames();
                            }
                        }
                    else {
                        // done
                        stopOutputAllFrames();
                        }
                    }
                }
            else {
                SpriteHandle sh = getSprite( spriteID );
                if( sh != NULL ) {
                    char f = inFlipH;
                    
                    if( f && spriteNoFlip ) {
                        f = false;
                        }
                    
                    drawSprite( sh, pos, 1.0, rot, 
                                logicalXOR( f, obj->spriteHFlip[i] ) );
                    }
                }
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                toggleAdditiveTextureColoring( false );
                }
            
            if( additive ) {
                toggleAdditiveBlend( false );
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


        // eye emot on top of eyes
        if( i == eyesIndex && !headless )
        for( int e=0; e<drawWithEmots.size(); e++ )
        if( drawWithEmots.getElementDirect(e)->eyeEmot != 0 ) {
            
            doublePair offset = obj->mainEyesOffset;

        
            if( inFlipH ) {
                offset.x *= -1;
                }                
        
            if( animHeadRotDelta != 0 ) {
                offset = rotate( offset, -2 * M_PI * animHeadRotDelta );
                }
        
        
            doublePair cPos = add( animHeadPos, offset );

            cPos = add( cPos, inPos );

            char used;
            drawObjectAnim( drawWithEmots.getElementDirect(e)->eyeEmot, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
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
                            0, NULL,
                            NULL );
            }



        // face emot on top of eyes, if they exist, or head
        // if not
        if( ( ( eyesIndex != -1 && i == eyesIndex ) 
              ||
              ( eyesIndex == -1 && i == headIndex ) )
            && obj->person  && !headless )
        for( int e=0; e<drawWithEmots.size(); e++ )
        if( drawWithEmots.getElementDirect(e)->faceEmot != 0 ) {
            
            char used;
            drawObjectAnim( drawWithEmots.getElementDirect(e)->faceEmot, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
                            inFadeTargetFrameTime,
                            inFrozenRotFrameTime,
                            &used,
                            endAnimType,
                            endAnimType,
                            add( animHeadPos, inPos ),
                            animHeadRotDelta,
                            true,
                            inFlipH,
                            -1,
                            0,
                            false,
                            false,
                            emptyClothing,
                            NULL,
                            0, NULL,
                            NULL );
            }


        


        // mouth on top of head
        // but only if there's a mouth to be replaced
        if( i == headIndex && mouthIndex != -1 && !headless )
        for( int e=0; e<drawWithEmots.size(); e++ )
        if( drawWithEmots.getElementDirect(e)->mouthEmot != 0 ) {
            
            char used;
            drawObjectAnim( drawWithEmots.getElementDirect(e)->mouthEmot, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
                            inFadeTargetFrameTime,
                            inFrozenRotFrameTime,
                            &used,
                            endAnimType,
                            endAnimType,
                            add( animHeadPos, inPos ),
                            animHeadRotDelta,
                            true,
                            inFlipH,
                            -1,
                            0,
                            false,
                            false,
                            emptyClothing,
                            NULL,
                            0, NULL,
                            NULL );
            }
        

        // other emot on top of head (but make sure it only applies to people)
        // other emote tests depend on eyes index or mouth index, which
        // are forced to 0 for non-people (and become -1 above), but 
        // this does not happen for headIndex
        if( i == headIndex && obj->person && !headless )
        for( int e=0; e<drawWithEmots.size(); e++ )
        if( drawWithEmots.getElementDirect(e)->otherEmot != 0 ) {
            
            char used;
            drawObjectAnim( drawWithEmots.getElementDirect(e)->otherEmot, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
                            inFadeTargetFrameTime,
                            inFrozenRotFrameTime,
                            &used,
                            endAnimType,
                            endAnimType,
                            add( animHeadPos, inPos ),
                            animHeadRotDelta,
                            true,
                            inFlipH,
                            -1,
                            0,
                            false,
                            false,
                            emptyClothing,
                            NULL,
                            0, NULL,
                            NULL );
            }




        // shoes on top of feet
        if( ! skipSprite && 
            inClothing.backShoe != NULL && i == backFootIndex ) {
            
            int numCont = 0;
            int *cont = NULL;
            if( inClothingContained != NULL ) {    
                numCont = inClothingContained[3].size();
                cont = inClothingContained[3].getElementArray();
                }

            char used;
            drawObjectAnimHighlighted( clothingHighlightFades[3],
                            inClothing.backShoe->id, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
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
                            numCont, cont,
                            NULL );

            if( cont != NULL ) {
                delete [] cont;
                }
            }
        else if( ! skipSprite && 
                 inClothing.frontShoe != NULL && i == frontFootIndex ) {
            int numCont = 0;
            int *cont = NULL;
            if( inClothingContained != NULL ) {    
                numCont = inClothingContained[2].size();
                cont = inClothingContained[2].getElementArray();
                }
            
            char used;
            drawObjectAnimHighlighted( clothingHighlightFades[2],
                            inClothing.frontShoe->id, 
                            clothingAnimType, 
                            inFrameTime,
                            inAnimFade, 
                            clothingFadeTargetAnimType,
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
                            numCont, cont,
                            NULL );

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
        drawObjectAnimHighlighted( clothingHighlightFades[0],
                        inClothing.hat->id, 
                        clothingAnimType, 
                        inFrameTime,
                        inAnimFade, 
                        clothingFadeTargetAnimType,
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
                        numCont, cont,
                        NULL );
        
        if( cont != NULL ) {
            delete [] cont;
            }
        }



    // head emot on top of everything, including hat
    if( obj->person )
    for( int e=0; e<drawWithEmots.size(); e++ )
    if( drawWithEmots.getElementDirect(e)->headEmot != 0 ) {
            
        char used;
        drawObjectAnim( drawWithEmots.getElementDirect(e)->headEmot, 
                        clothingAnimType, 
                        inFrameTime,
                        inAnimFade, 
                        clothingFadeTargetAnimType,
                        inFadeTargetFrameTime,
                        inFrozenRotFrameTime,
                        &used,
                        endAnimType,
                        endAnimType,
                        add( animHeadPos, inPos ),
                        animHeadRotDelta,
                        true,
                        inFlipH,
                        -1,
                        0,
                        false,
                        false,
                        emptyClothing,
                        NULL,
                        0, NULL,
                        NULL );
        }


    
    if( animLayerFades != NULL ) {
        delete [] animLayerFades;
        animLayerFades = NULL;
        }

    
    // now compute adjustments for slots based on their parent relationships
    // if requested
    if( outSlotRots != NULL && outSlotOffsets != NULL ) {
        
        for( int i=0; i<obj->numSlots; i++ ) {
            doublePair slotPos = obj->slotPos[i];
            double rot = 0;
            
            
            int nextParent = obj->slotParent[i];
        
            while( nextParent != -1 ) {
            
            
            
                if( workingDeltaRot[nextParent] != 0 ) {
                    rot += workingDeltaRot[ nextParent ];
                    
                    double angle = - 2 * M_PI * workingDeltaRot[ nextParent ];
                    
                    slotPos = add( slotPos, 
                                      rotate( 
                                          workingDeltaSpritePos[ nextParent ],
                                          -angle ) );

                    // add in positional change based on arm's length rotation
                    // around parent
            
                    doublePair childOffset = 
                        sub( slotPos, 
                             obj->spritePos[nextParent] );
                
                    doublePair newChildOffset = rotate( childOffset, angle );
                
                    slotPos = 
                        add( slotPos, sub( newChildOffset, childOffset ) );
                    }
                else {
                    slotPos = add( slotPos, 
                                   workingDeltaSpritePos[ nextParent ] );
                    }
                    
                nextParent = obj->spriteParent[nextParent];
                }
            
            outSlotRots[i] = rot;
            outSlotOffsets[i] = sub( slotPos, obj->slotPos[i] );
            }
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
                     int inNumContained, int *inContainedIDs,
                     SimpleVector<int> *inSubContained ) {
    
    if( inType == ground2 ) {
        inType = ground;
        }
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        setObjectDrawLayerCutoff( animLayerCutoff );
        animLayerCutoff = -1;

        drawObject( getObject( inObjectID ), inPos, inRot, inWorn, 
                    inFlipH, inAge, inHideClosestArm, inHideAllLimbs, 
                    inHeldNotInPlaceYet, inClothing,
                    inNumContained, inContainedIDs,
                    inSubContained );
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
                        inNumContained, inContainedIDs,
                        inSubContained );
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
                     int inNumContained, int *inContainedIDs,
                     SimpleVector<int> *inSubContained ) {
    
    ClothingSet emptyClothing = getEmptyClothingSet();

    ObjectRecord *obj = getObject( inObjectID );


    double *slotRots = NULL;
    doublePair *slotOffsets = NULL;
    

    if( obj->numSlots < MAX_WORKING_SPRITES ) {
        slotRots = workingSlotRots;
        slotOffsets = workingSlotOffsets;
        }


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
                    inClothingContained,
                    slotRots,
                    slotOffsets );
    
    char allBehind = true;
    for( int i=0; i< obj->numSprites; i++ ) {
        if( ! obj->spriteBehindSlots[i] ) {
            allBehind = false;
            break;
            }
        }
    
    
    // next, draw jiggling (never rotating) objects in slots
    // can't safely rotate them, because they may be compound objects

    // all of these are in contained mode
    setDrawnObjectContained( true );
    
    
    if( ! obj->slotsInvis )
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
            
            doublePair centerOffset;

            if( allBehind ) {
                centerOffset = getObjectBottomCenterOffset( contained );
                }
            else {
                centerOffset = getObjectCenterOffset( contained );
                }
            
            double rot = inRot;
            
            if( slotRots != NULL ) {
                if( inFlipH ) {
                    rot -= slotRots[i];
                    }
                else {
                    rot += slotRots[i];
                    }

                centerOffset = rotate( centerOffset, 
                                       - slotRots[i] * 2 * M_PI );
                }
            
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
            
            if( slotRots != NULL ) {
                pos = add( pos, slotOffsets[i] );                
                }
        
            doublePair posWiggle = { 0, 0 };
            

            if( i < inAnim->numSlots ) {
                
                posWiggle.x += 
                    inAnimFade *
                    getOscOffset( 
                        slotFrameTime,
                        inAnim->slotAnim[i].offset.x,
                        inAnim->slotAnim[i].xOscPerSec,
                        inAnim->slotAnim[i].xAmp,
                        inAnim->slotAnim[i].xPhase );
                
                posWiggle.y += 
                    inAnimFade * 
                    getOscOffset( 
                        slotFrameTime,
                        inAnim->slotAnim[i].offset.y,
                        inAnim->slotAnim[i].yOscPerSec,
                        inAnim->slotAnim[i].yAmp,
                        inAnim->slotAnim[i].yPhase );

                if( inAnimFade < 1 ) {
                    double targetWeight = 1 - inAnimFade;
                
                    posWiggle.x += 
                        targetWeight *
                        getOscOffset( 
                            targetSlotFrameTime,
                            inFadeTargetAnim->slotAnim[i].offset.x,
                            inFadeTargetAnim->slotAnim[i].xOscPerSec,
                            inFadeTargetAnim->slotAnim[i].xAmp,
                            inFadeTargetAnim->slotAnim[i].xPhase );
                    
                    posWiggle.y += 
                        targetWeight *
                        getOscOffset( 
                            targetSlotFrameTime,
                            inFadeTargetAnim->slotAnim[i].offset.y,
                            inFadeTargetAnim->slotAnim[i].yOscPerSec,
                            inFadeTargetAnim->slotAnim[i].yAmp,
                            inFadeTargetAnim->slotAnim[i].yPhase );
                    }
                }
                  
            if( slotRots != NULL ) {
                posWiggle = rotate( posWiggle, -2 * M_PI * slotRots[i] );
                }
            pos = add( pos, posWiggle );

            if( inFlipH ) {
                pos.x *= -1;
                }
  
            if( inRot != 0 ) {
                pos = rotate( pos, -2 * M_PI * inRot );
                }
            

            pos = add( pos, inPos );
            

            if( inSubContained != NULL &&
                inSubContained[i].size() > 0 ) {
                
                AnimationRecord *subAnim = getAnimation( contained->id,
                                                         inAnim->type );
                AnimationRecord *subFadeTargetAnim = 
                    getAnimation( contained->id,
                                  inFadeTargetAnim->type );
                


                // behind sub-contained
                drawObject( contained, 0, pos, rot, false, inFlipH,
                            inAge, 0, false, false, emptyClothing );


                for( int s=0; s<contained->numSlots; s++ ) {
                    if( s < inSubContained[i].size() ) {
                    
                        doublePair subPos = contained->slotPos[s];
                    

                        ObjectRecord *subContained = getObject( 
                            inSubContained[i].getElementDirect( s ) );
                    
                        doublePair subCenterOffset =
                            getObjectCenterOffset( subContained );
                    
                        double subRot = rot;

                        if( contained->slotVert[s] ) {
                            double rotOffset = 
                                0.25 + subContained->vertContainRotationOffset;
                
                            if( inFlipH ) {
                                subCenterOffset = 
                                    rotate( subCenterOffset, 
                                            - rotOffset * 2 * M_PI );
                                subRot -= rotOffset;
                                }
                            else {
                                subCenterOffset = 
                                    rotate( subCenterOffset, 
                                            - rotOffset * 2 * M_PI );
                                subRot += rotOffset;
                                }
                            }
                    
                        subPos = sub( subPos, subCenterOffset );
                    
                        // apply anim to sub-slots
                        if( subAnim != NULL && s < subAnim->numSlots ) {
                
                            subPos.x += 
                                inAnimFade *
                                getOscOffset( 
                                    slotFrameTime,
                                    subAnim->slotAnim[s].offset.x,
                                    subAnim->slotAnim[s].xOscPerSec,
                                    subAnim->slotAnim[s].xAmp,
                                    subAnim->slotAnim[s].xPhase );
                
                            subPos.y += 
                                inAnimFade * 
                                getOscOffset( 
                                    slotFrameTime,
                                    subAnim->slotAnim[s].offset.y,
                                    subAnim->slotAnim[s].yOscPerSec,
                                    subAnim->slotAnim[s].yAmp,
                                    subAnim->slotAnim[s].yPhase );

                            if( inAnimFade < 1 ) {
                                double targetWeight = 1 - inAnimFade;
                                
                                subPos.x += targetWeight * getOscOffset( 
                                    targetSlotFrameTime,
                                    subFadeTargetAnim->slotAnim[s].offset.x,
                                    subFadeTargetAnim->slotAnim[s].xOscPerSec,
                                    subFadeTargetAnim->slotAnim[s].xAmp,
                                        subFadeTargetAnim->slotAnim[s].xPhase );
                    
                                subPos.y += targetWeight * getOscOffset( 
                                    targetSlotFrameTime,
                                    subFadeTargetAnim->slotAnim[s].offset.y,
                                    subFadeTargetAnim->slotAnim[s].yOscPerSec,
                                    subFadeTargetAnim->slotAnim[s].yAmp,
                                    subFadeTargetAnim->slotAnim[s].yPhase );
                                }
                            }
            



                        if( inFlipH ) {
                            subPos.x *= -1;
                            }
                    
                        if( rot != 0 ) {
                            subPos = rotate( subPos, -2 * M_PI * rot );
                            }
            

                        subPos = add( subPos, pos );

                        drawObject( subContained, 2, subPos, subRot, 
                                    false, inFlipH,
                                    inAge, 0, false, false, emptyClothing );
                        }
                    }
                
                // in front of sub-contained
                drawObject( contained, 1, pos, rot, false, inFlipH,
                            inAge, 0, false, false, emptyClothing );

                }
            else {
                // no sub-contained
                // draw contained all at once
                drawObject( contained, 2, pos, rot, false, inFlipH,
                            inAge, 0, false, false, emptyClothing );
                }
            }
        
        } 

    // done drawing all contained/sub-contained items with drawObject calls
    setDrawnObjectContained( false );


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



AnimationRecord *copyRecord( AnimationRecord *inRecord,
                             char inCountLiveSoundUses ) {
    AnimationRecord *newRecord = new AnimationRecord;
    
    newRecord->objectID = inRecord->objectID;
    newRecord->type = inRecord->type;
    newRecord->randomStartPhase = inRecord->randomStartPhase;
    newRecord->forceZeroStart = inRecord->forceZeroStart;

    newRecord->numSounds = inRecord->numSounds;
    newRecord->soundAnim = new SoundAnimationRecord[ newRecord->numSounds ];
    
    memcpy( newRecord->soundAnim, inRecord->soundAnim,
            sizeof( SoundAnimationRecord ) * newRecord->numSounds );

    for( int i=0; i<newRecord->numSounds; i++ ) {
        newRecord->soundAnim[i] = copyRecord( inRecord->soundAnim[i],
                                              inCountLiveSoundUses );
        }

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



void freeRecord( AnimationRecord *inRecord ) {
    for( int s=0; s<inRecord->numSounds; s++ ) {
        freeRecord( &( inRecord->soundAnim[s] ) );
        }
    delete [] inRecord->soundAnim;
    delete [] inRecord->spriteAnim;
    delete [] inRecord->slotAnim;
    delete inRecord;
    }



SoundAnimationRecord copyRecord( SoundAnimationRecord inRecord,
                                 char inCountLiveSoundUses ) {
    SoundAnimationRecord copy = inRecord;
    
    copy.sound = copyUsage( inRecord.sound );
    
    if( inCountLiveSoundUses ) {
        countLiveUse( copy.sound );
        }
    
    return copy;
    }



void freeRecord( SoundAnimationRecord *inRecord ) {
    unCountLiveUse( inRecord->sound );
    clearSoundUsage( &( inRecord->sound ) );
    }





void zeroRecord( SpriteAnimationRecord *inRecord ) {
    inRecord->offset.x = 0;
    inRecord->offset.y = 0;
    
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
    inRecord->startPauseSec = 0;

    inRecord->rotationCenterOffset.x = 0;
    inRecord->rotationCenterOffset.y = 0;
    
    inRecord->fadeOscPerSec = 0;
    inRecord->fadeHardness = 0;
    inRecord->fadeMin = 0;
    inRecord->fadeMax = 1;
    inRecord->fadePhase = 0;
    }



void zeroRecord( SoundAnimationRecord *inRecord ) {
    clearSoundUsage( &( inRecord->sound ) );
    inRecord->repeatPerSec = 0;
    inRecord->repeatPhase = 0;
    inRecord->ageStart = -1;
    inRecord->ageEnd = -1;
    inRecord->footstep = 0;
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

    SimpleVector<AnimationRecord *> allAnims;
    
    for( int i=0; i<endAnimType; i++ ) {
        AnimationRecord *oldAnim = getAnimation( inObjectID, (AnimType)i );
        
        if( oldAnim == NULL ) {
            continue;
            }
        allAnims.push_back( oldAnim );
        }
    

    int numExtra = getNumExtraAnim( inObjectID );
    for( int j=0; j<numExtra; j++ ) {
        AnimationRecord *oldAnim = idExtraMap[inObjectID].getElementDirect( j );
        
        allAnims.push_back( oldAnim );
        }
    
    
    int extraIndex = 0;
    
    for( int i=0; i<allAnims.size(); i++ ) {
        AnimationRecord *oldAnim = allAnims.getElementDirect( i );

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

        
        if( r->type == extra ) {
            setExtraIndex( extraIndex );
            extraIndex++;
            }
        addAnimation( r );
        
        freeRecord( r );
        }
    

    }



char isSoundUsedByAnim( int inSoundID ) {
    for( int i=0; i<mapSize; i++ ) {
        for( int j=0; j<endAnimType; j++ ) {
            AnimationRecord *r = idMap[i][j];
            
            if( r != NULL ) {
                
                if( r->numSounds > 0 ) {
                    
                    for( int k=0; k < r->numSounds; k++ ) {
                        if( doesUseSound( r->soundAnim[k].sound, inSoundID ) ) {
                            return true;
                            }
                        }
                    }
                }
            }
        for( int j=0; j<idExtraMap[i].size(); j++ ) {
            AnimationRecord *r = idExtraMap[i].getElementDirect(j);
            
            if( r != NULL ) {
                
                if( r->numSounds > 0 ) {
                    
                    for( int k=0; k < r->numSounds; k++ ) {
                        if( doesUseSound( r->soundAnim[k].sound, inSoundID ) ) {
                            return true;
                            }
                        }
                    }
                }
            }
        }

    return false;
    }
