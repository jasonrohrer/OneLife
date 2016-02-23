
#include "objectBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"


#include "spriteBank.h"

#include "ageControl.h"


static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static ObjectRecord **idMap;


static StringTree tree;


// track objects that are marked with the person flag
static SimpleVector<int> personObjectIDs;


static JenkinsRandomSource randSource;


static ClothingSet emptyClothing = getEmptyClothingSet();



static int numFiles;
static File **childFiles;

static int currentFile;


static SimpleVector<ObjectRecord*> records;
static int maxID;




void setDrawColor( FloatRGB inColor ) {
    setDrawColor( inColor.r, 
                  inColor.g, 
                  inColor.b,
                  1 );
    }




void initObjectBankStart() {
    maxID = 0;

    numFiles = 0;
    currentFile = 0;
    
    File objectsDir( NULL, "objects" );
    if( objectsDir.exists() && objectsDir.isDirectory() ) {

        childFiles = objectsDir.getChildFiles( &numFiles );
        }
    }



float initObjectBankStep() {
        
    if( currentFile == numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

                
    if( ! childFiles[i]->isDirectory() ) {
                                        
        char *txtFileName = childFiles[i]->getFileName();
                        
        if( strstr( txtFileName, ".txt" ) != NULL &&
            strcmp( txtFileName, "nextObjectNumber.txt" ) != 0 ) {
                            
            // an object txt file!

            char *fullName = childFiles[i]->getFullFileName();
                            
            printf( "Loading object from path %s\n", fullName );
                            
            delete [] fullName;
                    
            char *objectText = childFiles[i]->readFileContents();
                    
        
            if( objectText != NULL ) {
                int numLines;
                        
                char **lines = split( objectText, "\n", &numLines );
                        
                delete [] objectText;

                if( numLines >= 14 ) {
                    ObjectRecord *r = new ObjectRecord;
                            
                    int next = 0;
                            
                    r->id = 0;
                    sscanf( lines[next], "id=%d", 
                            &( r->id ) );
                            
                    if( r->id > maxID ) {
                        maxID = r->id;
                        }
                            
                    next++;
                            
                    r->description = stringDuplicate( lines[next] );
                            
                    next++;
                            
                    int contRead = 0;                            
                    sscanf( lines[next], "containable=%d", 
                            &( contRead ) );
                            
                    r->containable = contRead;
                            
                    next++;
                    
                    r->containSize = 1;
                    sscanf( lines[next], "containSize=%d", 
                            &( r->containSize ) );
                            
                    next++;
                            
                    int permRead = 0;                            
                    sscanf( lines[next], "permanent=%d", 
                            &( permRead ) );
                            
                    r->permanent = permRead;

                    next++;


                    r->mapChance = 0;      
                    sscanf( lines[next], "mapChance=%f", 
                            &( r->mapChance ) );
                            
                    next++;


                    r->heatValue = 0;                            
                    sscanf( lines[next], "heatValue=%d", 
                            &( r->heatValue ) );
                            
                    next++;

                            

                    r->rValue = 0;                            
                    sscanf( lines[next], "rValue=%f", 
                            &( r->rValue ) );
                            
                    next++;



                    int personRead = 0;                            
                    sscanf( lines[next], "person=%d", 
                            &( personRead ) );
                            
                    r->person = personRead;
                            
                    next++;


                            
                    sscanf( lines[next], "foodValue=%d", 
                            &( r->foodValue ) );
                            
                    next++;
                            
                            
                            
                    sscanf( lines[next], "speedMult=%f", 
                            &( r->speedMult ) );
                            
                    next++;



                    r->heldOffset.x = 0;
                    r->heldOffset.y = 0;
                            
                    sscanf( lines[next], "heldOffset=%lf,%lf", 
                            &( r->heldOffset.x ),
                            &( r->heldOffset.y ) );
                            
                    next++;



                    r->clothing = 'n';
                            
                    sscanf( lines[next], "clothing=%c", 
                            &( r->clothing ));
                            
                    next++;
                            
                            
                            
                    r->clothingOffset.x = 0;
                    r->clothingOffset.y = 0;
                            
                    sscanf( lines[next], "clothingOffset=%lf,%lf", 
                            &( r->clothingOffset.x ),
                            &( r->clothingOffset.y ) );
                            
                    next++;
                            
                    
                    r->deadlyDistance = 0;
                    sscanf( lines[next], "deadlyDistance=%d", 
                            &( r->deadlyDistance ) );
                            
                    next++;


                    r->numSlots = 0;
                    sscanf( lines[next], "numSlots=%d", 
                            &( r->numSlots ) );
                            
                    next++;

                    r->slotSize = 1;
                    sscanf( lines[next], "slotSize=%d", 
                            &( r->slotSize ) );
                            
                    next++;

                    r->slotPos = new doublePair[ r->numSlots ];
                            
                    for( int i=0; i< r->numSlots; i++ ) {
                        sscanf( lines[ next ], "slotPos=%lf,%lf", 
                                &( r->slotPos[i].x ),
                                &( r->slotPos[i].y ) );
                        next++;
                        }
                            

                    r->numSprites = 0;
                    sscanf( lines[next], "numSprites=%d", 
                            &( r->numSprites ) );
                            
                    next++;

                    r->sprites = new int[r->numSprites];
                    r->spritePos = new doublePair[ r->numSprites ];
                    r->spriteRot = new double[ r->numSprites ];
                    r->spriteHFlip = new char[ r->numSprites ];
                    r->spriteColor = new FloatRGB[ r->numSprites ];
                    
                    r->spriteAgeStart = new double[ r->numSprites ];
                    r->spriteAgeEnd = new double[ r->numSprites ];

                    r->spriteAgesWithHead = new char[ r->numSprites ];

                    r->numNonAgingSprites = 0;
                    r->headIndex = 0;
                    
                    for( int i=0; i< r->numSprites; i++ ) {
                        sscanf( lines[next], "spriteID=%d", 
                                &( r->sprites[i] ) );
                                
                        next++;
                                
                        sscanf( lines[next], "pos=%lf,%lf", 
                                &( r->spritePos[i].x ),
                                &( r->spritePos[i].y ) );
                                
                        next++;
                                
                        sscanf( lines[next], "rot=%lf", 
                                &( r->spriteRot[i] ) );
                                
                        next++;
                                
                                
                        int flipRead = 0;
                                
                        sscanf( lines[next], "hFlip=%d", &flipRead );
                                
                        r->spriteHFlip[i] = flipRead;
                                
                        next++;


                        sscanf( lines[next], "color=%f,%f,%f", 
                                &( r->spriteColor[i].r ),
                                &( r->spriteColor[i].g ),
                                &( r->spriteColor[i].b ) );
                                
                        next++;


                        sscanf( lines[next], "ageRange=%lf,%lf", 
                                &( r->spriteAgeStart[i] ),
                                &( r->spriteAgeEnd[i] ) );
                                
                        next++;
                        

                        if( r->spriteAgeStart[i] == -1  &&
                            r->spriteAgeEnd[i] == -1 ) {
                            
                            r->numNonAgingSprites ++;
                            r->headIndex = i;
                            }

                        int agesWithHeadRead = 0;
                                
                        sscanf( lines[next], "agesWithHead=%d", 
                                &agesWithHeadRead );
                                
                        r->spriteAgesWithHead[i] = agesWithHeadRead;
                                
                        next++;
                        }
                    records.push_back( r );

                            
                    if( r->person ) {
                        personObjectIDs.push_back( r->id );
                        }
                    }
                            
                for( int i=0; i<numLines; i++ ) {
                    delete [] lines[i];
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
    
    

void initObjectBankFinish() {
    
    delete [] childFiles;
    
    mapSize = maxID + 1;
    
    idMap = new ObjectRecord*[ mapSize ];
    
    for( int i=0; i<mapSize; i++ ) {
        idMap[i] = NULL;
        }

    int numRecords = records.size();
    for( int i=0; i<numRecords; i++ ) {
        ObjectRecord *r = records.getElementDirect(i);
        
        idMap[ r->id ] = r;
        
        char *lowercase = stringToLowerCase( r->description );
        
        tree.insert( lowercase, r );

        delete [] lowercase;
        }

    printf( "Loaded %d objects from objects folder\n", numRecords );

    // resaveAll();
    }




static void freeObjectRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            char *lower = stringToLowerCase( idMap[inID]->description );
            
            tree.remove( lower, idMap[inID] );
            
            delete [] lower;
            

            delete [] idMap[inID]->description;
            delete [] idMap[inID]->slotPos;
            delete [] idMap[inID]->sprites;
            delete [] idMap[inID]->spritePos;
            delete [] idMap[inID]->spriteRot;
            delete [] idMap[inID]->spriteHFlip;
            delete [] idMap[inID]->spriteColor;

            delete [] idMap[inID]->spriteAgeStart;
            delete [] idMap[inID]->spriteAgeEnd;
            delete [] idMap[inID]->spriteAgesWithHead;
            
            delete idMap[inID];
            idMap[inID] = NULL;
            return ;
            }
        }
    
    personObjectIDs.deleteElementEqualTo( inID );
    }




void freeObjectBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            delete [] idMap[i]->slotPos;
            delete [] idMap[i]->description;
            delete [] idMap[i]->sprites;
            delete [] idMap[i]->spritePos;
            delete [] idMap[i]->spriteRot;
            delete [] idMap[i]->spriteHFlip;
            delete [] idMap[i]->spriteColor;

            delete [] idMap[i]->spriteAgeStart;
            delete [] idMap[i]->spriteAgeEnd;
            delete [] idMap[i]->spriteAgesWithHead;

            delete idMap[i];
            }
        }

    delete [] idMap;
    }



void resaveAll() {
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {

            addObject( idMap[i]->description,
                       idMap[i]->containable,
                       idMap[i]->containSize,
                       idMap[i]->permanent,
                       idMap[i]->mapChance,
                       idMap[i]->heatValue,
                       idMap[i]->rValue,
                       idMap[i]->person,
                       idMap[i]->foodValue,
                       idMap[i]->speedMult,
                       idMap[i]->heldOffset,
                       idMap[i]->clothing,
                       idMap[i]->clothingOffset,
                       idMap[i]->deadlyDistance,
                       idMap[i]->numSlots, 
                       idMap[i]->slotSize, 
                       idMap[i]->slotPos,
                       idMap[i]->numSprites, 
                       idMap[i]->sprites, 
                       idMap[i]->spritePos,
                       idMap[i]->spriteRot,
                       idMap[i]->spriteHFlip,
                       idMap[i]->spriteColor,
                       idMap[i]->spriteAgeStart,
                       idMap[i]->spriteAgeEnd,
                       idMap[i]->spriteAgesWithHead,
                       idMap[i]->id );
            }
        }
    }





ObjectRecord *getObject( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID];
            }
        }
    return NULL;
    }



int getNumContainerSlots( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return 0;
        }
    else {
        return r->numSlots;
        }
    }



char isContainable( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return false;
        }
    else {
        return r->containable;
        }
    }

    


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    char *lowerSearch = stringToLowerCase( inSearch );

    int numTotalMatches = tree.countMatches( lowerSearch );
        
    
    int numAfterSkip = numTotalMatches - inNumToSkip;
    
    int numToGet = inNumToGet;
    if( numToGet > numAfterSkip ) {
        numToGet = numAfterSkip;
        }
    
    *outNumRemaining = numAfterSkip - numToGet;
        
    ObjectRecord **results = new ObjectRecord*[ numToGet ];
    
    
    *outNumResults = 
        tree.getMatches( lowerSearch, inNumToSkip, numToGet, (void**)results );
    
    delete [] lowerSearch;

    return results;
    }



int addObject( const char *inDescription,
               char inContainable,
               int inContainSize,
               char inPermanent,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               double *inSpriteRot,
               char *inSpriteHFlip,
               FloatRGB *inSpriteColor,
               double *inSpriteAgeStart,
               double *inSpriteAgeEnd,
               char *inSpriteAgesWithHead,
               int inReplaceID ) {
    

    
    int newID = inReplaceID;


    // add it to file structure
    File objectsDir( NULL, "objects" );
            
    if( !objectsDir.exists() ) {
        objectsDir.makeDirectory();
        }
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {
                
                
        int nextObjectNumber = 1;
                
        File *nextNumberFile = 
            objectsDir.getChildFile( "nextObjectNumber.txt" );
                
        if( nextNumberFile->exists() ) {
                    
            char *nextNumberString = 
                nextNumberFile->readFileContents();

            if( nextNumberString != NULL ) {
                sscanf( nextNumberString, "%d", &nextObjectNumber );
                
                delete [] nextNumberString;
                }
            }
        
        if( newID == -1 ) {
            newID = nextObjectNumber;
            }
        
        char *fileName = autoSprintf( "%d.txt", newID );


        File *objectFile = objectsDir.getChildFile( fileName );
        

        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "id=%d", newID ) );
        lines.push_back( stringDuplicate( inDescription ) );

        lines.push_back( autoSprintf( "containable=%d", (int)inContainable ) );
        lines.push_back( autoSprintf( "containSize=%d", (int)inContainSize ) );
        lines.push_back( autoSprintf( "permanent=%d", (int)inPermanent ) );
        
        lines.push_back( autoSprintf( "mapChance=%f", inMapChance ) );
        lines.push_back( autoSprintf( "heatValue=%d", inHeatValue ) );
        lines.push_back( autoSprintf( "rValue=%f", inRValue ) );

        lines.push_back( autoSprintf( "person=%d", (int)inPerson ) );

        lines.push_back( autoSprintf( "foodValue=%d", inFoodValue ) );
        
        lines.push_back( autoSprintf( "speedMult=%f", inSpeedMult ) );

        lines.push_back( autoSprintf( "heldOffset=%f,%f",
                                      inHeldOffset.x, inHeldOffset.y ) );

        lines.push_back( autoSprintf( "clothing=%c", inClothing ) );

        lines.push_back( autoSprintf( "clothingOffset=%f,%f",
                                      inClothingOffset.x, 
                                      inClothingOffset.y ) );

        lines.push_back( autoSprintf( "deadlyDistance=%d", 
                                      inDeadlyDistance ) );
        
        
        lines.push_back( autoSprintf( "numSlots=%d", inNumSlots ) );
        lines.push_back( autoSprintf( "slotSize=%d", inSlotSize ) );

        for( int i=0; i<inNumSlots; i++ ) {
            lines.push_back( autoSprintf( "slotPos=%f,%f", 
                                          inSlotPos[i].x,
                                          inSlotPos[i].y ) );
            }

        
        lines.push_back( autoSprintf( "numSprites=%d", inNumSprites ) );

        for( int i=0; i<inNumSprites; i++ ) {
            lines.push_back( autoSprintf( "spriteID=%d", inSprites[i] ) );
            lines.push_back( autoSprintf( "pos=%f,%f", 
                                          inSpritePos[i].x,
                                          inSpritePos[i].y ) );
            lines.push_back( autoSprintf( "rot=%f", 
                                          inSpriteRot[i] ) );
            lines.push_back( autoSprintf( "hFlip=%d", 
                                          inSpriteHFlip[i] ) );
            lines.push_back( autoSprintf( "color=%f,%f,%f", 
                                          inSpriteColor[i].r,
                                          inSpriteColor[i].g,
                                          inSpriteColor[i].b ) );

            lines.push_back( autoSprintf( "ageRange=%f,%f", 
                                          inSpriteAgeStart[i],
                                          inSpriteAgeEnd[i] ) );

            lines.push_back( autoSprintf( "agesWithHead=%d", 
                                          inSpriteAgesWithHead[i] ) );
            }
        
        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );

        delete [] linesArray;
        lines.deallocateStringElements();
        

        objectFile->writeToFile( contents );
        
        delete [] contents;
        
            
        delete [] fileName;
        delete objectFile;
        
        if( inReplaceID == -1 ) {
            nextObjectNumber++;
            }

        
        char *nextNumberString = autoSprintf( "%d", nextObjectNumber );
        
        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        }
    
    if( newID == -1 ) {
        // failed to save it to disk
        return -1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        

        
        ObjectRecord **newMap = new ObjectRecord*[newMapSize];
        
        for( int i=0; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(ObjectRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }

    ObjectRecord *r = new ObjectRecord;
    
    r->id = newID;
    r->description = stringDuplicate( inDescription );

    r->containable = inContainable;
    r->containSize = inContainSize;
    r->permanent = inPermanent;
    
    r->mapChance = inMapChance;
    
    r->heatValue = inHeatValue;
    r->rValue = inRValue;

    r->person = inPerson;
    r->foodValue = inFoodValue;
    r->speedMult = inSpeedMult;
    r->heldOffset = inHeldOffset;
    r->clothing = inClothing;
    r->clothingOffset = inClothingOffset;
    r->deadlyDistance = inDeadlyDistance;

    r->numSlots = inNumSlots;
    r->slotSize = inSlotSize;
    
    r->slotPos = new doublePair[ inNumSlots ];
    
    memcpy( r->slotPos, inSlotPos, inNumSlots * sizeof( doublePair ) );
    

    r->numSprites = inNumSprites;
    
    r->sprites = new int[ inNumSprites ];
    r->spritePos = new doublePair[ inNumSprites ];
    r->spriteRot = new double[ inNumSprites ];
    r->spriteHFlip = new char[ inNumSprites ];
    r->spriteColor = new FloatRGB[ inNumSprites ];

    r->spriteAgeStart = new double[ inNumSprites ];
    r->spriteAgeEnd = new double[ inNumSprites ];
    r->spriteAgesWithHead = new char[ inNumSprites ];


    memcpy( r->sprites, inSprites, inNumSprites * sizeof( int ) );
    memcpy( r->spritePos, inSpritePos, inNumSprites * sizeof( doublePair ) );
    memcpy( r->spriteRot, inSpriteRot, inNumSprites * sizeof( double ) );
    memcpy( r->spriteHFlip, inSpriteHFlip, inNumSprites * sizeof( char ) );
    memcpy( r->spriteColor, inSpriteColor, inNumSprites * sizeof( FloatRGB ) );

    memcpy( r->spriteAgeStart, inSpriteAgeStart, 
            inNumSprites * sizeof( double ) );

    memcpy( r->spriteAgeEnd, inSpriteAgeEnd, 
            inNumSprites * sizeof( double ) );

    memcpy( r->spriteAgesWithHead, inSpriteAgesWithHead, 
            inNumSprites * sizeof( char ) );
    
    r->numNonAgingSprites = 0;
    r->headIndex = 0;
    
    for( int i=0; i<inNumSprites; i++ ) {

        if( r->spriteAgeStart[i] == -1  &&
            r->spriteAgeEnd[i] == -1 ) {
            
            r->numNonAgingSprites ++;
            r->headIndex = i;
            }
        }
    



    // delete old

    // grab this before freeing, in case inDescription is the same as
    // idMap[newID].description
    char *lower = stringToLowerCase( inDescription );
    
    freeObjectRecord( newID );
    
    idMap[newID] = r;
    
    tree.insert( lower, idMap[newID] );

    delete [] lower;
    
    if( r->person ) {    
        personObjectIDs.push_back( newID );
        }
    
    return newID;
    }


static char logicalXOR( char inA, char inB ) {
    return !inA != !inB;
    }



void drawObject( ObjectRecord *inObject, doublePair inPos,
                 char inFlipH, double inAge, ClothingSet inClothing,
                 double inScale ) {
    
    // don't count aging layers here
    // thus we can determine body parts (feet, body, head) for clothing
    // and aging without letting age-ranged add-on layers interfere
    int bodyIndex = 0;

    
    doublePair headPos = inObject->spritePos[ inObject->headIndex ];
    
    doublePair animHeadPos = headPos;

    // when tunic should be drawn, behind shoe
    doublePair animBodyPos = { 0, 0 };
    

    for( int i=0; i<inObject->numSprites; i++ ) {
        
        char agingLayer = false;
        
        if( inObject->person &&
            ( inObject->spriteAgeStart[i] != -1 ||
              inObject->spriteAgeEnd[i] != -1 ) ) {
            
            agingLayer = true;
            
            if( inAge < inObject->spriteAgeStart[i] ||
                inAge > inObject->spriteAgeEnd[i] ) {
                
                // skip drawing this aging layer entirely
                continue;
                }
            }


        doublePair spritePos = inObject->spritePos[i];


        
        if( inObject->person && 
            ( i == inObject->headIndex ||
              inObject->spriteAgesWithHead[i] ) ) {
            
            spritePos = add( spritePos, getAgeHeadOffset( inAge, headPos ) );
            }

        if( i == inObject->headIndex ) {
            // this is the head
            animHeadPos = spritePos;
            }
        
        if( inFlipH ) {
            spritePos.x *= -1;
            }

        spritePos = mult( spritePos, inScale );

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
        
        
        if( ! skipSprite ) {
            setDrawColor( inObject->spriteColor[i] );

            drawSprite( getSprite( inObject->sprites[i] ), pos, inScale,
                        inObject->spriteRot[i], 
                        logicalXOR( inFlipH, inObject->spriteHFlip[i] ) );
            
            }
            
        if( inObject->spriteAgeStart[i] == -1 &&
            inObject->spriteAgeEnd[i] == -1 ) {
        
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



void drawObject( ObjectRecord *inObject, doublePair inPos,
                 char inFlipH, double inAge,
                 ClothingSet inClothing,
                 int inNumContained, int *inContainedIDs ) {

    int numSlots = getNumContainerSlots( inObject->id );
    
    if( inNumContained > numSlots ) {
        inNumContained = numSlots;
        }
    
    for( int i=0; i<inNumContained; i++ ) {
        doublePair slotPos = inObject->slotPos[i];

        if( inFlipH ) {
            slotPos.x *= -1;
            }

        doublePair pos = add( slotPos, inPos );
        drawObject( getObject( inContainedIDs[i] ), pos, inFlipH, inAge,
                    emptyClothing );
        }
    
    drawObject( inObject, inPos, inFlipH, inAge, inClothing );
    }





void deleteObjectFromBank( int inID ) {
    
    File objectsDir( NULL, "objects" );
    
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {                
                    
        char *fileName = autoSprintf( "%d.txt", inID );
        
        File *objectFile = objectsDir.getChildFile( fileName );
            
        objectFile->remove();
        
        delete [] fileName;
        delete objectFile;
        }

    freeObjectRecord( inID );
    }



char isSpriteUsed( int inSpriteID ) {


    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            for( int s=0; s<idMap[i]->numSprites; s++ ) {
                if( idMap[i]->sprites[s] == inSpriteID ) {
                    return true;
                    }
                }
            }
        }
    return false;
    }




int getRandomPersonObject() {
    
    if( personObjectIDs.size() == 0 ) {
        return -1;
        }
    
        
    return personObjectIDs.getElementDirect( 
        randSource.getRandomBoundedInt( 0, 
                                        personObjectIDs.size() - 1  ) );
    }




ObjectRecord **getAllObjects( int *outNumResults ) {
    SimpleVector<ObjectRecord *> records;
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            records.push_back( idMap[i] );
            }
        }
    
    *outNumResults = records.size();
    
    return records.getElementArray();
    }




ClothingSet getEmptyClothingSet() {
    return emptyClothing;
    }





