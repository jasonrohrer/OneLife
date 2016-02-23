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



static ClothingSet emptyClothing = getEmptyClothingSet();



static int numFiles;
static File **childFiles;

static int currentFile;


static SimpleVector<AnimationRecord*> records;
static int maxID;


void initAnimationBankStart() {

    maxID = 0;

    numFiles = 0;
    currentFile = 0;
    
    File animationsDir( NULL, "animations" );
    if( animationsDir.exists() && animationsDir.isDirectory() ) {

        childFiles = animationsDir.getChildFiles( &numFiles );
        }
    }



float initAnimationBankStep() {
        
    if( currentFile == numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

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


    currentFile ++;
    return (float)( currentFile ) / (float)( numFiles );
    }



void initAnimationBankFinish() {
    
    delete [] childFiles;
    
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




void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimType inFadeTargetType,
                     doublePair inPos,
                     char inFlipH,
                     double inAge, 
                     ClothingSet inClothing ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
    

    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, inFlipH, inAge, 
                    inClothing );
        return;
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos, inFlipH, inAge, inClothing );
        }
    }



void drawObjectAnim( int inObjectID, AnimationRecord *inAnim, 
                     double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     doublePair inPos,
                     char inFlipH,
                     double inAge, 
                     ClothingSet inClothing ) {

    ObjectRecord *obj = getObject( inObjectID );


    // don't count aging layers here
    // thus we can determine body parts (feet, body, head) for clothing
    // and aging without letting age-ranged add-on layers interfere
    int bodyIndex = 0;

    doublePair headPos = obj->spritePos[ obj->headIndex ];
    
    doublePair animHeadPos = headPos;


    // when tunic should be drawn, behind shoe
    doublePair animBodyPos = { 0, 0 };

    
    for( int i=0; i<obj->numSprites; i++ ) {
        
        char agingLayer = false;

        if( obj->person &&
            ( obj->spriteAgeStart[i] != -1 ||
              obj->spriteAgeEnd[i] != -1 ) ) {
            
            agingLayer = true;
            
            if( inAge < obj->spriteAgeStart[i] ||
                inAge > obj->spriteAgeEnd[i] ) {
                
                // skip drawing this aging layer entirely
                continue;
                }
            }


        doublePair spritePos = obj->spritePos[i];
        
        if( obj->person && 
            ( i == obj->headIndex ||
              obj->spriteAgesWithHead[i] ) ) {
            spritePos = add( spritePos, getAgeHeadOffset( inAge, headPos ) );
            }

        double rot = obj->spriteRot[i];
        
        if( i < inAnim->numSprites ) {
            

            spritePos.x += 
                inAnimFade * 
                getOscOffset( 
                    inFrameTime,
                    inAnim->spriteAnim[i].xOscPerSec,
                    inAnim->spriteAnim[i].xAmp,
                    inAnim->spriteAnim[i].xPhase );
            
            spritePos.y += 
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
                
                spritePos.x += 
                    targetWeight *
                    getOscOffset( 
                        0,
                        inFadeTargetAnim->spriteAnim[i].xOscPerSec,
                        inFadeTargetAnim->spriteAnim[i].xAmp,
                        inFadeTargetAnim->spriteAnim[i].xPhase );
                
                spritePos.y += 
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
            

            rot += inAnim->spriteAnim[i].rotPerSec * inRotFrameTime + 
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
                        obj->spriteRot[i] +
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
                    targetRot = 
                        obj->spriteRot[i] +
                        inFadeTargetAnim->spriteAnim[i].rotPhase;
                    }
                else {
                    // one has negative rotation, continue around
                    // to zero point of target

                    targetRot = ceil( rot ) +
                        obj->spriteRot[i] +
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
        
        if( i == obj->headIndex ) {
            // this is the head
            animHeadPos = spritePos;
            }

        if( inFlipH ) {
            spritePos.x *= -1;
            rot *= -1;
            }
        
        doublePair pos = add( spritePos, inPos );


        char skipSprite = false;
        
        
        if( !agingLayer && ( ( bodyIndex == 0 && !inFlipH ) ||
              ( bodyIndex == 2 && inFlipH ) ) 
            && inClothing.backShoe != NULL ) {
            
            skipSprite = true;
            doublePair cPos = add( spritePos, 
                                   inClothing.backShoe->clothingOffset );
            if( inFlipH ) {
                cPos.x *= -1;
                }
            cPos = add( cPos, inPos );
            
            drawObject( inClothing.backShoe, cPos,
                        inFlipH, -1, emptyClothing );
            }
        else if( !agingLayer && bodyIndex == 1 && inClothing.tunic != NULL ) {
            skipSprite = true;

            
            animBodyPos = spritePos;
            // wait to draw tunic until we're about to draw front shoe
            // (so it's drawn behind all aging layers on body)
            }
        else if( !agingLayer && ( ( bodyIndex == 2 && !inFlipH ) ||
                                  ( bodyIndex == 0 && inFlipH ) ) ) {

            if( inClothing.tunic != NULL ) {
                
                // first, draw tunic behind shoe

                doublePair cPos = add( animBodyPos, 
                                       inClothing.tunic->clothingOffset );
                if( inFlipH ) {
                    cPos.x *= -1;
                    }
                cPos = add( cPos, inPos );
                
                drawObject( inClothing.tunic, cPos,
                            inFlipH, -1, emptyClothing );
                }
            
            if( inClothing.frontShoe != NULL ) {

                skipSprite = true;
                doublePair cPos = add( spritePos, 
                                       inClothing.frontShoe->clothingOffset );
                if( inFlipH ) {
                    cPos.x *= -1;
                    }
                cPos = add( cPos, inPos );
                
                drawObject( inClothing.frontShoe, cPos,
                            inFlipH, -1, emptyClothing );
                }
            }
        


        if( !skipSprite ) {
            setDrawColor( obj->spriteColor[i] );
            drawSprite( getSprite( obj->sprites[i] ), pos, 1.0, rot, 
                        logicalXOR( inFlipH, obj->spriteHFlip[i] ) );
            }
        

        if( obj->spriteAgeStart[i] == -1 &&
            obj->spriteAgeEnd[i] == -1 ) {
        
            bodyIndex++;
            }
        } 


    if( inClothing.hat != NULL ) {

        // hat on top of everything
            
        // relative to head

        doublePair cPos = add( animHeadPos, 
                               inClothing.hat->clothingOffset );
        if( inFlipH ) {
            cPos.x *= -1;
            }
        cPos = add( cPos, inPos );
        
        drawObject( inClothing.hat, cPos,
                    inFlipH, -1, emptyClothing );
        }
    }



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade, 
                     AnimType inFadeTargetType,
                     doublePair inPos,
                     char inFlipH,
                     double inAge,
                     ClothingSet inClothing,
                     int inNumContained, int *inContainedIDs ) {
    
    AnimationRecord *r = getAnimation( inObjectID, inType );
 
    if( r == NULL ) {
        drawObject( getObject( inObjectID ), inPos, inFlipH, inAge, inClothing,
                    inNumContained, inContainedIDs );
        }
    else {
        AnimationRecord *rB = getAnimation( inObjectID, inFadeTargetType );
        
        drawObjectAnim( inObjectID, r, inFrameTime, inRotFrameTime,
                        inAnimFade, rB, inPos, inFlipH, inAge, inClothing,
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
                     ClothingSet inClothing,
                     int inNumContained, int *inContainedIDs ) {
    
    ClothingSet emptyClothing = getEmptyClothingSet();
    
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
                        inAge, emptyClothing );
            }
        
        } 

    // draw animating object on top of contained slots
    drawObjectAnim( inObjectID, inAnim, inFrameTime, inRotFrameTime,
                    inAnimFade, inFadeTargetAnim, inPos, inFlipH,
                    inAge, inClothing );
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

        

