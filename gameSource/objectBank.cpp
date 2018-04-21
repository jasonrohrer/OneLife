
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

#include "soundBank.h"

#include "animationBank.h"





static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static ObjectRecord **idMap;


static StringTree tree;


// track objects that are marked with the person flag
static SimpleVector<int> personObjectIDs;

// track female people
static SimpleVector<int> femalePersonObjectIDs;

// track monument calls
static SimpleVector<int> monumentCallObjectIDs;



// anything above race 100 is put in bin for race 100
#define MAX_RACE 100

static SimpleVector<int> racePersonObjectIDs[ MAX_RACE + 1 ];

static SimpleVector<int> raceList;



#define MAX_BIOME 511

static float biomeHeatMap[ MAX_BIOME + 1 ];



static int recomputeObjectHeight(  int inNumSprites, int *inSprites,
                                   doublePair *inSpritePos );



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


static ClothingSet emptyClothing = { NULL, NULL, NULL, NULL, NULL, NULL };



static FolderCache cache;

static int currentFile;


static SimpleVector<ObjectRecord*> records;
static int maxID;


static int maxWideRadius = 0;



int getMaxObjectID() {
    return maxID;
    }


void setDrawColor( FloatRGB inColor ) {
    setDrawColor( inColor.r, 
                  inColor.g, 
                  inColor.b,
                  1 );
    }



static char autoGenerateUsedObjects = false;
static char autoGenerateVariableObjects = false;


int initObjectBankStart( char *outRebuildingCache, 
                         char inAutoGenerateUsedObjects,
                         char inAutoGenerateVariableObjects ) {
    maxID = 0;

    currentFile = 0;
    

    cache = initFolderCache( "objects", outRebuildingCache );

    autoGenerateUsedObjects = inAutoGenerateUsedObjects;
    autoGenerateVariableObjects = inAutoGenerateVariableObjects;

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
        strstr( txtFileName, "groundHeat_" ) == NULL &&
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
                r->vertContainRotationOffset = 0;
                
                sscanf( lines[next], "containSize=%d,vertSlotRot=%lf", 
                        &( r->containSize ),
                        &( r->vertContainRotationOffset ) );
                            
                next++;
                            
                int permRead = 0;                 
                r->minPickupAge = 3;
                sscanf( lines[next], "permanent=%d,minPickupAge=%d", 
                        &( permRead ),
                        &( r->minPickupAge ) );
                            
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
                
                r->leftBlockingRadius = 0;
                r->rightBlockingRadius = 0;
                
                int drawBehindPlayerRead = 0;
                
                sscanf( lines[next], 
                        "blocksWalking=%d,"
                        "leftBlockingRadius=%d,rightBlockingRadius=%d,"
                        "drawBehindPlayer=%d",
                        &( blocksWalkingRead ),
                        &( r->leftBlockingRadius ),
                        &( r->rightBlockingRadius ),
                        &( drawBehindPlayerRead ) );
                            
                r->blocksWalking = blocksWalkingRead;
                r->drawBehindPlayer = drawBehindPlayerRead;
                
                r->wide = ( r->leftBlockingRadius > 0 || 
                            r->rightBlockingRadius > 0 );

                if( r->wide ) {
                    r->drawBehindPlayer = true;
                    
                    if( r->leftBlockingRadius > maxWideRadius ) {
                        maxWideRadius = r->leftBlockingRadius;
                        }
                    if( r->rightBlockingRadius > maxWideRadius ) {
                        maxWideRadius = r->rightBlockingRadius;
                        }
                    }
                    

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
                int noSpawnRead = 0;
                sscanf( lines[next], "person=%d,noSpawn=%d", 
                        &personRead, &noSpawnRead );
                            
                r->person = ( personRead > 0 );
                
                r->race = personRead;
                
                r->personNoSpawn = noSpawnRead;

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


                r->homeMarker = false;
                
                if( strstr( lines[next], "homeMarker=" ) != NULL ) {
                    // home marker flag present
                    
                    int homeMarkerRead = 0;
                    sscanf( lines[next], "homeMarker=%d", &( homeMarkerRead ) );
                    
                    r->homeMarker = homeMarkerRead;
                    
                    next++;
                    }



                r->floor = false;
                
                if( strstr( lines[next], "floor=" ) != NULL ) {
                    // floor flag present
                    
                    int floorRead = 0;
                    sscanf( lines[next], "floor=%d", &( floorRead ) );
                    
                    r->floor = floorRead;
                    
                    next++;
                    }


                r->floorHugging = false;
                
                if( strstr( lines[next], "floorHugging=" ) != NULL ) {
                    // floorHugging flag present
                    
                    int hugRead = 0;
                    sscanf( lines[next], "floorHugging=%d", &( hugRead ) );
                    
                    r->floorHugging = hugRead;
                    
                    next++;
                    }

                            
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
                
                
                r->useDistance = 1;
                
                if( strstr( lines[next], 
                            "useDistance=" ) != NULL ) {
                    // use distance present
                    
                    sscanf( lines[next], "useDistance=%d", 
                            &( r->useDistance ) );
                    
                    next++;
                    }
                

                r->creationSound = blankSoundUsage;
                r->usingSound = blankSoundUsage;
                r->eatingSound = blankSoundUsage;
                r->decaySound = blankSoundUsage;
                
                
                if( strstr( lines[next], "sounds=" ) != NULL ) {
                    // sounds present

                    int numParts = 0;
                    
                    char **parts = split( &( lines[next][7] ), ",", &numParts );
                    
                    if( numParts == 4 ) {
                        r->creationSound = scanSoundUsage( parts[0] );
                        r->usingSound = scanSoundUsage( parts[1] );
                        r->eatingSound = scanSoundUsage( parts[2] );
                        r->decaySound = scanSoundUsage( parts[3] );
                        }
                    
                    for( int i=0; i<numParts; i++ ) {
                        delete [] parts[i];
                        }
                    delete [] parts;

                    next++;
                    }
                
                if( strstr( lines[next], 
                            "creationSoundInitialOnly=" ) != NULL ) {
                    // flag present
                    
                    int flagRead = 0;                            
                    sscanf( lines[next], "creationSoundInitialOnly=%d", 
                            &( flagRead ) );
                    
                    r->creationSoundInitialOnly = flagRead;
                            
                    next++;
                    }
                else {
                    r->creationSoundInitialOnly = 0;
                    }
                

                r->numSlots = 0;
                r->slotTimeStretch = 1.0f;
                
                
                if( strstr( lines[next], "#" ) != NULL ) {
                    sscanf( lines[next], "numSlots=%d#timeStretch=%f", 
                            &( r->numSlots ),
                            &( r->slotTimeStretch ) );
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
                r->slotVert = new char[ r->numSlots ];
                r->slotParent = new int[ r->numSlots ];
            
                for( int i=0; i< r->numSlots; i++ ) {
                    r->slotVert[i] = false;
                    r->slotParent[i] = -1;
                    
                    int vertRead = 0;
                    sscanf( lines[ next ], "slotPos=%lf,%lf,vert=%d,parent=%d", 
                            &( r->slotPos[i].x ),
                            &( r->slotPos[i].y ),
                            &vertRead,
                            &( r->slotParent[i] ) );
                    r->slotVert[i] = vertRead;
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
                r->spriteInvisibleWhenWorn = new int[ r->numSprites ];
                r->spriteBehindSlots = new char[ r->numSprites ];



                r->spriteIsHead = new char[ r->numSprites ];
                r->spriteIsBody = new char[ r->numSprites ];
                r->spriteIsBackFoot = new char[ r->numSprites ];
                r->spriteIsFrontFoot = new char[ r->numSprites ];
                

                memset( r->spriteIsHead, false, r->numSprites );
                memset( r->spriteIsBody, false, r->numSprites );
                memset( r->spriteIsBackFoot, false, r->numSprites );
                memset( r->spriteIsFrontFoot, false, r->numSprites );
                
                
                r->numUses = 1;
                r->spriteUseVanish = new char[ r->numSprites ];
                r->spriteUseAppear = new char[ r->numSprites ];
                r->useDummyIDs = NULL;
                r->isUseDummy = false;
                r->useDummyParent = 0;
                r->cachedHeight = -1;
                
                memset( r->spriteUseVanish, false, r->numSprites );
                memset( r->spriteUseAppear, false, r->numSprites );

                r->spriteSkipDrawing = new char[ r->numSprites ];
                memset( r->spriteSkipDrawing, false, r->numSprites );
                
                r->apocalypseTrigger = false;
                if( r->description[0] == 'T' &&
                    r->description[1] == 'h' &&
                    strstr( r->description, "The Apocalypse" ) == 
                    r->description ) {
                    
                    printf( "Object id %d (%s) seen as an apocalypse trigger\n",
                            r->id, r->description );

                    r->apocalypseTrigger = true;
                    }

                r->monumentStep = false;
                r->monumentDone = false;
                r->monumentCall = false;
                
                if( strstr( r->description, "monument" ) != NULL ) {
                    // some kind of monument state
                    if( strstr( r->description, "monumentStep" ) != NULL ) {
                        r->monumentStep = true;
                        }
                    else if( strstr( r->description, 
                                     "monumentDone" ) != NULL ) {
                        r->monumentDone = true;
                        }
                    else if( strstr( r->description, 
                                     "monumentCall" ) != NULL ) {
                        r->monumentCall = true;
                        monumentCallObjectIDs.push_back( r->id );
                        }
                    }
                
                r->numVariableDummyIDs = 0;
                r->variableDummyIDs = NULL;
                r->isVariableDummy = false;


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
                    int invisWornRead = 0;
                    int behindSlotsRead = 0;
                    
                    sscanf( lines[next], 
                            "invisHolding=%d,invisWorn=%d,behindSlots=%d", 
                            &invisRead, &invisWornRead,
                            &behindSlotsRead );
                                
                    r->spriteInvisibleWhenHolding[i] = invisRead;
                    r->spriteInvisibleWhenWorn[i] = invisWornRead;
                    r->spriteBehindSlots[i] = behindSlotsRead;
                                
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

                if( next < numLines ) {
                    // info about num uses and vanish/appear sprites
                    
                    sscanf( lines[next], "numUses=%d", 
                            &( r->numUses ) );
                            
                    next++;
                    
                    if( next < numLines ) {
                        sparseCommaLineToBoolArray( "useVanishIndex", 
                                                    lines[next],
                                                    r->spriteUseVanish, 
                                                    r->numSprites );
                        next++;

                        if( next < numLines ) {
                            sparseCommaLineToBoolArray( "useAppearIndex", 
                                                        lines[next],
                                                        r->spriteUseAppear, 
                                                        r->numSprites );
                            next++;
                            }
                        }
                    }
                
                if( next < numLines ) {
                    sscanf( lines[next], "pixHeight=%d", 
                            &( r->cachedHeight ) );
                    next++;
                    }       
                

                    
                records.push_back( r );

                            
                if( r->person && ! r->personNoSpawn ) {
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
    

static char makeNewObjectsSearchable = false;

void enableObjectSearch( char inEnable ) {
    makeNewObjectsSearchable = inEnable;
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

        if( makeNewObjectsSearchable ) {    
            char *lowercase = stringToLowerCase( r->description );

            tree.insert( lowercase, r );
            
            delete [] lowercase;
            }
        
        }
    
                        
    rebuildRaceList();

    printf( "Loaded %d objects from objects folder\n", numRecords );


    if( autoGenerateUsedObjects ) {
        int numAutoGenerated = 0;
        
        char oldSearch = makeNewObjectsSearchable;
        
        // don't need to be able to search for dummy objs
        makeNewObjectsSearchable = false;

        for( int i=0; i<mapSize; i++ ) {
            if( idMap[i] != NULL ) {
                ObjectRecord *o = idMap[i];
                

                if( o->numUses > 1 ) {
                    int mainID = o->id;
                    
                    int numUses = o->numUses;
                    
                    int numDummyObj = numUses - 1;
                    
                    o->useDummyIDs = new int[ numDummyObj ];
                    

                    for( int s=0; s<o->numSprites; s++ ) {
                        if( o->spriteUseAppear[s] ) {
                            // hide all appearing sprites in parent object
                            o->spriteSkipDrawing[s] = true;
                            }
                        }
                    
                    for( int d=1; d<=numDummyObj; d++ ) {
                        
                        numAutoGenerated ++;
                        
                        char *desc = autoSprintf( "%s# use %d",
                                                  o->description,
                                                  d );
                        
                        int dummyID = reAddObject( o, desc, true );
                        
                        delete [] desc;

                        o->useDummyIDs[ d - 1 ] = dummyID;
                        
                        ObjectRecord *dummyO = getObject( dummyID );
                        
                        // only main object has uses
                        // dummies set to 0 so we don't recursively make
                        // more dummies out of them
                        dummyO->numUses = 0;

                        // used objects never occur naturally
                        dummyO->mapChance = 0;
                        
                        dummyO->isUseDummy = true;
                        dummyO->useDummyParent = mainID;
                        
                        if( o->creationSoundInitialOnly ) {
                            clearSoundUsage( &( dummyO->creationSound ) );
                            }
                        
                        
                        setupSpriteUseVis( o, d, dummyO->spriteSkipDrawing );
                        
                        
                        // copy anims too
                        for( int t=0; t<endAnimType; t++ ) {
                            AnimationRecord *a = getAnimation( mainID,
                                                               (AnimType)t );

                            if( a != NULL ) {
                                
                                // feels more risky, but way faster
                                // than copying it
                                
                                // temporarily replace the object ID
                                // before adding this record
                                // it will be copied internally
                                a->objectID = dummyID;
                                
                                addAnimation( a, true );

                                // restore original record
                                a->objectID = mainID;
                                }
                            }
                        }
                    }
                
                }
            }
        
        makeNewObjectsSearchable = oldSearch;
        
        printf( "  Auto-generated %d 'used' objects\n", numAutoGenerated );
        }




    if( autoGenerateVariableObjects ) {
        int numAutoGenerated = 0;
        
        char oldSearch = makeNewObjectsSearchable;
        
        // don't need to be able to search for dummy objs
        makeNewObjectsSearchable = false;

        for( int i=0; i<mapSize; i++ ) {
            if( idMap[i] != NULL ) {
                ObjectRecord *o = idMap[i];
                

                char *dollarPos = strstr( o->description, "$" );
                
                if( dollarPos != NULL ) {
                    int mainID = o->id;
                    
                    char *afterDollarPos = &( dollarPos[1] );

                    int numVar = 0;
                    
                    int numRead = sscanf( afterDollarPos, "%d", &numVar );
                    
                    if( numRead != 1 || numVar < 2 ) {
                        continue;
                        }

                    
                    o->numVariableDummyIDs = numVar;
                    o->variableDummyIDs = new int[ numVar ];
                    
                    char *target = autoSprintf( "$%d", numVar );
                    
                    for( int d=1; d<=numVar; d++ ) {    
                        numAutoGenerated ++;
                        
                        char *sub = autoSprintf( "%d", d );
                        
                        char found;
                        char *desc = replaceOnce( o->description, target,
                                                  sub,
                                                  &found );
                        delete [] sub;
                            
                        
                        int dummyID = reAddObject( o, desc, true );
                        
                        delete [] desc;

                        o->variableDummyIDs[ d - 1 ] = dummyID;
                        
                        ObjectRecord *dummyO = getObject( dummyID );

                        dummyO->isVariableDummy = true;

                        // copy anims too
                        for( int t=0; t<endAnimType; t++ ) {
                            AnimationRecord *a = getAnimation( mainID,
                                                               (AnimType)t );

                            if( a != NULL ) {
                                
                                // feels more risky, but way faster
                                // than copying it
                                
                                // temporarily replace the object ID
                                // before adding this record
                                // it will be copied internally
                                a->objectID = dummyID;
                                
                                addAnimation( a, true );

                                // restore original record
                                a->objectID = mainID;
                                }
                            }
                        }
                    delete [] target;
                    }
                
                }
            }
        
        makeNewObjectsSearchable = oldSearch;
        
        printf( "  Auto-generated %d 'variable' objects\n", numAutoGenerated );
        }
    


    for( int i=0; i<=MAX_BIOME; i++ ) {
        biomeHeatMap[ i ] = 0;
        }

    SimpleVector<int> biomes;
    getAllBiomes( &biomes );
    
    // heat files stored in objects folder
    File groundHeatDir( NULL, "objects" );

    if( groundHeatDir.exists() && groundHeatDir.isDirectory() ) {
        
        for( int i=0; i<biomes.size(); i++ ) {
            int b = biomes.getElementDirect( i );
            
            float heat = 0;
            char *heatFileName = autoSprintf( "groundHeat_%d.txt", b );
            
            File *heatFile = groundHeatDir.getChildFile( heatFileName );
            
            if( heatFile->exists() && ! heatFile->isDirectory() ) {
                char *cont = heatFile->readFileContents();
                
                if( cont != NULL ) {
                    
                    sscanf( cont, "%f", &heat );
                    delete [] cont;
                    }
                }
            delete heatFile;
            delete [] heatFileName;
            
            if( b <= MAX_BIOME ) {
                biomeHeatMap[ b ] = heat;
                }
            }
        }

            // resaveAll();
    }




float getBiomeHeatValue( int inBiome ) {
    if( inBiome >= 0 && inBiome <= MAX_BIOME ) {
        return biomeHeatMap[ inBiome ];
        }
    return 0;
    }






// working vectors for this setup function
// don't re-compute these if we're repeating operation on same objectID
// (which we do when we compute use dummy objects at startup)
static int lastSetupObject = -1;
static int numVanishingSprites = 0;
static int numAppearingSprites = 0;

static SimpleVector<int> vanishingIndices;
static SimpleVector<int> appearingIndices;


void setupSpriteUseVis( ObjectRecord *inObject, int inUsesRemaining,
                        char *inSpriteSkipDrawing ) {
    
    memset( inSpriteSkipDrawing, false, inObject->numSprites );

    if( inObject->numUses == inUsesRemaining ) {
        
        for( int s=0; s<inObject->numSprites; s++ ) {
            if( inObject->spriteUseAppear[s] ) {
                // hide all appearing sprites 
                inSpriteSkipDrawing[s] = true;
                }
            }
        return;
        }
    else if( inUsesRemaining == 0 ) {
        for( int s=0; s<inObject->numSprites; s++ ) {
            if( inObject->spriteUseVanish[s] ) {
                // hide all vanishing sprites 
                inSpriteSkipDrawing[s] = true;
                }
            }
        }
    else {
        // generate vis for one of the use dummy objects
        int numUses = inObject->numUses;
                    
        if( inObject->id != lastSetupObject ) {
            
            numVanishingSprites = 0;
            numAppearingSprites = 0;
            
            vanishingIndices.deleteAll();
            appearingIndices.deleteAll();
            
            for( int s=0; s<inObject->numSprites; s++ ) {
                if( inObject->spriteUseVanish[s] ) {
                    numVanishingSprites ++;
                    vanishingIndices.push_back( s );
                    }
                }

        
        
            for( int s=0; s<inObject->numSprites; s++ ) {
                if( inObject->spriteUseAppear[s] ) {
                    numAppearingSprites ++;
                    appearingIndices.push_back( s );
                
                    // hide all appearing sprites as basis
                    inSpriteSkipDrawing[s] = true;
                    }
                }
            lastSetupObject = inObject->id;
            }
        else {
            for( int i=0; i<numAppearingSprites; i++ ) {
                // hide all appearing sprites as basis
                inSpriteSkipDrawing[ appearingIndices.getElementDirect(i) ] =
                    true;
                }
            }
        

        int d = inUsesRemaining;
        
        // hide some sprites
        
        int numSpritesLeft = 
            ( d * (numVanishingSprites) ) / numUses;
        
        int numInLastDummy = numVanishingSprites / numUses;
        
        if( numInLastDummy == 0 ) {
            // add 1 to everything to pad up, so last
            // dummy has 1 sprite in it
            numSpritesLeft += 1;
            }
                        

        if( numSpritesLeft > numVanishingSprites ) {
            numSpritesLeft = numVanishingSprites;
            }

        for( int v=numSpritesLeft; v<numVanishingSprites; v++ ) {
            
            inSpriteSkipDrawing[ vanishingIndices.getElementDirect( v ) ] = 
                true;
            }


        // now handle appearing sprites
        int numInvisSpritesLeft = 
            lrint( ( d * (numAppearingSprites) ) / (double)numUses );
                        
        /*
        // testing... do we need to do this?
        int numInvisInLastDummy = numAppearingSprites / numUses;
        
        if( numInLastDummy == 0 ) {
        // add 1 to everything to pad up, so last
        // dummy has 1 sprite in it
        numSpritesLeft += 1;
        }
        */
        
        if( numInvisSpritesLeft > numAppearingSprites ) {
            numInvisSpritesLeft = numAppearingSprites;
            }

        for( int v=0; v<numAppearingSprites - numInvisSpritesLeft; v++ ) {
            
            inSpriteSkipDrawing[ appearingIndices.getElementDirect( v ) ] = 
                false;
            }
        }
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
            delete [] idMap[inID]->slotVert;
            delete [] idMap[inID]->slotParent;
            delete [] idMap[inID]->sprites;
            delete [] idMap[inID]->spritePos;
            delete [] idMap[inID]->spriteRot;
            delete [] idMap[inID]->spriteHFlip;
            delete [] idMap[inID]->spriteColor;

            delete [] idMap[inID]->spriteAgeStart;
            delete [] idMap[inID]->spriteAgeEnd;
            delete [] idMap[inID]->spriteParent;

            delete [] idMap[inID]->spriteInvisibleWhenHolding;
            delete [] idMap[inID]->spriteInvisibleWhenWorn;
            delete [] idMap[inID]->spriteBehindSlots;

            delete [] idMap[inID]->spriteIsHead;
            delete [] idMap[inID]->spriteIsBody;
            delete [] idMap[inID]->spriteIsBackFoot;
            delete [] idMap[inID]->spriteIsFrontFoot;

            delete [] idMap[inID]->spriteUseVanish;
            delete [] idMap[inID]->spriteUseAppear;
            
            if( idMap[inID]->useDummyIDs != NULL ) {
                delete [] idMap[inID]->useDummyIDs;
                }

            if( idMap[inID]->variableDummyIDs != NULL ) {
                delete [] idMap[inID]->variableDummyIDs;
                }

            delete [] idMap[inID]->spriteSkipDrawing;
            
            clearSoundUsage( &( idMap[inID]->creationSound ) );
            clearSoundUsage( &( idMap[inID]->usingSound ) );
            clearSoundUsage( &( idMap[inID]->eatingSound ) );
            clearSoundUsage( &( idMap[inID]->decaySound ) );
            

            delete idMap[inID];
            idMap[inID] = NULL;

            personObjectIDs.deleteElementEqualTo( inID );
            femalePersonObjectIDs.deleteElementEqualTo( inID );
            monumentCallObjectIDs.deleteElementEqualTo( inID );
            
            
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
            delete [] idMap[i]->slotVert;
            delete [] idMap[i]->slotParent;
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
            delete [] idMap[i]->spriteInvisibleWhenWorn;
            delete [] idMap[i]->spriteBehindSlots;

            delete [] idMap[i]->spriteIsHead;
            delete [] idMap[i]->spriteIsBody;
            delete [] idMap[i]->spriteIsBackFoot;
            delete [] idMap[i]->spriteIsFrontFoot;

            delete [] idMap[i]->spriteUseVanish;
            delete [] idMap[i]->spriteUseAppear;
            
            if( idMap[i]->useDummyIDs != NULL ) {
                delete [] idMap[i]->useDummyIDs;
                }

            if( idMap[i]->variableDummyIDs != NULL ) {
                delete [] idMap[i]->variableDummyIDs;
                }

            delete [] idMap[i]->spriteSkipDrawing;

            //printf( "\n\nClearing sound usage for id %d\n", i );            
            clearSoundUsage( &( idMap[i]->creationSound ) );
            clearSoundUsage( &( idMap[i]->usingSound ) );
            clearSoundUsage( &( idMap[i]->eatingSound ) );
            clearSoundUsage( &( idMap[i]->decaySound ) );

            delete idMap[i];
            }
        }

    delete [] idMap;

    personObjectIDs.deleteAll();
    femalePersonObjectIDs.deleteAll();
    monumentCallObjectIDs.deleteAll();
    
    for( int i=0; i<= MAX_RACE; i++ ) {
        racePersonObjectIDs[i].deleteAll();
        }
    rebuildRaceList();
    }



int reAddObject( ObjectRecord *inObject,
                 char *inNewDescription,
                 char inNoWriteToFile, int inReplaceID ) {
    
    const char *desc = inObject->description;
    
    if( inNewDescription != NULL ) {
        desc = inNewDescription;
        }
    
    char *biomeString = getBiomesString( inObject );

    int id = addObject( desc,
                        inObject->containable,
                        inObject->containSize,
                        inObject->vertContainRotationOffset,
                        inObject->permanent,
                        inObject->minPickupAge,
                        inObject->heldInHand,
                        inObject->rideable,
                        inObject->blocksWalking,
                        inObject->leftBlockingRadius,
                        inObject->rightBlockingRadius,
                        inObject->drawBehindPlayer,
                        biomeString,
                        inObject->mapChance,
                        inObject->heatValue,
                        inObject->rValue,
                        inObject->person,
                        inObject->personNoSpawn,
                        inObject->male,
                        inObject->race,
                        inObject->deathMarker,
                        inObject->homeMarker,
                        inObject->floor,
                        inObject->floorHugging,
                        inObject->foodValue,
                        inObject->speedMult,
                        inObject->heldOffset,
                        inObject->clothing,
                        inObject->clothingOffset,
                        inObject->deadlyDistance,
                        inObject->useDistance,
                        inObject->creationSound,
                        inObject->usingSound,
                        inObject->eatingSound,
                        inObject->decaySound,
                        inObject->creationSoundInitialOnly,
                        inObject->numSlots, 
                        inObject->slotSize, 
                        inObject->slotPos,
                        inObject->slotVert,
                        inObject->slotParent,
                        inObject->slotTimeStretch,
                        inObject->numSprites, 
                        inObject->sprites, 
                        inObject->spritePos,
                        inObject->spriteRot,
                        inObject->spriteHFlip,
                        inObject->spriteColor,
                        inObject->spriteAgeStart,
                        inObject->spriteAgeEnd,
                        inObject->spriteParent,
                        inObject->spriteInvisibleWhenHolding,
                        inObject->spriteInvisibleWhenWorn,
                        inObject->spriteBehindSlots,
                        inObject->spriteIsHead,
                        inObject->spriteIsBody,
                        inObject->spriteIsBackFoot,
                        inObject->spriteIsFrontFoot,
                        inObject->numUses,
                        inObject->spriteUseVanish,
                        inObject->spriteUseAppear,
                        inNoWriteToFile,
                        inReplaceID );

    delete [] biomeString;

    return id;
    }



void resaveAll() {
    printf( "Starting to resave all objects\n..." );
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {

            ObjectRecord *o = idMap[i];

            char anyNotLoaded = true;
            
            while( anyNotLoaded ) {
                anyNotLoaded = false;
                
                for( int s=0; s< o->numSprites; s++ ) {
                    
                    char loaded = markSpriteLive( o->sprites[s] );
                    
                    if( ! loaded ) {
                        anyNotLoaded = true;
                        }
                    }
                stepSpriteBank();
                }

            reAddObject( idMap[i], NULL, false, i );
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


char isApocalypseTrigger( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return false;
        }
    else {
        return r->apocalypseTrigger;
        }
    }



int getMonumentStatus( int inID ) {
    ObjectRecord *r = getObject( inID );
    
    if( r == NULL ) {
        return 0;
        }
    else {
        if( r->monumentStep ) {
            return 1;
            }
        if( r->monumentDone ) {
            return 2;
            }
        if( r->monumentCall ) {
            return 3;
            }
        return 0;
        }
    }


SimpleVector<int> *getMonumentCallObjects() {
    return &monumentCallObjectIDs;
    }



    


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining ) {
    
    if( strcmp( inSearch, "" ) == 0 ) {
        // special case, show objects in reverse-id order, newest first
        SimpleVector< ObjectRecord *> results;
        
        int numSkipped = 0;
        int id = mapSize - 1;
        
        while( id > 0 && numSkipped < inNumToSkip ) {
            if( idMap[id] != NULL ) {
                numSkipped++;
                }
            id--;
            }
        
        int numGotten = 0;
        while( id > 0 && numGotten < inNumToGet ) {
            if( idMap[id] != NULL ) {
                results.push_back( idMap[id] );
                numGotten++;
                }
            id--;
            }
        
        // rough estimate
        *outNumRemaining = id;
        
        if( *outNumRemaining < 100 ) {
            // close enough to end, actually compute it
            *outNumRemaining = 0;
            
            while( id > 0 ) {
                if( idMap[id] != NULL ) {
                    *outNumRemaining = *outNumRemaining + 1;
                    }
                id--;
                }
            }
        

        *outNumResults = results.size();
        return results.getElementArray();
        }
    

    char *lowerSearch = stringToLowerCase( inSearch );

    int numTotalMatches = tree.countMatches( lowerSearch );
        
    
    int numAfterSkip = numTotalMatches - inNumToSkip;

    if( numAfterSkip < 0 ) {
        numAfterSkip = 0;
        }
    
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
               double inVertContainRotationOffset,
               char inPermanent,
               int inMinPickupAge,
               char inHeldInHand,
               char inRideable,
               char inBlocksWalking,
               int inLeftBlockingRadius, int inRightBlockingRadius,
               char inDrawBehindPlayer,
               char *inBiomes,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               char inPersonNoSpawn,
               char inMale,
               int inRace,
               char inDeathMarker,
               char inHomeMarker,
               char inFloor,
               char inFloorHugging,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inUseDistance,
               SoundUsage inCreationSound,
               SoundUsage inUsingSound,
               SoundUsage inEatingSound,
               SoundUsage inDecaySound,
               char inCreationSoundInitialOnly,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
               char *inSlotVert,
               int *inSlotParent,
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
               int *inSpriteInvisibleWhenWorn,
               char *inSpriteBehindSlots,
               char *inSpriteIsHead,
               char *inSpriteIsBody,
               char *inSpriteIsBackFoot,
               char *inSpriteIsFrontFoot,
               int inNumUses,
               char *inSpriteUseVanish,
               char *inSpriteUseAppear,
               char inNoWriteToFile,
               int inReplaceID ) {
    
    if( inSlotTimeStretch < 0.0001 ) {
        inSlotTimeStretch = 0.0001;
        }
    
    int newID = inReplaceID;
    
    int newHeight = recomputeObjectHeight( inNumSprites,
                                           inSprites, inSpritePos );

    // add it to file structure
    File objectsDir( NULL, "objects" );
            
    if( ! inNoWriteToFile && !objectsDir.exists() ) {
        objectsDir.makeDirectory();
        }


    int nextObjectNumber = 1;
    
    if( objectsDir.exists() && objectsDir.isDirectory() ) {
                
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

            if( newID < maxID + 1 ) {
                newID = maxID + 1;
                }
            }

        delete nextNumberFile;
        }
    

    if( ! inNoWriteToFile && 
        objectsDir.exists() && objectsDir.isDirectory() ) {
        
        char *fileName = autoSprintf( "%d.txt", newID );


        File *objectFile = objectsDir.getChildFile( fileName );
        

        SimpleVector<char*> lines;
        
        lines.push_back( autoSprintf( "id=%d", newID ) );
        lines.push_back( stringDuplicate( inDescription ) );

        lines.push_back( autoSprintf( "containable=%d", (int)inContainable ) );
        lines.push_back( autoSprintf( "containSize=%d,vertSlotRot=%f", 
                                      (int)inContainSize,
                                      inVertContainRotationOffset ) );
        lines.push_back( autoSprintf( "permanent=%d,minPickupAge=%d", 
                                      (int)inPermanent,
                                      inMinPickupAge ) );
        
        
        int heldInHandNumber = 0;
        
        if( inHeldInHand ) {
            heldInHandNumber = 1;
            }
        if( inRideable ) {
            // override
            heldInHandNumber = 2;
            }

        lines.push_back( autoSprintf( "heldInHand=%d", heldInHandNumber ) );
        
        lines.push_back( autoSprintf( 
                             "blocksWalking=%d,"
                             "leftBlockingRadius=%d," 
                             "rightBlockingRadius=%d,"
                             "drawBehindPlayer=%d", 
                             (int)inBlocksWalking,
                             inLeftBlockingRadius, 
                             inRightBlockingRadius,
                             (int)inDrawBehindPlayer ) );
        
        lines.push_back( autoSprintf( "mapChance=%f#biomes_%s", 
                                      inMapChance, inBiomes ) );
        
        lines.push_back( autoSprintf( "heatValue=%d", inHeatValue ) );
        lines.push_back( autoSprintf( "rValue=%f", inRValue ) );

        int personNumber = 0;
        if( inPerson ) {
            personNumber = inRace;
            }
        lines.push_back( autoSprintf( "person=%d,noSpawn=%d", personNumber,
                                      (int)inPersonNoSpawn ) );
        lines.push_back( autoSprintf( "male=%d", (int)inMale ) );
        lines.push_back( autoSprintf( "deathMarker=%d", (int)inDeathMarker ) );
        lines.push_back( autoSprintf( "homeMarker=%d", (int)inHomeMarker ) );
        
        lines.push_back( autoSprintf( "floor=%d", (int)inFloor ) );
        lines.push_back( autoSprintf( "floorHugging=%d", 
                                      (int)inFloorHugging ) );

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

        lines.push_back( autoSprintf( "useDistance=%d", 
                                      inUseDistance ) );

        char *usageStrings[4] = 
            { stringDuplicate( printSoundUsage( inCreationSound ) ),
              stringDuplicate( printSoundUsage( inUsingSound ) ),
              stringDuplicate( printSoundUsage( inEatingSound ) ),
              stringDuplicate( printSoundUsage( inDecaySound ) ) };
        
        
        lines.push_back( autoSprintf( "sounds=%s,%s,%s,%s",
                                      usageStrings[0],
                                      usageStrings[1],
                                      usageStrings[2],
                                      usageStrings[3] ) );
        for( int i=0; i<4; i++ ) {
            delete [] usageStrings[i];
            }

        lines.push_back( autoSprintf( "creationSoundInitialOnly=%d", 
                                      (int)inCreationSoundInitialOnly ) );
        
        lines.push_back( autoSprintf( "numSlots=%d#timeStretch=%f", 
                                      inNumSlots, inSlotTimeStretch ) );
        lines.push_back( autoSprintf( "slotSize=%d", inSlotSize ) );

        for( int i=0; i<inNumSlots; i++ ) {
            lines.push_back( autoSprintf( "slotPos=%f,%f,vert=%d,parent=%d", 
                                          inSlotPos[i].x,
                                          inSlotPos[i].y,
                                          (int)( inSlotVert[i] ),
                                          inSlotParent[i] ) );
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


            lines.push_back( autoSprintf( "invisHolding=%d,invisWorn=%d,"
                                          "behindSlots=%d", 
                                          inSpriteInvisibleWhenHolding[i],
                                          inSpriteInvisibleWhenWorn[i],
                                          inSpriteBehindSlots[i] ) );

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
        
        
        lines.push_back( autoSprintf( "numUses=%d",
                                      inNumUses ) );
        
        lines.push_back(
            boolArrayToSparseCommaString( "useVanishIndex",
                                          inSpriteUseVanish, 
                                          inNumSprites ) );
        lines.push_back(
            boolArrayToSparseCommaString( "useAppearIndex",
                                          inSpriteUseAppear, 
                                          inNumSprites ) );

        lines.push_back( autoSprintf( "pixHeight=%d",
                                      newHeight ) );


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
        
        File *nextNumberFile = 
            objectsDir.getChildFile( "nextObjectNumber.txt" );

        nextNumberFile->writeToFile( nextNumberString );
        
        delete [] nextNumberString;
                
        
        delete nextNumberFile;
        }
    
    if( newID == -1 && ! inNoWriteToFile ) {
        // failed to save it to disk
        return -1;
        }
    
    
    if( newID == -1 ) {
        newID = maxID + 1;
        }

    
    // now add it to live, in memory database
    if( newID >= mapSize ) {
        // expand map

        int newMapSize = newID + 1;
        

        
        ObjectRecord **newMap = new ObjectRecord*[newMapSize];
        
        for( int i=mapSize; i<newMapSize; i++ ) {
            newMap[i] = NULL;
            }

        memcpy( newMap, idMap, sizeof(ObjectRecord*) * mapSize );

        delete [] idMap;
        idMap = newMap;
        mapSize = newMapSize;
        }
    

    if( newID > maxID ) {
        maxID = newID;
        }


    ObjectRecord *r = new ObjectRecord;
    
    r->id = newID;
    r->description = stringDuplicate( inDescription );

    r->containable = inContainable;
    r->containSize = inContainSize;
    r->vertContainRotationOffset = inVertContainRotationOffset;
    
    r->permanent = inPermanent;
    r->minPickupAge = inMinPickupAge;
    r->heldInHand = inHeldInHand;
    r->rideable = inRideable;
    
    if( r->heldInHand && r->rideable ) {
        r->heldInHand = false;
        }
    
    r->blocksWalking = inBlocksWalking;
    r->leftBlockingRadius = inLeftBlockingRadius;
    r->rightBlockingRadius = inRightBlockingRadius;
    r->drawBehindPlayer = inDrawBehindPlayer;
    
    r->wide = ( r->leftBlockingRadius > 0 || r->rightBlockingRadius > 0 );
    
    if( r->wide ) {
        r->drawBehindPlayer = true;
        
        if( r->leftBlockingRadius > maxWideRadius ) {
            maxWideRadius = r->leftBlockingRadius;
            }
        if( r->rightBlockingRadius > maxWideRadius ) {
            maxWideRadius = r->rightBlockingRadius;
            }
        }



    fillObjectBiomeFromString( r, inBiomes );
    
    
    r->mapChance = inMapChance;
    
    r->heatValue = inHeatValue;
    r->rValue = inRValue;

    r->person = inPerson;
    r->personNoSpawn = inPersonNoSpawn;
    r->race = inRace;
    r->male = inMale;
    r->deathMarker = inDeathMarker;
    r->homeMarker = inHomeMarker;
    r->floor = inFloor;
    r->floorHugging = inFloorHugging;
    r->foodValue = inFoodValue;
    r->speedMult = inSpeedMult;
    r->heldOffset = inHeldOffset;
    r->clothing = inClothing;
    r->clothingOffset = inClothingOffset;
    r->deadlyDistance = inDeadlyDistance;
    r->useDistance = inUseDistance;
    r->creationSound = copyUsage( inCreationSound );
    r->usingSound = copyUsage( inUsingSound );
    r->eatingSound = copyUsage( inEatingSound );
    r->decaySound = copyUsage( inDecaySound );
    r->creationSoundInitialOnly = inCreationSoundInitialOnly;

    r->numSlots = inNumSlots;
    r->slotSize = inSlotSize;
    
    r->slotPos = new doublePair[ inNumSlots ];
    r->slotVert = new char[ inNumSlots ];
    r->slotParent = new int[ inNumSlots ];
    
    memcpy( r->slotPos, inSlotPos, inNumSlots * sizeof( doublePair ) );
    memcpy( r->slotVert, inSlotVert, inNumSlots * sizeof( char ) );
    memcpy( r->slotParent, inSlotParent, inNumSlots * sizeof( int ) );
    
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
    r->spriteInvisibleWhenWorn = new int[ inNumSprites ];
    r->spriteBehindSlots = new char[ inNumSprites ];

    r->spriteIsHead = new char[ inNumSprites ];
    r->spriteIsBody = new char[ inNumSprites ];
    r->spriteIsBackFoot = new char[ inNumSprites ];
    r->spriteIsFrontFoot = new char[ inNumSprites ];

    r->numUses = inNumUses;
    r->spriteUseVanish = new char[ inNumSprites ];
    r->spriteUseAppear = new char[ inNumSprites ];
    
    r->useDummyIDs = NULL;
    r->isUseDummy = false;
    r->useDummyParent = 0;
    r->cachedHeight = newHeight;
    
    r->spriteSkipDrawing = new char[ inNumSprites ];

    r->apocalypseTrigger = false;
    if( r->description[0] == 'T' &&
        r->description[1] == 'h' &&
        strstr( r->description, "The Apocalypse" ) == r->description ) {
        
        printf( "Object id %d (%s) seen as an apocalypse trigger\n",
                r->id, r->description );

        r->apocalypseTrigger = true;
        }
    
    r->monumentStep = false;
    r->monumentDone = false;
    r->monumentCall = false;

    monumentCallObjectIDs.deleteElementEqualTo( newID );
    
    if( strstr( r->description, "monument" ) != NULL ) {
        // some kind of monument state
        if( strstr( r->description, "monumentStep" ) != NULL ) {
            r->monumentStep = true;
            }
        else if( strstr( r->description, 
                         "monumentDone" ) != NULL ) {
            r->monumentDone = true;
            }
        else if( strstr( r->description, 
                         "monumentCall" ) != NULL ) {
            r->monumentCall = true;
            monumentCallObjectIDs.push_back( newID );
            }
        }
    
    r->numVariableDummyIDs = 0;
    r->variableDummyIDs = NULL;
    r->isVariableDummy = false;

    
    memset( r->spriteSkipDrawing, false, inNumSprites );
    

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

    memcpy( r->spriteInvisibleWhenWorn, inSpriteInvisibleWhenWorn, 
            inNumSprites * sizeof( int ) );

    memcpy( r->spriteBehindSlots, inSpriteBehindSlots, 
            inNumSprites * sizeof( char ) );


    memcpy( r->spriteIsHead, inSpriteIsHead, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBody, inSpriteIsBody, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBackFoot, inSpriteIsBackFoot, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsFrontFoot, inSpriteIsFrontFoot, 
            inNumSprites * sizeof( char ) );


    memcpy( r->spriteUseVanish, inSpriteUseVanish, 
            inNumSprites * sizeof( char ) );
    memcpy( r->spriteUseAppear, inSpriteUseAppear, 
            inNumSprites * sizeof( char ) );
    
    
    // delete old

    // grab this before freeing, in case inDescription is the same as
    // idMap[newID].description
    char *lower = stringToLowerCase( inDescription );


    ObjectRecord *oldRecord = getObject( newID );
    
    SimpleVector<int> oldSoundIDs;
    if( oldRecord != NULL ) {
        
        for( int i=0; i<oldRecord->creationSound.numSubSounds; i++ ) {    
            oldSoundIDs.push_back( oldRecord->creationSound.ids[i] );
            }
        for( int i=0; i<oldRecord->usingSound.numSubSounds; i++ ) {    
            oldSoundIDs.push_back( oldRecord->usingSound.ids[i] );
            }
        for( int i=0; i<oldRecord->eatingSound.numSubSounds; i++ ) {    
            oldSoundIDs.push_back( oldRecord->eatingSound.ids[i] );
            }
        for( int i=0; i<oldRecord->decaySound.numSubSounds; i++ ) {    
            oldSoundIDs.push_back( oldRecord->decaySound.ids[i] );
            }
        }
    
    
    
    freeObjectRecord( newID );
    
    idMap[newID] = r;
    
    if( makeNewObjectsSearchable ) {    
        tree.insert( lower, idMap[newID] );
        }
    
    delete [] lower;
    
    personObjectIDs.deleteElementEqualTo( newID );
    femalePersonObjectIDs.deleteElementEqualTo( newID );

    for( int i=0; i<=MAX_RACE; i++ ) {
        racePersonObjectIDs[ i ].deleteElementEqualTo( newID );
        }
    
    if( r->person && ! r->personNoSpawn ) {    
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


    // check if sounds still used (prevent orphan sounds)

    for( int i=0; i<oldSoundIDs.size(); i++ ) {
        checkIfSoundStillNeeded( oldSoundIDs.getElementDirect( i ) );
        }
    
    
    return newID;
    }


static char logicalXOR( char inA, char inB ) {
    return !inA != !inB;
    }



static int objectLayerCutoff = -1;

void setObjectDrawLayerCutoff( int inCutoff ) {
    objectLayerCutoff = inCutoff;
    }



HoldingPos drawObject( ObjectRecord *inObject, int inDrawBehindSlots,
                       doublePair inPos,
                       double inRot, char inWorn, char inFlipH, double inAge,
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
    

    int backHandIndex = getBackHandIndex( inObject, inAge );
    
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
    
    
    int limit = inObject->numSprites;
    
    if( objectLayerCutoff > -1 && objectLayerCutoff < limit ) {
        limit = objectLayerCutoff;
        }
    objectLayerCutoff = -1;
    

    for( int i=0; i<limit; i++ ) {
        if( inObject->spriteSkipDrawing != NULL &&
            inObject->spriteSkipDrawing[i] ) {
            continue;
            }
        if( inObject->person &&
            ! isSpriteVisibleAtAge( inObject, i, inAge ) ) {    
            // skip drawing this aging layer entirely
            continue;
            }
        if( inObject->clothing != 'n' && 
            inObject->spriteInvisibleWhenWorn[i] != 0 ) {
            
            if( inWorn &&
                inObject->spriteInvisibleWhenWorn[i] == 1 ) {
        
                // skip invisible layer in worn clothing
                continue;
                }
            else if( ! inWorn &&
                     inObject->spriteInvisibleWhenWorn[i] == 2 ) {
                // skip invisible layer in unworn clothing
                continue;
                }
            }
        
        
        if( inDrawBehindSlots != 2 ) {    
            if( inDrawBehindSlots == 0 && 
                ! inObject->spriteBehindSlots[i] ) {
                continue;
                }
            else if( inDrawBehindSlots == 1 && 
                     inObject->spriteBehindSlots[i] ) {
                continue;
                }
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
                drawObject( inClothing.bottom, 2, 
                            bottomPos, bottomRot, true,
                            inFlipH, -1, 0, false, false, emptyClothing );
                }
            if( inClothing.tunic != NULL ) {
                drawObject( inClothing.tunic, 2,
                            tunicPos, tunicRot, true,
                            inFlipH, -1, 0, false, false, emptyClothing );
                }
            if( inClothing.backpack != NULL ) {
                drawObject( inClothing.backpack, 2, 
                            backpackPos, backpackRot,
                            true,
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
                
                if( getTotalGlobalFade() < 1 ) {
                    
                    toggleAdditiveTextureColoring( true );
                    
                    // alpha ignored for multiplicative blend
                    // but leave 0 there so that they won't add to stencil
                    setDrawColor( 0.0f, 0.0f, 0.0f, 0.0f );
                    }
                else {
                    // set 0 so translucent layers never add to stencil
                    setDrawFade( 0.0f );
                    }
                }
            
            drawSprite( getSprite( inObject->sprites[i] ), pos, inScale,
                        rot, 
                        logicalXOR( inFlipH, inObject->spriteHFlip[i] ) );
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                toggleAdditiveTextureColoring( false );
                }
            
            
            // this is the front-most drawn hand
            // in unanimated, unflipped object
            if( i == backHandIndex && ( inHideClosestArm == 0 ) 
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
            drawObject( inClothing.backShoe, 2,
                        backShoePos, backShoeRot, true,
                        inFlipH, -1, 0, false, false, emptyClothing );
            }
        else if( inClothing.frontShoe != NULL && i == frontFootIndex ) {
            drawObject( inClothing.backShoe, 2,
                        frontShoePos, frontShoeRot, true,
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
        
        drawObject( inClothing.hat, 2, cPos, inRot, true,
                    inFlipH, -1, 0, false, false, emptyClothing );
        }

    return returnHoldingPos;
    }



HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos, double inRot,
                       char inWorn, char inFlipH, double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       int inNumContained, int *inContainedIDs,
                       SimpleVector<int> *inSubContained ) {
    
    drawObject( inObject, 0, inPos, inRot, inWorn, inFlipH, inAge, 
                inHideClosestArm,
                inHideAllLimbs,
                inHeldNotInPlaceYet,
                inClothing );

    
    int numSlots = getNumContainerSlots( inObject->id );
    
    if( inNumContained > numSlots ) {
        inNumContained = numSlots;
        }
    
    for( int i=0; i<inNumContained; i++ ) {

        ObjectRecord *contained = getObject( inContainedIDs[i] );
        

        doublePair centerOffset = getObjectCenterOffset( contained );
        
        double rot = inRot;
        
        if( inObject->slotVert[i] ) {
            double rotOffset = 0.25 + contained->vertContainRotationOffset;

            if( inFlipH ) {
                centerOffset = rotate( centerOffset, - rotOffset * 2 * M_PI );
                rot -= rotOffset;
                }
            else {
                centerOffset = rotate( centerOffset, - rotOffset * 2 * M_PI );
                rot += rotOffset;
                }
            }


        doublePair slotPos = sub( inObject->slotPos[i], 
                                  centerOffset );
        
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
        



        if( inSubContained != NULL &&
            inSubContained[i].size() > 0 ) {
                
            // behind sub-contained
            drawObject( contained, 0, pos, rot, false, inFlipH, inAge,
                        0,
                        false,
                        false,
                        emptyClothing );

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
            drawObject( contained, 1, pos, rot, false, inFlipH, inAge,
                        0,
                        false,
                        false,
                        emptyClothing );

            }
        else {
            // no sub-contained
            // draw contained all at once
            drawObject( contained, 2, pos, rot, false, inFlipH, inAge,
                        0,
                        false,
                        false,
                        emptyClothing );
            }
        
        }
    
    return drawObject( inObject, 1, inPos, inRot, inWorn, inFlipH, inAge, 
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



char isSoundUsedByObject( int inSoundID ) {

    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {            
            if( doesUseSound( idMap[i]->creationSound, inSoundID ) ||
                doesUseSound( idMap[i]->usingSound, inSoundID ) ||
                doesUseSound( idMap[i]->eatingSound, inSoundID ) ||
                doesUseSound( idMap[i]->decaySound,  inSoundID ) ) {
                return true;
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



ObjectRecord *getClothingAdded( ClothingSet *inOldSet, 
                                ClothingSet *inNewSet ) {
    
    for( int i=0; i<=5; i++ ) {
        ObjectRecord *oldC = 
            clothingByIndex( *inOldSet, i );
        
        ObjectRecord *newC = 
            clothingByIndex( *inNewSet, i );
        
        if( newC != NULL &&
            newC != oldC ) {
            return newC;
            }
        }
    
    return NULL;
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



int getObjectHeight( int inObjectID ) {
    ObjectRecord *o = getObject( inObjectID );
    
    if( o == NULL ) {
        return 0;
        }
    
    if( o->cachedHeight == -1 ) {
        o->cachedHeight =
            recomputeObjectHeight( o->numSprites, o->sprites, o->spritePos );
        }
    
    return o->cachedHeight;
    }



int recomputeObjectHeight( int inNumSprites, int *inSprites,
                           doublePair *inSpritePos ) {        
    
    double maxH = 0;
    
    for( int i=0; i<inNumSprites; i++ ) {
        doublePair pos = inSpritePos[i];
                
        SpriteRecord *spriteRec = getSpriteRecord( inSprites[i] );
        
        int rad = 0;
        
        // don't count transparent sprites as part of height
        if( spriteRec != NULL && ! spriteRec->multiplicativeBlend ) {

            char hit = false;
            
            if( spriteRec->hitMap != NULL ) {
                int h = spriteRec->h;
                int w = spriteRec->w;
                char *hitMap = spriteRec->hitMap;
                
                for( int y=0; y<h; y++ ) {
                    for( int x=0; x<w; x++ ) {
                     
                        int p = y * spriteRec->w + x;
                        
                        if( hitMap[p] ) {
                            hit = true;
                            // can be negative if anchor above top
                            // pixel
                            rad = 
                                ( h/2 + spriteRec->centerAnchorYOffset )
                                - y;
                            break;
                            }
                        }
                    if( hit ) {
                        break;
                        }
                    }
                }
            else {
                rad = spriteRec->h / 2;
                }
            }
        
        double h = pos.y + rad;
        
        if( h > maxH ) {
            maxH = h;
            }
        }

    int returnH = lrint( maxH );
    
    return returnH;
    }




double getClosestObjectPart( ObjectRecord *inObject,
                             ClothingSet *inClothing,
                             SimpleVector<int> *inContained,
                             SimpleVector<int> *inClothingContained,
                             char inWorn,
                             double inAge,
                             int inPickedLayer,
                             char inFlip,
                             float inXCenterOffset, float inYCenterOffset,
                             int *outSprite,
                             int *outClothing,
                             int *outSlot,
                             char inConsiderTransparent ) {
    
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
    

    AnimationRecord *a = NULL;
    if( inWorn ) {
        a = getAnimation( inObject->id, held );
        }
    

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
                                                    // clothing is worn
                                                    // on body currently
                                                    true,
                                                    -1,
                                                    -1,
                                                    inFlip,
                                                    cOffset.x, cOffset.y,
                                                    &sp, &cl, &sl,
                                                    inConsiderTransparent );
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
        
        if( !inConsiderTransparent &&
            sr->multiplicativeBlend ){
            // skip this transparent sprite
            continue;
            }

        if( inObject->clothing != 'n' ) {
            if( inObject->spriteInvisibleWhenWorn[i] != 0 ) {
                
                if( inWorn && inObject->spriteInvisibleWhenWorn[i] == 1 ) {
                    // this layer invisible when worn
                    continue;
                    }
                else if( ! inWorn && 
                         inObject->spriteInvisibleWhenWorn[i] == 2 ) {
                    
                    // this layer invisible when NOT worn
                    continue;
                    }
                }
            }
        
        
        if( inFlip ) {
            offset = rotate( offset, -2 * M_PI * inObject->spriteRot[i] );
            }
        else {
            offset = rotate( offset, 2 * M_PI * inObject->spriteRot[i] );
            }
        

        if( a != NULL ) {
            // apply simplified version of animation
            // a still snapshot at t=0 (only look at phases)
            
            // this also ignores parent relationships for now

            // however, since this only applies for worn clothing, and trying
            // to make worn clothing properly clickable when it has a held
            // rotation, it will work most of the time, since most clothing
            // has one sprite, and for multi-sprite clothing, this should
            // at least capture the rotation of the largest sprite
            if( a->numSprites > i && a->spriteAnim[i].rotPhase != 0 ) {
                
                doublePair rotCenter = a->spriteAnim[i].rotationCenterOffset;
                
                if( inFlip ) {
                    rotCenter.x *= -1;
                    }

                if( inObject->spriteRot[i] != 0 ) {
                    if( inFlip ) {
                        rotCenter = 
                            rotate( rotCenter, 
                                    -2 * M_PI * inObject->spriteRot[i] );
                        }
                    else {
                        rotCenter = 
                            rotate( rotCenter, 
                                    2 * M_PI * inObject->spriteRot[i] );
                        }
                    }
                

                doublePair tempOffset = 
                    sub( offset, rotCenter );
                
                if( inFlip ) {
                    tempOffset =
                        rotate( tempOffset, 
                                - a->spriteAnim[i].rotPhase * 2 * M_PI );
                    }
                else {
                    tempOffset = 
                        rotate( tempOffset,
                                a->spriteAnim[i].rotPhase * 2 * M_PI );
                    }
                
                offset = add( tempOffset, rotCenter );
                }
            }


        
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

    char closestBehindSlots = false;
    
    if( *outSprite != -1 && inObject->spriteBehindSlots[ *outSprite ] ) {
        closestBehindSlots = true;
        }
    
    
    if( ( *outSprite == -1 || closestBehindSlots )
        && *outClothing == -1 && *outSlot == -1 ) {
        // consider slots
        
        if( closestBehindSlots ) {
            // consider only direct contianed 
            // object hits or slot placeholder hits
            smallestDist = 16;
            }
        
        
        for( int i=inObject->numSlots-1; i>=0; i-- ) {
            doublePair slotPos = inObject->slotPos[i];
            
            if( inFlip ) {
                slotPos.x *= -1;
                }
            
            if( inContained != NULL && i <inContained->size() ) {
                ObjectRecord *contained = 
                    getObject( inContained->getElementDirect( i ) );
            
                doublePair centOffset = getObjectCenterOffset( contained );
                
                if( inObject->slotVert[i] ) {
                    double rotOffset = 
                        0.25 + contained->vertContainRotationOffset;
                    if( inFlip ) {
                        centOffset = rotate( centOffset, 
                                             - rotOffset * 2 * M_PI );
                        centOffset.x *= -1;
                        }
                    else {
                        centOffset = rotate( centOffset, 
                                             - rotOffset * 2 * M_PI );
                        }
                    }
                else if( inFlip ) {
                    centOffset.x *= -1;
                    }
            
                slotPos = sub( slotPos, centOffset );

                doublePair slotOffset = sub( pos, slotPos );
                

                if( inObject->slotVert[i] ) {
                    double rotOffset = 
                        0.25 + contained->vertContainRotationOffset;
                    if( inFlip ) {
                        slotOffset = rotate( slotOffset, 
                                             - rotOffset * 2 * M_PI );
                        }
                    else {
                        slotOffset = rotate( slotOffset, 
                                             rotOffset * 2 * M_PI );
                        }
                    }
                
                int sp, cl, sl;
                
                getClosestObjectPart( contained,
                                      NULL,
                                      NULL,
                                      NULL,
                                      false,
                                      -1,
                                      -1,
                                      inFlip,
                                      slotOffset.x, slotOffset.y,
                                      &sp, &cl, &sl,
                                      inConsiderTransparent );
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
        
        if( closestBehindSlots && smallestDist == 16 ) {
            // hit no slot, stick with sprite behind
            smallestDist = 0;
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

    if( outList->size() >= 2 ) {
        
        int bodyIndex = getBodyIndex( inObject, inAge );
        
        // add shadows to list, which we can find based on
        // being lower than body and having no parent
        
        doublePair bodyPos = inObject->spritePos[ bodyIndex ];
        
        for( int i=0; i<inObject->numSprites; i++ ) {
            if( outList->getElementIndex( i ) == -1 ) {
                if( bodyPos.y > inObject->spritePos[i].y &&
                    inObject->spriteParent[i] == -1 ) {
                    
                    outList->push_back( i );
                    }
                }
            }
        }
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
                       



int compareBiomeInt( const void *inA, const void *inB ) {
    int *a = (int*)inA;
    int *b = (int*)inB;
    
    if( *a > *b ) {
        return 1;
        }
    if( *a < *b ) {
        return -1;
        }
    return 0;
    }




static SimpleVector<int> biomeCache;


void getAllBiomes( SimpleVector<int> *inVectorToFill ) {
    if( biomeCache.size() == 0 ) {
        
        for( int i=0; i<mapSize; i++ ) {
            if( idMap[i] != NULL ) {
            
                for( int j=0; j< idMap[i]->numBiomes; j++ ) {
                    int b = idMap[i]->biomes[j];
                    
                    if( biomeCache.getElementIndex( b ) == -1 ) {
                        biomeCache.push_back( b );
                        }
                    }
                }
            }

        // now sort it

        int *a = biomeCache.getElementArray();
        int num = biomeCache.size();
        
        qsort( a, num, sizeof(int), compareBiomeInt );

        biomeCache.deleteAll();
        biomeCache.appendArray( a, num );
        
        delete [] a;
        }
    
    for( int i=0; i<biomeCache.size(); i++ ) {
        inVectorToFill->push_back( biomeCache.getElementDirect( i ) );
        }
    }



doublePair getObjectCenterOffset( ObjectRecord *inObject ) {


    // find center of widest sprite

    SpriteRecord *widestRecord = NULL;
    
    int widestIndex = -1;
    int widestWidth = 0;
    double widestYPos = 0;
    
    for( int i=0; i<inObject->numSprites; i++ ) {
        SpriteRecord *sprite = getSpriteRecord( inObject->sprites[i] );
    
        if( sprite->multiplicativeBlend ) {
            // don't consider translucent sprites when computing wideness
            continue;
            }
        

        int w = sprite->visibleW;
        
        double rot = inObject->spriteRot[i];
        
        if( rot != 0 ) {
            double rotAbs = fabs( rot );
            
            // just the fractional part
            rotAbs -= floor( rotAbs );
            
            if( rotAbs == 0.25 || rotAbs == 0.75 ) {
                w = sprite->visibleH;
                }
            }


        if( widestRecord == NULL ||
            // wider than what we've seen so far
            w > widestWidth ||
            // or tied for wideness, and lower
            ( w == widestWidth &&
              inObject->spritePos[i].y < widestYPos ) ) {

            widestRecord = sprite;
            widestIndex = i;
            widestWidth = w;
            widestYPos = inObject->spritePos[i].y;
            }
        }
    

    if( widestRecord == NULL ) {
        doublePair result = { 0, 0 };
        return result;
        }
    
    
        
    doublePair centerOffset = { (double)widestRecord->centerXOffset,
                                (double)widestRecord->centerYOffset };
        
    centerOffset = rotate( centerOffset, 
                           2 * M_PI * inObject->spriteRot[widestIndex] );

    doublePair spriteCenter = add( inObject->spritePos[widestIndex], 
                                   centerOffset );

    return spriteCenter;
    
    }



int getMaxWideRadius() {
    return maxWideRadius;
    }



char isSpriteSubset( int inSuperObjectID, int inSubObjectID ) {
    ObjectRecord *superO = getObject( inSuperObjectID );
    ObjectRecord *subO = getObject( inSubObjectID );
    
    if( superO == NULL || subO == NULL ) {
        return false;
        }

    if( subO->numSprites == 0 ) {
        return true;
        }

    // allow global position adjustments, as long as all sub-sprites in same
    // relative position to each other
    
    int spriteSubSeroID = subO->sprites[0];
    doublePair spriteSubZeroPos = subO->spritePos[0];

    doublePair spriteSuperZeroPos;
    
    // find zero sprite in super

    char found = false;
    for( int ss=0; ss<superO->numSprites; ss++ ) {
        if( superO->sprites[ ss ] == spriteSubSeroID ) {
            found = true;
            spriteSuperZeroPos = superO->spritePos[ ss ];
            break;
            }
        }
    

    if( !found ) {
        return false;
        }

    
    for( int s=0; s<subO->numSprites; s++ ) {
        int spriteID = subO->sprites[s];
        
        doublePair spritePosRel = sub( subO->spritePos[s],
                                       spriteSubZeroPos );

        double spriteRot = subO->spriteRot[s];
        
        char spriteHFlip = subO->spriteHFlip[s];

        // ignore sprite color for now
        //FloatRGB spriteColor = subO->spriteColor[s];

        char found = false;
        
        for( int ss=0; ss<superO->numSprites; ss++ ) {
            if( superO->sprites[ ss ] == spriteID &&
                equal( sub( superO->spritePos[ ss ],
                            spriteSuperZeroPos ), spritePosRel ) &&
                superO->spriteRot[ ss ] == spriteRot &&
                superO->spriteHFlip[ ss ] == spriteHFlip 
                /* &&
                   equal( superO->spriteColor[ ss ], spriteColor ) */ ) {
                

                found = true;
                break;
                }
            }

        if( !found ) {
            return false;
            }
        }
    
    return true;
    }




char equal( FloatRGB inA, FloatRGB inB ) {
    return 
        inA.r == inB.r &&
        inA.g == inB.g &&
        inA.b == inB.b;
    }

        

void getArmHoldingParameters( ObjectRecord *inHeldObject,
                              int *outHideClosestArm,
                              char *outHideAllLimbs ) {
    *outHideClosestArm = 0;
    *outHideAllLimbs = false;
    
    if( inHeldObject != NULL ) {
                    
        if( inHeldObject->heldInHand ) {
            *outHideClosestArm = 0;
            }
        else if( inHeldObject->rideable ) {
            *outHideClosestArm = 0;
            *outHideAllLimbs = true;
            }
        else {
            // try hiding no arms, but freezing them instead
            // -2 means body position still returned as held pos
            // instead of hand pos
            *outHideClosestArm = -2;
            *outHideAllLimbs = false;
            }
        }

    }



void computeHeldDrawPos( HoldingPos inHoldingPos, doublePair inPos,
                         ObjectRecord *inHeldObject,
                         char inFlipH,
                         doublePair *outHeldDrawPos, double *outHeldDrawRot ) {
    
    doublePair holdPos;
    double holdRot = 0;
    
    if( inHoldingPos.valid ) {
        holdPos = inHoldingPos.pos;
        }
    else {
        holdPos = inPos;
        }

        

        
    if( inHeldObject != NULL ) {
        
        doublePair heldOffset = inHeldObject->heldOffset;
        
        
        if( !inHeldObject->person ) {    
            heldOffset = sub( heldOffset, 
                              getObjectCenterOffset( inHeldObject ) );
            }
        
        if( inFlipH ) {
            heldOffset.x *= -1;
            }

        
        if( inHoldingPos.valid && inHoldingPos.rot != 0  &&
            ! inHeldObject->rideable ) {
            
            if( inFlipH ) {
                heldOffset = 
                    rotate( heldOffset, 
                            2 * M_PI * inHoldingPos.rot );
                }
            else {
                heldOffset = 
                    rotate( heldOffset, 
                            -2 * M_PI * inHoldingPos.rot );
                }
                        
            if( inFlipH ) {
                holdRot = -inHoldingPos.rot;
                }
            else {
                holdRot = inHoldingPos.rot;
                }

            if( holdRot > 1 ) {
                while( holdRot > 1 ) {
                    holdRot -= 1;
                    }
                }
            else if( holdRot < -1 ) {
                while( holdRot < -1 ) {
                    holdRot += 1;
                    }
                }
            }

        holdPos.x += heldOffset.x;
        holdPos.y += heldOffset.y;        
        }


    *outHeldDrawPos = holdPos;
    *outHeldDrawRot = holdRot;
    }




char bothSameUseParent( int inAObjectID, int inBObjectID ) {
    ObjectRecord *a = getObject( inAObjectID );
    ObjectRecord *b = getObject( inBObjectID );
    

    if( a != NULL && b != NULL ) {
        
        if( a->isUseDummy && b->isUseDummy ) {
            if( a->useDummyParent == b->useDummyParent ) {
                return true;
                }
            }
        if( ! a->isUseDummy && b->isUseDummy ) {
            return ( b->useDummyParent == inAObjectID );
            }
        if( a->isUseDummy && ! b->isUseDummy ) {
            return ( a->useDummyParent == inBObjectID );
            }
        }

    return false;
    }








