#include "animationBank.h"

#include "objectBank.h"
#include "spriteBank.h"

#include "stdlib.h"
#include "math.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"

#include "minorGems/game/gameGraphics.h"


#include "ageControl.h"



static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static AnimationRecord * **idMap;


void initAnimationBank() {
    SimpleVector<AnimationRecord*> records;
    
    int maxID = 0;
    
    
    File animationsDir( NULL, "animations" );
    if( animationsDir.exists() && animationsDir.isDirectory() ) {

        int numFiles;
        File **childFiles = animationsDir.getChildFiles( &numFiles );

        for( int i=0; i<numFiles; i++ ) {
            
            if( ! childFiles[i]->isDirectory() ) {
                                        
                char *txtFileName = childFiles[i]->getFileName();
                        
                if( strstr( txtFileName, ".txt" ) != NULL ) {
                    char *animText = childFiles[i]->readFileContents();
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
                                        "%lf %lf %lf %lf %lf %lf %lf %lf "
                                        "%lf %lf %lf",
                                        &( r->spriteAnim[j].xOscPerSec ),
                                        &( r->spriteAnim[j].xAmp ),
                                        &( r->spriteAnim[j].xPhase ),
                                        
                                        &( r->spriteAnim[j].yOscPerSec ),
                                        &( r->spriteAnim[j].yAmp ),
                                        &( r->spriteAnim[j].yPhase ),
                                        
                                        &( r->spriteAnim[j].rotPerSec ),
                                        &( r->spriteAnim[j].rotPhase ),
                                        
                                        &( r->spriteAnim[j].rockOscPerSec ),
                                        &( r->spriteAnim[j].rockAmp ),
                                        &( r->spriteAnim[j].rockPhase ) );
                                next++;
                                }

                            for( int j=0; j< r->numSlots && next < numLines;
                                 j++ ) {
                                
                                sscanf( lines[next], 
                                        "animParam="
                                        "%lf %lf %lf %lf %lf %lf",
                                        &( r->slotAnim[j].xOscPerSec ),
                                        &( r->slotAnim[j].xAmp ),
                                        &( r->slotAnim[j].xPhase ),
                                        
                                        &( r->slotAnim[j].yOscPerSec ),
                                        &( r->slotAnim[j].yAmp ),
                                        &( r->slotAnim[j].yPhase ) );
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
                }
            delete childFiles[i];
            }
        delete [] childFiles;
        }


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
                    "%lf %lf %lf %lf %lf %lf %lf %lf "
                    "%lf %lf %lf",
                    inRecord->spriteAnim[j].xOscPerSec,
                    inRecord->spriteAnim[j].xAmp,
                    inRecord->spriteAnim[j].xPhase,
                    
                    inRecord->spriteAnim[j].yOscPerSec,
                    inRecord->spriteAnim[j].yAmp,
                    inRecord->spriteAnim[j].yPhase,
                    
                    inRecord->spriteAnim[j].rotPerSec,
                    inRecord->spriteAnim[j].rotPhase,
                    
                    inRecord->spriteAnim[j].rockOscPerSec,
                    inRecord->spriteAnim[j].rockAmp,
                    inRecord->spriteAnim[j].rockPhase ) );
            }
        for( int j=0; j<inRecord->numSlots; j++ ) {
            lines.push_back( 
                autoSprintf( 
                    "animParam="
                    "%lf %lf %lf %lf %lf %lf",
                    inRecord->slotAnim[j].xOscPerSec,
                    inRecord->slotAnim[j].xAmp,
                    inRecord->slotAnim[j].xPhase,
                    
                    inRecord->slotAnim[j].yOscPerSec,
                    inRecord->slotAnim[j].yAmp,
                    inRecord->slotAnim[j].yPhase ) );
            }



        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );

        delete [] linesArray;
        lines.deallocateStringElements();
        

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

    File *animationFile = getFile( inObjectID, inType );
    animationFile->remove();
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
    
    if( curR == NULL || targetR == NULL ) {
        return false;
        }

    ObjectRecord *obj = getObject( inObjectID );
    
    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        // if current is moving at all, must fade

        if( curR->spriteAnim[i].xOscPerSec > 0 ) return true;
        
        if( curR->spriteAnim[i].xAmp != 0 &&
            curR->spriteAnim[i].xPhase != 0 &&
            curR->spriteAnim[i].xPhase != 0.5 ) return true;
        
        if( curR->spriteAnim[i].yOscPerSec > 0 ) return true;
        
        if( curR->spriteAnim[i].yAmp != 0 &&
            curR->spriteAnim[i].yPhase != 0 &&
            curR->spriteAnim[i].yPhase != 0.5 ) return true;
        
        
        if( curR->spriteAnim[i].rockOscPerSec > 0 ) return true;
        
        if( curR->spriteAnim[i].rockAmp != 0 &&
            curR->spriteAnim[i].rockPhase != 0 &&
            curR->spriteAnim[i].rockPhase != 0.5 ) return true;
        
        
        if( curR->spriteAnim[i].rotPerSec > 0 ) return true;
        
        

        // if target starts out of phase, must fade
        
        if( targetR->spriteAnim[i].xAmp != 0 &&
            targetR->spriteAnim[i].xPhase != 0 &&
            targetR->spriteAnim[i].xPhase != 0.5 ) return true;
        
        if( targetR->spriteAnim[i].yAmp != 0 &&
            targetR->spriteAnim[i].yPhase != 0 &&
            targetR->spriteAnim[i].yPhase != 0.5 ) return true;
        
        if( targetR->spriteAnim[i].rockAmp != 0 &&
            targetR->spriteAnim[i].rockPhase != 0 &&
            targetR->spriteAnim[i].rockPhase != 0.5 ) return true;
        
        if( targetR->spriteAnim[i].rotPerSec != 0 && 
            targetR->spriteAnim[i].rotPhase != 0 ) return true;
        }
    
    
    for( int i=0; i<obj->numSlots; i++ ) {
        // if current is moving at all, must fade

        if( curR->slotAnim[i].xOscPerSec > 0 ) return true;
        
        if( curR->slotAnim[i].xAmp != 0 &&
            ( curR->slotAnim[i].xPhase != 0 ||
              curR->slotAnim[i].xPhase != 0.5  ) ) return true;
        
        if( curR->slotAnim[i].yOscPerSec > 0 ) return true;
        
        if( curR->slotAnim[i].yAmp != 0 &&
            ( curR->slotAnim[i].yPhase != 0 ||
              curR->slotAnim[i].yPhase != 0.5  ) ) return true;        
        

        // if target starts out of phase, must fade
        
        if( targetR->slotAnim[i].xAmp != 0 &&
            ( targetR->slotAnim[i].xPhase != 0 ||
              targetR->slotAnim[i].xPhase != 0.5  ) ) return true;
        
        if( targetR->slotAnim[i].yAmp != 0 &&
            ( targetR->slotAnim[i].yPhase != 0 ||
              targetR->slotAnim[i].yPhase != 0.5  ) ) return true;
        }
    
    // current animation lines up with start of target
    // no fade
    return false;
    }




void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimType inFadeTargetType,
                     doublePair inPos,
                     char inFlipH,
                     double inAge ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, inFlipH, inAge );
        return;
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos, inFlipH, inAge );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim, 
                     double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     doublePair inPos,
                     char inFlipH,
                     double inAge ) {

    ObjectRecord *obj = getObject( inObjectID );

    for( int i=0; i<obj->numSprites; i++ ) {
        doublePair pos = obj->spritePos[i];
        
        if( obj->person && i == obj->numSprites - 1 ) {
            pos = add( pos, getAgeHeadOffset( inAge, pos ) );
            }

        double rot = 0;
        
        if( i < inAnim->numSprites ) {
            

            pos.x += 
                inAnimFade * 
                getOscOffset( 
                    inFrameTime,
                    inAnim->spriteAnim[i].xOscPerSec,
                    inAnim->spriteAnim[i].xAmp,
                    inAnim->spriteAnim[i].xPhase );
            
            pos.y += 
                inAnimFade *
                getOscOffset( 
                    inFrameTime,
                    inAnim->spriteAnim[i].yOscPerSec,
                    inAnim->spriteAnim[i].yAmp,
                    inAnim->spriteAnim[i].yPhase );
            
            double rock = inAnimFade * 
                getOscOffset( inFrameTime,
                              inAnim->spriteAnim[i].rockOscPerSec,
                              inAnim->spriteAnim[i].rockAmp,
                              inAnim->spriteAnim[i].rockPhase );

            if( inAnimFade < 1 ) {
                double targetWeight = 1 - inAnimFade;
                
                pos.x += 
                    targetWeight *
                    getOscOffset( 
                        0,
                        inFadeTargetAnim->spriteAnim[i].xOscPerSec,
                        inFadeTargetAnim->spriteAnim[i].xAmp,
                        inFadeTargetAnim->spriteAnim[i].xPhase );
                
                pos.y += 
                    targetWeight *
                    getOscOffset( 
                        0,
                        inFadeTargetAnim->spriteAnim[i].yOscPerSec,
                        inFadeTargetAnim->spriteAnim[i].yAmp,
                        inFadeTargetAnim->spriteAnim[i].yPhase );

                 rock += 
                     targetWeight *
                     getOscOffset( 
                         0,
                         inFadeTargetAnim->spriteAnim[i].rockOscPerSec,
                         inFadeTargetAnim->spriteAnim[i].rockAmp,
                         inFadeTargetAnim->spriteAnim[i].rockPhase );
                }
            

            rot = inAnim->spriteAnim[i].rotPerSec * inRotFrameTime + 
                inAnim->spriteAnim[i].rotPhase;

            if( inAnimFade < 1 ) {
                double targetRot;

                // rotate toward starting pos of fade target
                if( inAnim->spriteAnim[i].rotPerSec > 0 
                    ||
                    ( inAnim->spriteAnim[i].rotPerSec == 0 &&
                      inFadeTargetAnim->spriteAnim[i].rotPerSec > 0 ) ) {
                    
                    // one or other has positive rotation, keep going
                    // in that direction to come back to 0 point of target
                    
                    targetRot = floor( rot ) + 
                        inFadeTargetAnim->spriteAnim[i].rotPhase;
                    
                    while( targetRot < rot ) {
                        // behind us, push ahead one more rotation CW
                        targetRot += 1;
                        }
                    }
                else if( inAnim->spriteAnim[i].rotPerSec == 0 &&
                         inFadeTargetAnim->spriteAnim[i].rotPerSec == 0 ) {
                    // both are not rotating

                    // blend between phases
                    targetRot = inFadeTargetAnim->spriteAnim[i].rotPhase;
                    }
                else {
                    // one has negative rotation, continue around
                    // to zero point of target

                    targetRot = ceil( rot ) + 
                        inFadeTargetAnim->spriteAnim[i].rotPhase;
                    
                    while( targetRot > rot ) {
                        // behind us, push ahead one more rotation CCW
                        targetRot -= 1;
                        }
                    }
                
                if( rot != targetRot ) {
                    rot = inAnimFade * rot + (1 - inAnimFade) * targetRot;
                    }
                }

            rot += rock;
            }
        
        if( inFlipH ) {
            pos.x *= -1;
            rot *= -1;
            }
        pos = add( pos, inPos );
        
        drawSprite( getSprite( obj->sprites[i] ), pos, 1.0, rot, inFlipH );
        } 
    }



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade, 
                     AnimType inFadeTargetType,
                     doublePair inPos,
                     char inFlipH,
                     double inAge,
                     int inNumContained, int *inContainedIDs ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, inFlipH, inAge,
                    inNumContained, inContainedIDs );
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos, inFlipH, inAge,
                        inNumContained, inContainedIDs );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim,
                     double inFrameTime, double inRotFrameTime, 
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     doublePair inPos,
                     char inFlipH,
                     double inAge,
                     int inNumContained, int *inContainedIDs ) {
    
    // first, draw jiggling (never rotating) objects in slots
    // can't safely rotate them, because they may be compound objects
    
    ObjectRecord *obj = getObject( inObjectID );

    for( int i=0; i<obj->numSlots; i++ ) {
        if( i < inNumContained ) {

            doublePair pos = obj->slotPos[i];
        
            if( i < inAnim->numSlots ) {
                
                pos.x += 
                    inAnimFade *
                    getOscOffset( 
                        inFrameTime,
                        inAnim->slotAnim[i].xOscPerSec,
                        inAnim->slotAnim[i].xAmp,
                        inAnim->slotAnim[i].xPhase );
                
                pos.y += 
                    inAnimFade * 
                    getOscOffset( 
                        inFrameTime,
                        inAnim->slotAnim[i].yOscPerSec,
                        inAnim->slotAnim[i].yAmp,
                        inAnim->slotAnim[i].yPhase );

                if( inAnimFade < 1 ) {
                    double targetWeight = 1 - inAnimFade;
                
                    pos.x += 
                        targetWeight *
                        getOscOffset( 
                            0,
                            inFadeTargetAnim->slotAnim[i].xOscPerSec,
                            inFadeTargetAnim->slotAnim[i].xAmp,
                            inFadeTargetAnim->slotAnim[i].xPhase );
                    
                    pos.y += 
                        targetWeight *
                        getOscOffset( 
                            0,
                            inFadeTargetAnim->slotAnim[i].yOscPerSec,
                            inFadeTargetAnim->slotAnim[i].yAmp,
                            inFadeTargetAnim->slotAnim[i].yPhase );
                    }
                }
                  
            if( inFlipH ) {
                pos.x *= -1;
                }
  
            pos = add( pos, inPos );
            drawObject( getObject( inContainedIDs[i] ), pos, inFlipH,
                        inAge );
            }
        
        } 

    // draw animating object on top of contained slots
    drawObjectAnim( inObjectID, inAnim, inFrameTime, inRotFrameTime,
                    inAnimFade, inFadeTargetAnim, inPos, inFlipH,
                    inAge );
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
    }

        

