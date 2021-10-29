
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


#include "spriteDrawColorOverride.h"


// supplied by animation bank
extern void checkDrawPos( int inObjectID, doublePair inPos );




static int mapSize;
// maps IDs to records
// sparse, so some entries are NULL
static ObjectRecord **idMap;


// what object to return
static int defaultObjectID = -1;


static StringTree tree;


// track objects that are marked with the person flag
static SimpleVector<int> personObjectIDs;

// track female people
static SimpleVector<int> femalePersonObjectIDs;

// track monument calls
static SimpleVector<int> monumentCallObjectIDs;

// track death markers
static SimpleVector<int> deathMarkerObjectIDs;


// an extended list including special-case death markers 
// (marked with fromDeath in description)
static SimpleVector<int> allPossibleDeathMarkerIDs;

static SimpleVector<int> allPossibleFoodIDs;


static SimpleVector<TapoutRecord> tapoutRecords;


static int maxFoodValue = 0;




typedef struct GlobalTrigger {
        // meta trigger objects that are turned ON globally (triggering
        // all corresponding transitions)
        // when this trigger object comes into existence
        int onTriggerID;
    } GlobalTrigger;


SimpleVector<GlobalTrigger> globalTriggers;


int getNumGlobalTriggers() {
    return globalTriggers.size();
    }


int getMetaTriggerObject( int inTriggerIndex ) {
    return globalTriggers.getElementDirect( inTriggerIndex ).onTriggerID;
    }



typedef struct ToolSetRecord {
        // can be null for a single-tool set
        char *setTag;
        
        SimpleVector<int> setMembership;
    } ToolSetRecord;


SimpleVector<ToolSetRecord> toolSetRecords;


// inSetTage destroyed interally, NOT by caller
static int getToolSetIndex( char *inSetTag = NULL ) {
    if( inSetTag != NULL ) {
        
        for( int i=0; i<toolSetRecords.size(); i++ ) {
            ToolSetRecord *r = toolSetRecords.getElement( i );
            
            if( r->setTag != NULL ) {
                
                if( strcmp( inSetTag, r->setTag ) == 0 ) {
                    delete [] inSetTag;
                    return i;
                    }
                }
            }
        }
    
    // need to return a new record
    ToolSetRecord r = { inSetTag };
    
    toolSetRecords.push_back( r );
    
    return toolSetRecords.size() - 1;
    }


static void addToolSetMembership( int inToolSetIndex, int inObjectID ) {
    ToolSetRecord *r = toolSetRecords.getElement( inToolSetIndex );
    
    r->setMembership.push_back( inObjectID );
    }





// anything above race 100 is put in bin for race 100
#define MAX_RACE 100

static SimpleVector<int> racePersonObjectIDs[ MAX_RACE + 1 ];

static SimpleVector<int> raceList;


// allocated space that we can use when temporarily manipulating
// an object's skipDrawing array
static int skipDrawingWorkingAreaSize = -1;
static char *skipDrawingWorkingArea = NULL;


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



static char shouldFileBeCached( char *inFileName ) {
    if( strstr( inFileName, ".txt" ) != NULL &&
        strstr( inFileName, "groundHeat_" ) == NULL &&
        strcmp( inFileName, "nextObjectNumber.txt" ) != 0 ) {
        return true;
        }
    return false;
    }



static char autoGenerateUsedObjects = false;
static char autoGenerateVariableObjects = false;


int initObjectBankStart( char *outRebuildingCache, 
                         char inAutoGenerateUsedObjects,
                         char inAutoGenerateVariableObjects ) {
    maxID = 0;

    currentFile = 0;
    

    cache = initFolderCache( "objects", outRebuildingCache,
                             shouldFileBeCached );

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



static void setupEyesAndMouth( ObjectRecord *inR ) {
    ObjectRecord *r = inR;
    
    r->spriteIsEyes = new char[ r->numSprites ];
    r->spriteIsMouth = new char[ r->numSprites ];

    memset( r->spriteIsEyes, false, r->numSprites );
    memset( r->spriteIsMouth, false, r->numSprites );

    r->mainEyesOffset.x = 0;
    r->mainEyesOffset.y = 0;

    if( r->person && isSpriteBankLoaded() ) {
        
        int headIndex = getHeadIndex( r, 30 );

        for( int i = 0; i < r->numSprites; i++ ) {
            char *tag = getSpriteTag( r->sprites[i] );
            
            if( tag != NULL && strstr( tag, "Eyes" ) != NULL ) {
                r->spriteIsEyes[ i ] = true;
                }
            if( tag != NULL && strstr( tag, "Mouth" ) != NULL ) {
                r->spriteIsMouth[ i ] = true;
                }

            if( r->spriteIsEyes[i] && 
                r->spriteAgeStart[i] < 30 && r->spriteAgeEnd[i] > 30 ) {
                
                r->mainEyesOffset = 
                    sub( r->spritePos[i], r->spritePos[ headIndex ] );
                }
            }
        } 
    }




static void setupObjectWritingStatus( ObjectRecord *inR ) {
    inR->mayHaveMetadata = false;
                
    inR->written = false;
    inR->writable = false;
                
    if( strstr( inR->description, "&" ) != NULL ) {
        // some flags in name
        if( strstr( inR->description, "&written" ) != NULL ) {
            inR->written = true;
            inR->mayHaveMetadata = true;
            }
        if( strstr( inR->description, "&writable" ) != NULL ) {
            inR->writable = true;
            // writable objects don't have metadata yet
            // inR->mayHaveMetadata = true;
            }
        }
    }



static void setupObjectGlobalTriggers( ObjectRecord *inR ) {
    inR->isGlobalTriggerOn = false;
    inR->isGlobalTriggerOff = false;
    inR->isGlobalReceiver = false;

    inR->globalTriggerIndex = -1;

    if( strstr( inR->description, "*" ) != NULL ) {
        inR->isGlobalTriggerOn = true;        
        }
    else if( strstr( inR->description, "!" ) != NULL ) {
        inR->isGlobalTriggerOff = true;
        }

    char *carrotPos = strstr( inR->description, ">" );
    if( carrotPos != NULL && carrotPos != inR->description ) {
        // trigger receiver, and NOT a meta trigger object
        // (meta trigger objects start with '>')
        inR->isGlobalReceiver = true;
        }
    }



static int maxSpeechPipeIndex = 0;


static void setupObjectSpeechPipe( ObjectRecord *inR ) {
    inR->speechPipeIn = false;
    inR->speechPipeOut = false;
    
    inR->speechPipeIndex = -1;

    if( strstr( inR->description, "speech" ) == NULL ) {
        return;
        }

    char *inLoc = strstr( inR->description, "speechIn_" );
    if( inLoc != NULL ) {
        inR->speechPipeIn = true;        
        
        char *indexLoc = &( inLoc[ strlen( "speechIn_" ) ] );
        
        sscanf( indexLoc, "%d", &( inR->speechPipeIndex ) );
        }
    else {
        
        char *outLoc = strstr( inR->description, "speechOut_" );
        if( outLoc != NULL ) {
            inR->speechPipeOut = true;
            
            char *indexLoc = &( outLoc[ strlen( "speechOut_" ) ] );
        
            sscanf( indexLoc, "%d", &( inR->speechPipeIndex ) );
            }
        }

    if( inR->speechPipeIndex > maxSpeechPipeIndex ) {
        maxSpeechPipeIndex = inR->speechPipeIndex;
        }
    }



static void setupFlight( ObjectRecord *inR ) {
    inR->isFlying = false;
    inR->isFlightLanding = false;

    char *flyPos = strstr( inR->description, "+fly" );
    if( flyPos != NULL ) {
        inR->isFlying = true;
        }
    else {
        char *landPos = strstr( inR->description, "+land" );
        if( landPos != NULL ) {
            inR->isFlightLanding = true;
            }
        }
    }



static void setupOwned( ObjectRecord *inR ) {
    inR->isOwned = false;
    inR->isTempOwned = false;
    inR->isFollowerOwned = false;
    
    char *ownedPos = strstr( inR->description, "+owned" );
    if( ownedPos != NULL ) {
        inR->isOwned = true;
        }

    char *tempOwnedPos = strstr( inR->description, "+tempOwned" );
    if( tempOwnedPos != NULL ) {
        inR->isOwned = true;
        inR->isTempOwned = true;
        }

    char *followerOwnedPos = strstr( inR->description, "+followerOwned" );
    if( followerOwnedPos != NULL ) {
        inR->isOwned = true;
        inR->isFollowerOwned = true;
        }

    }



static void setupNoHighlight( ObjectRecord *inR ) {
    inR->noHighlight = false;
    
    if( strstr( inR->description, "+noHighlight" ) != NULL ) {
        inR->noHighlight = true;
        }
    }



static void setupMaxPickupAge( ObjectRecord *inR ) {
    inR->maxPickupAge = 9999999;
    

    const char *key = "maxPickupAge_";
    
    char *loc = strstr( inR->description, key );

    if( loc != NULL ) {
        
        char *indexLoc = &( loc[ strlen( key ) ] );
        
        sscanf( indexLoc, "%d", &( inR->maxPickupAge ) );
        }
    }



static void setupWall( ObjectRecord *inR ) {
    inR->wallLayer = inR->floorHugging;
    inR->frontWall = false;

    if( ! inR->wallLayer ) {    
        char *wallPos = strstr( inR->description, "+wall" );
        if( wallPos != NULL ) {
            inR->wallLayer = true;
            }
        }
    
    if( inR->wallLayer ) {
        char *noWallPos = strstr( inR->description, "-wall" );
        if( noWallPos != NULL ) {
            inR->wallLayer = false;
            }
        else {
            char *frontWallPos = strstr( inR->description, "+frontWall" );
            if( frontWallPos != NULL ) {
                inR->frontWall = true;
                }
            }
        }
    }



static void setupTapout( ObjectRecord *inR ) {
    inR->isTapOutTrigger = false;
    
    if( inR->isUseDummy || inR->isVariableDummy ) {
        // only parent object counts tapouts
        return;
        }
    

    char *triggerPos = strstr( inR->description, "+tapoutTrigger" );
                
    if( triggerPos != NULL ) {
        int xGrid, yGrid;
        int xLimit, yLimit;
        int buildCountLimit = -1;
        int postBuildLimitX = 0;
        int postBuildLimitY = 0;
        
        int numRead = sscanf( triggerPos, 
                              "+tapoutTrigger,%d,%d,%d,%d,"
                              "%d,%d,%d",
                              &xGrid, &yGrid,
                              &xLimit, &yLimit,
                              &buildCountLimit,
                              &postBuildLimitX,
                              &postBuildLimitY );
        if( numRead == 4 || numRead == 7 ) {
            // valid tapout trigger
            TapoutRecord r;
            
            r.triggerID = inR->id;
            r.gridSpacingX = xGrid;
            r.gridSpacingY = yGrid;
            r.limitX = xLimit;
            r.limitY = yLimit;
            
            r.buildCountLimit = buildCountLimit;
            r.buildCount = 0;
            r.postBuildLimitX = postBuildLimitX;
            r.postBuildLimitY = postBuildLimitY;
            
            tapoutRecords.push_back( r );
            
            inR->isTapOutTrigger = true;
            }
        }
    }




static void setupToolSet( ObjectRecord *inR ) {
    inR->toolSetIndex = -1;
    inR->toolLearned = false;    
    
    char *toolPos = strstr( inR->description, "+tool" );
                
    if( toolPos != NULL ) {
        
        char *skipPos = &( toolPos[5] );
        
        char *setTag = NULL;

        if( skipPos[0] != ' ' &&
            skipPos[0] != '\0' ) {
            
            char tag[100];

            int numRead = sscanf( skipPos, "%99s", tag );
            
            if( numRead == 1 ) {
                setTag = stringDuplicate( tag );
                }
            }
        
        inR->toolSetIndex = getToolSetIndex( setTag );
        addToolSetMembership( inR->toolSetIndex, inR->id );
        }
    }



static void setupDefaultObject( ObjectRecord *inR ) {
    if( strstr( inR->description, "+default" ) ) {
        defaultObjectID = inR->id;
        }
    }



static void setupAutoDefaultTrans( ObjectRecord *inR ) {
    inR->autoDefaultTrans = false;

    char *pos = strstr( inR->description, "+autoDefaultTrans" );
    if( pos != NULL ) {
        inR->autoDefaultTrans = true;
        }
    }


static void setupNoBackAccess( ObjectRecord *inR ) {
    inR->noBackAccess = false;

    char *pos = strstr( inR->description, "+noBackAccess" );
    if( pos != NULL ) {
        inR->noBackAccess = true;
        }
    }


static void setupAlcohol( ObjectRecord *inR ) {
    inR->alcohol = 0;

    char *pos = strstr( inR->description, "+alcohol" );

    if( pos != NULL ) {
        
        sscanf( pos, "+alcohol%d", &( inR->alcohol ) );
        }
    }


static void setupBlocksMoving( ObjectRecord *inR ) {
    inR->blocksMoving = false;
    
    if( inR->blocksWalking ) {
        inR->blocksMoving = true;
        return;
        }
    
    char *pos = strstr( inR->description, "+blocksMoving" );

    if( pos != NULL ) {
        inR->blocksMoving = true;
        }
    }



static void setupBlocksNonFollower( ObjectRecord *inR ) {
    inR->blocksNonFollower = false;    
    
    char *pos = strstr( inR->description, "+blocksNonFollower" );

    if( pos != NULL ) {
        inR->blocksNonFollower = true;
        }
    }



static void setupBadgePos( ObjectRecord *inR ) {
    inR->hasBadgePos = false;    
    inR->badgePos.x = 0;
    inR->badgePos.y = 0;
    
    char *pos = strstr( inR->description, "+badgePos" );

    if( pos != NULL ) {
        inR->hasBadgePos = true;
        
        sscanf( pos, "+badgePos%lf,%lf", 
                &( inR->badgePos.x ),
                &( inR->badgePos.y ) );
        }
    }



static void setupFamHomeland( ObjectRecord *inR ) {
    inR->famUseDist = 0;

    char *pos = strstr( inR->description, "+famUse" );

    if( pos != NULL ) {
        
        sscanf( pos, "+famUse%d", &( inR->famUseDist ) );
        }
    }


static void setupForcedBiome( ObjectRecord *inR ) {
    inR->forceBiome = -1;

    char *pos = strstr( inR->description, "+biomeSet" );

    if( pos != NULL ) {
        
        sscanf( pos, "+biomeSet%d", &( inR->forceBiome ) );
        }
    }



static void setupExpertFind( ObjectRecord *inR ) {
    inR->expertFind = false;

    char *pos = strstr( inR->description, "+expertFind" );

    if( pos != NULL ) {
        inR->expertFind = true;
        }
    }



static void setupNormalOnly( ObjectRecord *inR ) {
    inR->normalOnly = false;

    char *pos = strstr( inR->description, "+normalOnly" );

    if( pos != NULL ) {
        inR->normalOnly = true;
        }
    }


static void setupYumParent( ObjectRecord *inR ) {
    inR->yumParentID = -1;

    char *pos = strstr( inR->description, "+yum" );

    if( pos != NULL ) {
        sscanf( pos, "+yum%d", &( inR->yumParentID ) );
        }
    }


static void setupRoadParent( ObjectRecord *inR ) {
    inR->roadParentID = -1;

    char *pos = strstr( inR->description, "+road" );

    if( pos != NULL ) {
        sscanf( pos, "+road%d", &( inR->roadParentID ) );
        }

    // also deal with noCover
    inR->noCover = false;
    pos = strstr( inR->description, "+noCover" );
    if( pos != NULL ) {
        inR->noCover = true;
        }
    }



static void setupSlotsInvis( ObjectRecord *inR ) {
    inR->slotsInvis = false;
    char *pos = strstr( inR->description, "+slotsInvis" );

    if( pos != NULL ) {
        inR->slotsInvis = true;    
        }
    }



static void setupVarSerialNumber( ObjectRecord *inR ) {
    inR->useVarSerialNumbers = false;
    char *pos = strstr( inR->description, "+varSerialNumber" );

    if( pos != NULL ) {
        inR->useVarSerialNumbers = true;    
        }
    }



static void setupVarIsNumeral( ObjectRecord *inR ) {
    inR->varIsNumeral = false;
    char *pos = strstr( inR->description, "+varNumeral" );

    if( pos != NULL ) {
        inR->varIsNumeral = true;    
        }
    }



static void setupHideHead( ObjectRecord *inR ) {
    inR->hideHead = false;    
    
    char *pos = strstr( inR->description, "+hideHead" );

    if( pos != NULL ) {
        inR->hideHead = true;
        }
    }



static void setupHideRider( ObjectRecord *inR ) {
    inR->hideRider = false;    
    
    char *pos = strstr( inR->description, "+hideRider" );

    if( pos != NULL ) {
        inR->hideRider = true;
        }
    }



static void setupNeverDrop( ObjectRecord *inR ) {
    inR->neverDrop = false;    
    
    char *pos = strstr( inR->description, "+neverDrop" );

    if( pos != NULL ) {
        inR->neverDrop = true;
        }
    }


static void setupGiveClue( ObjectRecord *inR ) {
    inR->giveClue = false;    
    
    char *pos = strstr( inR->description, "+giveClue" );

    if( pos != NULL ) {
        inR->giveClue = true;
        }
    }
    



int getMaxSpeechPipeIndex() {
    return maxSpeechPipeIndex;
    }



float initObjectBankStep() {
        
    if( currentFile == cache.numFiles ) {
        return 1.0;
        }
    
    int i = currentFile;

                
    char *txtFileName = getFileName( cache, i );
            
    if( shouldFileBeCached( txtFileName ) ) {
                            
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
                         

                setupObjectWritingStatus( r );
                
                setupObjectGlobalTriggers( r );
                
                setupObjectSpeechPipe( r );
                
                setupFlight( r );
                
                setupOwned( r );
                
                setupNoHighlight( r );
                
                setupMaxPickupAge( r );
                
                setupAutoDefaultTrans( r );
                
                setupNoBackAccess( r );                

                setupAlcohol( r );
                
                setupFamHomeland( r );
                
                setupForcedBiome( r );
                
                setupExpertFind( r );

                setupNormalOnly( r );

                setupYumParent( r );
                
                setupRoadParent( r );
                
                setupSlotsInvis( r );
                
                setupVarSerialNumber( r );

                setupVarIsNumeral( r );


                // do this later, after we parse floorHugging
                // setupWall( r );
                

                r->isAutoOrienting = false;
                r->causeAutoOrientHOnly = false;
                r->causeAutoOrientVOnly = false;
                r->horizontalVersionID = -1;
                r->verticalVersionID = -1;
                r->cornerVersionID = -1;

                next++;
                            
                int contRead = 0;                            
                sscanf( lines[next], "containable=%d", 
                        &( contRead ) );
                            
                r->containable = contRead;
                            
                next++;
                    
                r->containSize = 1;
                r->vertContainRotationOffset = 0;
                
                sscanf( lines[next], "containSize=%f,vertSlotRot=%lf", 
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



                r->noFlip = false;

                if( strstr( lines[next], "noFlip=" ) != NULL ) {
                    int noFlipRead = 0;
                    
                    sscanf( lines[next], "noFlip=%d", 
                        &( noFlipRead ) );
                            
                    r->noFlip = noFlipRead;

                    next++;
                    }

                r->sideAccess = false;

                if( strstr( lines[next], "sideAccess=" ) != NULL ) {
                    int sideAccessRead = 0;
                    
                    sscanf( lines[next], "sideAccess=%d", 
                        &( sideAccessRead ) );
                            
                    r->sideAccess = sideAccessRead;

                    next++;
                    }
                



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

                
                setupBlocksMoving( r );
                setupBlocksNonFollower( r );
                setupBadgePos( r );
                setupHideHead( r );
                setupHideRider( r );
                setupNeverDrop( r );
                setupGiveClue( r );
                
                
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
                
                if( r->deathMarker ) {
                    deathMarkerObjectIDs.push_back( r->id );
                    }

                next++;

                
                if( strstr( r->description, "fromDeath" ) != NULL ) {
                    allPossibleDeathMarkerIDs.push_back( r->id );
                    }
                

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


                setupWall( r );

                            
                sscanf( lines[next], "foodValue=%d", 
                        &( r->foodValue ) );
                
                if( r->foodValue > maxFoodValue ) {
                    maxFoodValue = r->foodValue;
                    }
                
                if( r->foodValue > 0 ) {
                    allPossibleFoodIDs.push_back( r->id );
                    }

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
                
                if( strstr( lines[next], 
                            "creationSoundForce=" ) != NULL ) {
                    // flag present
                    
                    int flagRead = 0;                            
                    sscanf( lines[next], "creationSoundForce=%d", 
                            &( flagRead ) );
                    
                    r->creationSoundForce = flagRead;
                            
                    next++;
                    }
                else {
                    r->creationSoundForce = 0;
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
                sscanf( lines[next], "slotSize=%f", 
                        &( r->slotSize ) );
                            
                next++;

                r->slotsLocked = 0;
                if( strstr( lines[next], 
                            "slotsLocked=" ) != NULL ) {
                    // flag present
                    
                    int flagRead = 0;                            
                    sscanf( lines[next], "slotsLocked=%d", 
                            &( flagRead ) );
                    
                    r->slotsLocked = flagRead;
                            
                    next++;
                    }
                
                
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
                r->spriteInvisibleWhenContained = new char[ r->numSprites ];


                r->spriteIsHead = new char[ r->numSprites ];
                r->spriteIsBody = new char[ r->numSprites ];
                r->spriteIsBackFoot = new char[ r->numSprites ];
                r->spriteIsFrontFoot = new char[ r->numSprites ];
                

                memset( r->spriteIsHead, false, r->numSprites );
                memset( r->spriteIsBody, false, r->numSprites );
                memset( r->spriteIsBackFoot, false, r->numSprites );
                memset( r->spriteIsFrontFoot, false, r->numSprites );
                
                
                r->numUses = 1;
                r->useChance = 1.0f;
                
                r->spriteUseVanish = new char[ r->numSprites ];
                r->spriteUseAppear = new char[ r->numSprites ];
                r->useDummyIDs = NULL;
                r->isUseDummy = false;
                r->useDummyParent = 0;
                r->thisUseDummyIndex = -1;
                
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
                r->variableDummyParent = 0;
                r->thisVariableDummyIndex = -1;
                r->isVariableHidden = false;
                

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
                    
                    if( strstr( lines[next], "invisCont=" ) != NULL ) {
                        invisRead = 0;
                        sscanf( lines[next], "invisCont=%d", &invisRead );
                        
                        r->spriteInvisibleWhenContained[i] = invisRead;
                        next++;
                        }
                    else {
                        r->spriteInvisibleWhenContained[i] = 0;
                        }
                    }
                

                r->anySpritesBehindPlayer = false;
                r->spriteBehindPlayer = NULL;

                if( strstr( lines[next], "spritesDrawnBehind=" ) != NULL ) {
                    r->anySpritesBehindPlayer = true;
                    r->spriteBehindPlayer = new char[ r->numSprites ];
                    memset( r->spriteBehindPlayer, false, r->numSprites );
                    sparseCommaLineToBoolArray( "spritesDrawnBehind", 
                                                lines[next],
                                                r->spriteBehindPlayer, 
                                                r->numSprites );
                    next++;
                    }
                

                r->spriteAdditiveBlend = NULL;

                if( strstr( lines[next], "spritesAdditiveBlend=" ) != NULL ) {
                    r->spriteAdditiveBlend = new char[ r->numSprites ];
                    memset( r->spriteAdditiveBlend, false, r->numSprites );
                    sparseCommaLineToBoolArray( "spritesAdditiveBlend", 
                                                lines[next],
                                                r->spriteAdditiveBlend, 
                                                r->numSprites );
                    next++;
                    }

                    
                r->spriteNoFlipXPos = NULL;


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
                
                
                setupEyesAndMouth( r );


                if( next < numLines ) {
                    // info about num uses and vanish/appear sprites
                    
                    sscanf( lines[next], "numUses=%d,%f", 
                            &( r->numUses ),
                            &( r->useChance ) );
                            
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
                
                r->toolSetIndex = -1;
                
                r->isBiomeLimited = false;
                r->permittedBiomeMap = NULL;
                    
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



// turns a variable object number into a non-numeric label
// A, B, C, ... AA, AB, AC, etc.
// includes - AAZ hyphen offset character
static char *getVarObjectLabel( int inNumber ) {
    // in number starts at 1
    inNumber -= 1;
    
    SimpleVector<char> digits;
    
    int numLeft = inNumber;

    if( numLeft == 0 ) {
        digits.push_front( 'A' );
        }
    
    while( numLeft > 0 ) {
        int digitNumber = numLeft % 26;
        
        char digit = 'A' + digitNumber;
        
        digits.push_front( digit );
        numLeft -= digitNumber;
        
        if( numLeft == 26 ) {
            digits.push_front( 'A' );
            }
        numLeft /= 26;
        numLeft -= 1;
        }

    digits.push_front( ' ' );
    digits.push_front( '-' );
    
    return digits.getElementString();
    }



// turns a variable object number into a numeral label
// with a certain number of digits
// 01  02   03  .. 37
// includes - 01 hyphen offset character
static char *getVarObjectNumeral( int inNumber, int inMax ) {

    const char *formatString;
    
    if( inMax < 10 ) {
        formatString = "- %d";
        }
    else if( inMax > 9 && inMax < 100 ) {
        formatString = "- %02d";
        }
    else if( inMax > 99 && inMax < 1000 ) {
        formatString = "- %03d";
        }
    else if( inMax > 999 && inMax < 10000 ) {
        formatString = "- %04d";
        }
    else if( inMax > 9999 && inMax < 100000 ) {
        formatString = "- %05d";
        }
    else {
        // really big number?
        formatString = "- %d";
        }

    return autoSprintf( formatString, inNumber );
    }




// returns NULL if not found
static int *parseNumberList( char *inString, 
                             const char *inListKey, int *outNum ) {
    char *keyPos = strstr( inString, inListKey );
    
    if( keyPos == NULL ) {
        return NULL;
        }
    
    char *listStart = &keyPos[ strlen( inListKey ) ];
    int numParts = 0;
    
    char **parts = split( listStart, ",", &numParts );
    

    int *list = new int[ numParts ];
    
    for( int i=0; i<numParts; i++ ) {
        list[i] = 0;
        sscanf( parts[i], "%d", &( list[i] ) );
        delete [] parts[i];
        }
    delete [] parts;
    
    *outNum = numParts;
    return list;
    }



// counts objects containing a sprite that is not used by any other object
// (or the first object in the list to use the sprite)
static void countVisuallyUniqueObjects() {
    
    int uniqueCount = 0;

    SimpleVector<int> uniqueList;
    
    
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            
            ObjectRecord *rI = idMap[i];

            int unique = false;
            
            for( int s=0; s < rI->numSprites; s++ ) {
                int sID = rI->sprites[s];
            
                
                unique = true;
                
                // look at objects that we've already checked
                // is this sprite unique amoung them?
                // if so, it's the first object to use this sprite
                for( int j=0; j<i; j++ ) {
                    if( idMap[j] != NULL ) {
                        ObjectRecord *rJ = idMap[j];    
                
                        for( int sJ=0; sJ < rJ->numSprites; sJ++ ) {
                            
                            if( rJ->sprites[sJ] == sID ) {
                                unique = false;
                                break;
                                }
                            }
                        if( !unique ) {
                            break;
                            }
                        }
                    if( !unique ) {
                        break;
                        }
                    }
                if( unique ) {
                    break;
                    }
                }
            
            if( unique ) {
                uniqueCount++;
                
                uniqueList.push_back( rI->id );
                }            
            }
        }
    

    printf( "Objects that are the first on list to have a unique sprite: %d\n",
            uniqueCount );
    for( int i=0; i<uniqueList.size(); i++ ) {
        printf( "%d: %s\n", uniqueList.getElementDirect( i ),
                getObject( uniqueList.getElementDirect( i ) )->description );
        }
    }






void setupNumericSprites( ObjectRecord *inO, int inNumber, int inMax,
                          char *inSpriteVis ) {

    if( ! realSpriteBank() ) {
        return;
        }

    if( inO->spriteNoFlipXPos == NULL ) {
        inO->spriteNoFlipXPos = new double[ inO->numSprites ];
        }
    
    // find sprites from 0 to 9
    SimpleVector<int> numericalIndices[10];
    SimpleVector<double> numericalXPos[10];
    
    const char *key = "Numeral#";
    int keyLen = strlen( key );
    
    for( int i=0; i< inO->numSprites; i++ ) {
        // set all sprites to their own X pos
        inO->spriteNoFlipXPos[i] = inO->spritePos[i].x;
        
        char *tag = getSpriteTag( inO->sprites[ i ] );
        
        char *keyLoc = strstr( tag, key );
        
        if( keyLoc != NULL ) {
            int d = 0;
            sscanf( &( keyLoc[keyLen] ), "%d", &d );
            
            if( d < 10 && d >= 0 ) {
                
                // sort them by x pos as we insert them
                double xPos = inO->spritePos[ i ].x;
                
                char inserted = false;
                for( int j=0; j<numericalXPos[d].size(); j++ ) {
                    
                    if( numericalXPos[d].getElementDirect( j ) > xPos ) {
                        
                        numericalIndices[d].push_middle( i, j );
                        numericalXPos[d].push_middle( xPos, j );
                    
                        inserted = true;
                        break;
                        }
                    }
                if( ! inserted ) {
                    numericalIndices[d].push_back( i );
                    numericalXPos[d].push_back( xPos );
                    }
                
                
                // hide them all for now
                // make certain ones vis later, based on inNumber
                inSpriteVis[ i ] = true;
                }
            }
        }
    
    SimpleVector<int> digits;
    int numLeft = inNumber;
    
    while( numLeft > 0 ) {
        digits.push_front( numLeft % 10 );
        
        numLeft /= 10;
        }

    while( pow( 10, digits.size() ) < inMax ) {
        digits.push_front( 0 );
        }

    SimpleVector<int> visibleDigitSpriteIndex;

    for( int i=0; i<digits.size(); i++ ) {
        int d = digits.getElementDirect( i );

        if( numericalIndices[d].size() > i ) {
            int spriteInd = numericalIndices[d].getElementDirect( i );
        
            inSpriteVis[ spriteInd ] = false;
            visibleDigitSpriteIndex.push_back( spriteInd );
            }
        }

    // for each visible digit, set up a swap pos with the digit on the opposite
    // end
    // if there are an odd number o digits, middle digit will be skipped
    // because it's pos doesn't need flipping
    for( int i=0; i<digits.size() / 2; i++ ) {
        int oppositeI = ( digits.size() - 1 ) - i;

        int spriteI = visibleDigitSpriteIndex.getElementDirect( i );
        int spriteOppI = visibleDigitSpriteIndex.getElementDirect( oppositeI );
        
        inO->spriteNoFlipXPos[spriteI] = inO->spritePos[spriteOppI].x;
        inO->spriteNoFlipXPos[spriteOppI] = inO->spritePos[spriteI].x;
        }
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


    
    // print report on baby distribution per mother
    // testing family code
    if( false )
    for( int i=0; i<femalePersonObjectIDs.size(); i++ ) {

        int id = femalePersonObjectIDs.getElementDirect( i );
        ObjectRecord *motherObject = getObject( id );
        int numPeople = personObjectIDs.size();
        int *hitCounts = new int[ numPeople ];

        int maleCount = 0;
        
        memset( hitCounts, 0, sizeof( int ) * numPeople );
    
        int trials = 1000;

        for( int j=0; j<trials; j++ ) {
            int childID = getRandomFamilyMember( motherObject->race, id, 2 );
            
            ObjectRecord *childO = getObject( childID );
            
            int index = personObjectIDs.getElementIndex( childID );
            
            hitCounts[ index ] ++;

            if( childO->male ) {
                maleCount++;
                }
            }

        printf( "Mother id %d had %d/%d males and these hits:\n",
                id, maleCount, trials );
        
        for( int j=0; j<personObjectIDs.size(); j++ ) {
            printf( "%d:  %d\n",
                    personObjectIDs.getElementDirect( j ),
                    hitCounts[j] );
            }
        printf( "\n\n" );
        }
    




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
                        dummyO->thisUseDummyIndex = d - 1;
                        
                        if( o->creationSoundInitialOnly && d != 1 ) {
                            // only keep creation sound for last dummy
                            // which might get created from some other
                            // object via reverse use
                            // We will never play this when readching d1 from
                            // d2, because they share the same parent
                            
                            // clear creation sounds on all other dummies
                            // if creation is initial only
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

                    char numericLabel = false;
                    
                    if( o->varIsNumeral ) {
                        numericLabel = true;
                        }
                    
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
                        
                        char *sub;
                        
                        if( numericLabel ) {
                            sub = getVarObjectNumeral( d, numVar );
                            }
                        else {
                            sub = getVarObjectLabel( d );
                            }
                        
                        char variableHidden = false;
                        
                        char *targetPos = strstr( o->description, target );
                        char *commentPos = strstr( o->description, "#" );
                        
                        if( commentPos != NULL && targetPos != NULL &&
                            commentPos < targetPos ) {
                            // variable comes after comment
                            variableHidden = true;
                            }
                        

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
                        dummyO->variableDummyParent = mainID;
                        dummyO->thisVariableDummyIndex = d - 1;
                        dummyO->isVariableHidden = variableHidden;
                        
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
                        if( numericLabel ) {
                            setupNumericSprites( dummyO, d, numVar,
                                                 dummyO->spriteSkipDrawing );
                            }
                        }

                    // replace original description too, but not
                    // with a number for variable
                    // call it "- ?" instead
                    char found;
                    char *desc = replaceOnce( o->description, target,
                                              "- ?",
                                              &found );
                    delete [] o->description;
                    o->description = desc;
                    
                    delete [] target;
                    }
                
                }
            }
        
        makeNewObjectsSearchable = oldSearch;
        
        printf( "  Auto-generated %d 'variable' objects\n", numAutoGenerated );
        }

    

    // fill global triggers
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            
            if( o->isGlobalTriggerOn || o->isGlobalTriggerOff ) {
                GlobalTrigger gRec = { -1 };
                                
                char *triggerName;

                if( o->isGlobalTriggerOn ) {
                    triggerName = strstr( o->description, "*" );
                    }
                else {
                    triggerName = strstr( o->description, "!" );
                    }
                
                if( triggerName != NULL ) {
    
                    triggerName = stringDuplicate( triggerName );

                    // replace * (or !) with >
                    triggerName[0] = '>';
                    
                    // trim after space
                    char *spacePos = strstr( triggerName, " " );
                    
                    if( spacePos != NULL ) {
                        spacePos[0] = '\0';
                        }
                    for( int j=0; j<mapSize; j++ ) {
                        if( i != j && idMap[j] != NULL ) {
                            ObjectRecord *oJ = idMap[j];
                            if( ! oJ->isGlobalTriggerOn && 
                                ! oJ->isGlobalTriggerOff && 
                                ! oJ->isGlobalReceiver && 
                                strstr( oJ->description, triggerName ) ==
                                oJ->description ) {
                                
                                // starts with >triggerName
                                // this is a meta trigger object
                                
                                gRec.onTriggerID = oJ->id;
                                break;
                                }
                            }
                        }
                    
                    // see if record matching this trigger set
                    // already exists
                    for( int r=0; r<globalTriggers.size(); r++ ) {
                        GlobalTrigger *gRecOld = 
                            globalTriggers.getElement( r );
                        
                        if( gRecOld->onTriggerID == gRec.onTriggerID ) {
                            
                            o->globalTriggerIndex = r;
                            break;
                            }
                        }
                    
                    // if not, add new record
                    if( o->globalTriggerIndex == -1 ) {    
                        globalTriggers.push_back( gRec );
                        o->globalTriggerIndex = globalTriggers.size() - 1;
                        }
                    
                    delete [] triggerName;
                    }
                }
            }
        }
    // fill global trigger receivers
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            
            if( o->isGlobalReceiver ) {
                char *triggerName = strstr( o->description, ">" );
                
                if( triggerName != NULL ) {
                    
                    
                    triggerName = stringDuplicate( triggerName );

                    // replace > with * to find trigger sender (triggerOn)
                    triggerName[0] = '*';
                    
                    // trim after space
                    char *spacePos = strstr( triggerName, " " );
                    
                    if( spacePos != NULL ) {
                        spacePos[0] = '\0';
                        }

                    for( int j=0; j<mapSize; j++ ) {
                        if( i != j && idMap[j] != NULL ) {
                            ObjectRecord *oJ = idMap[j];
                            if( oJ->isGlobalTriggerOn
                                && 
                                strstr( oJ->description, triggerName ) != 
                                NULL ) {
                                // found this trigger sender
                                // they share the same global trigger index
                                o->globalTriggerIndex =
                                    oJ->globalTriggerIndex;
                                }
                            }
                        }
                    delete [] triggerName;
                    }
                }
            }
        }
    

    // setup tapout triggers
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            setupTapout( o );
            }
        }
    
    // setup default object
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            setupDefaultObject( o );
            }
        }
    
    
    if( defaultObjectID == -1 ) {
        // no default defined
        // pick first object
        for( int i=0; i<mapSize; i++ ) {
            if( idMap[i] != NULL ) {
                defaultObjectID = i;
                break;
                }
            }
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

    
    // populate vertical and corner version pointers for walls and fences
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            
            const char *key = "+horizontal";
            
            char *pos = strstr( o->description, key );
            
            if( pos != NULL ) {
                char *skipKey = &( pos[ strlen( key ) ] );
                
                char label[20];
                int numRead = sscanf( skipKey, "%19s", label );
                
                if( numRead != 1 ) {
                    continue;
                    }

                char *vertKey = autoSprintf( "+vertical%s", label );
                char *cornerKey = autoSprintf( "+corner%s", label );
                
                int vertKeyLen = strlen( vertKey );
                int cornerKeyLen = strlen( cornerKey );

                for( int j=0; j<mapSize; j++ ) {
                    // consider self too, because horizontal and vertical
                    // might be the same
                    if( idMap[j] != NULL ) {
                        ObjectRecord *oOther = idMap[j];
                        
                        char *keyPos = strstr( oOther->description, vertKey );
                        if( keyPos != NULL &&
                            ( keyPos[ vertKeyLen ] == ' ' ||
                              keyPos[ vertKeyLen ] == '\0' ) ) {
                            o->verticalVersionID = oOther->id;
                            }
                        // not else if
                        // vert and corner might be the same
                        // (in case of door, which doesn't have a corner
                        //  version)
                        keyPos = strstr( oOther->description, cornerKey );
                        if( keyPos != NULL &&
                            ( keyPos[ cornerKeyLen ] == ' ' ||
                              keyPos[ cornerKeyLen ] == '\0' ) ) {
                            o->cornerVersionID = oOther->id;
                            }
                        }
                    }

                delete [] vertKey;
                delete [] cornerKey;
                
                if( o->verticalVersionID != -1 && o->cornerVersionID != -1 ) {
                    o->horizontalVersionID = o->id;
                    o->isAutoOrienting = true;
                    
                    // make sure they all know about each other
                    ObjectRecord *vertO = getObject( o->verticalVersionID );
                    ObjectRecord *cornerO = getObject( o->cornerVersionID );
                    
                    vertO->horizontalVersionID = o->id;
                    vertO->verticalVersionID = vertO->id;
                    vertO->cornerVersionID = cornerO->id;
                    vertO->isAutoOrienting = true;

                    cornerO->horizontalVersionID = o->id;
                    cornerO->verticalVersionID = vertO->id;
                    cornerO->cornerVersionID = cornerO->id;
                    cornerO->isAutoOrienting = true;
                    }
                }
            else if( strstr( o->description, "+causeAutoOrient" ) ) {
                // an object that participates in auto-orienting
                // but doesn't have orientation versions defined
                // (example:  horizontal wall with shelf installed)
                o->isAutoOrienting = true;
                
                if( strstr( o->description, "+causeAutoOrientH" ) ) {
                    o->causeAutoOrientHOnly = true;
                    }
                else if( strstr( o->description, "+causeAutoOrientV" ) ) {
                    o->causeAutoOrientVOnly = true;
                    }
                }
            }
        }


    // generate all tool sets
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];

            if( ! o->isUseDummy && ! o->isVariableDummy ) {
                
                setupToolSet( o );
                
                if( o->toolSetIndex != -1 ) {                    
                    
                    if( o->numUses > 1 && o->useDummyIDs != NULL ) {
                        for( int d=0; d < o->numUses - 1; d++ ) {
                            int dID = o->useDummyIDs[d];
                            
                            getObject( dID )->toolSetIndex = o->toolSetIndex;
                            addToolSetMembership( o->toolSetIndex, dID );
                            }
                        }
                    if( o->numVariableDummyIDs > 1 && 
                        o->variableDummyIDs != NULL ) {
                        
                        for( int d=0; d < o->numVariableDummyIDs; d++ ) {
                            int dID = o->variableDummyIDs[d];
                            
                            getObject( dID )->toolSetIndex = o->toolSetIndex;
                            addToolSetMembership( o->toolSetIndex, dID );
                            }
                        }
                    }
                }
            }
        }



    int maxActualBiome = 0;
    for( int i=0; i<biomes.size(); i++ ) {
        int b =  biomes.getElementDirect( i );
        if( b > maxActualBiome ) {
            maxActualBiome = b;
            }
        }
    
    
    // setup biome limitations
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];
            
            int numReq;
            int *reqList = parseNumberList( o->description, 
                                            "+biomeReq",
                                            &numReq );
            int numBlock;
            int *blockList = parseNumberList( o->description, 
                                            "+biomeBlock",
                                            &numBlock );
            

            if( reqList == NULL &&
                blockList == NULL ) {
                continue;
                }
            
            o->isBiomeLimited = true;
            
            o->maxBiomeMapEntry = maxActualBiome;
            
            o->permittedBiomeMap = new char[ maxActualBiome + 1 ];
            memset( o->permittedBiomeMap, true, maxActualBiome + 1 );
            
            if( reqList!= NULL ) {
                // all but req are blocked
                memset( o->permittedBiomeMap, false, maxActualBiome + 1 );
                for( int i=0; i<numReq; i++ ) {
                    if( reqList[i] <= maxActualBiome ) {
                        o->permittedBiomeMap[ reqList[i] ] = true;
                        }
                    }
                delete [] reqList;
                }
            if( blockList != NULL ) {
                // remove blocked
                for( int i=0; i<numBlock; i++ ) {
                    if( blockList[i] <= maxActualBiome ) {
                        o->permittedBiomeMap[ blockList[i] ] = false;
                        }
                    }
                delete [] blockList;
                }
            }
        }
    

    
    if( false ) {
        countVisuallyUniqueObjects();
        }
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
            delete [] idMap[inID]->spriteInvisibleWhenContained;

            delete [] idMap[inID]->spriteIsHead;
            delete [] idMap[inID]->spriteIsBody;
            delete [] idMap[inID]->spriteIsBackFoot;
            delete [] idMap[inID]->spriteIsFrontFoot;

            delete [] idMap[inID]->spriteIsEyes;
            delete [] idMap[inID]->spriteIsMouth;

            delete [] idMap[inID]->spriteUseVanish;
            delete [] idMap[inID]->spriteUseAppear;
            
            if( idMap[inID]->useDummyIDs != NULL ) {
                delete [] idMap[inID]->useDummyIDs;
                }

            if( idMap[inID]->variableDummyIDs != NULL ) {
                delete [] idMap[inID]->variableDummyIDs;
                }
            
            if( idMap[inID]->spriteBehindPlayer != NULL ) {
                delete [] idMap[inID]->spriteBehindPlayer;
                }

            if( idMap[inID]->spriteAdditiveBlend != NULL ) {
                delete [] idMap[inID]->spriteAdditiveBlend;
                }

            if( idMap[inID]->spriteNoFlipXPos != NULL ) {
                delete [] idMap[inID]->spriteNoFlipXPos;
                }
            

            delete [] idMap[inID]->spriteSkipDrawing;
            
            clearSoundUsage( &( idMap[inID]->creationSound ) );
            clearSoundUsage( &( idMap[inID]->usingSound ) );
            clearSoundUsage( &( idMap[inID]->eatingSound ) );
            clearSoundUsage( &( idMap[inID]->decaySound ) );
            
            if( idMap[inID]->permittedBiomeMap != NULL ) {
                delete [] idMap[inID]->permittedBiomeMap;
                }

            delete idMap[inID];
            idMap[inID] = NULL;

            personObjectIDs.deleteElementEqualTo( inID );
            femalePersonObjectIDs.deleteElementEqualTo( inID );
            monumentCallObjectIDs.deleteElementEqualTo( inID );
            deathMarkerObjectIDs.deleteElementEqualTo( inID );
            allPossibleDeathMarkerIDs.deleteElementEqualTo( inID );
            allPossibleFoodIDs.deleteElementEqualTo( inID );

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
            delete [] idMap[i]->spriteInvisibleWhenContained;

            delete [] idMap[i]->spriteIsHead;
            delete [] idMap[i]->spriteIsBody;
            delete [] idMap[i]->spriteIsBackFoot;
            delete [] idMap[i]->spriteIsFrontFoot;

            delete [] idMap[i]->spriteIsEyes;
            delete [] idMap[i]->spriteIsMouth;


            delete [] idMap[i]->spriteUseVanish;
            delete [] idMap[i]->spriteUseAppear;
            
            if( idMap[i]->useDummyIDs != NULL ) {
                delete [] idMap[i]->useDummyIDs;
                }

            if( idMap[i]->variableDummyIDs != NULL ) {
                delete [] idMap[i]->variableDummyIDs;
                }
            
            if( idMap[i]->spriteBehindPlayer != NULL ) {
                delete [] idMap[i]->spriteBehindPlayer;
                }

            if( idMap[i]->spriteAdditiveBlend != NULL ) {
                delete [] idMap[i]->spriteAdditiveBlend;
                }
            
            if( idMap[i]->spriteNoFlipXPos != NULL ) {
                delete [] idMap[i]->spriteNoFlipXPos;
                }
            

            delete [] idMap[i]->spriteSkipDrawing;

            //printf( "\n\nClearing sound usage for id %d\n", i );            
            clearSoundUsage( &( idMap[i]->creationSound ) );
            clearSoundUsage( &( idMap[i]->usingSound ) );
            clearSoundUsage( &( idMap[i]->eatingSound ) );
            clearSoundUsage( &( idMap[i]->decaySound ) );

            if( idMap[i]->permittedBiomeMap != NULL ) {
                delete [] idMap[i]->permittedBiomeMap;
                }

            delete idMap[i];
            }
        }

    delete [] idMap;

    personObjectIDs.deleteAll();
    femalePersonObjectIDs.deleteAll();
    monumentCallObjectIDs.deleteAll();
    deathMarkerObjectIDs.deleteAll();
    allPossibleDeathMarkerIDs.deleteAll();
    allPossibleFoodIDs.deleteAll();
    
    for( int i=0; i<= MAX_RACE; i++ ) {
        racePersonObjectIDs[i].deleteAll();
        }
    rebuildRaceList();

    if( skipDrawingWorkingArea != NULL ) {
        delete [] skipDrawingWorkingArea;
        }
    skipDrawingWorkingArea = NULL;
    skipDrawingWorkingAreaSize = -1;


    for( int i=0; i<toolSetRecords.size(); i++ ) {
        ToolSetRecord *r = toolSetRecords.getElement( i );
        
        if( r->setTag != NULL ) {
            delete [] r->setTag;
            }
        }
    toolSetRecords.deleteAll();
    
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
                        inObject->noFlip,
                        inObject->sideAccess,
                        inObject->minPickupAge,
                        inObject->heldInHand,
                        inObject->rideable,
                        inObject->blocksWalking,
                        inObject->leftBlockingRadius,
                        inObject->rightBlockingRadius,
                        inObject->drawBehindPlayer,
                        inObject->spriteBehindPlayer,
                        inObject->spriteAdditiveBlend,
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
                        inObject->creationSoundForce,
                        inObject->numSlots, 
                        inObject->slotSize, 
                        inObject->slotPos,
                        inObject->slotVert,
                        inObject->slotParent,
                        inObject->slotTimeStretch,
                        inObject->slotsLocked,
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
                        inObject->spriteInvisibleWhenContained,
                        inObject->spriteIsHead,
                        inObject->spriteIsBody,
                        inObject->spriteIsBackFoot,
                        inObject->spriteIsFrontFoot,
                        inObject->numUses,
                        inObject->useChance,
                        inObject->spriteUseVanish,
                        inObject->spriteUseAppear,
                        inNoWriteToFile,
                        inReplaceID,
                        inObject->cachedHeight );

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




#include "objectMetadata.h"


ObjectRecord *getObject( int inID, char inNoDefault ) {
    inID = extractObjectID( inID );
    
    if( inID < mapSize ) {
        if( idMap[inID] != NULL ) {
            return idMap[inID];
            }
        }

    if( ! inNoDefault && defaultObjectID != -1 ) {
        if( defaultObjectID < mapSize ) {
            if( idMap[ defaultObjectID ] != NULL ) {
                return idMap[ defaultObjectID ];
                }
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
               float inContainSize,
               double inVertContainRotationOffset,
               char inPermanent,
               char inNoFlip,
               char inSideAccess,
               int inMinPickupAge,
               char inHeldInHand,
               char inRideable,
               char inBlocksWalking,
               int inLeftBlockingRadius, int inRightBlockingRadius,
               char inDrawBehindPlayer,
               char *inSpriteBehindPlayer,
               char *inSpriteAdditiveBlend,
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
               char inCreationSoundForce,
               int inNumSlots, float inSlotSize, doublePair *inSlotPos,
               char *inSlotVert,
               int *inSlotParent,
               float inSlotTimeStretch,
               char inSlotsLocked,
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
               char *inSpriteInvisibleWhenContained,
               char *inSpriteIsHead,
               char *inSpriteIsBody,
               char *inSpriteIsBackFoot,
               char *inSpriteIsFrontFoot,
               int inNumUses,
               float inUseChance,
               char *inSpriteUseVanish,
               char *inSpriteUseAppear,
               char inNoWriteToFile,
               int inReplaceID,
               int inExistingObjectHeight ) {
    
    if( inSlotTimeStretch < 0.0001 ) {
        inSlotTimeStretch = 0.0001;
        }


    SimpleVector<int> drawBehindIndicesList;
    
    if( inSpriteBehindPlayer != NULL ) {    
        for( int i=0; i<inNumSprites; i++ ) {
            if( inSpriteBehindPlayer[i] ) {
                drawBehindIndicesList.push_back( i );
                }
            }
        }

    SimpleVector<int> additiveBlendIndicesList;
    
    if( inSpriteAdditiveBlend != NULL ) {    
        for( int i=0; i<inNumSprites; i++ ) {
            if( inSpriteAdditiveBlend[i] ) {
                additiveBlendIndicesList.push_back( i );
                }
            }
        }
    
    
    
    int newID = inReplaceID;
    
    int newHeight = inExistingObjectHeight;


    if( newHeight == -1 ) {
        newHeight = recomputeObjectHeight( inNumSprites,
                                           inSprites, inSpritePos );
        }
    
    // add it to file structure
    File objectsDir( NULL, "objects" );
            
    if( ! inNoWriteToFile && !objectsDir.exists() ) {
        objectsDir.makeDirectory();
        }


    int nextObjectNumber = 1;
    
    if( ! inNoWriteToFile &&
        objectsDir.exists() && objectsDir.isDirectory() ) {
                
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
        lines.push_back( autoSprintf( "containSize=%f,vertSlotRot=%f", 
                                      inContainSize,
                                      inVertContainRotationOffset ) );
        lines.push_back( autoSprintf( "permanent=%d,minPickupAge=%d", 
                                      (int)inPermanent,
                                      inMinPickupAge ) );
        
        lines.push_back( autoSprintf( "noFlip=%d", (int)inNoFlip ) );
        lines.push_back( autoSprintf( "sideAccess=%d", (int)inSideAccess ) );


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
        lines.push_back( autoSprintf( "creationSoundForce=%d", 
                                      (int)inCreationSoundForce ) );
        
        lines.push_back( autoSprintf( "numSlots=%d#timeStretch=%f", 
                                      inNumSlots, inSlotTimeStretch ) );
        lines.push_back( autoSprintf( "slotSize=%f", inSlotSize ) );
        lines.push_back( autoSprintf( "slotsLocked=%d", (int)inSlotsLocked ) );

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

            lines.push_back( autoSprintf( "invisCont=%d", 
                                          inSpriteInvisibleWhenContained[i] ) );

            }
        


        if( drawBehindIndicesList.size() > 0 ) {    
            lines.push_back(
                boolArrayToSparseCommaString( "spritesDrawnBehind",
                                              inSpriteBehindPlayer, 
                                              inNumSprites ) );
            }
        
        if( additiveBlendIndicesList.size() > 0 ) {
            lines.push_back(
                boolArrayToSparseCommaString( "spritesAdditiveBlend",
                                              inSpriteAdditiveBlend, 
                                              inNumSprites ) );
            }
        
        
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
        
        
        lines.push_back( autoSprintf( "numUses=%d,%f",
                                      inNumUses, inUseChance ) );
        
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
            
        
            char *nextNumberString = autoSprintf( "%d", nextObjectNumber );
        
            File *nextNumberFile = 
                objectsDir.getChildFile( "nextObjectNumber.txt" );
            
            nextNumberFile->writeToFile( nextNumberString );
            
            delete [] nextNumberString;
            
            
            delete nextNumberFile;
            }
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

        // same trick used in other banks:  speed it up by a factor of 100x
        // by adding 100 new slots at a time
        // (without wasting as much space on last expansion as 
        // doubling it would)
        int newMapSize = newID + 100;
        

        
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
    r->noFlip = inNoFlip;
    r->sideAccess = inSideAccess;
    
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


    r->anySpritesBehindPlayer = false;
    r->spriteBehindPlayer = NULL;

    if( drawBehindIndicesList.size() > 0 ) {    
        r->anySpritesBehindPlayer = true;
        r->spriteBehindPlayer = new char[ inNumSprites ];
        memcpy( r->spriteBehindPlayer, inSpriteBehindPlayer, inNumSprites );
        }
    
    
    r->spriteAdditiveBlend = NULL;
    
    if( additiveBlendIndicesList.size() > 0 ) {
        r->spriteAdditiveBlend = new char[ inNumSprites ];
        memcpy( r->spriteAdditiveBlend, inSpriteAdditiveBlend, inNumSprites );
        }

    
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
    
    r->spriteNoFlipXPos = NULL;


    fillObjectBiomeFromString( r, inBiomes );
    
    
    r->mapChance = inMapChance;
    
    r->heatValue = inHeatValue;
    r->rValue = inRValue;

    r->person = inPerson;
    r->personNoSpawn = inPersonNoSpawn;
    r->race = inRace;
    r->male = inMale;
    r->deathMarker = inDeathMarker;
    
    deathMarkerObjectIDs.deleteElementEqualTo( newID );
    allPossibleDeathMarkerIDs.deleteElementEqualTo( newID );
    allPossibleFoodIDs.deleteElementEqualTo( newID );
    
    if( r->deathMarker ) {
        deathMarkerObjectIDs.push_back( newID );
        }
    if( strstr( r->description, "fromDeath" ) != NULL ) {
        allPossibleDeathMarkerIDs.push_back( newID );
        }
    
    
    r->homeMarker = inHomeMarker;
    r->floor = inFloor;
    r->floorHugging = inFloorHugging;
    r->foodValue = inFoodValue;

    
    // do NOT add to food list
    // addObject is only called for generated objects NOT loaded from disk 
    // (use dummies, etc).
    // Don't include them in list of foods
    
    // if( r->foodValue > 0 ) {
    //    allPossibleFoodIDs.push_back( newID );
    //    }

    
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
    r->creationSoundForce = inCreationSoundForce;

    r->numSlots = inNumSlots;
    r->slotSize = inSlotSize;
    
    r->slotPos = new doublePair[ inNumSlots ];
    r->slotVert = new char[ inNumSlots ];
    r->slotParent = new int[ inNumSlots ];
    
    memcpy( r->slotPos, inSlotPos, inNumSlots * sizeof( doublePair ) );
    memcpy( r->slotVert, inSlotVert, inNumSlots * sizeof( char ) );
    memcpy( r->slotParent, inSlotParent, inNumSlots * sizeof( int ) );
    
    r->slotTimeStretch = inSlotTimeStretch;
    r->slotsLocked = inSlotsLocked;

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
    r->spriteInvisibleWhenContained = new char[ inNumSprites ];

    r->spriteIsHead = new char[ inNumSprites ];
    r->spriteIsBody = new char[ inNumSprites ];
    r->spriteIsBackFoot = new char[ inNumSprites ];
    r->spriteIsFrontFoot = new char[ inNumSprites ];

    r->numUses = inNumUses;
    r->useChance = inUseChance;
    r->spriteUseVanish = new char[ inNumSprites ];
    r->spriteUseAppear = new char[ inNumSprites ];
    
    r->useDummyIDs = NULL;
    r->isUseDummy = false;
    r->useDummyParent = 0;
    r->thisUseDummyIndex = -1;
    
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
    r->variableDummyParent = 0;
    r->thisVariableDummyIndex = -1;
    r->isVariableHidden = false;


    setupObjectWritingStatus( r );
    
    setupObjectGlobalTriggers( r );
    
    setupObjectSpeechPipe( r );
    
    setupFlight( r );
    
    setupOwned( r );
    
    setupNoHighlight( r );
                
    setupMaxPickupAge( r );

    setupAutoDefaultTrans( r );

    setupNoBackAccess( r );            

    setupAlcohol( r );

    setupFamHomeland( r );
    
    setupForcedBiome( r );

    setupExpertFind( r );

    setupNormalOnly( r );
    
    setupYumParent( r );
    
    setupRoadParent( r );
    
    setupSlotsInvis( r );

    setupVarSerialNumber( r );
    
    setupVarIsNumeral( r );

    
    setupWall( r );

    setupBlocksMoving( r );
    setupBlocksNonFollower( r );
    setupBadgePos( r );
    setupHideHead( r );
    setupHideRider( r );
    setupNeverDrop( r );
    setupGiveClue( r );
    
    
    r->toolSetIndex = -1;
    
    r->isBiomeLimited = false;
    r->permittedBiomeMap = NULL;

    r->isAutoOrienting = false;
    r->causeAutoOrientHOnly = false;
    r->causeAutoOrientVOnly = false;
    r->horizontalVersionID = -1;
    r->verticalVersionID = -1;
    r->cornerVersionID = -1;

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

    memcpy( r->spriteInvisibleWhenContained, inSpriteInvisibleWhenContained, 
            inNumSprites * sizeof( char ) );


    memcpy( r->spriteIsHead, inSpriteIsHead, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBody, inSpriteIsBody, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsBackFoot, inSpriteIsBackFoot, 
            inNumSprites * sizeof( char ) );

    memcpy( r->spriteIsFrontFoot, inSpriteIsFrontFoot, 
            inNumSprites * sizeof( char ) );


    setupEyesAndMouth( r );
    


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


static char drawingContained = false;

void setDrawnObjectContained( char inContained ) {
    drawingContained = inContained;
    }



HoldingPos drawObject( ObjectRecord *inObject, int inDrawBehindSlots,
                       doublePair inPos,
                       double inRot, char inWorn, char inFlipH, double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       double inScale ) {

    checkDrawPos( inObject->id, inPos );
    
    if( inObject->noFlip ) {
        inFlipH = false;
        }

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
        if( drawingContained &&
            inObject->spriteInvisibleWhenContained[i] ) {
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

        char spriteNoFlip = false;
        
        if( inFlipH ) {
            spriteNoFlip = getNoFlip( inObject->sprites[i] );
            }
        
        
        if( inFlipH ) {
            
            if( spriteNoFlip && inObject->spriteNoFlipXPos != NULL ) {
                spritePos.x = inObject->spriteNoFlipXPos[i];
                }
            
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
            
            if( spriteColorOverrideOn ) {
                setDrawColor( spriteColorOverride );
                }
            else {
                setDrawColor( inObject->spriteColor[i] );
                }

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

            char additive = false;
            if( inObject->spriteAdditiveBlend != NULL ) {
                additive = inObject->spriteAdditiveBlend[i];
                }
            if( additive ) {
                toggleAdditiveBlend( true );
                }

            SpriteHandle sh = getSprite( inObject->sprites[i] );
            if( sh != NULL ) {
                char f = inFlipH;
                if( f && spriteNoFlip ) {    
                    f = false;
                    }
                
                drawSprite( sh, pos, inScale,
                            rot, 
                            logicalXOR( f, inObject->spriteHFlip[i] ) );
                }
            
            if( multiplicative ) {
                toggleMultiplicativeBlend( false );
                toggleAdditiveTextureColoring( false );
                }
            if( additive ) {
                toggleAdditiveBlend( false );
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
        if( ! skipSprite && 
            inClothing.backShoe != NULL && i == backFootIndex ) {
            drawObject( inClothing.backShoe, 2,
                        backShoePos, backShoeRot, true,
                        inFlipH, -1, 0, false, false, emptyClothing );
            }
        else if( ! skipSprite &&
                 inClothing.frontShoe != NULL && i == frontFootIndex ) {
            drawObject( inClothing.frontShoe, 2,
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

    char allBehind = true;
    for( int i=0; i< inObject->numSprites; i++ ) {
        if( ! inObject->spriteBehindSlots[i] ) {
            allBehind = false;
            break;
            }
        }

    setDrawnObjectContained( true );
    
    int numSlots = getNumContainerSlots( inObject->id );
    
    if( inNumContained > numSlots ) {
        inNumContained = numSlots;
        }

    if( ! inObject->slotsInvis )
    for( int i=0; i<inNumContained; i++ ) {

        ObjectRecord *contained = getObject( inContainedIDs[i] );
        

        doublePair centerOffset;

        if( allBehind ) {
            centerOffset = getObjectBottomCenterOffset( contained );
            }
        else {
            centerOffset = getObjectCenterOffset( contained );
            }

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
    
    setDrawnObjectContained( false );

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



int getRaceSize( int inRace ) {
    if( inRace > MAX_RACE ) {
        return 0;
        }
    return racePersonObjectIDs[ inRace ].size();
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



int getRandomFamilyMember( int inRace, int inMotherID, int inFamilySpan,
                           char inForceGirl ) {
    
    if( inRace > MAX_RACE ) {
        inRace = MAX_RACE;
        }
    
    if( racePersonObjectIDs[ inRace ].size() == 0 ) {
        return -1;
        }

    if( racePersonObjectIDs[ inRace ].size() == 1 ) {
        // no choice in this race, return mother
        return racePersonObjectIDs[ inRace ].getElementDirect( 0 );
        }
    
    int motherIndex = 
        racePersonObjectIDs[ inRace ].getElementIndex( inMotherID );

    if( motherIndex == -1 ) {
        return getRandomPersonObjectOfRace( inRace );
        }
    
    if( racePersonObjectIDs[ inRace ].size() == 2 ) {
        // no choice, return non-mother
        int nonMotherIndex = 0;
        if( motherIndex == 0 ) {
            nonMotherIndex = 1;
            }
        return racePersonObjectIDs[ inRace ].getElementDirect( nonMotherIndex );
        }
    
    
    // at least 3 people in this race

    
    // never have offset 0, so we can't ever have ourself as a baby
    if( inFamilySpan < 1 ) {
        inFamilySpan = 1;
        }

    // first, collect all people in this span
    SimpleVector<int> spanPeople;
    int boyCount = 0;
    int girlCount = 0;
    
    for( int o=1; o<=inFamilySpan; o++ ) {
        for( int s=-1; s<=1; s+=2 ) {
            
            int familyIndex = motherIndex + o * s;
        
            while( familyIndex >= racePersonObjectIDs[ inRace ].size() ) {
                familyIndex -= racePersonObjectIDs[ inRace ].size();
                }
            while( familyIndex < 0 ) {
                familyIndex += racePersonObjectIDs[ inRace ].size();
                }
            
            if( familyIndex != motherIndex ) {
                // never add mother to collection

                int pID = 
                    racePersonObjectIDs[ inRace ].
                    getElementDirect( familyIndex );
                if( spanPeople.getElementIndex( pID ) == -1 ) {
                    // not added yet
                    spanPeople.push_back( pID );
                    
                    if( getObject( pID )->male ) {
                        boyCount++;
                        }
                    else {
                        girlCount++;
                        }
                    }
                }
            }    
        }
    
    // now we have a collection of unique possible offspring

    while( boyCount > girlCount && girlCount >= 1 ) {
        // duplicate a girl
        for( int p=0; p<spanPeople.size(); p++ ) {
            int pID = spanPeople.getElementDirect( p );
            
            if( ! getObject( pID )->male ) {
                spanPeople.push_back( pID );
                girlCount++;
                if( girlCount == boyCount ) {
                    break;
                    }
                }
            }
        }

    while( girlCount > boyCount && boyCount >= 1 ) {
        // duplicate a boy
        for( int p=0; p<spanPeople.size(); p++ ) {
            int pID = spanPeople.getElementDirect( p );
            
            if( getObject( pID )->male ) {
                spanPeople.push_back( pID );
                boyCount++;
                if( girlCount == boyCount ) {
                    break;
                    }
                }
            }
        }

    
    if( inForceGirl && girlCount > 0 ) {
        // remove boys from list
        for( int p=0; p<spanPeople.size(); p++ ) {
            int pID = spanPeople.getElementDirect( p );
            
            if( getObject( pID )->male ) {
                spanPeople.deleteElement( p );
                p--;
                }
            }    
        }
    

    int pick = randSource.getRandomBoundedInt( 0, spanPeople.size() - 1 );
    
    return spanPeople.getElementDirect( pick );
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

    if( deathMarkerObjectIDs.size() == 0 ) {
        return -1;
        }
    
        
    return deathMarkerObjectIDs.getElementDirect( 
        randSource.getRandomBoundedInt( 0, 
                                        deathMarkerObjectIDs.size() - 1  ) );
    }



SimpleVector<int> *getAllPossibleDeathIDs() {
    return &allPossibleDeathMarkerIDs;
    }



SimpleVector<int> *getAllPossibleFoodIDs() {
    return &allPossibleFoodIDs;
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
                             char inConsiderTransparent,
                             char inConsiderEmptySlots ) {
    
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
            else if( !tunicChecked && i < headIndex ) {
                // bottom, tunic, and backpack behind back arm
                // but ignore the arm when checking for clothing hit
                // we never want to click on arm instead of the clothing
                
                // don't count clicks that land on head or above
                // (head is in front of tunic)
                
                
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
        
        if( sr == NULL ) {
            continue;
            }

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
            else if( inConsiderEmptySlots ) {
                
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

    if( inHandOrFootIndex == 0 &&
        ! ( inObject->spriteInvisibleWhenHolding[ inHandOrFootIndex ] ||
            inObject->spriteIsFrontFoot[ inHandOrFootIndex ] ||
            inObject->spriteIsBackFoot[ inHandOrFootIndex ] ) ) {
        // 0 index usually returned if no hand or foot found
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


// top-most body part that is flagged
static int getBodyPartIndex( ObjectRecord *inObject,
                             char *inBodyPartFlagArray,
                             double inAge ) {
    if( ! inObject->person ) {
        return 0;
        }
    
    for( int i = inObject->numSprites - 1; i >= 0; i-- ) {
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



int getEyesIndex( ObjectRecord *inObject,
                  double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsEyes, inAge );
    }



int getMouthIndex( ObjectRecord *inObject,
                   double inAge ) {
    return getBodyPartIndex( inObject, inObject->spriteIsMouth, inAge );
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

        if( inObject->spriteInvisibleWhenWorn[i] == 2 ) {
            // don't consider parts visible only when worn
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




doublePair getObjectBottomCenterOffset( ObjectRecord *inObject ) {


    // find center of lowessprite

    SpriteRecord *lowestRecord = NULL;
    
    int lowestIndex = -1;
    double lowestYPos = 0;
    
    for( int i=0; i<inObject->numSprites; i++ ) {
        SpriteRecord *sprite = getSpriteRecord( inObject->sprites[i] );
    
        if( sprite->multiplicativeBlend ) {
            // don't consider translucent sprites when finding bottom
            continue;
            }

        if( inObject->spriteInvisibleWhenWorn[i] == 2 ) {
            // don't consider parts visible only when worn
            continue;
            }
        

        double y = inObject->spritePos[i].y;


        if( lowestRecord == NULL ||
            // wider than what we've seen so far
            y < lowestYPos ) {

            lowestRecord = sprite;
            lowestIndex = i;
            lowestYPos = inObject->spritePos[i].y;
            }
        }
    

    if( lowestRecord == NULL ) {
        doublePair result = { 0, 0 };
        return result;
        }
    
    
        
    doublePair centerOffset = { (double)lowestRecord->centerXOffset,
                                (double)lowestRecord->centerYOffset };
        
    centerOffset = rotate( centerOffset, 
                           2 * M_PI * inObject->spriteRot[lowestIndex] );

    doublePair spriteCenter = add( inObject->spritePos[lowestIndex], 
                                   centerOffset );

    doublePair wideCenter = getObjectCenterOffset( inObject );
    
    
    // adjust y based on lowest sprite
    // but keep center from widest sprite
    // (in case object has "feet" that are not centered)
    wideCenter.y = spriteCenter.y;

    return wideCenter;    
    }



int getMaxWideRadius() {
    return maxWideRadius;
    }



char isSpriteSubset( int inSuperObjectID, int inSubObjectID,
                     char inIgnoreColors,
                     SimpleVector<SubsetSpriteIndexMap> *outMapping ) {

    ObjectRecord *superO = getObject( inSuperObjectID );
    ObjectRecord *subO = getObject( inSubObjectID );
    
    if( superO == NULL || subO == NULL ) {
        return false;
        }

    if( subO->numSprites == 0 ) {
        return true;
        }
    else if( subO->numSprites == 1 &&
             superO->numSprites >= 1 ) {
        // special case:
        // new object is a single-sprite object
        
        // treat it as a subset of old object if that sprite occurs
        // at all, regardless of rotation, position, flip, etc.
        int spriteID = subO->sprites[0];
        
        for( int ss=0; ss<superO->numSprites; ss++ ) {
            if( superO->sprites[ ss ] == spriteID ) {

                // do make sure that color matches too
                if( inIgnoreColors || equal( superO->spriteColor[ ss ],
                                             subO->spriteColor[ 0 ] ) ) {
                    return true;
                    }
                }
            }
        // if our sub-obj's single sprite does not occur, 
        // it's definitely not a subset
        return false;
        }
    
    // allow global position adjustments, as long as all sub-sprites in same
    // relative position to each other
    
    int spriteSubZeroID = subO->sprites[0];
    doublePair spriteSubZeroPos = subO->spritePos[0];    
    double spriteSubZeroRot = subO->spriteRot[0];
    char spriteSubZeroFlip = subO->spriteHFlip[0];

    doublePair spriteSuperZeroPos;
    
    // find sub's zero sprite in super
    // if there is more than one matching, find one that is closest
    // to pos of sub's zero sprite
    char found = false;
    double minDist = 9999999;

    for( int ss=0; ss<superO->numSprites; ss++ ) {
        if( superO->sprites[ ss ] == spriteSubZeroID &&
            superO->spriteRot[ ss ] == spriteSubZeroRot &&
            superO->spriteHFlip[ ss ] == spriteSubZeroFlip ) {
            
            found = true;
            doublePair pos = superO->spritePos[ ss ];

            double d = distance( pos, spriteSubZeroPos );
            
            if( d < minDist ) {
                minDist = d;
                spriteSuperZeroPos = pos;
                }
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

        FloatRGB spriteColor = subO->spriteColor[s];

        char found = false;
        
        for( int ss=0; ss<superO->numSprites; ss++ ) {
            if( superO->sprites[ ss ] == spriteID &&
                equal( sub( superO->spritePos[ ss ],
                            spriteSuperZeroPos ), spritePosRel ) &&
                superO->spriteRot[ ss ] == spriteRot &&
                superO->spriteHFlip[ ss ] == spriteHFlip 
                &&
                ( inIgnoreColors ||
                  equal( superO->spriteColor[ ss ], spriteColor ) ) ) {
                
                if( outMapping != NULL ) {
                    SubsetSpriteIndexMap m = { s, ss };
                    outMapping->push_back( m );
                    }
                found = true;
                break;
                }
            }

        if( !found ) {
            if( outMapping != NULL ) {
                outMapping->deleteAll();
                }
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



int getObjectParent( int inObjectID ) {
    if( inObjectID > 0 ) {    
        ObjectRecord *o = getObject( inObjectID );
        
        if( o != NULL ) {
            if( o->isUseDummy ) {
                return o->useDummyParent;
                }
            if( o->isVariableDummy ) {
                return o->variableDummyParent;
                }
            }
        }
    
    return inObjectID;
    }




int hideIDForClient( int inObjectID ) {    
    if( inObjectID > 0 ) {
        ObjectRecord *o = getObject( inObjectID );
        if( o->isVariableDummy && o->isVariableHidden ) {
            // hide from client
            inObjectID = o->variableDummyParent;
            }
        else {
            // this has any metadata stripped off
            inObjectID = o->id;
            }
        }
    return inObjectID;
    }



void prepareToSkipSprites( ObjectRecord *inObject, 
                           char inDrawBehind, char inSkipAll ) {
    if( skipDrawingWorkingArea != NULL ) {
        if( skipDrawingWorkingAreaSize < inObject->numSprites ) {
            delete [] skipDrawingWorkingArea;
            skipDrawingWorkingArea = NULL;
            
            skipDrawingWorkingAreaSize = 0;
            }
        }
    if( skipDrawingWorkingArea == NULL ) {
        skipDrawingWorkingAreaSize = inObject->numSprites;
        skipDrawingWorkingArea = new char[ skipDrawingWorkingAreaSize ];
        }
    
    memcpy( skipDrawingWorkingArea, 
            inObject->spriteSkipDrawing, inObject->numSprites );
    
    for( int i=0; i< inObject->numSprites; i++ ) {
        
        if( inSkipAll ) {
            inObject->spriteSkipDrawing[i] = true;
            }
        else if( inObject->spriteBehindPlayer[i] && ! inDrawBehind ) {
            inObject->spriteSkipDrawing[i] = true;
            }
        else if( ! inObject->spriteBehindPlayer[i] && inDrawBehind ) {
            inObject->spriteSkipDrawing[i] = true;
            }
        }
    }



void restoreSkipDrawing( ObjectRecord *inObject ) {
    memcpy( inObject->spriteSkipDrawing, skipDrawingWorkingArea,
            inObject->numSprites );
    }



char canPickup( int inObjectID, double inPlayerAge ) {
    ObjectRecord *o = getObject( inObjectID );
    
    if( o->minPickupAge > inPlayerAge ) {
        return false;
        }
    
    if( o->maxPickupAge < inPlayerAge ) {
        return false;
        }
    
    return true;
    }



void stripDescriptionComment( char *inString ) {
    // pound sign is used for trailing developer comments
    // that aren't show to end user, cut them off if they exist
    char *firstPound = strstr( inString, "#" );
            
    if( firstPound != NULL ) {
        firstPound[0] = '\0';
        }
    }


SimpleVector<int> findObjectsMatchingWords( char *inWords, 
                                            int inIgnoreObjectID,
                                            int inLimit,
                                            int *outNumFilterHits ) {
    unsigned int filterLength = strlen( inWords );
        
    int numHits = 0;
    int numRemain = 0;
    ObjectRecord **hits = searchObjects( inWords,
                                         0,
                                         inLimit,
                                         &numHits, &numRemain );
        
    SimpleVector<int> hitMatchIDs;

    *outNumFilterHits = numHits;

    SimpleVector<int> exactHitMatchIDs;
        

    for( int i=0; i<numHits; i++ ) {
        if( hits[i]->id == inIgnoreObjectID ) {
            // don't count the object itself as a hit
            continue;
            }
        char *des = stringToUpperCase( hits[i]->description );
            
        stripDescriptionComment( des );
        
        if( strcmp( des, inWords ) == 0 ) {
            exactHitMatchIDs.push_back( hits[i]->id );
            }

        char *searchPos = strstr( des, inWords );
            
        // only count if occurrence of filter string matches whole words
        // not partial words
        if( searchPos != NULL && strlen( searchPos ) >= filterLength ) {
                
            unsigned int remainLen = strlen( searchPos );

            char frontOK = false;
            char backOK = false;
                
            // space or start of string in front of search phrase
            if( searchPos == des ||
                searchPos[-1] == ' ' ) {
                frontOK = true;
                }
                
            // space or end of string after search phrase
            if( remainLen == filterLength ||
                searchPos[filterLength] == ' ' ) {
                backOK = true;
                }

            if( frontOK && backOK ) {
                hitMatchIDs.push_back( hits[i]->id );
                }
            }
            
        delete [] des;
        }
        
        
    // now find shallowest matching objects

    numHits = hitMatchIDs.size();
        
    int startDepth = 0;

    if( inIgnoreObjectID > 0 ) {
        startDepth = getObjectDepth( inIgnoreObjectID );
        
        ObjectRecord *startObject = getObject( inIgnoreObjectID );
        if( startObject->isUseDummy ) {
            startDepth = getObjectDepth( startObject->useDummyParent );
            }
        }
        

    int shallowestDepth = UNREACHABLE;
       
    for( int i=0; i<numHits; i++ ) {
            
        int depth = getObjectDepth( hitMatchIDs.getElementDirect( i ) );
            
        if( depth >= startDepth && depth < shallowestDepth ) {
            shallowestDepth = depth;
            }
        }

    SimpleVector<int> hitIDs;

    for( int i=0; i<numHits; i++ ) {
        int id = hitMatchIDs.getElementDirect( i );
            
        int depth = getObjectDepth( id );
            
        if( depth == shallowestDepth ) {
            hitIDs.push_back( id );
            }
        }



    int shallowestExactDepth = UNREACHABLE;
       
    for( int i=0; i<exactHitMatchIDs.size(); i++ ) {
            
        int depth = getObjectDepth( exactHitMatchIDs.getElementDirect( i ) );
            
        if( depth >= startDepth && depth < shallowestExactDepth ) {
            shallowestExactDepth = depth;
            }
        }

    SimpleVector<int> exactHitIDs;

    for( int i=0; i<exactHitMatchIDs.size(); i++ ) {
        int id = exactHitMatchIDs.getElementDirect( i );
            
        int depth = getObjectDepth( id );
            
        if( depth == shallowestExactDepth ) {
            exactHitIDs.push_back( id );
            }
        }
    
        

    if( hits != NULL ) {    
        delete [] hits;
        }
        
    // there are exact matches
    // use those instead
    if( exactHitIDs.size() > 0 ) {
        hitIDs.deleteAll();
        hitIDs.push_back_other( &exactHitIDs );
        }
        


    numHits = hitIDs.size();

    return hitIDs;
    }




TapoutRecord *getTapoutRecord( int inObjectID ) {
    for( int i=0; i<tapoutRecords.size(); i++ ) {
        TapoutRecord *r = tapoutRecords.getElement( i );
        
        if( r->triggerID == inObjectID ) {
            return r;
            }
        }
    return NULL;
    }



void clearTapoutCounts() {
    for( int i=0; i<tapoutRecords.size(); i++ ) {
        TapoutRecord *r = tapoutRecords.getElement( i );
        r->buildCount = 0;
        }
    }



void clearToolLearnedStatus() {
    for( int i=0; i<mapSize; i++ ) {
        if( idMap[i] != NULL ) {
            ObjectRecord *o = idMap[i];

            o->toolLearned = false;
            }
        }
    }



void getToolSetMembership( int inToolSetIndex, 
                           SimpleVector<int> *outListToFill ) {
    ToolSetRecord *r = toolSetRecords.getElement( inToolSetIndex );
    
    outListToFill->push_back_other( &( r->setMembership ) );
    }



void getAllToolSets( SimpleVector<int> *outListToFill ) {
    for( int i=0; i<toolSetRecords.size(); i++ ) {
        outListToFill->push_back( i );
        }
    }



char canBuildInBiome( ObjectRecord *inObj, int inTargetBiome ) {
    if( ! inObj->isBiomeLimited || inObj->permittedBiomeMap == NULL ||
        inTargetBiome > inObj->maxBiomeMapEntry ) {
        return true;
        }
    
    return inObj->permittedBiomeMap[ inTargetBiome ];
    }



int getMaxFoodValue() {
    return maxFoodValue;
    }



char sameRoadClass( int inFloorA, int inFloorB ) {
    if( inFloorA <= 0 || inFloorB <= 0 ) {
        return false;
        }
    
    if( inFloorA == inFloorB ) {
        return true;
        }
    
    int aParent = getObject( inFloorA )->roadParentID;
    int bParent = getObject( inFloorB )->roadParentID;

    if( aParent == inFloorB ||
        bParent == inFloorA ||
        ( aParent != -1 && aParent == bParent ) ) {
        
        return true;
        }

    return false;
    }


    

// from spriteDrawColorOverride.h
// instantiated here so we don't need a separate cpp file for these

char spriteColorOverrideOn = false;

FloatColor spriteColorOverride = {1, 1, 1, 1};



typedef struct SerialCountRecord {
        int serialNumberCategory;
        
        int numInstancesCreated;
    } SerialCountRecord;


SimpleVector<SerialCountRecord> serialRecords;



static SerialCountRecord *getSerialRecord( ObjectRecord *inO ) {
    
    const char *key = "+varSerialNumber";
    
    char *pos = strstr( inO->description, key );

    int categoryNumber = 0;
    
    if( pos != NULL ) {
        sscanf( &( pos[ strlen( key ) ] ), "%d", &categoryNumber );    
        }
    
    for( int i=0; i<serialRecords.size(); i++ ) {
        SerialCountRecord *r = serialRecords.getElement( i );
        
        if( r->serialNumberCategory == categoryNumber ) {
            return r;
            }
        }

    // none found, make one
    SerialCountRecord rec = { categoryNumber, 0 };
    
    serialRecords.push_back( rec );    
    
    return serialRecords.getElement( serialRecords.size() - 1 );
    }

    


int getNextVarSerialNumberChild( ObjectRecord *inO ) {
    
    ObjectRecord *parent = inO;
    
    if( inO->isVariableDummy ) {
        parent = getObject( inO->variableDummyParent );
        }

    if( ! parent->useVarSerialNumbers ) {
        return inO->id;
        }
    
    if( parent->numVariableDummyIDs == 0 ) {
        return inO->id;
        }
    
    SerialCountRecord *r = getSerialRecord( inO );

    int nextDummyIndex = 
        r->numInstancesCreated % parent->numVariableDummyIDs;


    r->numInstancesCreated ++;
    
    return parent->variableDummyIDs[ nextDummyIndex ];
    }

