
#include "objectBank.h"

#include "minorGems/util/StringTree.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/io/file/File.h"

#include "minorGems/graphics/converters/TGAImageConverter.h"


#include "spriteBank.h"

#include "ageControl.h"

#include "folderCache.h"


static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static ObjectRecord **idMap;


static StringTree tree;


// track objects that are marked with the person flag
static SimpleVector<int> personObjectIDs;

// track female people
static SimpleVector<int> femalePersonObjectIDs;


// anything above race 100 is put in bin for race 100
#define MAX_RACE 100

static SimpleVector<int> racePersonObjectIDs[ MAX_RACE + 1 ];

static SimpleVector<int> raceList;


static void rebuildRaceList() {
    raceList.deleteAll();
    
    for( int i=0; i <= MAX_RACE; i++ ) {
        if( racePersonObjectIDs[ i ].size() > 0 ) {
            raceList.push_back( i );

            // now sort into every-other gender order
            int num = racePersonObjectIDs[i].size();

            SimpleVector<int> boys;
            SimpleVector<int> girls;
            
            for( int j=0; j<num; j++ ) {
                
                int id = racePersonObjectIDs[i].getElementDirect( j );
                
                ObjectRecord *o = getObject( id );
                
                if( o->male ) {
                    boys.push_back( id );
                    }
                else {
                    girls.push_back( id );
                    }
                }
            
            racePersonObjectIDs[i].deleteAll();
            
            int boyIndex = 0;
            int girlIndex = 0;
            
            int boysLeft = boys.size();
            int girlsLeft = girls.size();

            int flip = 0;
            
            for( int j=0; j<num; j++ ) {
                
                if( ( flip && boysLeft > 0 ) 
                    ||
                    girlsLeft == 0 ) {
                    
                    racePersonObjectIDs[i].push_back( 
                        boys.getElementDirect( boyIndex ) );
                    
                    boysLeft--;
                    boyIndex++;
                    }
                else {
                    racePersonObjectIDs[i].push_back( 
                        girls.getElementDirect( girlIndex ) );
                    
                    girlsLeft--;
                    girlIndex++;
                    }
                flip = !flip;
                }
            }
        }
    }




static JenkinsRandomSource randSource;


static ClothingSet emptyClothing = getEmptyClothingSet();



static FolderCache cache;

static int currentFile;


static SimpleVector<ObjectRecord*> records;
static int maxID;



int getMaxObjectID() {
    return maxID;
    }


void setDrawColor( FloatRGB inColor ) {
    setDrawColor( inColor.r, 
                  inColor.g, 
                  inColor.b,
                  1 );
    }




int initObjectBankStart( char *outRebuildingCache ) {
    maxID = 0;

    currentFile = 0;
    

    cache = initFolderCache( "objects", outRebuildingCache );

    return cache.numFiles;
    }




char *boolArrayToSparseCommaString( const char *inLineName,
                                    char *inArray, int inLength ) {
    char numberBuffer[20];
    
    SimpleVector<char> resultBuffer;


    resultBuffer.appendElementString( inLineName );
    resultBuffer.push_back( '=' );

    char firstWritten = false;
    for( int i=0; i<inLength; i++ ) {
        if( inArray[i] ) {
            
            if( firstWritten ) {
                resultBuffer.push_back( ',' );
                }
            
            sprintf( numberBuffer, "%d", i );
            
            resultBuffer.appendElementString( numberBuffer );
            
            firstWritten = true;
            }
        }
    
    if( !firstWritten ) {
        resultBuffer.appendElementString( "-1" );
        }
    
    return resultBuffer.getElementString();
    }



void sparseCommaLineToBoolArray( const char *inExpectedLineName,
                                 char *inLine,
                                 char *inBoolArray,
                                 int inBoolArrayLength ) {

    if( strstr( inLine, inExpectedLineName ) == NULL ) {
        printf( "Expected line name %s not found in line %s\n",
                inExpectedLineName, inLine );
        return;
        }
    

    char *listStart = strstr( inLine, "=" );
    
    if( listStart == NULL ) {
        printf( "Expected character '=' not found in line %s\n",
                inLine );
        return;
        }
    
    listStart = &( listStart[1] );
    
    
    int numParts;
    char **listNumberStrings = split( listStart, ",", &numParts );
    

    for( int i=0; i<numParts; i++ ) {
        
        int scannedInt = -1;
        
        sscanf( listNumberStrings[i], "%d", &scannedInt );

        if( scannedInt >= 0 &&
            scannedInt < inBoolArrayLength ) {
            inBoolArray[ scannedInt ] = true;
            }

        delete [] listNumberStrings[i];
        }
    delete [] listNumberStrings;
    }



static void fillObjectBiomeFromString( ObjectRecord *inRecord, 
                                       char *inBiomes ) {    
    char **biomeParts = split( inBiomes, ",", &( inRecord->numBiomes ) );    
    inRecord->biomes = new int[ inRecord->numBiomes ];
    for( int i=0; i< inRecord->numBiomes; i++ ) {
        sscanf( biomeParts[i], "%d", &( inRecord->biomes[i] ) );
        
        delete [] biomeParts[i];
        }
    delete [] biomeParts;
    }



float initObjectBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

                
    char *txtFileName = getFileName( cache, i );
            
    if( strstr( txtFileName, ".txt" ) != NULL &&
        strcmp( txtFileName, "nextObjectNumber.txt" ) != 0 ) {
                            
        // an object txt file!
                    
        char *objectText = getFileContents( cache, i );
        
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



                int heldInHandRead = 0;                            
                sscanf( lines[next], "heldInHand=%d", 
                        &( heldInHandRead ) );
                          
                r->heldInHand = false;
                r->rideable = false;
                
                if( heldInHandRead == 1 ) {
                    r->heldInHand = true;
                    }
                else if( heldInHandRead == 2 ) {
                    r->rideable = true;
                    }

                next++;


                int blocksWalkingRead = 0;                            
                sscanf( lines[next], "blocksWalking=%d",
                        &( blocksWalkingRead ) );
                            
                r->blocksWalking = blocksWalkingRead;

                next++;


                
                
                r->mapChance = 0;      
                char biomeString[200];
                int numRead = sscanf( lines[next], 
                                      "mapChance=%f#biomes_%199s", 
                                      &( r->mapChance ), biomeString );
                
                if( numRead != 2 ) {
                    // biome not present (old format), treat as 0
                    biomeString[0] = '0';
                    biomeString[1] = '\0';
                    
                    sscanf( lines[next], "mapChance=%f", &( r->mapChance ) );
                
                    // NOTE:  I've avoided too many of these format
                    // bandaids, and forced whole-folder file rewrites 
                    // in the past.
                    // But now we're part way into production, so bandaids
                    // are more effective.
                    }
                
                fillObjectBiomeFromString( r, biomeString );

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
                            
                r->person = ( personRead > 0 );
                
                r->race = personRead;
                
                next++;


                int maleRead = 0;                            
                sscanf( lines[next], "male=%d", 
                        &( maleRead ) );
                    
                r->male = maleRead;
                            
                next++;


                int deathMarkerRead = 0;     
                sscanf( lines[next], "deathMarker=%d", 
                        &( deathMarkerRead ) );
                    
                r->deathMarker = deathMarkerRead;
                            
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
                r->slotTimeStretch = 1.0f;
                
                
                if( strstr( lines[next], "#" ) != NULL ) {
                    sscanf( lines[next], "numSlots=%d#timeStretch=%f", 
                            &( r->numSlots ),
                            &( r->slotTimeStretch ) );
                    printf( "Read from line '%s' strech %f\n",
                            lines[next],
                            r->slotTimeStretch );
                    }
                else {
                    sscanf( lines[next], "numSlots=%d", 
                            &( r->numSlots ) );
                    }
                

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

                r->spriteParent = new int[ r->numSprites ];
                r->spriteInvisibleWhenHolding = new char[ r->numSprites ];



                r->spriteIsHead = new char[ r->numSprites ];
                r->spriteIsBody = new char[ r->numSprites ];
                r->spriteIsBackFoot = new char[ r->numSprites ];
                r->spriteIsFrontFoot = new char[ r->numSprites ];
                

                memset( r->spriteIsHead, false, r->numSprites );
                memset( r->spriteIsBody, false, r->numSprites );
                memset( r->spriteIsBackFoot, false, r->numSprites );
                memset( r->spriteIsFrontFoot, false, r->numSprites );
                
                


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
                        

                    sscanf( lines[next], "parent=%d", 
                            &( r->spriteParent[i] ) );
                        
                    next++;


                    int invisRead = 0;
                                
                    sscanf( lines[next], "invisHolding=%d", &invisRead );
                                
                    r->spriteInvisibleWhenHolding[i] = invisRead;
                                
                    next++;                        
                    }



                sparseCommaLineToBoolArray( "headIndex", lines[next],
                                            r->spriteIsHead, r->numSprites );
                next++;


                sparseCommaLineToBoolArray( "bodyIndex", lines[next],
                                            r->spriteIsBody, r->numSprites );
                next++;


                sparseCommaLineToBoolArray( "backFootIndex", lines[next],
                                            r->spriteIsBackFoot, 
                                            r->numSprites );
                next++;


                sparseCommaLineToBoolArray( "frontFootIndex", lines[next],
                                            r->spriteIsFrontFoot, 
                                            r->numSprites );
                next++;


                records.push_back( r );

                            
                if( r->person ) {
                    personObjectIDs.push_back( r->id );
                    
                    if( ! r->male ) {
                        femalePersonObjectIDs.push_back( r->id );
                        }

                    if( r->race <= MAX_RACE ) {
                        racePersonObjectIDs[ r->race ].push_back( r->id );
                        }
                    else {
                        racePersonObjectIDs[ MAX_RACE ].push_back( r->id );
                        }
                    }
                }
                            
            for( int i=0; i<numLines; i++ ) {
                delete [] lines[i];
                }
            delete [] lines;
            }
        }
                
    delete [] txtFileName;


    currentFile ++;
    return (float)( currentFile ) / (float)( cache.numFiles );
    }
    
    

void initObjectBankFinish() {
  
    freeFolderCache( cache );
    
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
    
                        
    rebuildRaceList();

    printf( "Loaded %d objects from objects folder\n", numRecords );

    // resaveAll();
    }




static void freeObjectRecord( int inID ) {
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            
            char *lower = stringToLowerCase( idMap[inID]->description );
            
            tree.remove( lower, idMap[inID] );
            
            delete [] lower;
            
            int race = idMap[inID]->race;

            delete [] idMap[inID]->description;
            
            delete [] idMap[inID]->biomes;
            
            delete [] idMap[inID]->slotPos;
            delete [] idMap[inID]->sprites;
            delete [] idMap[inID]->spritePos;
            delete [] idMap[inID]->spriteRot;
            delete [] idMap[inID]->spriteHFlip;
            delete [] idMap[inID]->spriteColor;

            delete [] idMap[inID]->spriteAgeStart;
            delete [] idMap[inID]->spriteAgeEnd;
            delete [] idMap[inID]->spriteParent;

            delete [] idMap[inID]->spriteInvisibleWhenHolding;

            delete [] idMap[inID]->spriteIsHead;
            delete [] idMap[inID]->spriteIsBody;
            delete [] idMap[inID]->spriteIsBackFoot;
            delete [] idMap[inID]->spriteIsFrontFoot;
            
            delete idMap[inID];
            idMap[inID] = NULL;

            personObjectIDs.deleteElementEqualTo( inID );
            femalePersonObjectIDs.deleteElementEqualTo( inID );
            
            
            if( race <= MAX_RACE ) {
                racePersonObjectIDs[ race ].deleteElementEqualTo( inID );
                }
            else {
                racePersonObjectIDs[ MAX_RACE ].deleteElementEqualTo( inID );
                }
            
            rebuildRaceList();
            }
        }    
    }




void freeObjectBank() {
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            delete [] idMap[i]->slotPos;
            delete [] idMap[i]->description;
            delete [] idMap[i]->biomes;
            
            delete [] idMap[i]->sprites;
            delete [] idMap[i]->spritePos;
            delete [] idMap[i]->spriteRot;
            delete [] idMap[i]->spriteHFlip;
            delete [] idMap[i]->spriteColor;

            delete [] idMap[i]->spriteAgeStart;
            delete [] idMap[i]->spriteAgeEnd;
            delete [] idMap[i]->spriteParent;

            delete [] idMap[i]->spriteInvisibleWhenHolding;

            delete [] idMap[i]->spriteIsHead;
            delete [] idMap[i]->spriteIsBody;
            delete [] idMap[i]->spriteIsBackFoot;
            delete [] idMap[i]->spriteIsFrontFoot;

            delete idMap[i];
            }
        }

    delete [] idMap;

    personObjectIDs.deleteAll();
    femalePersonObjectIDs.deleteAll();
    
    for( int i=0; i<= MAX_RACE; i++ ) {
        racePersonObjectIDs[i].deleteAll();
        }
    rebuildRaceList();
    }



void resaveAll() {
    printf( "Starting to resave all objects\n..." );
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {

            char *biomeString = getBiomesString( idMap[i] );

            addObject( idMap[i]->description,
                       idMap[i]->containable,
                       idMap[i]->containSize,
                       idMap[i]->permanent,
                       idMap[i]->heldInHand,
                       idMap[i]->rideable,
                       idMap[i]->blocksWalking,
                       biomeString,
                       idMap[i]->mapChance,
                       idMap[i]->heatValue,
                       idMap[i]->rValue,
                       idMap[i]->person,
                       idMap[i]->male,
                       idMap[i]->race,
                       idMap[i]->deathMarker,
                       idMap[i]->foodValue,
                       idMap[i]->speedMult,
                       idMap[i]->heldOffset,
                       idMap[i]->clothing,
                       idMap[i]->clothingOffset,
                       idMap[i]->deadlyDistance,
                       idMap[i]->numSlots, 
                       idMap[i]->slotSize, 
                       idMap[i]->slotPos,
                       idMap[i]->slotTimeStretch,
                       idMap[i]->numSprites, 
                       idMap[i]->sprites, 
                       idMap[i]->spritePos,
                       idMap[i]->spriteRot,
                       idMap[i]->spriteHFlip,
                       idMap[i]->spriteColor,
                       idMap[i]->spriteAgeStart,
                       idMap[i]->spriteAgeEnd,
                       idMap[i]->spriteParent,
                       idMap[i]->spriteInvisibleWhenHolding,
                       idMap[i]->spriteIsHead,
                       idMap[i]->spriteIsBody,
                       idMap[i]->spriteIsBackFoot,
                       idMap[i]->spriteIsFrontFoot,
                       idMap[i]->id );

            delete [] biomeString;
            }
        }
    printf( "...done with resave\n" );
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
               char inHeldInHand,
               char inRideable,
               char inBlocksWalking,
               char *inBiomes,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               char inMale,
               int inRace,
               char inDeathMarker,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
               float inSlotTimeStretch,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               double *inSpriteRot,
               char *inSpriteHFlip,
               FloatRGB *inSpriteColor,
               double *inSpriteAgeStart,
               double *inSpriteAgeEnd,
               int *inSpriteParent,
               char *inSpriteInvisibleWhenHolding,
               char *inSpriteIsHead,
               char *inSpriteIsBody,
               char *inSpriteIsBackFoot,
               char *inSpriteIsFrontFoot,
               int inReplaceID ) {
    
    if( inSlotTimeStretch < 0.0001 ) {
        inSlotTimeStretch = 0.0001;
        }
    
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
        
        
        int heldInHandNumber = 0;
        
        if( inHeldInHand ) {
            heldInHandNumber = 1;
            }
        if( inRideable ) {
            // override
            heldInHandNumber = 2;
            }

        lines.push_back( autoSprintf( "heldInHand=%d", heldInHandNumber ) );
        
        lines.push_back( autoSprintf( "blocksWalking=%d", 
                                      (int)inBlocksWalking ) );
        
        lines.push_back( autoSprintf( "mapChance=%f#biomes_%s", 
                                      inMapChance, inBiomes ) );
        
        lines.push_back( autoSprintf( "heatValue=%d", inHeatValue ) );
        lines.push_back( autoSprintf( "rValue=%f", inRValue ) );

        int personNumber = 0;
        if( inPerson ) {
            personNumber = inRace;
            }
        lines.push_back( autoSprintf( "person=%d", (int)personNumber ) );
        lines.push_back( autoSprintf( "male=%d", (int)inMale ) );
        lines.push_back( autoSprintf( "deathMarker=%d", (int)inDeathMarker ) );

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
        
        
        lines.push_back( autoSprintf( "numSlots=%d#timeStretch=%f", 
                                      inNumSlots, inSlotTimeStretch ) );
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

            lines.push_back( autoSprintf( "parent=%d", 
                                          inSpriteParent[i] ) );


            lines.push_back( autoSprintf( "invisHolding=%d", 
                                          inSpriteInvisibleWhenHolding[i] ) );

            }
        

        // FIXME

        lines.push_back(
            boolArrayToSparseCommaString( "headIndex",
                                          inSpriteIsHead, inNumSprites ) );

        lines.push_back(
            boolArrayToSparseCommaString( "bodyIndex",
                                          inSpriteIsBody, inNumSprites ) );

        lines.push_back(
            boolArrayToSparseCommaString( "backFootIndex",
                                          inSpriteIsBackFoot, inNumSprites ) );

        lines.push_back(
            boolArrayToSparseCommaString( "frontFootIndex",
                                          inSpriteIsFrontFoot, 
                                          inNumSprites ) );
        

        char **linesArray = lines.getElementArray();
        
        
        char *contents = join( linesArray, lines.size(), "\n" );

        delete [] linesArray;
        lines.deallocateStringElements();
        

        File *cacheFile = objectsDir.getChildFile( "cache.fcz" );

        cacheFile->remove();
        
        delete cacheFile;


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
    r->heldInHand = inHeldInHand;
    r->rideable = inRideable;
    
    if( r->heldInHand && r->rideable ) {
        r->heldInHand = false;
        }
    
    r->blocksWalking = inBlocksWalking;
    

    fillObjectBiomeFromString( r, inBiomes );
    
    
    r->mapChance = inMapChance;
    
    r->heatValue = inHeatValue;
    r->rValue = inRValue;

    r->person = inPerson;
    r->race = inRace;
    r->male = inMale;
    r->deathMarker = inDeathMarker;
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
    
    r->slotTimeStretch = inSlotTimeStretch;
    

    r->numSprites = inNumSprites;
    
    r->sprites = new int[ inNumSprites ];
    r->spritePos = new doublePair[ inNumSprites ];
    r->spriteRot = new double[ inNumSprites ];
    r->spriteHFlip = new char[ inNumSprites ];
    r->spriteColor = new FloatRGB[ inNumSprites ];

    r->spriteAgeStart = new double[ inNumSprites ];
    r->spriteAgeEnd = new double[ inNumSprites ];
    
    r->spriteParent = new int[ inNumSprites ];
    r->spriteInvisibleWhenHolding = new char[ inNumSprites ];

    r->spriteIsHead = new char[ inNumSprites ];
    r->spriteIsBody = new char[ inNumSprites ];
    r->spriteIsBackFoot = new char[ inNumSprites ];
    r->spriteIsFrontFoot = new char[ inNumSprites ];


    memcpy( r->sprites, inSprites, inNumSprites * sizeof( int ) );
    memcpy( r->spritePos, inSpritePos, inNumSprites * sizeof( doublePair ) );
    memcpy( r->spriteRot, inSpriteRot, inNumSprites * sizeof( double ) );
    memcpy( r->spriteHFlip, inSpriteHFlip, inNumSprites * sizeof( char ) );
    memcpy( r->spriteColor, inSpriteColor, inNumSprites * sizeof( FloatRGB ) );

    memcpy( r->spriteAgeStart, inSpriteAgeStart, 
            inNumSprites * sizeof( double ) );

    memcpy( r->spriteAgeEnd, inSpriteAgeEnd, 
            inNumSprites * sizeof( double ) );

    memcpy( r->spriteParent, inSpriteParent, 
            inNumSprites * sizeof( int ) );

    memcpy( r->spriteInvisibleWhenHolding, inSpriteInvisibleWhenHolding, 
            inNumSprites * sizeof( char ) );


    memcpy( r->spriteIsHead, inSpriteIsHead, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBody, inSpriteIsBody, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBackFoot, inSpriteIsBackFoot, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsFrontFoot, inSpriteIsFrontFoot, 
            inNumSprites * sizeof( char ) );




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
        
        if( ! r->male ) {
            femalePersonObjectIDs.push_back( newID );
            }
        
        
        if( r->race <= MAX_RACE ) {
            racePersonObjectIDs[ r->race ].push_back( r->id );
            }
        else {
            racePersonObjectIDs[ MAX_RACE ].push_back( r->id );
            }
        rebuildRaceList();
        }
    
    return newID;
    }


static char logicalXOR( char inA, char inB ) {
    return !inA != !inB;
    }



HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos,
                       double inRot, char inFlipH, double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       double inScale ) {
    
    HoldingPos returnHoldingPos = { false, {0, 0}, 0 };
    
    SimpleVector <int> frontArmIndices;
    getFrontArmIndices( inObject, inAge, &frontArmIndices );

    SimpleVector <int> backArmIndices;
    getBackArmIndices( inObject, inAge, &backArmIndices );

    SimpleVector <int> legIndices;
    getAllLegIndices( inObject, inAge, &legIndices );
    
    
    int headIndex = getHeadIndex( inObject, inAge );
    int bodyIndex = getBodyIndex( inObject, inAge );
    int backFootIndex = getBackFootIndex( inObject, inAge );
    int frontFootIndex = getFrontFootIndex( inObject, inAge );

    
    int topBackArmIndex = -1;
    
    if( backArmIndices.size() > 0 ) {
        topBackArmIndex =
            backArmIndices.getElementDirect( backArmIndices.size() - 1 );
        }
    

    int frontHandIndex = getFrontHandIndex( inObject, inAge );
    
    doublePair headPos = inObject->spritePos[ headIndex ];

    doublePair frontFootPos = inObject->spritePos[ frontFootIndex ];

    doublePair bodyPos = inObject->spritePos[ bodyIndex ];

    doublePair animHeadPos = headPos;
    

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
    
    
    for( int i=0; i<inObject->numSprites; i++ ) {
        
        if( inObject->person &&
            ! isSpriteVisibleAtAge( inObject, i, inAge ) ) {    
            // skip drawing this aging layer entirely
            continue;
            }


        doublePair spritePos = inObject->spritePos[i];


        
        if( inObject->person && 
            ( i == headIndex ||
              checkSpriteAncestor( inObject, i,
                                   headIndex ) ) ) {
            
            spritePos = add( spritePos, getAgeHeadOffset( inAge, headPos,
                                                          bodyPos,
                                                          frontFootPos ) );
            }
        if( inObject->person && 
            ( i == headIndex ||
              checkSpriteAncestor( inObject, i,
                                   bodyIndex ) ) ) {
            
            spritePos = add( spritePos, getAgeBodyOffset( inAge, bodyPos ) );
            }

        if( i == headIndex ) {
            // this is the head
            animHeadPos = spritePos;
            }
        
        
        if( inFlipH ) {
            spritePos.x *= -1;            
            }


        if( inRot != 0 ) {    
            spritePos = rotate( spritePos, -2 * M_PI * inRot );
            }


        spritePos = mult( spritePos, inScale );
        
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
        else if( !inHeldNotInPlaceYet &&
                 inHideAllLimbs ) {
            if( frontArmIndices.getElementIndex( i ) != -1 
                ||
                backArmIndices.getElementIndex( i ) != -1
                ||
                legIndices.getElementIndex( i ) != -1 ) {
        
                skipSprite = true;
                }
            }
        



        if( i == backFootIndex 
            && inClothing.backShoe != NULL ) {
                        
            doublePair cPos = add( spritePos, 
                                   inClothing.backShoe->clothingOffset );
            if( inFlipH ) {
                cPos.x *= -1;
                }
            cPos = add( cPos, inPos );
            
            backShoePos = cPos;
            backShoeRot = inRot;
            }
        
        if( i == bodyIndex ) {
            if( inClothing.tunic != NULL ) {
            


                doublePair cPos = add( spritePos, 
                                       inClothing.tunic->clothingOffset );
                if( inFlipH ) {
                    cPos.x *= -1;
                    }
                cPos = add( cPos, inPos );
            
                
                tunicPos = cPos;
                tunicRot = inRot;
                }
            if( inClothing.bottom != NULL ) {
            


                doublePair cPos = add( spritePos, 
                                       inClothing.bottom->clothingOffset );
                if( inFlipH ) {
                    cPos.x *= -1;
                    }
                cPos = add( cPos, inPos );
            
                
                bottomPos = cPos;
                bottomRot = inRot;
                }
            if( inClothing.backpack != NULL ) {
            


                doublePair cPos = add( spritePos, 
                                       inClothing.backpack->clothingOffset );
                if( inFlipH ) {
                    cPos.x *= -1;
                    }
                cPos = add( cPos, inPos );
            
                
                backpackPos = cPos;
                backpackRot = inRot;
                }
            }
        else if( i == topBackArmIndex ) {
            // draw under top of back arm

            if( inClothing.bottom != NULL ) {
                drawObject( inClothing.bottom, bottomPos, bottomRot,
                            inFlipH, -1, 0, false, false, emptyClothing );
                }
            if( inClothing.tunic != NULL ) {
                drawObject( inClothing.tunic, tunicPos, tunicRot,
                            inFlipH, -1, 0, false, false, emptyClothing );
                }
            if( inClothing.backpack != NULL ) {
                drawObject( inClothing.backpack, backpackPos, backpackRot,
                            inFlipH, -1, 0, false, false, emptyClothing );
                }
            }

        
        if( i == frontFootIndex 
            && inClothing.frontShoe != NULL ) {

            doublePair cPos = add( spritePos, 
                                   inClothing.frontShoe->clothingOffset );
            if( inFlipH ) {
                cPos.x *= -1;
                }
            cPos = add( cPos, inPos );
            
            frontShoePos = cPos;
            frontShoeRot = inRot;
            }
        

        
        
        if( ! skipSprite ) {
            setDrawColor( inObject->spriteColor[i] );

            double rot = inObject->spriteRot[i];

            if( inFlipH ) {
                rot *= -1;
                }
            
            rot += inRot;
            
            char multiplicative = 
                getUsesMultiplicativeBlending( inObject->sprites[i] );
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( true );
                }
            
            drawSprite( getSprite( inObject->sprites[i] ), pos, inScale,
                        rot, 
                        logicalXOR( inFlipH, inObject->spriteHFlip[i] ) );
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                }
            
            
            // this is the front-most drawn hand
            // in unanimated, unflipped object
            if( i == frontHandIndex && ( inHideClosestArm == 0 ) 
                && !inHideAllLimbs ) {
                
                returnHoldingPos.valid = true;
                // return screen pos for hand, which may be flipped, etc.
                returnHoldingPos.pos = pos;
                returnHoldingPos.rot = rot;
                }
            else if( i == bodyIndex && inHideClosestArm != 0 ) {
                returnHoldingPos.valid = true;
                // return screen pos for body, which may be flipped, etc.
                returnHoldingPos.pos = pos;
                returnHoldingPos.rot = rot;
                }
            }
        
        // shoes on top of feet
        if( inClothing.backShoe != NULL && i == backFootIndex ) {
            drawObject( inClothing.backShoe, backShoePos, backShoeRot,
                        inFlipH, -1, 0, false, false, emptyClothing );
            }
        else if( inClothing.frontShoe != NULL && i == frontFootIndex ) {
            drawObject( inClothing.backShoe, frontShoePos, frontShoeRot,
                        inFlipH, -1, 0, false, false, emptyClothing );
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
        
        drawObject( inClothing.hat, cPos, inRot,
                    inFlipH, -1, 0, false, false, emptyClothing );
        }

    return returnHoldingPos;
    }



HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos, double inRot,
                       char inFlipH, double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       int inNumContained, int *inContainedIDs ) {
    
    int numSlots = getNumContainerSlots( inObject->id );
    
    if( inNumContained > numSlots ) {
        inNumContained = numSlots;
        }
    
    for( int i=0; i<inNumContained; i++ ) {

        ObjectRecord *contained = getObject( inContainedIDs[i] );
        

        doublePair slotPos = sub( inObject->slotPos[i], 
                                  getObjectCenterOffset( contained ) );
        
        if( inRot != 0 ) {    
            if( inFlipH ) {
                slotPos = rotate( slotPos, 2 * M_PI * inRot );
                }
            else {
                slotPos = rotate( slotPos, -2 * M_PI * inRot );
                }
            }

        if( inFlipH ) {
            slotPos.x *= -1;
            }


        doublePair pos = add( slotPos, inPos );
        drawObject( contained, pos, inRot, inFlipH, inAge,
                    0,
                    false,
                    false,
                    emptyClothing );
        }
    
    return drawObject( inObject, inPos, inRot, inFlipH, inAge, 
                       inHideClosestArm,
                       inHideAllLimbs,
                       inHeldNotInPlaceYet,
                       inClothing );
    }





void deleteObjectFromBank( int inID ) {
    
    File objectsDir( NULL, "objects" );
    
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {

        File *cacheFile = objectsDir.getChildFile( "cache.fcz" );

        cacheFile->remove();
        
        delete cacheFile;


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



int getRandomFemalePersonObject() {
    
    if( femalePersonObjectIDs.size() == 0 ) {
        return -1;
        }
    
        
    return femalePersonObjectIDs.getElementDirect( 
        randSource.getRandomBoundedInt( 0, 
                                        femalePersonObjectIDs.size() - 1  ) );
    }


int *getRaces( int *outNumRaces ) {
    *outNumRaces = raceList.size();
    
    return raceList.getElementArray();
    }



int getRandomPersonObjectOfRace( int inRace ) {
    if( inRace > MAX_RACE ) {
        inRace = MAX_RACE;
        }
    
    if( racePersonObjectIDs[ inRace ].size() == 0 ) {
        return -1;
        }
    
        
    return racePersonObjectIDs[ inRace ].getElementDirect( 
        randSource.getRandomBoundedInt( 
            0, 
            racePersonObjectIDs[ inRace ].size() - 1  ) );
    }



int getRandomFamilyMember( int inRace, int inMotherID, int inFamilySpan ) {
    
    if( inRace > MAX_RACE ) {
        inRace = MAX_RACE;
        }
    
    if( racePersonObjectIDs[ inRace ].size() == 0 ) {
        return -1;
        }
    
    int motherIndex = 
        racePersonObjectIDs[ inRace ].getElementIndex( inMotherID );

    if( motherIndex == -1 ) {
        return getRandomPersonObjectOfRace( inRace );
        }
    

    // never have offset 0, so we can't ever have ourself as a baby
    int offset = randSource.getRandomBoundedInt( 1, inFamilySpan );
    
    if( randSource.getRandomBoolean() ) {
        offset = -offset;
        }
    
    int familyIndex = motherIndex + offset;
    
    while( familyIndex >= racePersonObjectIDs[ inRace ].size() ) {
        familyIndex -= racePersonObjectIDs[ inRace ].size();
        }
    while( familyIndex < 0  ) {
        familyIndex += racePersonObjectIDs[ inRace ].size();
        }
    
    return racePersonObjectIDs[ inRace ].getElementDirect( familyIndex );
    }





int getNextPersonObject( int inCurrentPersonObjectID ) {
    if( personObjectIDs.size() == 0 ) {
        return -1;
        }
    
    int numPeople = personObjectIDs.size();

    for( int i=0; i<numPeople - 1; i++ ) {
        if( personObjectIDs.getElementDirect( i ) == 
            inCurrentPersonObjectID ) {
            
            return personObjectIDs.getElementDirect( i + 1 );
            }
        }

    return personObjectIDs.getElementDirect( 0 );
    }



int getPrevPersonObject( int inCurrentPersonObjectID ) {
    if( personObjectIDs.size() == 0 ) {
        return -1;
        }
    
    int numPeople = personObjectIDs.size();

    for( int i=1; i<numPeople; i++ ) {
        if( personObjectIDs.getElementDirect( i ) == 
            inCurrentPersonObjectID ) {
            
            return personObjectIDs.getElementDirect( i - 1 );
            }
        }

    return personObjectIDs.getElementDirect( numPeople - 1 );
    }




int getRandomDeathMarker() {


    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            if( idMap[i]->deathMarker ) {
                return i;
                }
            }
        }
    return 0;
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



static ObjectRecord **clothingPointerByIndex( ClothingSet *inSet, 
                                              int inIndex ) {
    switch( inIndex ) {
        case 0:
            return &( inSet->hat );
        case 1:
            return &( inSet->tunic );
        case 2:
            return &( inSet->frontShoe );
        case 3:
            return &( inSet->backShoe );
        case 4:
            return &( inSet->bottom );
        case 5:
            return &( inSet->backpack );        
        }
    return NULL;
    }



ObjectRecord *clothingByIndex( ClothingSet inSet, int inIndex ) {
    ObjectRecord **pointer = clothingPointerByIndex( &inSet, inIndex );
    
    if( pointer != NULL ) {    
        return *( pointer );
        }
    
    return NULL;
    }



void setClothingByIndex( ClothingSet *inSet, int inIndex, 
                         ObjectRecord *inClothing ) {
    ObjectRecord **pointer = clothingPointerByIndex( inSet, inIndex );

    *pointer = inClothing;
    }




char checkSpriteAncestor( ObjectRecord *inObject, int inChildIndex,
                          int inPossibleAncestorIndex ) {
    
    int nextParent = inChildIndex;
    while( nextParent != -1 && nextParent != inPossibleAncestorIndex ) {
                
        nextParent = inObject->spriteParent[nextParent];
        }

    if( nextParent == inPossibleAncestorIndex ) {
        return true;
        }
    return false;
    }




int getMaxDiameter( ObjectRecord *inObject ) {
    int maxD = 0;
    
    for( int i=0; i<inObject->numSprites; i++ ) {
        doublePair pos = inObject->spritePos[i];
                
        int rad = getSpriteRecord( inObject->sprites[i] )->maxD / 2;
        
        int xR = lrint( fabs( pos.x ) + rad );
        int yR = lrint( fabs( pos.y ) + rad );
        
        int xD = 2 * xR;
        int yD = 2 * yR;
        
        if( xD > maxD ) {
            maxD = xD;
            }
        if( yD > maxD ) {
            maxD = yD;
            }
        }

    return maxD;
    }



double getClosestObjectPart( ObjectRecord *inObject,
                             ClothingSet *inClothing,
                             SimpleVector<int> *inContained,
                             SimpleVector<int> *inClothingContained,
                             double inAge,
                             int inPickedLayer,
                             char inFlip,
                             float inXCenterOffset, float inYCenterOffset,
                             int *outSprite,
                             int *outClothing,
                             int *outSlot ) {
    
    doublePair pos = { inXCenterOffset, inYCenterOffset };
    
    *outSprite = -1;
    *outClothing = -1;
    *outSlot = -1;

    doublePair headPos = {0,0};

    int headIndex = getHeadIndex( inObject, inAge );

    if( headIndex < inObject->numSprites ) {
        headPos = inObject->spritePos[ headIndex ];
        }

    
    doublePair frontFootPos = {0,0};

    int frontFootIndex = getFrontFootIndex( inObject, inAge );

    if( frontFootIndex < inObject->numSprites ) {
        frontFootPos = 
            inObject->spritePos[ frontFootIndex ];
        }

    doublePair backFootPos = {0,0};

    int backFootIndex = getBackFootIndex( inObject, inAge );

    if( backFootIndex < inObject->numSprites ) {
        backFootPos = 
            inObject->spritePos[ backFootIndex ];
        }


    doublePair bodyPos = {0,0};


    int bodyIndex = getBodyIndex( inObject, inAge );

    if( bodyIndex < inObject->numSprites ) {
        bodyPos = inObject->spritePos[ bodyIndex ];
        }

    int backArmTopIndex = getBackArmTopIndex( inObject, inAge );
    
    char tunicChecked = false;
    char hatChecked = false;
    
    for( int i=inObject->numSprites-1; i>=0; i-- ) {

        // first check for clothing that is above this part
        // (array because 3 pieces of clothing attached to body)
        ObjectRecord *cObj[3] = { NULL, NULL, NULL };
        
        int cObjIndex[3] = { -1, -1, -1 };
        
        
        doublePair cObjBodyPartPos[3];
        
        if( inClothing != NULL ) {
            
            if( i <= inObject->numSprites - 1 && !hatChecked ) {
                // hat above everything
                cObj[0] = inClothing->hat;
                cObjIndex[0] = 0;
                cObjBodyPartPos[0] = add( headPos, 
                                          getAgeHeadOffset( inAge, headPos,
                                                            bodyPos,
                                                            frontFootPos ) );
                if( checkSpriteAncestor( inObject, headIndex, bodyIndex ) ) {
                    cObjBodyPartPos[0] = add( cObjBodyPartPos[0],
                                              getAgeBodyOffset( inAge, 
                                                                bodyPos ) );
                    }
                hatChecked = true;
                }
            else if( i < backArmTopIndex && ! tunicChecked ) {
                // bottom, tunic, and backpack behind back arm
                
                
                cObj[0] = inClothing->backpack;        
                cObjIndex[0] = 5;
                cObjBodyPartPos[0] = add( bodyPos, 
                                          getAgeBodyOffset( inAge, bodyPos ) );
                cObj[1] = inClothing->tunic;        
                cObjIndex[1] = 1;
                cObjBodyPartPos[1] = add( bodyPos, 
                                          getAgeBodyOffset( inAge, bodyPos ) );
                
                cObj[2] = inClothing->bottom;        
                cObjIndex[2] = 4;
                cObjBodyPartPos[2] = add( bodyPos, 
                                          getAgeBodyOffset( inAge, bodyPos ) );
                tunicChecked = true;
                }
            else if( i == frontFootIndex ) {
                cObj[0] = inClothing->frontShoe;        
                cObjIndex[0] = 2;
                cObjBodyPartPos[0] = frontFootPos;
                }
            else if( i == backFootIndex ) {
                cObj[0] = inClothing->backShoe;        
                cObjIndex[0] = 3;
                cObjBodyPartPos[0] = backFootPos;
                }
            }
        
        for( int c=0; c<3; c++ ) {
            
            if( cObj[c] != NULL ) {
                int sp, cl, sl;
            
                doublePair clothingOffset = cObj[c]->clothingOffset;
                
                if( inFlip ) {
                    clothingOffset.x *= -1;
                    cObjBodyPartPos[c].x *= -1;
                    }

                
                doublePair cSpritePos = add( cObjBodyPartPos[c], 
                                             clothingOffset );
                                
                doublePair cOffset = sub( pos, cSpritePos );
                
                SimpleVector<int> *clothingCont = NULL;
                
                if( inClothingContained != NULL ) {
                    clothingCont = &( inClothingContained[ cObjIndex[c] ] );
                    }
                
                double dist = getClosestObjectPart( cObj[c],
                                                    NULL,
                                                    clothingCont,
                                                    NULL,
                                                    -1,
                                                    -1,
                                                    inFlip,
                                                    cOffset.x, cOffset.y,
                                                    &sp, &cl, &sl );
                if( sp != -1 ) {
                    *outClothing = cObjIndex[c];
                    break;
                    }
                else if( sl != -1  && dist == 0 ) {
                    *outClothing = cObjIndex[c];
                    *outSlot = sl;
                    break;
                    }
                }
            }
        
        if( *outClothing != -1 ) {
            break;
            }


        
        // clothing not hit, check sprite layer

        doublePair thisSpritePos = inObject->spritePos[i];
        

        if( inObject->person  ) {
            
            if( ! isSpriteVisibleAtAge( inObject, i, inAge ) ) {
                    
                if( i != inPickedLayer ) {
                    // invisible, don't let them pick it
                    continue;
                    }
                }

            if( i == headIndex ||
                checkSpriteAncestor( inObject, i,
                                     headIndex ) ) {
            
                thisSpritePos = add( thisSpritePos, 
                                     getAgeHeadOffset( inAge, headPos,
                                                       bodyPos,
                                                       frontFootPos ) );
                }
            if( i == bodyIndex ||
                checkSpriteAncestor( inObject, i,
                                     bodyIndex ) ) {
            
                thisSpritePos = add( thisSpritePos, 
                                     getAgeBodyOffset( inAge, bodyPos ) );
                }

            
            }
        
        if( inFlip ) {
            thisSpritePos.x *= -1;
            }
        
        doublePair offset = sub( pos, thisSpritePos );
        
        SpriteRecord *sr = getSpriteRecord( inObject->sprites[i] );
        
        
        
        offset = rotate( offset, 2 * M_PI * inObject->spriteRot[i] );
        
        if( inObject->spriteHFlip[i] ) {
            offset.x *= -1;
            }
        if( inFlip ) {
            offset.x *= -1;
            }

        offset.x += sr->centerAnchorXOffset;
        offset.y -= sr->centerAnchorYOffset;

        if( getSpriteHit( inObject->sprites[i], 
                          lrint( offset.x ),
                          lrint( offset.y ) ) ) {
            *outSprite = i;
            break;
            }
        }
    
    
    double smallestDist = 9999999;

    if( *outSprite == -1 && *outClothing == -1 && *outSlot == -1 ) {
        // consider slots
        
        
        for( int i=inObject->numSlots-1; i>=0; i-- ) {
            doublePair slotPos = inObject->slotPos[i];
            
            if( inFlip ) {
                slotPos.x *= -1;
                }
            
            if( inContained != NULL && i <inContained->size() ) {
                ObjectRecord *contained = 
                    getObject( inContained->getElementDirect( i ) );
            
                doublePair centOffset = getObjectCenterOffset( contained );
                
                if( inFlip ) {
                    centOffset.x *= -1;
                    }
            
                slotPos = sub( slotPos, centOffset );

                doublePair slotOffset = sub( pos, slotPos );
                

                int sp, cl, sl;
                
                getClosestObjectPart( contained,
                                      NULL,
                                      NULL,
                                      NULL,
                                      -1,
                                      -1,
                                      inFlip,
                                      slotOffset.x, slotOffset.y,
                                      &sp, &cl, &sl );
                if( sp != -1 ) {
                    *outSlot = i;
                    smallestDist = 0;
                    break;
                    }
                }
            else {
                
                double dist = distance( pos, inObject->slotPos[i] );
            
                if( dist < smallestDist ) {
                    *outSprite = -1;
                    *outSlot = i;
                    
                    smallestDist = dist;
                    }
                }
            }
        
        }
    else{ 
        smallestDist = 0;
        }

    return smallestDist;
    }



// gets index of hands in no order by finding lowest two holders
static void getHandIndices( ObjectRecord *inObject, double inAge,
                            int *outHandOneIndex, int *outHandTwoIndex ) {
    *outHandOneIndex = -1;
    *outHandTwoIndex = -1;
    
    double handOneY = 9999999;
    double handTwoY = 9999999;
    
    for( int i=0; i< inObject->numSprites; i++ ) {
        if( inObject->spriteInvisibleWhenHolding[i] ) {
            
            if( inObject->spriteAgeStart[i] != -1 ||
                inObject->spriteAgeEnd[i] != -1 ) {
                        
                if( inAge < inObject->spriteAgeStart[i] ||
                    inAge >= inObject->spriteAgeEnd[i] ) {
                
                    // skip this layer
                    continue;
                    }
                }
            
            if( inObject->spritePos[i].y < handOneY ) {
                *outHandTwoIndex = *outHandOneIndex;
                handTwoY = handOneY;
                
                *outHandOneIndex = i;
                handOneY = inObject->spritePos[i].y;
                }
            else if( inObject->spritePos[i].y < handTwoY ) {
                *outHandTwoIndex = i;
                handTwoY = inObject->spritePos[i].y;
                }
            }
        }

    }




int getBackHandIndex( ObjectRecord *inObject,
                      double inAge ) {
    
    int handOneIndex;
    int handTwoIndex;
    
    getHandIndices( inObject, inAge, &handOneIndex, &handTwoIndex );
    
    if( handOneIndex != -1 ) {
        
        if( handTwoIndex != -1 ) {
            if( inObject->spritePos[handOneIndex].x <
                inObject->spritePos[handTwoIndex].x ) {
                return handOneIndex;
                }
            else {
                return handTwoIndex;
                }
            }
        else {
            return handTwoIndex;
            }
        }
    else {
        return -1;
        }
    }



int getFrontHandIndex( ObjectRecord *inObject,
                       double inAge ) {
        
    int handOneIndex;
    int handTwoIndex;
    
    getHandIndices( inObject, inAge, &handOneIndex, &handTwoIndex );
    
    if( handOneIndex != -1 ) {
        
        if( handTwoIndex != -1 ) {
            if( inObject->spritePos[handOneIndex].x >
                inObject->spritePos[handTwoIndex].x ) {
                return handOneIndex;
                }
            else {
                return handTwoIndex;
                }
            }
        else {
            return handTwoIndex;
            }
        }
    else {
        return -1;
        }
    }



static void getLimbIndices( ObjectRecord *inObject,
                            double inAge, SimpleVector<int> *outList,
                            int inHandOrFootIndex ) {
    
    if( inHandOrFootIndex == -1 ) {
        return;
        }
    
    int nextLimbPart = inHandOrFootIndex;
    
    while( nextLimbPart != -1 && ! inObject->spriteIsBody[ nextLimbPart ] ) {
        outList->push_back( nextLimbPart );
        nextLimbPart = inObject->spriteParent[ nextLimbPart ];
        }
    }



void getFrontArmIndices( ObjectRecord *inObject,
                         double inAge, SimpleVector<int> *outList ) {
    getLimbIndices( inObject, inAge, outList,
                    getFrontHandIndex( inObject, inAge ) );
    }



void getBackArmIndices( ObjectRecord *inObject,
                        double inAge, SimpleVector<int> *outList ) {
    
    getLimbIndices( inObject, inAge, outList,
                    getBackHandIndex( inObject, inAge ) );

    }



int getBackArmTopIndex( ObjectRecord *inObject, double inAge ) {
    SimpleVector<int> list;
    getBackArmIndices( inObject, inAge, &list );
    
    if( list.size() > 0 ) {
        return list.getElementDirect( list.size() - 1 );
        }
    else {
        return -1;
        }
    }



void getAllLegIndices( ObjectRecord *inObject,
                       double inAge, SimpleVector<int> *outList ) {
    getLimbIndices( inObject, inAge, outList,
                    getBackFootIndex( inObject, inAge ) );
    
    getLimbIndices( inObject, inAge, outList,
                    getFrontFootIndex( inObject, inAge ) );
    }




char isSpriteVisibleAtAge( ObjectRecord *inObject,
                           int inSpriteIndex,
                           double inAge ) {
    
    if( inObject->spriteAgeStart[inSpriteIndex] != -1 ||
        inObject->spriteAgeEnd[inSpriteIndex] != -1 ) {
                        
        if( inAge < inObject->spriteAgeStart[inSpriteIndex] ||
            inAge >= inObject->spriteAgeEnd[inSpriteIndex] ) {
         
            return false;
            }
        }
    return true;
    }



static int getBodyPartIndex( ObjectRecord *inObject,
                             char *inBodyPartFlagArray,
                             double inAge ) {
    
    for( int i=0; i< inObject->numSprites; i++ ) {
        if( inBodyPartFlagArray[i] ) {
            
            if( ! isSpriteVisibleAtAge( inObject, i, inAge ) ) {
                // skip this layer
                continue;
                }
                            
            return i;
            }
        }
    
    // default
    // don't return -1 here, so it can be blindly used as an index
    return 0;
    }



int getHeadIndex( ObjectRecord *inObject,
                  double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsHead, inAge );
    }


int getBodyIndex( ObjectRecord *inObject,
                  double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsBody, inAge );
    }


int getBackFootIndex( ObjectRecord *inObject,
                  double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsBackFoot, inAge );
    }


int getFrontFootIndex( ObjectRecord *inObject,
                  double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsFrontFoot, inAge );
    }



char *getBiomesString( ObjectRecord *inObject ) {
    SimpleVector <char>stringBuffer;
    
    for( int i=0; i<inObject->numBiomes; i++ ) {
        
        if( i != 0 ) {
            stringBuffer.push_back( ',' );
            }
        char *intString = autoSprintf( "%d", inObject->biomes[i] );
        
        stringBuffer.appendElementString( intString );
        delete [] intString;
        }

    return stringBuffer.getElementString();
    }
                       


void getAllBiomes( SimpleVector<int> *inVectorToFill ) {
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            for( int j=0; j< idMap[i]->numBiomes; j++ ) {
                int b = idMap[i]->biomes[j];
                
                if( inVectorToFill->getElementIndex( b ) == -1 ) {
                    inVectorToFill->push_back( b );
                    }
                }
            }
        }
    }



doublePair getObjectCenterOffset( ObjectRecord *inObject ) {


    // compute average of all sprite centers
    
    double minX = DBL_MAX;
    double maxX = -DBL_MAX;

    double minY = DBL_MAX;
    double maxY = -DBL_MAX;
    
    
    for( int i=0; i<inObject->numSprites; i++ ) {
        SpriteRecord *sprite = getSpriteRecord( inObject->sprites[i] );
        
        doublePair centerOffset = { (double)sprite->centerXOffset,
                                    (double)sprite->centerYOffset };
        
        centerOffset = rotate( centerOffset, 
                               2 * M_PI * inObject->spriteRot[i] );

        doublePair spriteCenter = add( inObject->spritePos[i], centerOffset );
        
        if( spriteCenter.x < minX ) {
            minX = spriteCenter.x;
            }
        if( spriteCenter.x > maxX ) {
            maxX = spriteCenter.x;
            }

        if( spriteCenter.y < minY ) {
            minY = spriteCenter.y;
            }
        if( spriteCenter.y > maxY ) {
            maxY = spriteCenter.y;
            }
        }

    doublePair result = { round( ( maxX + minX ) / 2 ), 
                          round( ( maxY + minY ) / 2 ) };
    
    return result;
    }


    






