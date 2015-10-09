#include "animationBank.h"

#include "objectBank.h"
#include "spriteBank.h"

#include "stdlib.h"
#include "math.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/io/file/File.h"

#include "minorGems/game/gameGraphics.h"



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
                                        "%lf %lf %lf %lf %lf %lf %lf %lf",
                                        &( r->spriteAnim[j].xOscPerSec ),
                                        &( r->spriteAnim[j].xAmp ),
                                        &( r->spriteAnim[j].xPhase ),
                                        
                                        &( r->spriteAnim[j].yOscPerSec ),
                                        &( r->spriteAnim[j].yAmp ),
                                        &( r->spriteAnim[j].yPhase ),
                                        
                                        &( r->spriteAnim[j].rotPerSec ),
                                        &( r->spriteAnim[j].rotPhase ) );
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
                                        &( r->slotAnim[j].yPhase ),
                                        
                                        &( r->slotAnim[j].rotPerSec ),
                                        &( r->slotAnim[j].rotPhase ) );
                                next++;
                                }
                            

                            records.push_back( r );
                            }
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
                    "%lf %lf %lf %lf %lf %lf %lf %lf",
                    inRecord->spriteAnim[j].xOscPerSec,
                    inRecord->spriteAnim[j].xAmp,
                    inRecord->spriteAnim[j].xPhase,
                    
                    inRecord->spriteAnim[j].yOscPerSec,
                    inRecord->spriteAnim[j].yAmp,
                    inRecord->spriteAnim[j].yPhase,
                    
                    inRecord->spriteAnim[j].rotPerSec,
                    inRecord->spriteAnim[j].rotPhase ) );
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
                    
                    inRecord->slotAnim[j].rotPerSec,
                    inRecord->slotAnim[j].rotPhase ) );
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



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimType inFadeTargetType,
                     doublePair inPos ) {
    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos );
        return;
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim, 
                     double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     doublePair inPos ) {

    ObjectRecord *obj = getObject( inObjectID );

    for( int i=0; i<obj->numSprites; i++ ) {
        doublePair pos = add( obj->spritePos[i], inPos );
        
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
                }
            

            rot = inAnim->spriteAnim[i].rotPerSec * inRotFrameTime + 
                inAnim->spriteAnim[i].rotPhase;

            if( inAnimFade < 1 ) {
                double targetRot;

                // rotate toward starting pos of fade target
                if( inAnim->spriteAnim[i].rotPerSec > 0 
                    ||
                    ( inAnim->spriteAnim[i].rotPerSec == 0 &&
                      inFadeTargetAnim->spriteAnim[i].rotPerSec > 0 ) 
                    ||
                    ( inAnim->spriteAnim[i].rotPerSec == 0 &&
                      inFadeTargetAnim->spriteAnim[i].rotPerSec == 0 ) ) {
                    
                    targetRot = floor( rot ) + 
                        inFadeTargetAnim->spriteAnim[i].rotPhase;
                    
                    while( targetRot < rot ) {
                        // behind us, push ahead one more rotation CW
                        targetRot += 1;
                        }
                    }
                else {
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
            }
        
        drawSprite( getSprite( obj->sprites[i] ), pos, 1.0, rot );
        } 
    }



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade, 
                     AnimType inFadeTargetType,
                     doublePair inPos,
                     int inNumContained, int *inContainedIDs ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos,
                    inNumContained, inContainedIDs );
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos,
                        inNumContained, inContainedIDs );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim,
                     double inFrameTime, double inRotFrameTime, 
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     doublePair inPos,
                     int inNumContained, int *inContainedIDs ) {
    
    // first, draw jiggling (never rotating) objects in slots
    // can't safely rotate them, because they may be compound objects
    
    ObjectRecord *obj = getObject( inObjectID );

    for( int i=0; i<obj->numSlots; i++ ) {
        if( i < inNumContained ) {

            doublePair pos = add( obj->slotPos[i], inPos );
        
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
                    
            drawObject( getObject( inContainedIDs[i] ), pos );
            }
        
        } 

    // draw animating object on top of contained slots
    drawObjectAnim( inObjectID, inAnim, inFrameTime, inRotFrameTime,
                    inAnimFade, inFadeTargetAnim, inPos );
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
    }

        

