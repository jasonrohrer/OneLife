#include "map.h"
#include "HashTable.h"
#include "monument.h"
#include "arcReport.h"

#include "CoordinateTimeTracking.h"

#include "eveMovingGrid.h"


// cell pixel dimension on client
#define CELL_D 128

#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"
//#include "stackdb.h"
//#include "lineardb.h"
#include "lineardb3.h"

#include "minorGems/util/crc32.h"


/*
#define DB KISSDB
#define DB_open KISSDB_open
#define DB_close KISSDB_close
#define DB_get KISSDB_get
#define DB_put KISSDB_put
// no distinction between insert and replace in KISSS
#define DB_put_new KISSDB_put
#define DB_Iterator  KISSDB_Iterator
#define DB_Iterator_init  KISSDB_Iterator_init
#define DB_Iterator_next  KISSDB_Iterator_next
#define DB_maxStack (int)( db.num_hash_tables )
// no support for shrinking
#define DB_getShrinkSize( dbP, n )  dbP->hashTableSize
#define DB_getCurrentSize( dbP )  dbP->hashTableSize
// no support for counting records
#define DB_getNumRecords( dbP ) 0
*/


/*
#define DB STACKDB
#define DB_open STACKDB_open
#define DB_close STACKDB_close
#define DB_get STACKDB_get
#define DB_put STACKDB_put
// stack DB has faster insert
#define DB_put_new STACKDB_put_new
#define DB_Iterator  STACKDB_Iterator
#define DB_Iterator_init  STACKDB_Iterator_init
#define DB_Iterator_next  STACKDB_Iterator_next
#define DB_maxStack db.maxStackDepth
// no support for shrinking
#define DB_getShrinkSize( dbP, n )  dbP->hashTableSize
#define DB_getCurrentSize( dbP )  dbP->hashTableSize
// no support for counting records
#define DB_getNumRecords( dbP ) 0
*/

/*
#define DB LINEARDB
#define DB_open LINEARDB_open
#define DB_close LINEARDB_close
#define DB_get LINEARDB_get
#define DB_put LINEARDB_put
// no distinction between put and put_new in lineardb
#define DB_put_new LINEARDB_put
#define DB_Iterator  LINEARDB_Iterator
#define DB_Iterator_init  LINEARDB_Iterator_init
#define DB_Iterator_next  LINEARDB_Iterator_next
#define DB_maxStack db.maxProbeDepth
#define DB_getShrinkSize  LINEARDB_getShrinkSize
#define DB_getCurrentSize  LINEARDB_getCurrentSize
#define DB_getNumRecords LINEARDB_getNumRecords
*/


#define DB LINEARDB3
#define DB_open LINEARDB3_open
#define DB_close LINEARDB3_close
#define DB_get LINEARDB3_get
#define DB_put LINEARDB3_put
// no distinction between put and put_new in lineardb3
#define DB_put_new LINEARDB3_put
#define DB_Iterator  LINEARDB3_Iterator
#define DB_Iterator_init  LINEARDB3_Iterator_init
#define DB_Iterator_next  LINEARDB3_Iterator_next
#define DB_maxStack db.maxOverflowDepth
#define DB_getShrinkSize  LINEARDB3_getShrinkSize
#define DB_getCurrentSize  LINEARDB3_getCurrentSize
#define DB_getNumRecords LINEARDB3_getNumRecords





    


    
#include "dbCommon.h"


#include <stdarg.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>


#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/GridPos.h"

#include "../gameSource/GridPos.h"
#include "../gameSource/objectMetadata.h"



timeSec_t startFrozenTime = -1;

timeSec_t frozenTime() {
    if( startFrozenTime == -1 ) {
        startFrozenTime = Time::timeSec();
        }
    return startFrozenTime;
    }


timeSec_t startFastTime = -1;

timeSec_t fastTime() {
    if( startFastTime == -1 ) {
        startFastTime = Time::timeSec();
        }
    return 1000 * ( Time::timeSec() - startFastTime ) + startFastTime;
    }



timeSec_t slowTime() {
    if( startFastTime == -1 ) {
        startFastTime = Time::timeSec();
        }
    return ( Time::timeSec() - startFastTime ) / 4 + startFastTime;
    }


// can replace with frozenTime to freeze time
// or slowTime to slow it down
#define MAP_TIMESEC Time::timeSec()
//#define MAP_TIMESEC frozenTime()
//#define MAP_TIMESEC fastTime()
//#define MAP_TIMESEC slowTime()


extern GridPos getClosestPlayerPos( int inX, int inY );

extern int getNumPlayers();



// track recent placements to determine camp where
// we'll stick next Eve
#define NUM_RECENT_PLACEMENTS 100

typedef struct RecentPlacement {
        GridPos pos;
        // depth of object in tech tree
        int depth;
    } RecentPlacement;


static RecentPlacement recentPlacements[ NUM_RECENT_PLACEMENTS ];

// ring buffer
static int nextPlacementIndex = 0;

static int eveRadiusStart = 2;
static int eveRadius = eveRadiusStart;



GridPos eveLocation = { 0,0 };
static int eveLocationUsage = 0;
static int maxEveLocationUsage = 3;

// eves are placed along an Archimedean spiral
// we track the angle of the last Eve to compute the position on
// the spiral of the next Eve
static double eveAngle = 2 * M_PI;

static char eveStartSpiralPosSet = false;
static GridPos eveStartSpiralPos = { 0, 0 };



static int evePrimaryLocSpacingX = 0;
static int evePrimaryLocSpacingY = 0;
static int evePrimaryLocObjectID = -1;
static SimpleVector<int> eveSecondaryLocObjectIDs;

static GridPos lastEvePrimaryLocation = {0,0};

static SimpleVector<GridPos> recentlyUsedPrimaryEvePositions;
static SimpleVector<int> recentlyUsedPrimaryEvePositionPlayerIDs;
// when they were place, so they can time out
static SimpleVector<double> recentlyUsedPrimaryEvePositionTimes;
// one hour
static double recentlyUsedPrimaryEvePositionTimeout = 3600;

static int eveHomeMarkerObjectID = -1;


static char allowSecondPlaceBiomes = false;



// what human-placed stuff, together, counts as a camp
static int campRadius = 20;

static float minEveCampRespawnAge = 60.0;


static int barrierRadius = 250;

static int barrierOn = 1;

static int longTermCullEnabled = 1;


static unsigned int biomeRandSeedA = 727;
static unsigned int biomeRandSeedB = 941;


static SimpleVector<int> barrierItemList;


static FILE *mapChangeLogFile = NULL;

static double mapChangeLogTimeStart = -1;



extern int apocalypsePossible;
extern char apocalypseTriggered;
extern GridPos apocalypseLocation;






// what object is placed on edge of map
static int edgeObjectID = 0;



static int currentResponsiblePlayer = -1;


void setResponsiblePlayer( int inPlayerID ) {
    currentResponsiblePlayer = inPlayerID;
    }



static double gapIntScale = 1000000.0;




// object ids that occur naturally on map at random, per biome
static int numBiomes;
static int *biomes;
static float *biomeWeights;
static float *biomeCumuWeights;
static float biomeTotalWeight;
static int regularBiomeLimit;

static int numSpecialBiomes;
static int *specialBiomes;
static float *specialBiomeCumuWeights;
static float specialBiomeTotalWeight;


static int specialBiomeBandMode;
static int specialBiomeBandThickness;
static SimpleVector<int> specialBiomeBandOrder;
// contains indices into biomes array instead of biome numbers
static SimpleVector<int> specialBiomeBandIndexOrder;

static SimpleVector<int> specialBiomeBandYCenter;

static int minActivePlayersForBirthlands;



// the biome index to use in place of special biomes outside of the north-most
// or south-most band
static int specialBiomeBandDefaultIndex;



// one vector per biome
static SimpleVector<int> *naturalMapIDs;
static SimpleVector<float> *naturalMapChances;

typedef struct MapGridPlacement {
        int id;
        int spacingX, spacingY;
        int phaseX, phaseY;
        int wiggleScaleX, wiggleScaleY;
        SimpleVector<int> permittedBiomes;
    } MapGridPlacement;

static SimpleVector<MapGridPlacement> gridPlacements;



static SimpleVector<int> allNaturalMapIDs;

static float *totalChanceWeight;


static int getBiomeIndex( int inBiome ) {
    for( int i=0; i<numBiomes; i++ ) {
        if( biomes[i] == inBiome ) {
            return i;
            }
        }
    return -1;
    }



// tracking when a given map cell was last seen
static DB lookTimeDB;
static char lookTimeDBOpen = false;



static DB db;
static char dbOpen = false;


static DB timeDB;
static char timeDBOpen = false;


static DB biomeDB;
static char biomeDBOpen = false;


static DB floorDB;
static char floorDBOpen = false;

static DB floorTimeDB;
static char floorTimeDBOpen = false;


static DB graveDB;
static char graveDBOpen = false;


// per-player memory of where they should spawn as eve
static DB eveDB;
static char eveDBOpen = false;


static DB metaDB;
static char metaDBOpen = false;



static int randSeed = 124567;
//static JenkinsRandomSource randSource( randSeed );
static CustomRandomSource randSource( randSeed );



#define DECAY_SLOT 1
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

#define NO_DECAY_SLOT -1


// decay slots for contained items start after container slots


// 15 minutes
static int maxSecondsForActiveDecayTracking = 900;

// 15 seconds (before no-look regions are purged from live tracking)
static int maxSecondsNoLookDecayTracking = 15;

// live players look at their surrounding map region every 5 seconds
// we count a region as stale after no one looks at it for 10 seconds
// (we actually purge the live tracking of that region after 15 seconds).
// This gives us some wiggle room with the timing, so we always make
// sure to re-look at a region (when walking back into it) that is >10
// seconds old, because it may (or may not) have fallen out of our live
// tracking (if our re-look time was 15 seconds to match the time stuff actually
// is dropped from live tracking, we might miss some stuff, depending
// on how the check calls are interleaved time-wise).
static int noLookCountAsStaleSeconds = 10;



typedef struct LiveDecayRecord {
        int x, y;
        
        // 0 means main object decay
        // 1 - NUM_CONT_SLOT means contained object decay
        int slot;
        
        timeSec_t etaTimeSeconds;

        // 0 means main object
        // >0 indexs sub containers of object
        int subCont;

        // the transition that will apply when this decay happens
        // this allows us to avoid marking certain types of move decays
        // as stale when not looked at in a while (all other types of decays
        // go stale)
        // Can be NULL if we don't care about the transition
        // associated with this decay (for contained item decay, for example)
        TransRecord *applicableTrans;

    } LiveDecayRecord;



#include "minorGems/util/MinPriorityQueue.h"

static MinPriorityQueue<LiveDecayRecord> liveDecayQueue;


// for quick lookup of existing records in liveDecayQueue
// store the eta time here
// before storing a new record in the queue, we can check this hash
// table to see whether it already exists
static HashTable<timeSec_t> liveDecayRecordPresentHashTable( 1024 );

// times in seconds that a tracked live decay map cell or slot
// was last looked at
static HashTable<timeSec_t> liveDecayRecordLastLookTimeHashTable( 1024 );


typedef struct ContRecord {
    int maxSlots;
    int maxSubSlots;
    } ContRecord;

static ContRecord defaultContRecord = { 0, 0 };


// track max tracked contained for each x,y
// this allows us to update last look times without getting contained count
// from map
// indexed as x, y, 0, 0
static HashTable<ContRecord> 
liveDecayRecordLastLookTimeMaxContainedHashTable( 1024, defaultContRecord );



static CoordinateTimeTracking lookTimeTracking;



// track currently in-process movements so that we can be queried
// about whether arrival has happened or not
typedef struct MovementRecord {
        int x, y;
        int sourceX, sourceY;
        int id;
        char deadly;
        double etaTime;
        double totalTime;
    } MovementRecord;


// clock time in fractional seconds of destination ETA
// indexed as x, y, 0
static HashTable<double> liveMovementEtaTimes( 1024, 0 );

static MinPriorityQueue<MovementRecord> liveMovements;



    

// track all map changes that happened since the last
// call to stepMap
static SimpleVector<ChangePosition> mapChangePosSinceLastStep;


static char anyBiomesInDB = false;
static int maxBiomeXLoc = -2000000000;
static int maxBiomeYLoc = -2000000000;
static int minBiomeXLoc = 2000000000;
static int minBiomeYLoc = 2000000000;




// if true, rest of natural map is blank
static char useTestMap = false;

// read from testMap.txt
// unless testMapStale.txt is present

// each line contains data in this order:
// x y biome floor id_and_contained
// id and contained are in CONTAINER OBJECT FORMAT described in protocol.txt
// biome = -1 means use naturally-occurring biome
typedef struct TestMapRecord {
        int x, y;
        int biome;
        int floor;
        int id;
        SimpleVector<int> contained;
        SimpleVector< SimpleVector<int> > subContained;
    } TestMapRecord;





typedef struct Homeland {
        int x, y;

        int radius;

        int lineageEveID;
        
        double lastBabyBirthTime;
        
        char expired;
        
        char changed;
        
        // did the creation of this homeland tapout-trigger
        // a +primaryHomeland object?
        char primary;

        // should Eve placement ignore this homeland?
        char ignoredForEve;

        // was this homeland the first for this lineageEveID?
        // this can be true even if not primary, if eve resettles an old village
        // and does not tapout trigger anything
        char firstHomelandForFamily;

    } Homeland;



static SimpleVector<Homeland> homelands;


static void expireHomeland( Homeland *inH ) {
    inH->expired = true;
    inH->changed = true;

    
    if( ! inH->primary ) {
        // apply expiration transition to whatever is at center of
        // non-primary homeland 
        // (object operating on itself defines this)
        
        int centerID = getMapObject( inH->x, inH->y );
        
        if( centerID > 0 ) {
            TransRecord *expireTrans = getTrans( centerID, centerID );
            if( expireTrans != NULL ) {
                setMapObject( inH->x, inH->y, expireTrans->newTarget );
                }
            }
        }
    }




// NULL if not found
static Homeland *getHomeland( int inX, int inY, 
                              char includeExpired = false ) {
    double t = Time::getCurrentTime();
    
    int staleTime = 
        SettingsManager::getIntSetting( "homelandStaleSeconds", 3600 );
    
    double tooOldTime = t - staleTime;

    for( int i=0; i<homelands.size(); i++ ) {
        Homeland *h = homelands.getElement( i );
        
        // watch for stale
        if( ! h->expired && h->lastBabyBirthTime < tooOldTime ) {
            expireHomeland( h );
            }

        
        if( ! includeExpired && h->expired ) {
            continue;
            }


        if( inX < h->x + h->radius &&
            inX > h->x - h->radius &&
            inY < h->y + h->radius &&
            inY > h->y - h->radius ) {
            return h;
            }
        }
    return NULL;
    }



static char hasPrimaryHomeland( int inLineageEveID ) {
    for( int i=0; i<homelands.size(); i++ ) {
        Homeland *h = homelands.getElement( i );
        if( h->primary && h->lineageEveID == inLineageEveID ) {
            return true;
            }
        }
    return false;
    }



#include "../commonSource/fractalNoise.h"

















timeSec_t dbLookTimeGet( int inX, int inY );
void dbLookTimePut( int inX, int inY, timeSec_t inTime );




// returns -1 if not found
static int biomeDBGet( int inX, int inY, 
                       int *outSecondPlaceBiome = NULL,
                       double *outSecondPlaceGap = NULL ) {
    unsigned char key[8];
    unsigned char value[12];

    // look for changes to default in database
    intPairToKey( inX, inY, key );
    
    int result = DB_get( &biomeDB, key, value );
    
    if( result == 0 ) {
        // found
        int biome = valueToInt( &( value[0] ) );
        
        if( outSecondPlaceBiome != NULL ) {
            *outSecondPlaceBiome = valueToInt( &( value[4] ) );
            }
        
        if( outSecondPlaceGap != NULL ) {
            *outSecondPlaceGap = valueToInt( &( value[8] ) ) / gapIntScale;
            }
        
        return biome;
        }
    else {
        return -1;
        }
    }



static void biomeDBPut( int inX, int inY, int inValue, int inSecondPlace,
                        double inSecondPlaceGap ) {
    unsigned char key[8];
    unsigned char value[12];
    

    intPairToKey( inX, inY, key );
    intToValue( inValue, &( value[0] ) );
    intToValue( inSecondPlace, &( value[4] ) );
    intToValue( lrint( inSecondPlaceGap * gapIntScale ), 
                &( value[8] ) );
            
    
    anyBiomesInDB = true;

    if( inX > maxBiomeXLoc ) {
        maxBiomeXLoc = inX;
        }
    if( inX < minBiomeXLoc ) {
        minBiomeXLoc = inX;
        }
    if( inY > maxBiomeYLoc ) {
        maxBiomeYLoc = inY;
        }
    if( inY < minBiomeYLoc ) {
        minBiomeYLoc = inY;
        }
    

    DB_put( &biomeDB, key, value );
    }
    



// returns -1 on failure, 1 on success
static int eveDBGet( const char *inEmail, int *outX, int *outY, 
                     int *outRadius ) {
    unsigned char key[50];
    
    unsigned char value[12];


    emailToKey( inEmail, key );
    
    int result = DB_get( &eveDB, key, value );
    
    if( result == 0 ) {
        // found
        *outX = valueToInt( &( value[0] ) );
        *outY = valueToInt( &( value[4] ) );
        *outRadius = valueToInt( &( value[8] ) );
        
        return 1;
        }
    else {
        return -1;
        }
    }



static void eveDBPut( const char *inEmail, int inX, int inY, int inRadius ) {
    unsigned char key[50];
    unsigned char value[12];
    

    emailToKey( inEmail, key );
    
    intToValue( inX, &( value[0] ) );
    intToValue( inY, &( value[4] ) );
    intToValue( inRadius, &( value[8] ) );
            
    
    DB_put( &eveDB, key, value );
    }



static void dbFloorPut( int inX, int inY, int inValue );




// inKnee in 0..inf, smaller values make harder knees
// intput in 0..1
// output in 0..1

// from Simplest AI trick in the book:
// Normalized Tunable SIgmoid Function 
// Dino Dini, GDC 2013
double sigmoid( double inInput, double inKnee ) {
    
    // in -1,-1
    double shiftedInput = inInput * 2 - 1;
    

    double sign = 1;
    if( shiftedInput < 0 ) {
        sign = -1;
        }
    
    
    double k = -1 - inKnee;
    
    double absInput = fabs( shiftedInput );

    // out in -1..1
    double out = sign * absInput * k / ( 1 + k - absInput );
    
    return ( out + 1 ) * 0.5;
    }






// optimization:
// cache biomeIndex results in RAM

// 3.1 MB of RAM for this.
#define BIOME_CACHE_SIZE 131072

typedef struct BiomeCacheRecord {
        int x, y;
        int biome, secondPlace;
        double secondPlaceGap;
    } BiomeCacheRecord;
    
static BiomeCacheRecord biomeCache[ BIOME_CACHE_SIZE ];


#define CACHE_PRIME_A 776509273
#define CACHE_PRIME_B 904124281
#define CACHE_PRIME_C 528383237
#define CACHE_PRIME_D 148497157

static int computeXYCacheHash( int inKeyA, int inKeyB ) {
    
    int hashKey = ( inKeyA * CACHE_PRIME_A + 
                    inKeyB * CACHE_PRIME_B ) % BIOME_CACHE_SIZE;
    if( hashKey < 0 ) {
        hashKey += BIOME_CACHE_SIZE;
        }
    return hashKey;
    }



static void initBiomeCache() {
    BiomeCacheRecord blankRecord = { 0, 0, -2, 0, 0 };
    for( int i=0; i<BIOME_CACHE_SIZE; i++ ) {
        biomeCache[i] = blankRecord;
        }
    }

    


// returns -2 on miss
static int biomeGetCached( int inX, int inY, 
                           int *outSecondPlaceIndex,
                           double *outSecondPlaceGap ) {
    BiomeCacheRecord r =
        biomeCache[ computeXYCacheHash( inX, inY ) ];

    if( r.x == inX && r.y == inY ) {
        *outSecondPlaceIndex = r.secondPlace;
        *outSecondPlaceGap = r.secondPlaceGap;
        
        return r.biome;
        }
    else {
        return -2;
        }
    }



static void biomePutCached( int inX, int inY, int inBiome, int inSecondPlace,
                            double inSecondPlaceGap ) {
    BiomeCacheRecord r = { inX, inY, inBiome, inSecondPlace, inSecondPlaceGap };
    
    biomeCache[ computeXYCacheHash( inX, inY ) ] = r;
    }




static int getSpecialBiomeIndexForYBand( int inY, char *outOfBand = NULL ) {
    if( outOfBand != NULL ) {
        *outOfBand = false;
        }
    
    // new method, use y centers and thickness
    int radius = specialBiomeBandThickness / 2;
    
    for( int i=0; i<specialBiomeBandYCenter.size(); i++ ) {
        int yCenter = specialBiomeBandYCenter.getElementDirect( i );
        
        if( abs( inY - yCenter ) <= radius ) {
            return specialBiomeBandIndexOrder.getElementDirect( i );
            }
        }
    

    // else not in radius of any band
    if( outOfBand != NULL ) {
        *outOfBand = true;
        }
    
    return specialBiomeBandDefaultIndex;
    }




// new code, topographic rings
static int computeMapBiomeIndex( int inX, int inY, 
                                 int *outSecondPlaceIndex = NULL,
                                 double *outSecondPlaceGap = NULL ) {
        
    int secondPlace = -1;
    
    double secondPlaceGap = 0;


    int pickedBiome = biomeGetCached( inX, inY, &secondPlace, &secondPlaceGap );
        
    if( pickedBiome != -2 ) {
        // hit cached

        if( outSecondPlaceIndex != NULL ) {
            *outSecondPlaceIndex = secondPlace;
            }
        if( outSecondPlaceGap != NULL ) {
            *outSecondPlaceGap = secondPlaceGap;
            }
    
        return pickedBiome;
        }

    // else cache miss
    pickedBiome = -1;


    // try topographical altitude mapping

    setXYRandomSeed( biomeRandSeedA, biomeRandSeedB );

    double randVal = 
        ( getXYFractal( inX, inY,
                        0.55, 
                        0.83332 + 0.08333 * numBiomes ) );

    // push into range 0..1, based on sampled min/max values
    randVal -= 0.099668;
    randVal *= 1.268963;


    // flatten middle
    //randVal = ( pow( 2*(randVal - 0.5 ), 3 ) + 1 ) / 2;


    // push into range 0..1 with manually tweaked values
    // these values make it pretty even in terms of distribution:
    //randVal -= 0.319;
    //randVal *= 3;

    

    // these values are more intuitve to make a map that looks good
    //randVal -= 0.23;
    //randVal *= 1.9;
    

    

    
    // apply gamma correction
    //randVal = pow( randVal, 1.5 );
    /*
    randVal += 0.4* sin( inX / 40.0 );
    randVal += 0.4 *sin( inY / 40.0 );
    
    randVal += 0.8;
    randVal /= 2.6;
    */

    // slow arc n to s:

    // pow version has flat area in middle
    //randVal += 0.7 * pow( ( inY / 354.0 ), 3 ) ;

    // sin version 
    //randVal += 0.3 * sin( 0.5 * M_PI * inY / 354.0 );
    

    /*
        ( sin( M_PI * inY / 708 ) + 
          (1/3.0) * sin( 3 * M_PI * inY / 708 ) );
    */
    //randVal += 0.5;
    //randVal /= 2.0;


    
    float i = randVal * biomeTotalWeight;
    
    pickedBiome = 0;
    while( pickedBiome < numBiomes &&
           i > biomeCumuWeights[pickedBiome] ) {
        pickedBiome++;
        }
    if( pickedBiome >= numBiomes ) {
        pickedBiome = numBiomes - 1;
        }
    


    if( pickedBiome >= regularBiomeLimit && numSpecialBiomes > 0 ) {
        // special case:  on a peak, place a special biome here


        if( specialBiomeBandMode ) {
            // use band mode for these
            pickedBiome = getSpecialBiomeIndexForYBand( inY );
            
            secondPlace = regularBiomeLimit - 1;
            secondPlaceGap = 0.1;
            }
        else {
            // use patches mode for these
            pickedBiome = -1;
            
            
            double maxValue = -10;
            double secondMaxVal = -10;
            
            for( int i=regularBiomeLimit; i<numBiomes; i++ ) {
                int biome = biomes[i];
                
                setXYRandomSeed( biome * 263 + biomeRandSeedA + 38475,
                                 biomeRandSeedB );
                
                double randVal = getXYFractal(  inX,
                                                inY,
                                                0.55, 
                                                2.4999 + 
                                                0.2499 * numSpecialBiomes );
                
                if( randVal > maxValue ) {
                    if( maxValue != -10 ) {
                        secondMaxVal = maxValue;
                        }
                    maxValue = randVal;
                    pickedBiome = i;
                    }
                }
            
            if( maxValue - secondMaxVal < 0.03 ) {
                // close!  that means we're on a boundary between special biomes
                
                // stick last regular biome on this boundary, so special
                // biomes never touch
                secondPlace = pickedBiome;
                secondPlaceGap = 0.1;
                pickedBiome = regularBiomeLimit - 1;
                }        
            else {
                secondPlace = regularBiomeLimit - 1;
                secondPlaceGap = 0.1;
                }
            }
        }
    else {
        // second place for regular biome rings
        
        secondPlace = pickedBiome - 1;
        if( secondPlace < 0 ) {
            secondPlace = pickedBiome + 1;
            }
        secondPlaceGap = 0.1;
        }
    

    if( ! allowSecondPlaceBiomes ) {
        // make the gap ridiculously big, so that second-place placement
        // never happens.
        // but keep secondPlace set different from pickedBiome
        // (elsewhere in code, we avoid placing animals if 
        // secondPlace == picked
        secondPlaceGap = 10.0;
        }


    biomePutCached( inX, inY, pickedBiome, secondPlace, secondPlaceGap );
    
    
    if( outSecondPlaceIndex != NULL ) {
        *outSecondPlaceIndex = secondPlace;
        }
    if( outSecondPlaceGap != NULL ) {
        *outSecondPlaceGap = secondPlaceGap;
        }
    
    return pickedBiome;
    }




// old code, separate height fields per biome that compete
// and create a patchwork layout
static int computeMapBiomeIndexOld( int inX, int inY, 
                                 int *outSecondPlaceIndex = NULL,
                                 double *outSecondPlaceGap = NULL ) {
        
    int secondPlace = -1;
    
    double secondPlaceGap = 0;


    int pickedBiome = biomeGetCached( inX, inY, &secondPlace, &secondPlaceGap );
        
    if( pickedBiome != -2 ) {
        // hit cached

        if( outSecondPlaceIndex != NULL ) {
            *outSecondPlaceIndex = secondPlace;
            }
        if( outSecondPlaceGap != NULL ) {
            *outSecondPlaceGap = secondPlaceGap;
            }
    
        return pickedBiome;
        }

    // else cache miss
    pickedBiome = -1;


    double maxValue = -DBL_MAX;

    
    for( int i=0; i<numBiomes; i++ ) {
        int biome = biomes[i];
        
        setXYRandomSeed( biome * 263 + biomeRandSeedA, biomeRandSeedB );

        double randVal = getXYFractal(  inX,
                                        inY,
                                        0.55, 
                                        0.83332 + 0.08333 * numBiomes );
        
        if( randVal > maxValue ) {
            // a new first place
            
            // old first moves into second
            secondPlace = pickedBiome;
            secondPlaceGap = randVal - maxValue;
            

            maxValue = randVal;
            pickedBiome = i;
            }
        else if( randVal > maxValue - secondPlaceGap ) {
            // a better second place
            secondPlace = i;
            secondPlaceGap = maxValue - randVal;
            }
        }
    
    biomePutCached( inX, inY, pickedBiome, secondPlace, secondPlaceGap );
    
    
    if( outSecondPlaceIndex != NULL ) {
        *outSecondPlaceIndex = secondPlace;
        }
    if( outSecondPlaceGap != NULL ) {
        *outSecondPlaceGap = secondPlaceGap;
        }
    
    return pickedBiome;
    }




static int getMapBiomeIndex( int inX, int inY, 
                             int *outSecondPlaceIndex = NULL,
                             double *outSecondPlaceGap = NULL ) {
    
    int secondPlaceBiome = -1;
    
    int dbBiome = -1;
    
    if( anyBiomesInDB && 
        inX >= minBiomeXLoc && inX <= maxBiomeXLoc &&
        inY >= minBiomeYLoc && inY <= maxBiomeYLoc ) {
        // don't bother with this call unless biome DB has
        // something in it, and this inX,inY is in the region where biomes
        // exist in the database (tutorial loading, or test maps)
        dbBiome = biomeDBGet( inX, inY,
                              &secondPlaceBiome,
                              outSecondPlaceGap );
        }
    

    if( dbBiome != -1 ) {

        int index = getBiomeIndex( dbBiome );
        
        if( index != -1 ) {
            // biome still exists!

            char secondPlaceFailed = false;
            
            if( outSecondPlaceIndex != NULL ) {
                int secondIndex = getBiomeIndex( secondPlaceBiome );

                if( secondIndex != -1 ) {
                    
                    *outSecondPlaceIndex = secondIndex;
                    }
                else {
                    secondPlaceFailed = true;
                    }
                }

            if( ! secondPlaceFailed ) {
                return index;
                }
            }
        else {
            dbBiome = -1;
            }
        
        // else a biome or second place in biome.db that isn't in game anymore
        // ignore it
        }
    
        
    int secondPlace = -1;
    
    double secondPlaceGap = 0;


    int pickedBiome = computeMapBiomeIndex( inX, inY, 
                                            &secondPlace, &secondPlaceGap );

    
    if( outSecondPlaceIndex != NULL ) {
        *outSecondPlaceIndex = secondPlace;
        }
    if( outSecondPlaceGap != NULL ) {
        *outSecondPlaceGap = secondPlaceGap;
        }


    if( dbBiome == -1 || secondPlaceBiome == -1 ) {
        // not stored, OR some part of stored stale, re-store it

        secondPlaceBiome = 0;
        if( secondPlace != -1 ) {
            secondPlaceBiome = biomes[ secondPlace ];
            }
        
        // skip saving proc-genned biomes for now
        // huge RAM impact as players explore distant areas of map
        
        // we still check the biomeDB above for loading test maps
        /*
        biomeDBPut( inX, inY, biomes[pickedBiome], 
                    secondPlaceBiome, secondPlaceGap );
        */
        }
    
    
    return pickedBiome;
    }




// gets procedurally-generated base map at a given spot
// player modifications are overlayed on top of this

// SIDE EFFECT:
// if biome at x,y needed to be determined in order to compute map
// at this spot, it is saved into lastCheckedBiome

static int lastCheckedBiome = -1;
static int lastCheckedBiomeX = 0;
static int lastCheckedBiomeY = 0;


// 1671 shy of int max
static int xLimit = 2147481977;
static int yLimit = 2147481977;





typedef struct BaseMapCacheRecord {
        int x, y;
        int id;
        char gridPlacement;
    } BaseMapCacheRecord;


// should be a power of 2
// cache will contain squared number of records
#define BASE_MAP_CACHE_SIZE 256

// if BASE_MAP_CACHE_SIZE is a power of 2, then this is the bit mask
// of solid 1's that can limit an integer to that range
static int mapCacheBitMask = BASE_MAP_CACHE_SIZE - 1;

BaseMapCacheRecord baseMapCache[ BASE_MAP_CACHE_SIZE ][ BASE_MAP_CACHE_SIZE ];

static void mapCacheClear() {
    for( int y=0; y<BASE_MAP_CACHE_SIZE; y++ ) {
        for( int x=0; x<BASE_MAP_CACHE_SIZE; x++ ) {
            baseMapCache[y][x].x = 0;
            baseMapCache[y][x].y = 0;
            baseMapCache[y][x].id = -1;
            }
        }
    }



static BaseMapCacheRecord *mapCacheRecordLookup( int inX, int inY ) {
    // apply bitmask to x and y
    return &( baseMapCache[ inY & mapCacheBitMask ][ inX & mapCacheBitMask ] ); 
    }



// returns -1 if not in cache
static int mapCacheLookup( int inX, int inY, char *outGridPlacement = NULL ) {
    BaseMapCacheRecord *r = mapCacheRecordLookup( inX, inY );
    
    if( r->x == inX && r->y == inY ) {
        if( outGridPlacement != NULL ) {
            *outGridPlacement = r->gridPlacement;
            }
        return r->id;
        }

    return -1;
    }



static void mapCacheInsert( int inX, int inY, int inID, 
                            char inGridPlacement = false ) {
    BaseMapCacheRecord *r = mapCacheRecordLookup( inX, inY );
    
    r->x = inX;
    r->y = inY;
    r->id = inID;
    r->gridPlacement = inGridPlacement;
    }

    

static int getBaseMapCallCount = 0;


static int getBaseMap( int inX, int inY, char *outGridPlacement = NULL ) {
    
    if( inX > xLimit || inX < -xLimit ||
        inY > yLimit || inY < -yLimit ) {
    
        return edgeObjectID;
        }
    
    int cachedID = mapCacheLookup( inX, inY, outGridPlacement );
    
    if( cachedID != -1 ) {
        return cachedID;
        }
    
    getBaseMapCallCount ++;


    if( outGridPlacement != NULL ) {
        *outGridPlacement = false;
        }
    

    // see if any of our grids apply
    setXYRandomSeed( 9753 );

    for( int g=0; g < gridPlacements.size(); g++ ) {
        MapGridPlacement *gp = gridPlacements.getElement( g );


        /*
        double gridWiggleX = getXYFractal( inX / gp->spacingX, 
                                           inY / gp->spacingY, 
                                           0.1, 0.25 );
        
        double gridWiggleY = getXYFractal( inX / gp->spacingX, 
                                           inY / gp->spacingY + 392387, 
                                           0.1, 0.25 );
        */
        // turn wiggle off for now
        double gridWiggleX = 0;
        double gridWiggleY = 0;

        if( ( inX + gp->phaseX + lrint( gridWiggleX * gp->wiggleScaleX ) ) 
            % gp->spacingX == 0 &&
            ( inY + gp->phaseY + lrint( gridWiggleY * gp->wiggleScaleY ) ) 
            % gp->spacingY == 0 ) {
            
            // hits this grid

            // make sure this biome is on the list for this object
            int secondPlace;
            double secondPlaceGap;
        
            int pickedBiome = getMapBiomeIndex( inX, inY, &secondPlace,
                                                &secondPlaceGap );
        
            if( pickedBiome == -1 ) {
                mapCacheInsert( inX, inY, 0 );
                return 0;
                }

            if( gp->permittedBiomes.getElementIndex( pickedBiome ) != -1 ) {
                mapCacheInsert( inX, inY, gp->id, true );

                if( outGridPlacement != NULL ) {
                    *outGridPlacement = true;
                    }
                return gp->id;
                }
            }    
        }
             


    setXYRandomSeed( 5379 );
    
    // first step:  save rest of work if density tells us that
    // nothing is here anyway
    double density = getXYFractal( inX, inY, 0.1, 0.25 );
    
    // correction
    density = sigmoid( density, 0.1 );
    
    // scale
    density *= .4;
    // good for zoom in to map for teaser
    //density = .70;
    
    setXYRandomSeed( 9877 );
    
    if( getXYRandom( inX, inY ) < density ) {




        // next step, pick top two biomes
        int secondPlace;
        double secondPlaceGap;
        
        int pickedBiome = getMapBiomeIndex( inX, inY, &secondPlace,
                                            &secondPlaceGap );
        
        if( pickedBiome == -1 ) {
            mapCacheInsert( inX, inY, 0 );
            return 0;
            }
        
        // only override if it's not already set
        // if it's already set, then we're calling getBaseMap for neighboring
        // map cells (wide, tall, moving objects, etc.)
        // getBaseMap is always called for our cell in question first
        // before examining neighboring cells if needed
        if( lastCheckedBiome == -1 ) {    
            lastCheckedBiome = biomes[pickedBiome];
            lastCheckedBiomeX = inX;
            lastCheckedBiomeY = inY;
            }
        

        
        // randomly let objects from second place biome peek through
        
        // if gap is 0, this should happen 50 percent of the time

        // if gap is 1.0, it should never happen

        // larger values make second place less likely
        double secondPlaceReduction = 10.0;

        //printf( "Second place gap = %f, random(%d,%d)=%f\n", secondPlaceGap,
        //        inX, inY, getXYRandom( 2087 + inX, 793 + inY ) );
        
        setXYRandomSeed( 348763 );
        
        if( getXYRandom( inX, inY ) > 
            .5 + secondPlaceReduction * secondPlaceGap ) {
        
            // note that lastCheckedBiome is NOT changed, so ground
            // shows the true, first-place biome, but object placement
            // follows the second place biome
            pickedBiome = secondPlace;
            }
        

        int numObjects = naturalMapIDs[pickedBiome].size();

        if( numObjects == 0  ) {
            mapCacheInsert( inX, inY, 0 );
            return 0;
            }

    
  
        // something present here

        
        // special object in this region is 10x more common than it 
        // would be otherwise


        int specialObjectIndex = -1;
        double maxValue = -DBL_MAX;
        

        for( int i=0; i<numObjects; i++ ) {
            
            setXYRandomSeed( 793 * i + 123 );
        
            double randVal = getXYFractal(  inX, 
                                            inY, 
                                            0.3, 
                                            0.15 + 0.016666 * numObjects );

            if( randVal > maxValue ) {
                maxValue = randVal;
                specialObjectIndex = i;
                }
            }



        float oldSpecialChance = 
            naturalMapChances[pickedBiome].getElementDirect( 
                specialObjectIndex );
        
        float newSpecialChance = oldSpecialChance * 10;
        
        *( naturalMapChances[pickedBiome].getElement( specialObjectIndex ) )
            = newSpecialChance;
        
        float oldTotalChanceWeight = totalChanceWeight[pickedBiome];
        
        totalChanceWeight[pickedBiome] -= oldSpecialChance;
        totalChanceWeight[pickedBiome] += newSpecialChance;
        

        // pick one of our natural objects at random

        // pick value between 0 and total weight
        
        setXYRandomSeed( 4593873 );
        
        double randValue = 
            totalChanceWeight[pickedBiome] * getXYRandom( inX, inY );

        // walk through objects, summing weights, until one crosses threshold
        int i = 0;
        float weightSum = 0;        
        
        while( weightSum < randValue && i < numObjects ) {
            weightSum += naturalMapChances[pickedBiome].getElementDirect( i );
            i++;
            }
        
        i--;
        

        // restore chance of special object
        *( naturalMapChances[pickedBiome].getElement( specialObjectIndex ) )
            = oldSpecialChance;

        totalChanceWeight[pickedBiome] = oldTotalChanceWeight;

        if( i >= 0 ) {
            int returnID = naturalMapIDs[pickedBiome].getElementDirect( i );
            
            if( pickedBiome == secondPlace ) {
                // object peeking through from second place biome

                // make sure it's not a moving object (animal)
                // those are locked to their target biome only
                TransRecord *t = getPTrans( -1, returnID );
                if( t != NULL && t->move != 0 ) {
                    // put empty tile there instead
                    returnID = 0;
                    }
                }

            mapCacheInsert( inX, inY, returnID );
            return returnID;
            }
        else {
            mapCacheInsert( inX, inY, 0 );
            return 0;
            }
        }
    else {
        mapCacheInsert( inX, inY, 0 );
        return 0;
        }
    
    }











#include "minorGems/graphics/Image.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"
#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"

void outputMapImage() {
    
    // output a chunk of the map as an image

    int w =  708;
    int h = 708;
    
    Image objIm( w, h, 3, true );
    Image biomeIm( w, h, 3, true );
    
    SimpleVector<Color> objColors;
    SimpleVector<Color> biomeColors;
    SimpleVector<int> objCounts;
    
    int totalCounts = 0;

    for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
        Color *c = Color::makeColorFromHSV( (float)i / allNaturalMapIDs.size(),
                                            1, 1 );
        objColors.push_back( *c );

        objCounts.push_back( 0 );
        
        delete c;
        }

    SimpleVector<int> biomeCounts;
    int totalBiomeCount = 0;

    for( int j=0; j<numBiomes; j++ ) {        
        biomeCounts.push_back( 0 );
        
        Color *c;
        
        int biomeNumber = biomes[j];

        switch( biomeNumber ) {
            case 0:
                c = new Color( 0, 0.8, .1 );
                break;
            case 1:
                c = new Color( 0.4, 0.2, 0.7 );
                break;
            case 2:
                c = new Color( 1, .8, 0 );
                break;
            case 3:
                c = new Color( 0.6, 0.6, 0.6 );
                break;
            case 4:
                c = new Color( 1, 1, 1 );
                break;
            case 5:
                c = new Color( 0.7, 0.6, 0.0 );
                break;
            case 6:
                c = new Color( 0.0, 0.5, 0.0 );
                break;
            default:
                c = Color::makeColorFromHSV( (float)j / numBiomes, 1, 1 );
            }
        
        biomeColors.push_back( *c );
        delete c;
        }
    
    /*
    double startTime = Time::getCurrentTime();
    for( int y = 0; y<h; y++ ) {
        
        for( int x = 0; x<w; x++ ) {
            // discard id output
            // just invoking this to time it
            getBaseMap( x, y );
            }
        }
    
    printf( "Generating %d map spots took %f sec\n",
            w * h, Time::getCurrentTime() - startTime );
    //exit(0);
    
    */


    for( int y = 0; y<h; y++ ) {
        
        for( int x = 0; x<w; x++ ) {

            /*
            // raw rand output and correlation test
            uint32_t xHit = xxTweakedHash2D( x, y );
            uint32_t yHit = xxTweakedHash2D( x+1, y+1 );
            //uint32_t xHit = getXYRandom_test( x, y );
            //uint32_t yHit = getXYRandom_test( x+1, y );
            
            xHit = xHit % w;
            yHit = yHit % h;
            
            double val = xxTweakedHash2D( x, y ) * oneOverIntMax;
            
            Color c = im.getColor( yHit * w + xHit );
            c.r += 0.1;
            c.g += 0.1;
            c.b += 0.1;
            
            im.setColor( yHit * w + xHit, c );
              
            c.r = val;
            c.g = val;
            c.b = val;
            
            //im.setColor( y * w + x, c );
            */
            
            
            int id = getBaseMap( x - h/2, - ( y - h/2 ) );
            
            int biomeInd = getMapBiomeIndex( x - h/2, -( y - h/2 ) );

            if( id > 0 ) {
                for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
                    if( allNaturalMapIDs.getElementDirect(i) == id ) {
                        objIm.setColor( y * w + x,
                                     objColors.getElementDirect( i ) );
                        
                        (* objCounts.getElement( i ) )++;
                        totalCounts++;
                        break;
                        }
                    }
                }
            
            biomeIm.setColor( y * w + x,
                              biomeColors.getElementDirect( biomeInd ) );
            ( *( biomeCounts.getElement( biomeInd ) ) ) ++;
            totalBiomeCount++;
            }
        }


    const char *biomeNames[] = { "Grass ",
                                 "Swamp ",
                                 "Yellow",
                                 "Gray  ",
                                 "Snow  ",
                                 "Desert",
                                 "Jungle" };    

    for( int j=0; j<numBiomes; j++ ) {
        const char *name = "unknwn";
        
        if( biomes[j] < 7 ) {
            name = biomeNames[ biomes[j] ];
            }
        int c = biomeCounts.getElementDirect( j );
        
        printf( "Biome %d (%s) \tcount = %d\t%.1f%%\n", 
                biomes[j], name, c, 100 * (float)c / totalBiomeCount );
        }


    
    for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
        ObjectRecord *obj = getObject( allNaturalMapIDs.getElementDirect( i ) );
        
        int count = objCounts.getElementDirect( i );
        
        printf( 
            "%d\t%-30s  (actual=%f, expected=%f\n",
            objCounts.getElementDirect( i ),
            obj->description,
            count / (double)totalCounts,
            obj->mapChance / allNaturalMapIDs.size() );
        }

    // rough legend in corner
    for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
        if( i < h ) {
            objIm.setColor( i * w + 0, objColors.getElementDirect( i ) );
            }
        }
    

    File tgaFile( NULL, "mapOut.tga" );
    FileOutputStream tgaStream( &tgaFile );
    
    TGAImageConverter converter;
    
    converter.formatImage( &objIm, &tgaStream );


    File tgaBiomeFile( NULL, "mapBiomeOut.tga" );
    FileOutputStream tgaBiomeStream( &tgaBiomeFile );
    
    converter.formatImage( &biomeIm, &tgaBiomeStream );
    
    exit(0);
    }




void outputBiomeFractals() {
    for( int scale = 1; scale <=4; scale *= 2 ) {
        
        for( int b=0; b<numBiomes; b++ ) {
            int biome = biomes[ b ];
            
            setXYRandomSeed( biome * 263 + 723 );

            int r = 100 * scale;
            
            Image outIm( r * 2, r * 2, 4 );
            
            for( int y=-r; y<r; y++ ) {
                for( int x=-r; x<r; x++ ) {
                    
                    double v = getXYFractal(  x, 
                                              y, 
                                              0.55, 
                                              scale );
                    Color c( v, v, v, 1 );

                    int imX = x + r;
                    int imY = y + r;
                    
                    outIm.setColor( imY * 2 * r  + imX, c );
                    }
                }
            
            char *name = autoSprintf( "fractal_b%d_s%d.tga",
                                      biome, scale );
            
            File tgaFile( NULL, name );
            
            FileOutputStream tgaStream( &tgaFile );
    
            TGAImageConverter converter;
    
            converter.formatImage( &outIm, &tgaStream );
            printf( "Outputting file %s\n", name );
            
            delete [] name;
            }
        }       
    }






int *getContainedRaw( int inX, int inY, int *outNumContained, 
                      int inSubCont = 0 );

void setMapObjectRaw( int inX, int inY, int inID );

static void dbPut( int inX, int inY, int inSlot, int inValue, 
                   int inSubCont = 0 );




void writeRecentPlacements() {
    FILE *placeFile = fopen( "recentPlacements.txt", "w" );
    if( placeFile != NULL ) {
        for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
            fprintf( placeFile, "%d,%d %d\n", recentPlacements[i].pos.x,
                     recentPlacements[i].pos.y,
                     recentPlacements[i].depth );
            }
        fprintf( placeFile, "nextPlacementIndex=%d\n", nextPlacementIndex );
        
        fclose( placeFile );
        }
    }



static void writeEveRadius() {
    FILE *eveRadFile = fopen( "eveRadius.txt", "w" );
    if( eveRadFile != NULL ) {
        
        fprintf( eveRadFile, "%d", eveRadius );

        fclose( eveRadFile );
        }
    }



void doubleEveRadius() {
    if( eveRadius < 1024 ) {
        eveRadius *= 2;
        writeEveRadius();
        }
    }



void resetEveRadius() {
    eveRadius = eveRadiusStart;
    writeEveRadius();
    }



void clearRecentPlacements() {
    for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
        recentPlacements[i].pos.x = 0;
        recentPlacements[i].pos.y = 0;
        recentPlacements[i].depth = 0;
        }

    writeRecentPlacements();
    }




void printBiomeSamples() {
    int *biomeSamples = new int[ numBiomes ];
    
    for( int i=0; i<numBiomes; i++ ) {
        biomeSamples[i] = 0;
        }
    
    JenkinsRandomSource sampleRandSource;

    int numSamples = 10000;

    int range = 2000;

    for( int i=0; i<numSamples; i++ ) {
        int x = sampleRandSource.getRandomBoundedInt( -range, range );
        int y = sampleRandSource.getRandomBoundedInt( -range, range );
        
        biomeSamples[ computeMapBiomeIndex( x, y ) ] ++;
        }
    
    for( int i=0; i<numBiomes; i++ ) {
        printf( "Biome %d:  %d (%.2f)\n",
                biomes[ i ], biomeSamples[i], 
                biomeSamples[i] / (double)numSamples );
        }
    }



int printObjectSamples( int inXCenter, int inYCenter ) {
    int objectToCount = 2285;
    
    JenkinsRandomSource sampleRandSource;

    int numSamples = 0;

    int rangeX = 354;
    int rangeY = 354;

    int count = 0;
    
    for( int y=-rangeY; y<=rangeY; y++ ) {
        for( int x=-rangeX; x<=rangeX; x++ ) {
            int obj = getMapObjectRaw( x  + inXCenter, y + inYCenter );
            
            
            if( obj == objectToCount ) {
                count++;
                }
            numSamples++;
            }
        }
    

    int rangeSize = (rangeX + rangeX + 1 ) * ( rangeY + rangeY + 1 );

    float sampleFraction = 
        numSamples / 
        ( float ) rangeSize;
    
    printf( "Counted %d objects in %d/%d samples, expect %d total\n",
            count, numSamples, rangeSize, (int)( count / sampleFraction ) );
    
    return count;
    }





// optimization:
// cache dbGet results in RAM

// 2.6 MB of RAM for this.
#define DB_CACHE_SIZE 131072

typedef struct DBCacheRecord {
        int x, y, slot, subCont;
        int value;
    } DBCacheRecord;
    
static DBCacheRecord dbCache[ DB_CACHE_SIZE ];



static int computeDBCacheHash( int inKeyA, int inKeyB, 
                               int inKeyC, int inKeyD ) {
    
    int hashKey = ( inKeyA * CACHE_PRIME_A + 
                    inKeyB * CACHE_PRIME_B + 
                    inKeyC * CACHE_PRIME_C +
                    inKeyD * CACHE_PRIME_D ) % DB_CACHE_SIZE;
    if( hashKey < 0 ) {
        hashKey += DB_CACHE_SIZE;
        }
    return hashKey;
    }



static int computeBLCacheHash( int inKeyA, int inKeyB ) {
    
    int hashKey = ( inKeyA * CACHE_PRIME_A + 
                    inKeyB * CACHE_PRIME_B ) % DB_CACHE_SIZE;
    if( hashKey < 0 ) {
        hashKey += DB_CACHE_SIZE;
        }
    return hashKey;
    }



typedef struct DBTimeCacheRecord {
        int x, y, slot, subCont;
        timeSec_t timeVal;
    } DBTimeCacheRecord;
    
static DBTimeCacheRecord dbTimeCache[ DB_CACHE_SIZE ];



typedef struct BlockingCacheRecord {
        int x, y;
        // -1 if not present
        signed char blocking;
    } BlockingCacheRecord;
    
static BlockingCacheRecord blockingCache[ DB_CACHE_SIZE ];





static void initDBCaches() {
    DBCacheRecord blankRecord = { 0, 0, 0, 0, -2 };
    for( int i=0; i<DB_CACHE_SIZE; i++ ) {
        dbCache[i] = blankRecord;
        }
    // 1 for empty (because 0 is a valid value)
    DBTimeCacheRecord blankTimeRecord = { 0, 0, 0, 0, 1 };
    for( int i=0; i<DB_CACHE_SIZE; i++ ) {
        dbTimeCache[i] = blankTimeRecord;
        }
    // -1 for empty
    BlockingCacheRecord blankBlockingRecord = { 0, 0, -1 };
    for( int i=0; i<DB_CACHE_SIZE; i++ ) {
        blockingCache[i] = blankBlockingRecord;
        }
    }

    


// returns -2 on miss
static int dbGetCached( int inX, int inY, int inSlot, int inSubCont ) {
    DBCacheRecord r =
        dbCache[ computeDBCacheHash( inX, inY, inSlot, inSubCont ) ];

    if( r.x == inX && r.y == inY && 
        r.slot == inSlot && r.subCont == inSubCont &&
        r.value != -2 ) {
        return r.value;
        }
    else {
        return -2;
        }
    }



static void dbPutCached( int inX, int inY, int inSlot, int inSubCont, 
                        int inValue ) {
    DBCacheRecord r = { inX, inY, inSlot, inSubCont, inValue };
    
    dbCache[ computeDBCacheHash( inX, inY, inSlot, inSubCont ) ] = r;
    }





// returns 1 on miss
static int dbTimeGetCached( int inX, int inY, int inSlot, int inSubCont ) {
    DBTimeCacheRecord r =
        dbTimeCache[ computeDBCacheHash( inX, inY, inSlot, inSubCont ) ];

    if( r.x == inX && r.y == inY && 
        r.slot == inSlot && r.subCont == inSubCont &&
        r.timeVal != 1 ) {
        return r.timeVal;
        }
    else {
        return 1;
        }
    }



static void dbTimePutCached( int inX, int inY, int inSlot, int inSubCont, 
                         timeSec_t inValue ) {
    DBTimeCacheRecord r = { inX, inY, inSlot, inSubCont, inValue };
    
    dbTimeCache[ computeDBCacheHash( inX, inY, inSlot, inSubCont ) ] = r;
    }





// returns -1 on miss
static signed char blockingGetCached( int inX, int inY ) {
    BlockingCacheRecord r =
        blockingCache[ computeBLCacheHash( inX, inY ) ];

    if( r.x == inX && r.y == inY &&
        r.blocking != -1 ) {
        return r.blocking;
        }
    else {
        return -1;
        }
    }



static void blockingPutCached( int inX, int inY, char inBlocking ) {
    BlockingCacheRecord r = { inX, inY, inBlocking };
    
    blockingCache[ computeBLCacheHash( inX, inY ) ] = r;
    }


static void blockingClearCached( int inX, int inY ) {
    
    BlockingCacheRecord *r =
        &( blockingCache[ computeBLCacheHash( inX, inY ) ] );

    if( r->x == inX && r->y == inY ) {
        r->blocking = -1;
        }
    }





char lookTimeDBEmpty = false;
char skipLookTimeCleanup = 0;
char skipRemovedObjectCleanup = 0;

// if lookTimeDBEmpty, then we init all map cell look times to NOW
int cellsLookedAtToInit = 0;



// version of open call that checks whether look time exists in lookTimeDB
// for each record in opened DB, and clears any entries that are not
// rebuilding file storage for DB in the process
// lookTimeDB MUST be open before calling this
//
// If lookTimeDBEmpty, this call just opens the target DB normally without
// shrinking it.
//
// Can handle max key and value size of 16 and 12 bytes
// Assumes that first 8 bytes of key are xy as 32-bit ints
int DB_open_timeShrunk(
	DB *db,
	const char *path,
	int mode,
	unsigned long hash_table_size,
	unsigned long key_size,
	unsigned long value_size) {

    File dbFile( NULL, path );
    
    if( ! dbFile.exists() || lookTimeDBEmpty || skipLookTimeCleanup ) {

        if( lookTimeDBEmpty ) {
            AppLog::infoF( "No lookTimes present, not cleaning %s", path );
            }
        
        int error = DB_open( db, 
                                 path, 
                                 mode,
                                 hash_table_size,
                                 key_size,
                                 value_size );

        if( ! error && ! skipLookTimeCleanup ) {
            // add look time for cells in this DB to present
            // essentially resetting all look times to NOW
            
            DB_Iterator dbi;
    
    
            DB_Iterator_init( db, &dbi );
    
            // key and value size that are big enough to handle all of our DB
            unsigned char key[16];
    
            unsigned char value[12];
    
            while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
                int x = valueToInt( key );
                int y = valueToInt( &( key[4] ) );

                cellsLookedAtToInit++;
                
                dbLookTimePut( x, y, MAP_TIMESEC );
                }
            }
        return error;
        }
    
    char *dbTempName = autoSprintf( "%s.temp", path );
    File dbTempFile( NULL, dbTempName );
    
    if( dbTempFile.exists() ) {
        dbTempFile.remove();
        }
    
    if( dbTempFile.exists() ) {
        AppLog::errorF( "Failed to remove temp DB file %s", dbTempName );

        delete [] dbTempName;

        return DB_open( db, 
                            path, 
                            mode,
                            hash_table_size,
                            key_size,
                            value_size );
        }
    
    DB oldDB;
    
    int error = DB_open( &oldDB, 
                             path, 
                             mode,
                             hash_table_size,
                             key_size,
                             value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        path );
        delete [] dbTempName;

        return error;
        }

    

    

    
    DB_Iterator dbi;
    
    
    DB_Iterator_init( &oldDB, &dbi );
    
    // key and value size that are big enough to handle all of our DB
    unsigned char key[16];
    
    unsigned char value[12];
    
    int total = 0;
    int stale = 0;
    int nonStale = 0;
    
    // first, just count
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;

        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );

        if( dbLookTimeGet( x, y ) > 0 ) {
            // keep
            nonStale++;
            }
        else {
            // stale
            // ignore
            stale++;
            }
        }



    // optimial size for DB of remaining elements
    unsigned int newSize = DB_getShrinkSize( &oldDB, nonStale );

    AppLog::infoF( "Shrinking hash table in %s from %d down to %d", 
                   path, 
                   DB_getCurrentSize( &oldDB ), 
                   newSize );


    DB tempDB;
    
    error = DB_open( &tempDB, 
                         dbTempName, 
                         mode,
                         newSize,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        dbTempName );
        delete [] dbTempName;
        DB_close( &oldDB );
        return error;
        }


    // now that we have new temp db properly sized,
    // iterate again and insert, but don't count
    DB_Iterator_init( &oldDB, &dbi );

    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        int x = valueToInt( key );
        int y = valueToInt( &( key[4] ) );

        if( dbLookTimeGet( x, y ) > 0 ) {
            // keep
            // insert it in temp
            DB_put_new( &tempDB, key, value );
            }
        else {
            // stale
            // ignore
            }
        }


    
    AppLog::infoF( "Cleaned %d / %d stale map cells from %s", stale, total,
                   path );

    printf( "\n" );
    
    
    DB_close( &tempDB );
    DB_close( &oldDB );

    dbTempFile.copy( &dbFile );
    dbTempFile.remove();

    delete [] dbTempName;

    // now open new, shrunk file
    return DB_open( db, 
                        path, 
                        mode,
                        hash_table_size,
                        key_size,
                        value_size );
    }



int countNewlines( char *inString ) {
    int len = strlen( inString );
    int num = 0;
    for( int i=0; i<len; i++ ) {
        if( inString[i] == '\n' ) {
            num++;
            }
        }
    return num;
    }




#include "../gameSource/categoryBank.h"


// true if ID is a non-pattern category
char getIsCategory( int inID ) {
    CategoryRecord *r = getCategory( inID );
    
    if( r == NULL ) {
        return false;
        }
    if( r->isPattern ) {
        return false;
        }
    return true;
    }



// for large inserts, like tutorial map loads, we don't want to 
// track individual map changes.
static char skipTrackingMapChanges = false;



// returns num set after
int cleanMap() {
    AppLog::info( "\nCleaning map of objects that have been removed..." );
    
    skipTrackingMapChanges = true;
    
    DB_Iterator dbi;
    
    
    DB_Iterator_init( &db, &dbi );
    
    unsigned char key[16];
    
    unsigned char value[4];


    // keep list of x,y coordinates in map that need clearing
    SimpleVector<int> xToClear;
    SimpleVector<int> yToClear;

    // container slots that need clearing
    SimpleVector<int> xContToCheck;
    SimpleVector<int> yContToCheck;
    
    int totalDBRecordCount = 0;
    
    int totalSetCount = 0;
    int numClearedCount = 0;
    int totalNumContained = 0;
    int numContainedCleared = 0;
    
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        totalDBRecordCount++;
        
        int s = valueToInt( &( key[8] ) );
        int b = valueToInt( &( key[12] ) );
       
        if( s == 0 ) {
            int id = valueToInt( value );
            
            if( id > 0 ) {
                totalSetCount++;
                
                ObjectRecord *o = getObject( id );
                
                if( o == NULL || getIsCategory( id ) 
                    || o->description[0] == '@' 
                    || o->isOwned ) {
                    // id doesn't exist anymore
                    
                    // OR it's a non-pattern category
                    // those should never exist in map
                    // may be left over from a non-clean shutdown
                    
                    // OR object is flagged with @
                    // this may be a pattern category that is actually
                    // a place-holder

                    // OR it's owned (no owned objects should be left
                    // on map after server restarts... server must have
                    // crashed and not shut down properly)

                    numClearedCount++;
                    
                    int x = valueToInt( key );
                    int y = valueToInt( &( key[4] ) );
                    
                    xToClear.push_back( x );
                    yToClear.push_back( y );
                    }
                }
            }
        if( s == 2 && b == 0 ) {
            int numSlots = valueToInt( value );
            if( numSlots > 0 ) {
                totalNumContained += numSlots;
                
                int x = valueToInt( key );
                int y = valueToInt( &( key[4] ) );
                xContToCheck.push_back( x );
                yContToCheck.push_back( y );
                }
            }
        }
    

    for( int i=0; i<xToClear.size(); i++ ) {
        int x = xToClear.getElementDirect( i );
        int y = yToClear.getElementDirect( i );
        
        clearAllContained( x, y );
        setMapObject( x, y, 0 );
        }

    for( int i=0; i<xContToCheck.size(); i++ ) {
        int x = xContToCheck.getElementDirect( i );
        int y = yContToCheck.getElementDirect( i );
        
        if( getMapObjectRaw( x, y ) != 0 ) {
            int numCont;
            int *cont = getContainedRaw( x, y, &numCont );
            timeSec_t *decay = getContainedEtaDecay( x, y, &numCont );
            
            SimpleVector<int> newCont;
            SimpleVector<timeSec_t> newDecay;

            SimpleVector< SimpleVector<int> > newSubCont;
            SimpleVector< SimpleVector<timeSec_t> > newSubContDecay;
            
            char anyRemoved = false;
            

            for( int c=0; c<numCont; c++ ) {
                
                SimpleVector<int> subCont;
                SimpleVector<timeSec_t> subContDecay;
                

                char thisKept = false;
                
                if( cont[c] < 0 ) {
                    
                    ObjectRecord *o = getObject( - cont[c] );
                    
                    if( o != NULL && ! getIsCategory( - cont[c] ) ) {
                        
                        thisKept = true;
                        
                        newCont.push_back( cont[c] );
                        newDecay.push_back( decay[c] );
                        
                        int numSub;
                        
                        int *contSub = 
                            getContainedRaw( x, y, &numSub, c + 1 );
                        timeSec_t *decaySub = 
                            getContainedEtaDecay( x, y, &numSub, c + 1 );

                        for( int s=0; s<numSub; s++ ) {
                            
                            if( getObject( contSub[s] ) != NULL &&
                                ! getIsCategory( contSub[s] ) ) {
                                
                                subCont.push_back( contSub[s] );
                                subContDecay.push_back( decaySub[s] );
                                }
                            else {
                                anyRemoved = true;
                                }
                            }
                        
                        if( contSub != NULL ) {
                            delete [] contSub;
                            }
                        if( decaySub != NULL ) {
                            delete [] decaySub;
                            }
                        numContainedCleared += numSub - subCont.size();
                        }
                    }
                else {
                    ObjectRecord *o = getObject( cont[c] );
                    if( o != NULL && ! getIsCategory( cont[c] ) ) {
                        
                        thisKept = true;
                        newCont.push_back( cont[c] );
                        newDecay.push_back( decay[c] );
                        }
                    else {
                        anyRemoved = true;
                        }
                    }

                if( thisKept ) {        
                    newSubCont.push_back( subCont );
                    newSubContDecay.push_back( subContDecay );
                    }
                else {
                    anyRemoved = true;
                    }
                }
            


            delete [] cont;
            delete [] decay;
            
            if( anyRemoved ) {
                
                numContainedCleared +=
                    ( numCont - newCont.size() );
                
                int *newContArray = newCont.getElementArray();
                timeSec_t *newDecayArray = newDecay.getElementArray();
            
                setContained( x, y, newCont.size(), newContArray );
                setContainedEtaDecay( x, y, newDecay.size(), newDecayArray );
            
                for( int c=0; c<newCont.size(); c++ ) {
                    int numSub =
                        newSubCont.getElementDirect( c ).size();
                
                    if( numSub > 0 ) {
                        int *newSubArray = 
                            newSubCont.getElementDirect( c ).getElementArray();
                        timeSec_t *newSubDecayArray = 
                            newSubContDecay.
                            getElementDirect( c ).getElementArray();
                    
                        setContained( x, y, numSub, newSubArray, c + 1 );

                        setContainedEtaDecay( x, y, numSub, newSubDecayArray,
                                              c + 1 );
                    
                        delete [] newSubArray;
                        delete [] newSubDecayArray;
                        }
                    else {
                        clearAllContained( x, y, c + 1 );
                        }
                    }

                delete [] newContArray;
                delete [] newDecayArray;
                }
            }
        }
    

    AppLog::infoF( "...%d map cells were set, and %d needed to be cleared.",
                   totalSetCount, numClearedCount );
    AppLog::infoF( 
        "...%d contained objects present, and %d needed to be cleared.",
        totalNumContained, numContainedCleared );
    AppLog::infoF( "...%d database records total (%d max hash bin depth).", 
                   totalDBRecordCount, DB_maxStack );
    
    printf( "\n" );

    skipTrackingMapChanges = false;

    return totalSetCount;
    }








// reads lines from inFile until EOF reached or inTimeLimitSec passes
// leaves file pos at end of last line read, ready to read more lines
// on future calls
// returns true if there's more file to read, or false if end of file reached
static char loadIntoMapFromFile( FILE *inFile, 
                                 int inOffsetX = 0, 
                                 int inOffsetY = 0,
                                 double inTimeLimitSec = 0 ) {

    skipTrackingMapChanges = true;
    
    
    double startTime = Time::getCurrentTime();

    char moreFileLeft = true;

    // break out when read fails
    // or if time limit passed
    while( inTimeLimitSec == 0 || 
           Time::getCurrentTime() < startTime + inTimeLimitSec ) {
        
        TestMapRecord r;
                
        char stringBuff[1000];
                
        int numRead = fscanf( inFile, "%d %d %d %d %999s", 
                              &(r.x), &(r.y), &(r.biome),
                              &(r.floor),
                              stringBuff );
                
        if( numRead != 5 ) {
            moreFileLeft = false;
            break;
            }
        r.x += inOffsetX;
        r.y += inOffsetY;
        
        int numSlots;
                
        char **slots = split( stringBuff, ",", &numSlots );
                
        for( int i=0; i<numSlots; i++ ) {
                    
            if( i == 0 ) {
                r.id = atoi( slots[0] );
                }
            else {
                        
                int numSub;
                char **subSlots = split( slots[i], ":", &numSub );
                        
                for( int j=0; j<numSub; j++ ) {
                    if( j == 0 ) {
                        int contID = atoi( subSlots[0] );
                                
                        if( numSub > 1 ) {
                            contID *= -1;
                            }
                                
                        r.contained.push_back( contID );
                        SimpleVector<int> subVec;
                                
                        r.subContained.push_back( subVec );
                        }
                    else {
                        SimpleVector<int> *subVec =
                            r.subContained.getElement( i - 1 );
                        subVec->push_back( atoi( subSlots[j] ) );
                        }
                    delete [] subSlots[j];
                    }
                delete [] subSlots;
                }
                    
            delete [] slots[i];
            }
        delete [] slots;



        // set all test map directly in database
        biomeDBPut( r.x, r.y, r.biome, r.biome, 0.5 );
                
        dbFloorPut( r.x, r.y, r.floor );

        setMapObject( r.x, r.y, r.id );
                
        int *contArray = r.contained.getElementArray();
                
        setContained( r.x, r.y, r.contained.size(), contArray );
        delete [] contArray;
                
        for( int c=0; c<r.contained.size(); c++ ) {

            int *subContArray = 
                r.subContained.getElement( c )->getElementArray();
                    
            setContained( r.x, r.y, 
                          r.subContained.getElement(c)->size(),
                          subContArray, c + 1 );
                    
            delete [] subContArray;
            }
        }

    skipTrackingMapChanges = false;

    return moreFileLeft;
    }



static inline void changeContained( int inX, int inY, int inSlot, 
                                    int inSubCont, int inID ) {
    dbPut( inX, inY, FIRST_CONT_SLOT + inSlot, inID, inSubCont );    
    }



typedef struct GlobalTriggerState {
        SimpleVector<GridPos> triggerOnLocations;
        
        // receivers for this trigger that are waiting to be turned on
        SimpleVector<GridPos> receiverLocations;
        
        SimpleVector<GridPos> triggeredLocations;
        SimpleVector<int> triggeredIDs;
        // what we revert to when global trigger turns off (back to receiver)
        SimpleVector<int> triggeredRevertIDs;
    } GlobalTriggerState;
        


static SimpleVector<GlobalTriggerState> globalTriggerStates;


static int numSpeechPipes = 0;

static SimpleVector<GridPos> *speechPipesIn = NULL;

static SimpleVector<GridPos> *speechPipesOut = NULL;



static SimpleVector<GridPos> flightLandingPos;



static char isAdjacent( GridPos inPos, int inX, int inY ) {
    if( inX <= inPos.x + 1 &&
        inX >= inPos.x - 1 &&
        inY <= inPos.y + 1 &&
        inY >= inPos.y - 1 ) {
        return true;
        }
    return false;
    }



void getSpeechPipesIn( int inX, int inY, SimpleVector<int> *outIndicies ) {
    for( int i=0; i<numSpeechPipes; i++ ) {
        
        for( int p=0; p<speechPipesIn[ i ].size(); p++ ) {
            
            GridPos inPos = speechPipesIn[i].getElementDirect( p );
            if( isAdjacent( inPos, inX, inY ) ) {
                 
                // make sure pipe-in is still here
                int id = getMapObjectRaw( inPos.x, inPos.y );
                    
                char stillHere = false;
            
                if( id > 0 ) {
                    ObjectRecord *oIn = getObject( id );
                
                    if( oIn->speechPipeIn && 
                        oIn->speechPipeIndex == i ) {
                        stillHere = true;
                        }
                    }
                if( ! stillHere ) {
                    speechPipesIn[ i ].deleteElement( p );
                    p--;
                    }
                else {
                    outIndicies->push_back( i );
                    break;
                    }
                }
            }
        }
    }



SimpleVector<GridPos> *getSpeechPipesOut( int inIndex ) {
    // first, filter them to make sure they are all still here
    for( int p=0; p<speechPipesOut[ inIndex ].size(); p++ ) {
        
        GridPos outPos = speechPipesOut[ inIndex ].getElementDirect( p );
        // make sure pipe-out is still here
        int id = getMapObjectRaw( outPos.x, outPos.y );
        
        char stillHere = false;
        
        if( id > 0 ) {
            ObjectRecord *oOut = getObject( id );
            
            if( oOut->speechPipeOut && 
                oOut->speechPipeIndex == inIndex ) {
                stillHere = true;
                }
            }
        if( ! stillHere ) {
            speechPipesOut[ inIndex ].deleteElement( p );
            p--;
            }
        }
    
    return &( speechPipesOut[ inIndex ] );
    }



static void deleteFileByName( const char *inFileName ) {
    File f( NULL, inFileName );
    
    if( f.exists() ) {
        f.remove();
        }
    }



static void setupMapChangeLogFile() {
    File logFolder( NULL, "mapChangeLogs" );
    
    if( ! logFolder.exists() ) {
        logFolder.makeDirectory();
        }

    // always close file and start a new one when this is called

    if( mapChangeLogFile != NULL ) {
        fclose( mapChangeLogFile );
        mapChangeLogFile = NULL;
        }
    

    if( logFolder.isDirectory() ) {
        
        if( mapChangeLogFile == NULL ) {

            // file does not exist
            char *newFileName = 
                autoSprintf( "%.ftime_mapLog.txt",
                             Time::getCurrentTime() );
            
            File *f = logFolder.getChildFile( newFileName );
            
            delete [] newFileName;
            
            char *fullName = f->getFullFileName();
            
            delete f;
        
            mapChangeLogFile = fopen( fullName, "a" );
            delete [] fullName;
            }
        }

    mapChangeLogTimeStart = Time::getCurrentTime();
    fprintf( mapChangeLogFile, "startTime: %.2f\n", mapChangeLogTimeStart );
    }




void reseedMap( char inForceFresh ) {
    
    FILE *seedFile = NULL;
    
    if( ! inForceFresh ) {
        seedFile = fopen( "biomeRandSeed.txt", "r" );
        }
    

    char set = false;
    
    if( seedFile != NULL ) {
        int numRead = 
            fscanf( seedFile, "%u %u", &biomeRandSeedA, &biomeRandSeedB );
        fclose( seedFile );
        
        if( numRead == 2 ) {
            AppLog::infoF( "Reading map rand seed from file: %u %u\n", 
                           biomeRandSeedA, biomeRandSeedB );
            set = true;
            }
        }
    


    if( !set ) {
        // no seed set, or ignoring it, make a new one
        
        if( !inForceFresh ) {
            // not forced (not internal apocalypse)
            // seed file wiped externally, so it's like a manual apocalypse
            // report a fresh arc starting
            reportArcEnd();
            }

        char *secretA =
            SettingsManager::getStringSetting( "statsServerSharedSecret", 
                                               "secret" );
        int secretALen = strlen( secretA );
        
        unsigned int seedBaseA = 
            crc32( (unsigned char*)secretA, secretALen );
        
        const char *nonce = 
            "8TX8sr7weEK8UIqrE0xV"
            "voZgafknTgZAVQCTD8UG"
            "6FKWSgi9N1wDhUQ7VCuw"
            "uJbKsMAnOzLwbnnB7nQs"
            "a6mI5rjqijo1oMjPiYbk"
            "uezCnYjrn744AvSP7Zux"
            "wOiZLLDUn5tUe1Ym3vTG"
            "0I80QFzhFPht5TOiiYqT"
            "jeZx0k9reFeknKkGUac3"
            "fHlp0rg1PEOtZZ0LZsme";
        
        // assumption:  secret has way more than 32 bits of entropy
        // crc32 only extracts 32 bits, though.
        
        // by XORing secret with a random nonce and passing through crc32
        // again, we get another 32 bits of entropy out of it.

        // Not the best way of doing this, but it is probably sufficient
        // to prevent brute-force test-guessing of the map seed based
        // on sample map data.

        char *secretB = stringDuplicate( secretA );
        
        int nonceLen = strlen( nonce );
        
        for( int i=0; i<secretALen; i++ ) {
            if( i > nonceLen ) {
                break;
                }
            secretB[i] = secretB[i] ^ nonce[i];
            }

        unsigned int seedBaseB = 
            crc32( (unsigned char*)secretB, secretALen );


        delete [] secretA;
        delete [] secretB;

        unsigned int modTimeSeedA = 
            (unsigned int)fmod( Time::getCurrentTime() + seedBaseA, 
                                4294967295U );
        
        JenkinsRandomSource tempRandSourceA( modTimeSeedA );

        biomeRandSeedA = tempRandSourceA.getRandomInt();

        unsigned int modTimeSeedB = 
            (unsigned int)fmod( Time::getCurrentTime() + seedBaseB, 
                                4294967295U );
        
        JenkinsRandomSource tempRandSourceB( modTimeSeedB );

        biomeRandSeedB = tempRandSourceB.getRandomInt();
        
        AppLog::infoF( "Generating fresh map rand seeds and saving to file: "
                       "%u %u\n", biomeRandSeedA, biomeRandSeedB );

        // and save it
        seedFile = fopen( "biomeRandSeed.txt", "w" );
        if( seedFile != NULL ) {
            
            fprintf( seedFile, "%u %u", biomeRandSeedA, biomeRandSeedB );
            fclose( seedFile );
            }



        // re-place rand placement objects
        CustomRandomSource placementRandSource( biomeRandSeedA );

        int numObjects;
        ObjectRecord **allObjects = getAllObjects( &numObjects );
    
        for( int i=0; i<numObjects; i++ ) {
            ObjectRecord *o = allObjects[i];

            float p = o->mapChance;
            if( p > 0 ) {
                int id = o->id;
            
                char *randPlacementLoc =
                    strstr( o->description, "randPlacement" );
                
                if( randPlacementLoc != NULL ) {
                    // special random placement
                
                    int count = 10;                
                    sscanf( randPlacementLoc, "randPlacement%d", &count );
                
                    printf( "Placing %d random occurences of %d (%s) "
                            "inside %d square radius:\n",
                            count, id, o->description, barrierRadius );
                    for( int p=0; p<count; p++ ) {
                        // sample until we find target biome
                        int safeR = barrierRadius - 2;
                        
                        char placed = false;
                        while( ! placed ) {                    
                            int pickX = 
                                placementRandSource.
                                getRandomBoundedInt( -safeR, safeR );
                            int pickY = 
                                placementRandSource.
                                getRandomBoundedInt( -safeR, safeR );
                            
                            int pickB = getMapBiome( pickX, pickY );
                            
                            for( int j=0; j< o->numBiomes; j++ ) {
                                int b = o->biomes[j];
                                
                                if( b == pickB ) {
                                    // hit
                                    placed = true;
                                    printf( "  (%d,%d)\n", pickX, pickY );
                                    setMapObject( pickX, pickY, id );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        delete [] allObjects;
        }

                
    setupMapChangeLogFile();

    if( !set && mapChangeLogFile != NULL ) {
        // whenever we actually change the seed, save it to a separate
        // file in log folder

        File logFolder( NULL, "mapChangeLogs" );
        
        char *newFileName = 
            autoSprintf( "%.ftime_mapSeed.txt",
                         Time::getCurrentTime() );
            
        File *f = logFolder.getChildFile( newFileName );
            
        delete [] newFileName;
        
        char *fullName = f->getFullFileName();
        
        delete f;
        
        FILE *seedFile = fopen( fullName, "w" );
        delete [] fullName;
        
        fprintf( seedFile, "%u %u", biomeRandSeedA, biomeRandSeedB );
        
        fclose( seedFile );
        }
    }




char initMap() {

    
    
    numSpeechPipes = getMaxSpeechPipeIndex() + 1;
    
    speechPipesIn = new SimpleVector<GridPos>[ numSpeechPipes ];
    speechPipesOut = new SimpleVector<GridPos>[ numSpeechPipes ];
    

    eveSecondaryLocObjectIDs.deleteAll();
    recentlyUsedPrimaryEvePositionTimes.deleteAll();
    recentlyUsedPrimaryEvePositions.deleteAll();
    recentlyUsedPrimaryEvePositionPlayerIDs.deleteAll();
    

    initDBCaches();
    initBiomeCache();

    mapCacheClear();
    
    edgeObjectID = SettingsManager::getIntSetting( "edgeObject", 0 );
    
    minEveCampRespawnAge = 
        SettingsManager::getFloatSetting( "minEveCampRespawnAge", 60.0f );
    

    barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
    barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );
    
    longTermCullEnabled =
        SettingsManager::getIntSetting( "longTermNoLookCullEnabled", 1 );

    
    SimpleVector<int> *list = 
        SettingsManager::getIntSettingMulti( "barrierObjects" );
        
    barrierItemList.deleteAll();
    barrierItemList.push_back_other( list );
    delete list;
    
    

    for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
        recentPlacements[i].pos.x = 0;
        recentPlacements[i].pos.y = 0;
        recentPlacements[i].depth = 0;
        }
    

    nextPlacementIndex = 0;
    
    FILE *placeFile = fopen( "recentPlacements.txt", "r" );
    if( placeFile != NULL ) {
        for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
            fscanf( placeFile, "%d,%d %d", 
                    &( recentPlacements[i].pos.x ),
                    &( recentPlacements[i].pos.y ),
                    &( recentPlacements[i].depth ) );
            }
        fscanf( placeFile, "\nnextPlacementIndex=%d", &nextPlacementIndex );
        
        fclose( placeFile );
        }
    

    FILE *eveRadFile = fopen( "eveRadius.txt", "r" );
    if( eveRadFile != NULL ) {
        
        fscanf( eveRadFile, "%d", &eveRadius );

        fclose( eveRadFile );
        }

    FILE *eveLocFile = fopen( "lastEveLocation.txt", "r" );
    if( eveLocFile != NULL ) {
        
        fscanf( eveLocFile, "%d,%d", &( eveLocation.x ), &( eveLocation.y ) );

        fclose( eveLocFile );

        printf( "Loading lastEveLocation %d,%d\n", 
                eveLocation.x, eveLocation.y );
        }

    // override if shutdownLongLineagePos exists
    FILE *lineagePosFile = fopen( "shutdownLongLineagePos.txt", "r" );
    if( lineagePosFile != NULL ) {
        
        fscanf( lineagePosFile, "%d,%d", 
                &( eveLocation.x ), &( eveLocation.y ) );

        fclose( lineagePosFile );

        printf( "Overriding eveLocation with shutdownLongLineagePos %d,%d\n", 
                eveLocation.x, eveLocation.y );
        }
    else {
        printf( "No shutdownLongLineagePos.txt file exists\n" );
        
        // look for longest monument log file
        // that has been touched in last 24 hours
        // (ignore spots that may have been culled)
        File f( NULL, "monumentLogs" );
        if( f.exists() && f.isDirectory() ) {
            int numChildFiles;
            File **childFiles = f.getChildFiles( &numChildFiles );
            
            timeSec_t longTime = 0;
            int longLen = 0;
            int longX = 0;
            int longY = 0;
            
            timeSec_t curTime = Time::timeSec();

            int secInDay = 3600 * 24;
            
            for( int i=0; i<numChildFiles; i++ ) {
                timeSec_t modTime = childFiles[i]->getModificationTime();
                
                if( curTime - modTime < secInDay ) {
                    char *cont = childFiles[i]->readFileContents();
                    
                    int numNewlines = countNewlines( cont );
                    
                    delete [] cont;
                    
                    if( numNewlines > longLen ||
                        ( numNewlines == longLen && modTime > longTime ) ) {
                        
                        char *name = childFiles[i]->getFileName();
                        
                        int x, y;
                        int numRead = sscanf( name, "%d_%d_",
                                              &x, &y );

                        delete [] name;
                        
                        if( numRead == 2 ) {
                            longTime = modTime;
                            longLen = numNewlines;
                            longX = x;
                            longY = y;
                            }
                        }
                    }
                delete childFiles[i];
                }
            delete [] childFiles;

            if( longLen > 0 ) {
                eveLocation.x = longX;
                eveLocation.y = longY;
                
                printf( "Overriding eveLocation with "
                        "tallest recent monument location %d,%d\n", 
                        eveLocation.x, eveLocation.y );
                }
            }
        }
    




    
    const char *lookTimeDBName = "lookTime.db";
    
    char lookTimeDBExists = false;
    
    File lookTimeDBFile( NULL, lookTimeDBName );



    if( lookTimeDBFile.exists() &&
        SettingsManager::getIntSetting( "flushLookTimes", 0 ) ) {
        
        AppLog::info( "flushLookTimes.ini set, deleting lookTime.db" );
        
        lookTimeDBFile.remove();
        }


    
    lookTimeDBExists = lookTimeDBFile.exists();

    if( ! lookTimeDBExists ) {
        lookTimeDBEmpty = true;
        }


    skipLookTimeCleanup = 
        SettingsManager::getIntSetting( "skipLookTimeCleanup", 0 );


    if( skipLookTimeCleanup ) {
        AppLog::info( "skipLookTimeCleanup.ini flag set, "
                      "not cleaning databases based on stale look times." );
        }

    LINEARDB3_setMaxLoad( 0.80 );
    
    if( ! skipLookTimeCleanup ) {
        DB lookTimeDB_old;
        
        int error = DB_open( &lookTimeDB_old, 
                             lookTimeDBName, 
                             KISSDB_OPEN_MODE_RWCREAT,
                             80000,
                             8, // two 32-bit ints, xy
                             8 // one 64-bit double, representing an ETA time
                               // in whatever binary format and byte order
                               // "double" on the server platform uses
                             );
    
        if( error ) {
            AppLog::errorF( "Error %d opening look time KissDB", error );
            return false;
            }
    

        int staleSec = 
            SettingsManager::getIntSetting( "mapCellForgottenSeconds", 0 );
    
        if( lookTimeDBExists && staleSec > 0 ) {
            AppLog::info( "\nCleaning stale look times from map..." );
            
            
            static DB lookTimeDB_temp;
        
            const char *lookTimeDBName_temp = "lookTime_temp.db";

            File tempDBFile( NULL, lookTimeDBName_temp );
        
            if( tempDBFile.exists() ) {
                tempDBFile.remove();
                }
        

        
            DB_Iterator dbi;
        
        
            DB_Iterator_init( &lookTimeDB_old, &dbi );
        
    
            timeSec_t curTime = MAP_TIMESEC;
        
            unsigned char key[8];
            unsigned char value[8];

            int total = 0;
            int stale = 0;
            int nonStale = 0;
            
            // first, just count them
            while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
                total++;

                timeSec_t t = valueToTime( value );
            
                if( curTime - t >= staleSec ) {
                    // stale cell
                    // ignore
                    stale++;
                    }
                else {
                    // non-stale
                    nonStale++;
                    }
                }

            // optimial size for DB of remaining elements
            unsigned int newSize = DB_getShrinkSize( &lookTimeDB_old,
                                                     nonStale );
            
            AppLog::infoF( "Shrinking hash table for lookTimes from "
                           "%d down to %d", 
                           DB_getCurrentSize( &lookTimeDB_old ), 
                           newSize );
            

            error = DB_open( &lookTimeDB_temp, 
                             lookTimeDBName_temp, 
                             KISSDB_OPEN_MODE_RWCREAT,
                             newSize,
                             8, // two 32-bit ints, xy
                             8 // one 64-bit double, representing an ETA time
                             // in whatever binary format and byte order
                             // "double" on the server platform uses
                             );
    
            if( error ) {
                AppLog::errorF( 
                    "Error %d opening look time temp KissDB", error );
                return false;
                }
            

            // now that we have new temp db properly sized,
            // iterate again and insert
            DB_Iterator_init( &lookTimeDB_old, &dbi );
        
            while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
                timeSec_t t = valueToTime( value );
            
                if( curTime - t >= staleSec ) {
                    // stale cell
                    // ignore
                    }
                else {
                    // non-stale
                    // insert it in temp
                    DB_put_new( &lookTimeDB_temp, key, value );
                    }
                }


            AppLog::infoF( "Cleaned %d / %d stale look times", stale, total );

            printf( "\n" );

            if( total == 0 ) {
                lookTimeDBEmpty = true;
                }

            DB_close( &lookTimeDB_temp );
            DB_close( &lookTimeDB_old );

            tempDBFile.copy( &lookTimeDBFile );
            tempDBFile.remove();
            }
        else {
            DB_close( &lookTimeDB_old );
            }
        }


    int error = DB_open( &lookTimeDB, 
                         lookTimeDBName, 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         8 // one 64-bit double, representing an ETA time
                         // in whatever binary format and byte order
                         // "double" on the server platform uses
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening look time KissDB", error );
        return false;
        }
    
    lookTimeDBOpen = true;
    
    


    // note that the various decay ETA slots in map.db 
    // are define but unused, because we store times separately
    // in mapTime.db
    error = DB_open_timeShrunk( &db, 
                         "map.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         16, // four 32-bit ints, xysb
                             // s is the slot number 
                             // s=0 for base object
                             // s=1 decay ETA seconds (wall clock time)
                             // s=2 for count of contained objects
                             // s=3 first contained object
                             // s=4 second contained object
                             // s=... remaining contained objects
                             // Then decay ETA for each slot, in order,
                             //   after that.
                             // s = -1
                             //  is a special flag slot set to 0 if NONE
                             //  of the contained items have ETA decay
                             //  or 1 if some of the contained items might 
                             //  have ETA decay.
                             //  (this saves us from having to check each
                             //   one)
                             // If a contained object id is negative,
                             // that indicates that it sub-contains
                             // other objects in its corresponding b slot
                             //
                             // b is for indexing sub-container slots
                             // b=0 is the main object 
                             // b=1 is the first sub-slot, etc.
                         4 // one int, object ID at x,y in slot (s-3)
                           // OR contained count if s=2
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening map KissDB", error );
        return false;
        }
    
    dbOpen = true;



    // this DB uses the same slot numbers as the map.db
    // however, only times are stored here, because they require 8 bytes
    // so, slot 0 and 2 are never used, for example
    error = DB_open_timeShrunk( &timeDB, 
                         "mapTime.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         16, // four 32-bit ints, xysb
                         // s is the slot number 
                         // s=0 for base object
                         // s=1 decay ETA seconds (wall clock time)
                         // s=2 for count of contained objects
                         // s=3 first contained object
                         // s=4 second contained object
                         // s=... remaining contained objects
                         // Then decay ETA for each slot, in order,
                         //   after that.
                             // If a contained object id is negative,
                             // that indicates that it sub-contains
                             // other objects in its corresponding b slot
                             //
                             // b is for indexing sub-container slots
                             // b=0 is the main object 
                             // b=1 is the first sub-slot, etc.
                         8 // one 64-bit double, representing an ETA time
                           // in whatever binary format and byte order
                           // "double" on the server platform uses
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening map time KissDB", error );
        return false;
        }
    
    timeDBOpen = true;






    error = DB_open_timeShrunk( &biomeDB, 
                         "biome.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         12 // three ints,  
                         // 1: biome number at x,y 
                         // 2: second place biome number at x,y 
                         // 3: second place biome gap as int (float gap
                         //    multiplied by 1,000,000)
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening biome KissDB", error );
        return false;
        }
    
    biomeDBOpen = true;



    // see if any biomes are listed in DB
    // if not, we don't even need to check it when generating map
    DB_Iterator biomeDBi;
    DB_Iterator_init( &biomeDB, &biomeDBi );
    
    unsigned char biomeKey[8];
    unsigned char biomeValue[12];
    

    while( DB_Iterator_next( &biomeDBi, biomeKey, biomeValue ) > 0 ) {
        int x = valueToInt( biomeKey );
        int y = valueToInt( &( biomeKey[4] ) );
        
        anyBiomesInDB = true;
        
        if( x > maxBiomeXLoc ) {
            maxBiomeXLoc = x;
            }
        if( x < minBiomeXLoc ) {
            minBiomeXLoc = x;
            }
        if( y > maxBiomeYLoc ) {
            maxBiomeYLoc = y;
            }
        if( y < minBiomeYLoc ) {
            minBiomeYLoc = y;
            }
        }
    
    printf( "Min (x,y) of biome in db = (%d,%d), "
            "Max (x,y) of biome in db = (%d,%d)\n",
            minBiomeXLoc, minBiomeYLoc,
            maxBiomeXLoc, maxBiomeYLoc );
    
            


    error = DB_open_timeShrunk( &floorDB, 
                         "floor.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         4 // one int, the floor object ID at x,y 
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening floor KissDB", error );
        return false;
        }
    
    floorDBOpen = true;



    error = DB_open_timeShrunk( &floorTimeDB, 
                         "floorTime.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         8 // one 64-bit double, representing an ETA time
                           // in whatever binary format and byte order
                           // "double" on the server platform uses
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening floor time KissDB", error );
        return false;
        }
    
    floorTimeDBOpen = true;



    // ALWAYS delete old grave DB at each server startup
    // grave info is only player ID, and server only remembers players
    // live, in RAM, while it is still running
    deleteFileByName( "grave.db" );


    error = DB_open( &graveDB, 
                     "grave.db", 
                     KISSDB_OPEN_MODE_RWCREAT,
                     80000,
                     8, // two 32-bit ints, xy
                     4 // one int, the grave player ID at x,y 
                     );
    
    if( error ) {
        AppLog::errorF( "Error %d opening grave KissDB", error );
        return false;
        }
    
    graveDBOpen = true;





    error = DB_open( &eveDB, 
                         "eve.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         // this can be a lot smaller than other DBs
                         // it's not performance-critical, and the keys are
                         // much longer, so stackdb will waste disk space
                         5000,
                         50, // first 50 characters of email address
                             // append spaces to the end if needed 
                         12 // three ints,  x_center, y_center, radius
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening eve KissDB", error );
        return false;
        }
    
    eveDBOpen = true;




    error = DB_open( &metaDB, 
                     "meta.db", 
                     KISSDB_OPEN_MODE_RWCREAT,
                     // starting size doesn't matter here
                     500,
                     4, // one 32-bit int as key
                     // data
                     MAP_METADATA_LENGTH
                     );
    
    if( error ) {
        AppLog::errorF( "Error %d opening meta KissDB", error );
        return false;
        }
    
    metaDBOpen = true;

    DB_Iterator metaIterator;
    
    DB_Iterator_init( &metaDB, &metaIterator );

    unsigned char metaKey[4];
    
    unsigned char metaValue[MAP_METADATA_LENGTH];

    int maxMetaID = 0;
    int numMetaRecords = 0;
    
    while( DB_Iterator_next( &metaIterator, metaKey, metaValue ) > 0 ) {
        numMetaRecords++;
        
        int metaID = valueToInt( metaKey );

        if( metaID > maxMetaID ) {
            maxMetaID = metaID;
            }
        }
    
    AppLog::infoF( 
        "MetadataDB:  Found %d records with max MetadataID of %d",
        numMetaRecords, maxMetaID );
    
    setLastMetadataID( maxMetaID );
    


    


    if( lookTimeDBEmpty && cellsLookedAtToInit > 0 ) {
        printf( "Since lookTime db was empty, we initialized look times "
                "for %d cells to now.\n\n", cellsLookedAtToInit );
        }

    

    int numObjects;
    ObjectRecord **allObjects = getAllObjects( &numObjects );
    
    
    // first, find all biomes
    SimpleVector<int> biomeList;
    
    
    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = allObjects[i];
        
        if( o->mapChance > 0 ) {
            
            for( int j=0; j< o->numBiomes; j++ ) {
                int b = o->biomes[j];
                
                if( biomeList.getElementIndex(b) == -1 ) {
                    biomeList.push_back( b );
                    }
                }
            }
        
        }


    // manually controll order
    SimpleVector<int> *biomeOrderList =
        SettingsManager::getIntSettingMulti( "biomeOrder" );

    SimpleVector<float> *biomeWeightList =
        SettingsManager::getFloatSettingMulti( "biomeWeights" );

    for( int i=0; i<biomeOrderList->size(); i++ ) {
        int b = biomeOrderList->getElementDirect( i );
        
        if( biomeList.getElementIndex( b ) == -1 ) {
            biomeOrderList->deleteElement( i );
            biomeWeightList->deleteElement( i );
            i--;
            }
        }
    
    // now add any discovered biomes to end of list
    for( int i=0; i<biomeList.size(); i++ ) {
        int b = biomeList.getElementDirect( i );
        if( biomeOrderList->getElementIndex( b ) == -1 ) {
            biomeOrderList->push_back( b );
            // default weight
            biomeWeightList->push_back( 0.1 );
            }
        }
    
    numBiomes = biomeOrderList->size();
    biomes = biomeOrderList->getElementArray();
    biomeWeights = biomeWeightList->getElementArray();
    biomeCumuWeights = new float[ numBiomes ];
    
    biomeTotalWeight = 0;
    for( int i=0; i<numBiomes; i++ ) {
        biomeTotalWeight += biomeWeights[i];
        biomeCumuWeights[i] = biomeTotalWeight;
        }
    
    delete biomeOrderList;
    delete biomeWeightList;


    SimpleVector<int> *specialBiomeList =
        SettingsManager::getIntSettingMulti( "specialBiomes" );
    
    numSpecialBiomes = specialBiomeList->size();
    specialBiomes = specialBiomeList->getElementArray();
    
    regularBiomeLimit = numBiomes - numSpecialBiomes;

    delete specialBiomeList;

    specialBiomeCumuWeights = new float[ numSpecialBiomes ];
    
    specialBiomeTotalWeight = 0;
    for( int i=regularBiomeLimit; i<numBiomes; i++ ) {
        specialBiomeTotalWeight += biomeWeights[i];
        specialBiomeCumuWeights[i-regularBiomeLimit] = specialBiomeTotalWeight;
        }


    specialBiomeBandMode = 
        SettingsManager::getIntSetting( "specialBiomeBandMode", 0 );
    
    specialBiomeBandThickness = 
        SettingsManager::getIntSetting( "specialBiomeBandThickness",
                                        300 );
    
    SimpleVector<int> *specialBiomeOrderList =
        SettingsManager::getIntSettingMulti( "specialBiomeBandOrder" );

    specialBiomeBandOrder.push_back_other( specialBiomeOrderList );
    
    // look up biome index for each special biome
    for( int i=0; i<specialBiomeBandOrder.size(); i++ ) {
        int biomeNumber = specialBiomeBandOrder.getElementDirect( i );
    
        int biomeIndex = 0;

        for( int j=0; j<numBiomes; j++ ) {
            if( biomes[j] == biomeNumber ) {
                biomeIndex = j;
                break;
                }
            }
        specialBiomeBandIndexOrder.push_back( biomeIndex );
        }

    delete specialBiomeOrderList;


    int specialBiomeBandDefault = 
        SettingsManager::getIntSetting( "specialBiomeBandDefault", 0 );
    
    // look up biome index
    specialBiomeBandDefaultIndex = 0;
    for( int j=0; j<numBiomes; j++ ) {
        if( biomes[j] == specialBiomeBandDefault ) {
            specialBiomeBandDefaultIndex = j;
            break;
            }
        }
    

    SimpleVector<int> *specialBiomeBandYCenterList =
        SettingsManager::getIntSettingMulti( "specialBiomeBandYCenter" );

    specialBiomeBandYCenter.push_back_other( specialBiomeBandYCenterList );

    delete specialBiomeBandYCenterList;


    minActivePlayersForBirthlands = 
        SettingsManager::getIntSetting( "minActivePlayersForBirthlands", 15 );

    

    naturalMapIDs = new SimpleVector<int>[ numBiomes ];
    naturalMapChances = new SimpleVector<float>[ numBiomes ];
    totalChanceWeight = new float[ numBiomes ];

    for( int j=0; j<numBiomes; j++ ) {
        totalChanceWeight[j] = 0;
        }
    

    CustomRandomSource phaseRandSource( randSeed );

    
    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = allObjects[i];

        if( strstr( o->description, "eveSecondaryLoc" ) != NULL ) {
            eveSecondaryLocObjectIDs.push_back( o->id );
            }
        if( strstr( o->description, "eveHomeMarker" ) != NULL ) {
            eveHomeMarkerObjectID = o->id;
            }
        


        float p = o->mapChance;
        if( p > 0 ) {
            int id = o->id;
            
            allNaturalMapIDs.push_back( id );

            char *gridPlacementLoc =
                strstr( o->description, "gridPlacement" );

            char *randPlacementLoc =
                strstr( o->description, "randPlacement" );
                
            if( gridPlacementLoc != NULL ) {
                // special grid placement
                
                int spacingX = 10;
                int spacingY = 10;
                int phaseX = 0;
                int phaseY = 0;
                
                int numRead = sscanf( gridPlacementLoc, 
                                      "gridPlacement%d,%d,p%d,p%d", 
                                      &spacingX, &spacingY,
                                      &phaseX, &phaseY );
                if( numRead < 2 ) {
                    // only X specified, square grid
                    spacingY = spacingX;
                    }
                if( numRead < 4 ) {
                    // only X specified, square grid
                    phaseY = phaseX;
                    }
                
                

                if( strstr( o->description, "evePrimaryLoc" ) != NULL ) {
                    evePrimaryLocObjectID = id;
                    evePrimaryLocSpacingX = spacingX;
                    evePrimaryLocSpacingY = spacingY;
                    }

                SimpleVector<int> permittedBiomes;
                for( int b=0; b<o->numBiomes; b++ ) {
                    permittedBiomes.push_back( 
                        getBiomeIndex( o->biomes[ b ] ) );
                    }

                int wiggleScaleX = 4;
                int wiggleScaleY = 4;
                
                if( spacingX > 12 ) {
                    wiggleScaleX = spacingX / 3;
                    }
                if( spacingY > 12 ) {
                    wiggleScaleY = spacingY / 3;
                    }
                
                MapGridPlacement gp =
                    { id, 
                      spacingX, spacingY,
                      phaseX, phaseY,
                      //phaseRandSource.getRandomBoundedInt( 0, 
                      //                                     spacingX - 1 ),
                      //phaseRandSource.getRandomBoundedInt( 0, 
                      //                                     spacingY - 1 ),
                      wiggleScaleX,
                      wiggleScaleY,
                      permittedBiomes };
                
                gridPlacements.push_back( gp );
                }
            else if( randPlacementLoc != NULL ) {
                // special random placement
                
                // don't actually place these now, do it on reseed
                // but skip adding them to list of natural objects
                }
            else {
                // regular fractal placement
                
                for( int j=0; j< o->numBiomes; j++ ) {
                    int b = o->biomes[j];
                    
                    int bIndex = getBiomeIndex( b );
                    naturalMapIDs[bIndex].push_back( id );
                    naturalMapChances[bIndex].push_back( p );
                    
                    totalChanceWeight[bIndex] += p;
                    }
                }
            }
        }


    for( int j=0; j<numBiomes; j++ ) {    
        AppLog::infoF( 
            "Biome %d:  Found %d natural objects with total weight %f",
            biomes[j], naturalMapIDs[j].size(), totalChanceWeight[j] );
        }
    
    delete [] allObjects;

    
    skipRemovedObjectCleanup = 
        SettingsManager::getIntSetting( "skipRemovedObjectCleanup", 0 );




    
    FILE *dummyFile = fopen( "mapDummyRecall.txt", "r" );
    
    if( dummyFile != NULL ) {
        AppLog::info( "Found mapDummyRecall.txt file, restoring dummy objects "
                      "on map" );
        
        skipTrackingMapChanges = true;
        
        int numRead = 5;
        
        int numSet = 0;
        
        int numStale = 0;

        while( numRead == 5 || numRead == 7 ) {
            
            int x, y, parentID, dummyIndex, slot, b;
            
            char marker;            
            
            slot = -1;
            b = 0;
            
            numRead = fscanf( dummyFile, "(%d,%d) %c %d %d [%d %d]\n", 
                              &x, &y, &marker, &parentID, &dummyIndex,
                              &slot, &b );
            if( numRead == 5 || numRead == 7 ) {

                if( dbLookTimeGet( x, y ) <= 0 ) {
                    // stale area of map
                    numStale++;
                    continue;
                    }                
                
                ObjectRecord *parent = getObject( parentID );
                
                int dummyID = -1;
                
                if( parent != NULL ) {
                    
                    if( marker == 'u' && parent->numUses-1 > dummyIndex ) {
                        dummyID = parent->useDummyIDs[ dummyIndex ];
                        }
                    else if( marker == 'v' && 
                             parent->numVariableDummyIDs > dummyIndex ) {
                        dummyID = parent->variableDummyIDs[ dummyIndex ];
                        }
                    }
                if( dummyID > 0 ) {
                    if( numRead == 5 ) {
                        setMapObjectRaw( x, y, dummyID );
                        }
                    else {
                        changeContained( x, y, slot, b, dummyID );
                        }
                    numSet++;
                    }
                }
            }
        skipTrackingMapChanges = false;
        
        fclose( dummyFile );
        
        
        AppLog::infoF( "Restored %d dummy objects to map "
                       "(%d skipped as stale)", numSet, numStale );
        
        remove( "mapDummyRecall.txt" );
        
        printf( "\n" );
        }



    // clean map after restoring dummy objects
    int totalSetCount = 1;

    if( ! skipRemovedObjectCleanup ) {
        totalSetCount = cleanMap();
        }
    else {
        AppLog::info( "Skipping cleaning map of removed objects" );
        }
    


    
    if( totalSetCount == 0 ) {
        // map has been cleared

        // ignore old value for placements
        clearRecentPlacements();
        }



    globalTriggerStates.deleteAll();
    
    int numTriggers = getNumGlobalTriggers();
    for( int i=0; i<numTriggers; i++ ) {
        GlobalTriggerState s;
        globalTriggerStates.push_back( s );
        }



    useTestMap = SettingsManager::getIntSetting( "useTestMap", 0 );
    

    if( useTestMap ) {        

        FILE *testMapFile = fopen( "testMap.txt", "r" );
        FILE *testMapStaleFile = fopen( "testMapStale.txt", "r" );
        
        if( testMapFile != NULL && testMapStaleFile == NULL ) {
            
            testMapStaleFile = fopen( "testMapStale.txt", "w" );
            
            if( testMapStaleFile != NULL ) {
                fprintf( testMapStaleFile, "1" );
                fclose( testMapStaleFile );
                testMapStaleFile = NULL;
                }
            
            printf( "Loading testMap.txt\n" );
            
            loadIntoMapFromFile( testMapFile );

            fclose( testMapFile );
            testMapFile = NULL;
            }
        
        if( testMapFile != NULL ) {
            fclose( testMapFile );
            }
        if( testMapStaleFile != NULL ) {
            fclose( testMapStaleFile );
            }
        }


    SimpleVector<char*> *specialPlacements = 
        SettingsManager::getSetting( "specialMapPlacements" );
    
    if( specialPlacements != NULL ) {
        
        for( int i=0; i<specialPlacements->size(); i++ ) {
            char *line = specialPlacements->getElementDirect( i );
            
            int x, y, id;
            id = -1;
            int numRead = sscanf( line, "%d_%d_%d", &x, &y, &id );
            
            if( numRead == 3 && id > -1 ) {
                
                }
            setMapObject( x, y, id );
            }


        specialPlacements->deallocateStringElements();
        delete specialPlacements;
        }
    
    
    reseedMap( false );
        
    

    
    // for debugging the map
    // printObjectSamples();
    // printBiomeSamples();
    //outputMapImage();

    //outputBiomeFractals();


    return true;
    }







void freeAndNullString( char **inStringPointer ) {
    if( *inStringPointer != NULL ) {
        delete [] *inStringPointer;
        *inStringPointer = NULL;
        }
    }



static void rememberDummy( FILE *inFile, int inX, int inY, 
                           ObjectRecord *inDummyO, 
                           int inSlot = -1, int inB = 0 ) {
    
    if( inFile == NULL ) {
        return;
        }
    
    int parent = -1;
    int dummyIndex = -1;

    char marker = 'x';

    if( inDummyO->isUseDummy ) {
        marker = 'u';
        
        parent = inDummyO->useDummyParent;
        ObjectRecord *parentO = getObject( parent );
        
        if( parentO != NULL ) {    
            for( int i=0; i<parentO->numUses - 1; i++ ) {
                if( parentO->useDummyIDs[i] == inDummyO->id ) {
                    dummyIndex = i;
                    break;
                    }
                }
            }
        }
    else if( inDummyO->isVariableDummy ) {
        marker = 'v';
        
        parent = inDummyO->variableDummyParent;
        ObjectRecord *parentO = getObject( parent );
        
        if( parentO != NULL ) {    
            for( int i=0; i<parentO->numVariableDummyIDs; i++ ) {
                if( parentO->variableDummyIDs[i] == inDummyO->id ) {
                    dummyIndex = i;
                    break;
                    }
                }
            }
        }
    
    if( parent > 0 && dummyIndex >= 0 ) {
        if( inSlot == -1 && inB == 0 ) {   
            fprintf( inFile, "(%d,%d) %c %d %d\n", 
                     inX, inY, 
                     marker, parent, dummyIndex );
            }
        else {
            fprintf( inFile, "(%d,%d) %c %d %d [%d %d]\n", 
                     inX, inY, 
                     marker, parent, dummyIndex, inSlot, inB );
            }
        }
    }



void freeMap( char inSkipCleanup ) {
    if( mapChangeLogFile != NULL ) {
        fclose( mapChangeLogFile );
        mapChangeLogFile = NULL;
        }
    
    printf( "%d calls to getBaseMap\n", getBaseMapCallCount );

    skipTrackingMapChanges = true;
    
    if( lookTimeDBOpen ) {
        DB_close( &lookTimeDB );
        lookTimeDBOpen = false;
        }


    if( dbOpen && ! inSkipCleanup ) {
        
        AppLog::infoF( "Cleaning up map database on server shutdown." );
        
        // iterate through DB and look for useDummy objects
        // replace them with unused version object
        // useDummy objects aren't real objects in objectBank,
        // and their IDs may change in the future, so they're
        // not safe to store in the map between server runs.
        
        DB_Iterator dbi;
    
    
        DB_Iterator_init( &db, &dbi );
    
        unsigned char key[16];
    
        unsigned char value[4];


        // keep list of x,y coordinates in map that need replacing
        SimpleVector<int> xToPlace;
        SimpleVector<int> yToPlace;

        SimpleVector<int> idToPlace;
        

        // container slots that need replacing
        SimpleVector<int> xContToCheck;
        SimpleVector<int> yContToCheck;
        SimpleVector<int> bContToCheck;
        
        
        int skipUseDummyCleanup = 
            SettingsManager::getIntSetting( "skipUseDummyCleanup", 0 );
        
        

        
        if( !skipUseDummyCleanup ) {    
            
            FILE *dummyFile = fopen( "mapDummyRecall.txt", "w" );
            
            while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        
                int s = valueToInt( &( key[8] ) );
                int b = valueToInt( &( key[12] ) );
       
                if( s == 0 ) {
                    int id = valueToInt( value );
            
                    if( id > 0 ) {
                    
                        ObjectRecord *mapO = getObject( id );
                    
                    
                        if( mapO != NULL ) {
                            if( mapO->isUseDummy ) {
                                int x = valueToInt( key );
                                int y = valueToInt( &( key[4] ) );
                    
                                xToPlace.push_back( x );
                                yToPlace.push_back( y );
                                idToPlace.push_back( mapO->useDummyParent );
                                
                                rememberDummy( dummyFile, x, y, mapO );
                                }
                            else if( mapO->isVariableDummy ) {
                                int x = valueToInt( key );
                                int y = valueToInt( &( key[4] ) );
                            
                                xToPlace.push_back( x );
                                yToPlace.push_back( y );
                                idToPlace.push_back( 
                                    mapO->variableDummyParent );
                                
                                rememberDummy( dummyFile, x, y, mapO );
                                }
                            }
                        }
                    }
                else if( s == 2 ) {
                    int numSlots = valueToInt( value );
                    if( numSlots > 0 ) {
                        int x = valueToInt( key );
                        int y = valueToInt( &( key[4] ) );
                        xContToCheck.push_back( x );
                        yContToCheck.push_back( y );
                        bContToCheck.push_back( b );
                        }
                    }
                }
        

            for( int i=0; i<xToPlace.size(); i++ ) {
                int x = xToPlace.getElementDirect( i );
                int y = yToPlace.getElementDirect( i );
            
                setMapObjectRaw( x, y, idToPlace.getElementDirect( i ) );
                }

        
            int numContChanged = 0;
        
            for( int i=0; i<xContToCheck.size(); i++ ) {
                int x = xContToCheck.getElementDirect( i );
                int y = yContToCheck.getElementDirect( i );
                int b = bContToCheck.getElementDirect( i );
        
                if( getMapObjectRaw( x, y ) != 0 ) {

                    int numCont;
                    int *cont = getContainedRaw( x, y, &numCont, b );
                
                    char anyChanged = false;

                    for( int c=0; c<numCont; c++ ) {

                        char subCont = false;
                    
                        if( cont[c] < 0 ) {
                            cont[c] *= -1;
                            subCont = true;
                            }
                    
                        ObjectRecord *contObj = getObject( cont[c] );
                    
                        if( contObj != NULL ) {
                            if( contObj->isUseDummy ) {
                                cont[c] = contObj->useDummyParent;
                                rememberDummy( dummyFile, x, y, contObj, c, b );
                            
                                anyChanged = true;
                                numContChanged ++;
                                }
                            else if( contObj->isVariableDummy ) {
                                cont[c] = contObj->variableDummyParent;
                                rememberDummy( dummyFile, x, y, contObj, c, b );
                            
                                anyChanged = true;
                                numContChanged ++;
                                }
                            }
                   
                        if( subCont ) {
                            cont[c] *= -1;
                            }
                        }
                
                    if( anyChanged ) {
                        setContained( x, y, numCont, cont, b );
                        }
                
                    delete [] cont;
                    }
                }
                    
            if( dummyFile != NULL ) {
                fclose( dummyFile );
                }
            
            AppLog::infoF(
                "...%d useDummy/variable objects present that were changed "
                "back into their unused parent.",
                xToPlace.size() );
            AppLog::infoF( 
                "...%d contained useDummy/variable objects present and changed "
                "back to usused parent.",
                numContChanged );
            }
        else {
            AppLog::info( "Skipping use dummy cleanup." );
            }
        
        printf( "\n" );

        if( ! skipRemovedObjectCleanup ) {
            AppLog::info( "Now running normal map clean..." );
            cleanMap();
            }
        else {
            AppLog::info( "Skipping running normal map clean." );
            }
        
        
        DB_close( &db );
        dbOpen = false;
        }
    else if( dbOpen ) {
        // just close with no cleanup
        DB_close( &db );
        dbOpen = false;
        }
    
    if( timeDBOpen ) {
        DB_close( &timeDB );
        timeDBOpen = false;
        }

    if( biomeDBOpen ) {
        DB_close( &biomeDB );
        biomeDBOpen = false;
        }


    if( floorDBOpen ) {
        DB_close( &floorDB );
        floorDBOpen = false;
        }

    if( floorTimeDBOpen ) {
        DB_close( &floorTimeDB );
        floorTimeDBOpen = false;
        }


    if( graveDBOpen ) {
        DB_close( &graveDB );
        graveDBOpen = false;
        }


    if( eveDBOpen ) {
        DB_close( &eveDB );
        eveDBOpen = false;
        }

    if( metaDBOpen ) {
        DB_close( &metaDB );
        metaDBOpen = false;
        }
    

    writeEveRadius();
    writeRecentPlacements();

    delete [] biomes;
    delete [] biomeWeights;
    delete [] biomeCumuWeights;
    delete [] specialBiomes;
    delete [] specialBiomeCumuWeights;
    
    delete [] naturalMapIDs;
    delete [] naturalMapChances;
    delete [] totalChanceWeight;

    
    allNaturalMapIDs.deleteAll();

    liveDecayQueue.clear();
    liveDecayRecordPresentHashTable.clear();
    liveDecayRecordLastLookTimeHashTable.clear();
    liveMovementEtaTimes.clear();

    liveMovements.clear();
    
    mapChangePosSinceLastStep.deleteAll();
    
    skipTrackingMapChanges = false;
    
    
    delete [] speechPipesIn;
    delete [] speechPipesOut;
    
    speechPipesIn = NULL;
    speechPipesOut = NULL;

    flightLandingPos.deleteAll();

    homelands.deleteAll();
    }







void wipeMapFiles() {
    deleteFileByName( "biome.db" );
    deleteFileByName( "eve.db" );
    deleteFileByName( "floor.db" );
    deleteFileByName( "floorTime.db" );
    deleteFileByName( "grave.db" );
    deleteFileByName( "lookTime.db" );
    deleteFileByName( "map.db" );
    deleteFileByName( "mapTime.db" );
    deleteFileByName( "playerStats.db" );
    deleteFileByName( "meta.db" );
    }







// returns -1 if not found
static int dbGet( int inX, int inY, int inSlot, int inSubCont = 0 ) {
    
    int cachedVal = dbGetCached( inX, inY, inSlot, inSubCont );
    if( cachedVal != -2 ) {
        
        return cachedVal;
        }
    

    unsigned char key[16];
    unsigned char value[4];

    // look for changes to default in database
    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    
    int result = DB_get( &db, key, value );
    
    
    
    int returnVal;
    
    if( result == 0 ) {
        // found
        returnVal = valueToInt( value );
        }
    else {
        returnVal = -1;
        }

    dbPutCached( inX, inY, inSlot, inSubCont, returnVal );
    
    return returnVal;
    }




// returns 0 if not found
static timeSec_t dbTimeGet( int inX, int inY, int inSlot, int inSubCont = 0 ) {

    timeSec_t cachedVal = dbTimeGetCached( inX, inY, inSlot, inSubCont );
    if( cachedVal != 1 ) {
        
        return cachedVal;
        }

    
    unsigned char key[16];
    unsigned char value[8];

    // look for changes to default in database
    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    
    int result = DB_get( &timeDB, key, value );
    
    timeSec_t timeVal;
    
    if( result == 0 ) {
        // found
        timeVal = valueToTime( value );
        }
    else {
        timeVal = 0;
        }

    dbTimePutCached( inX, inY, inSlot, inSubCont, timeVal );
    
    return timeVal;
    }



static int dbFloorGet( int inX, int inY ) {
    unsigned char key[9];
    unsigned char value[4];

    // look for changes to default in database
    intPairToKey( inX, inY, key );
    
    int result = DB_get( &floorDB, key, value );
    
    if( result == 0 ) {
        // found
        int returnVal = valueToInt( value );
        
        return returnVal;
        }
    else {
        return -1;
        }
    }



// returns 0 if not found
static timeSec_t dbFloorTimeGet( int inX, int inY ) {
    unsigned char key[8];
    unsigned char value[8];

    intPairToKey( inX, inY, key );
    
    int result = DB_get( &floorTimeDB, key, value );
    
    if( result == 0 ) {
        // found
        return valueToTime( value );
        }
    else {
        return 0;
        }
    }



// returns 0 if not found
timeSec_t dbLookTimeGet( int inX, int inY ) {
    unsigned char key[8];
    unsigned char value[8];

    intPairToKey( inX/100, inY/100, key );
    
    int result = DB_get( &lookTimeDB, key, value );
    
    if( result == 0 ) {
        // found
        return valueToTime( value );
        }
    else {
        return 0;
        }
    }




static void dbPut( int inX, int inY, int inSlot, int inValue, 
                   int inSubCont ) {
    
    if( inSlot == 0 && inSubCont == 0 ) {
        // object has changed
        // clear blocking cache
        blockingClearCached( inX, inY );
        }
    

    if( ! skipTrackingMapChanges ) {
        
        // count all slot changes as changes, because we're storing
        // time in a separate database now (so we don't need to worry
        // about time changes being reported as map changes)
        
        char found = false;
        for( int i=0; i<mapChangePosSinceLastStep.size(); i++ ) {
            
            ChangePosition *p = mapChangePosSinceLastStep.getElement( i );
            
            if( p->x == inX && p->y == inY ) {
                found = true;
                
                // update it
                p->responsiblePlayerID = currentResponsiblePlayer;
                break;
                }
            }
        
        if( ! found ) {
            ChangePosition p = { inX, inY, false, currentResponsiblePlayer,
                                 0, 0, 0.0 };
            mapChangePosSinceLastStep.push_back( p );
            }
        }
    
    

    if( apocalypsePossible && inValue > 0 && inSlot == 0 && inSubCont == 0 ) {
        // a primary tile put
        // check if this triggers the apocalypse
        if( isApocalypseTrigger( inValue ) &&
            getNumPlayers() >=
            SettingsManager::getIntSetting( "minActivePlayersForApocalypse", 
                                            15 ) ) {
            apocalypseTriggered = true;
            apocalypseLocation.x = inX;
            apocalypseLocation.y = inY;
            }
        }
    if( inValue > 0 && inSlot == 0 && inSubCont == 0  ) {
        
        int status = getMonumentStatus( inValue );
        
        if( status > 0 ) {
            int player = currentResponsiblePlayer;
            if( player < 0 ) {
                player = -player;
                }
            monumentAction( inX, inY, inValue, player, 
                            status );
            }
        }
    
    

    unsigned char key[16];
    unsigned char value[4];
    

    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    intToValue( inValue, value );
            
    
    DB_put( &db, key, value );

    dbPutCached( inX, inY, inSlot, inSubCont, inValue );
    }





static void dbTimePut( int inX, int inY, int inSlot, timeSec_t inTime,
                       int inSubCont = 0 ) {
    // ETA decay changes don't get reported as map changes    
    
    unsigned char key[16];
    unsigned char value[8];
    

    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    timeToValue( inTime, value );
            
    
    DB_put( &timeDB, key, value );

    dbTimePutCached( inX, inY, inSlot, inSubCont, inTime );
    }




static void dbFloorPut( int inX, int inY, int inValue ) {
    

    if( ! skipTrackingMapChanges ) {
        
        char found = false;
        for( int i=0; i<mapChangePosSinceLastStep.size(); i++ ) {
            
            ChangePosition *p = mapChangePosSinceLastStep.getElement( i );
            
            if( p->x == inX && p->y == inY ) {
                found = true;
                
                // update it
                p->responsiblePlayerID = currentResponsiblePlayer;
                break;
                }
            }
        
        if( ! found ) {
            ChangePosition p = { inX, inY, false, currentResponsiblePlayer,
                                 0, 0, 0.0 };
            mapChangePosSinceLastStep.push_back( p );
            }
        }
    
    
    unsigned char key[8];
    unsigned char value[4];
    

    intPairToKey( inX, inY, key );
    intToValue( inValue, value );
            
    
    DB_put( &floorDB, key, value );
    }



static void dbFloorTimePut( int inX, int inY, timeSec_t inTime ) {
    // ETA decay changes don't get reported as map changes    
    
    unsigned char key[8];
    unsigned char value[8];
    

    intPairToKey( inX, inY, key );
    timeToValue( inTime, value );
            
    
    DB_put( &floorTimeDB, key, value );
    }



void dbLookTimePut( int inX, int inY, timeSec_t inTime ) {
    if( !lookTimeDBOpen ) return;
    
    unsigned char key[8];
    unsigned char value[8];
    

    intPairToKey( inX/100, inY/100, key );
    timeToValue( inTime, value );
            
    
    DB_put( &lookTimeDB, key, value );
    }



// certain types of movement transitions should always be live
// tracked, even when out of view (NSEW moves, for human-made traveling objects
// for example)
static char isDecayTransAlwaysLiveTracked( TransRecord *inTrans ) {
    if( inTrans != NULL &&
        inTrans->move >=4 && inTrans->move <= 7 ) {

        return true;
        }

    return false;
    }



// slot is 0 for main map cell, or higher for container slots
static void trackETA( int inX, int inY, int inSlot, timeSec_t inETA,
                      int inSubCont = 0, 
                      TransRecord *inApplicableTrans = NULL ) {
    
    timeSec_t timeLeft = inETA - MAP_TIMESEC;
        
    if( timeLeft < maxSecondsForActiveDecayTracking ) {
        // track it live
            
        // duplicates okay
        // we'll deal with them when they ripen
        // (we check the true ETA stored in map before acting
        //   on one stored in this queue)
        LiveDecayRecord r = { inX, inY, inSlot, inETA, inSubCont, 
                              inApplicableTrans };
            
        char exists;
        timeSec_t existingETA =
            liveDecayRecordPresentHashTable.lookup( inX, inY, inSlot,
                                                    inSubCont,
                                                    &exists );

        if( !exists || existingETA != inETA ) {
            
            liveDecayQueue.insert( r, inETA );
            
            liveDecayRecordPresentHashTable.insert( inX, inY, inSlot, 
                                                    inSubCont, inETA );

            char exists;
            
            liveDecayRecordLastLookTimeHashTable.lookup( inX, inY, inSlot,
                                                         inSubCont,
                                                         &exists );
            
            if( !exists ) {
                // don't overwrite old one
                liveDecayRecordLastLookTimeHashTable.insert( inX, inY, inSlot,
                                                             inSubCont,
                                                             MAP_TIMESEC );
                if( inSlot > 0 || inSubCont > 0 ) {
                    
                    ContRecord *oldCount = 
                        liveDecayRecordLastLookTimeMaxContainedHashTable.
                        lookupPointer( inX, inY, 0, 0 );
                    
                    if( oldCount != NULL ) {
                        // update if needed
                        if( oldCount->maxSlots < inSlot ) {
                            oldCount->maxSlots = inSlot;
                            }
                        if( oldCount->maxSubSlots < inSubCont ) {
                            oldCount->maxSubSlots = inSubCont;
                            }
                        }
                    else {
                        // insert new
                        ContRecord r = { inSlot, inSubCont };
                        
                        liveDecayRecordLastLookTimeMaxContainedHashTable.
                            insert( inX, inY, 0, 0, r );
                        }
                    }
                }
            }
        }
    }





float getMapContainerTimeStretch( int inX, int inY, int inSubCont=0 ) {
    
    float stretch = 1.0f;
                        
    int containerID;

    if( inSubCont == 0 ) {
        containerID = getMapObjectRaw( inX, inY );
        }
    else {
        containerID = getContained( inX, inY, inSubCont - 1 );
        }
    
    if( containerID < 0 ) {
        containerID *= -1;
        }

    if( containerID != 0 ) {
        stretch = getObject( containerID )->slotTimeStretch;
        }
    return stretch;
    }                        



void checkDecayContained( int inX, int inY, int inSubCont = 0 );







int *getContainedRaw( int inX, int inY, int *outNumContained, 
                      int inSubCont ) {
    int num = getNumContained( inX, inY, inSubCont );

    *outNumContained = num;
    
    if( num == 0 ) {
        return NULL;
        }
   
    int *contained = new int[ num ];

    int trueNum = 0;
    
    for( int i=0; i<num; i++ ) {
        int result = dbGet( inX, inY, FIRST_CONT_SLOT + i, inSubCont );
        if( result == -1 ) {
            result = 0;
            }
        if( result != 0 ) {
            contained[trueNum] = result;
            trueNum++;
            }        
        }
    
    *outNumContained = trueNum;

    if( trueNum < num ) {
        // fix filled count permanently in DB
        dbPut( inX, inY, NUM_CONT_SLOT, trueNum, inSubCont );
        }

    if( trueNum > 0 ) {
        return contained;
        }
    else {
        delete [] contained;
        return NULL;
        }
    }



// returns true if no contained items will decay
char getSlotItemsNoDecay( int inX, int inY, int inSubCont ) {
    int result = dbGet( inX, inY, NO_DECAY_SLOT, inSubCont );
    
    if( result != -1 ) {
        // found
        return ( result == 0 );
        }
    else {
        // default, some may decay
        return false;
        }
    }


void setSlotItemsNoDecay( int inX, int inY, int inSubCont, char inNoDecay ) {
    int val = 1;
    if( inNoDecay ) {
        val = 0;
        }
    dbPut( inX, inY, NO_DECAY_SLOT, val, inSubCont );
    }




int *getContained( int inX, int inY, int *outNumContained, int inSubCont ) {
    if( ! getSlotItemsNoDecay( inX, inY, inSubCont ) ) {
        checkDecayContained( inX, inY, inSubCont );
        }
    int *result = getContainedRaw( inX, inY, outNumContained, inSubCont );
    
    // look at these slots if they are subject to live decay
    timeSec_t currentTime = MAP_TIMESEC;
    
    for( int i=0; i<*outNumContained; i++ ) {
        timeSec_t *oldLookTime =
            liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY,
                                                                i + 1,
                                                                inSubCont );
        if( oldLookTime != NULL ) {
            // look at it now
            *oldLookTime = currentTime;
            }
        }
    
    return result;
    }


int *getContainedNoLook( int inX, int inY, int *outNumContained, 
                         int inSubCont = 0 ) {
    if( ! getSlotItemsNoDecay( inX, inY, inSubCont ) ) {
        checkDecayContained( inX, inY, inSubCont );
        }
    return getContainedRaw( inX, inY, outNumContained, inSubCont );
    }



// gets DB slot number where a given container slot's decay time is stored
// if inNumContained is -1, it will be looked up in database
static int getContainerDecaySlot( int inX, int inY, int inSlot, 
                                  int inSubCont = 0,
                                  int inNumContained = -1 ) {
    if( inNumContained == -1 ) {    
        inNumContained = getNumContained( inX, inY, inSubCont );
        }
    
    return FIRST_CONT_SLOT + inNumContained + inSlot;
    }





timeSec_t *getContainedEtaDecay( int inX, int inY, int *outNumContained,
                                 int inSubCont ) {
    int num = getNumContained( inX, inY, inSubCont );

    *outNumContained = num;
    
    if( num == 0 ) {
        return NULL;
        }
   
    timeSec_t *containedEta = new timeSec_t[ num ];

    for( int i=0; i<num; i++ ) {
        // can be 0 if not found, which is okay
        containedEta[i] = dbTimeGet( inX, inY, 
                                     getContainerDecaySlot( inX, inY, i,
                                                            inSubCont, num ),
                                     inSubCont );
        }
    return containedEta;
    }



int checkDecayObject( int inX, int inY, int inID ) {
    if( inID == 0 ) {
        return inID;
        }
    
    TransRecord *t = getPTrans( -1, inID );

    if( t == NULL ) {
        // no auto-decay for this object
        return inID;
        }
    
    
    // else decay exists for this object
    
    int newID = inID;
    int movingObjID = newID;
    int leftBehindID = 0;
    
    // in case of movement
    int newX = inX;
    int newY = inY;
    

    // is eta stored in map?
    timeSec_t mapETA = getEtaDecay( inX, inY );
    
    if( mapETA != 0 ) {
        
        if( (int)mapETA <= MAP_TIMESEC ) {
            
            // object in map has decayed (eta expired)

            // apply the transition
            newID = t->newTarget;
            movingObjID = newID;

            int oldSlots = getNumContainerSlots( inID );

            int newSlots = getNumContainerSlots( newID );
            
            if( newSlots < oldSlots ) {
                shrinkContainer( inX, inY, newSlots );
                }
            if( newSlots > 0 ) {    
                restretchMapContainedDecays( inX, inY, inID, newID );
                }


            
            if( t->move != 0 ) {
                // moving
                doublePair dir = { 0, 0 };
                
                TransRecord *destTrans = NULL;
                
                int desiredMoveDist = t->desiredMoveDist;

                char stayInBiome = false;
                
                char avoidFloor = false;
                
                if( t->newTarget > 0 &&
                    strstr( getObject( t->newTarget )->description, 
                            "groundOnly" ) ) {
                    avoidFloor = true;
                    }
                

                if( t->move < 3 ) {
                    
                    GridPos p = getClosestPlayerPos( inX, inY );
                    
                    double dX = (double)p.x - (double)inX;
                    double dY = (double)p.y - (double)inY;

                    double dist = sqrt( dX * dX + dY * dY );
                    
                    if( dist <= 10 &&
                        ( p.x != 0 || p.y != 0 ) ) {
                        
                        if( t->move == 1 && dist <= desiredMoveDist ) {
                            // chase.  Try to land exactly on them
                            // if they're close enough to do it in one move
                            desiredMoveDist = lrint( dist );
                            }

                        dir.x = dX;
                        dir.y = dY;
                        dir = normalize( dir );
                        
                        // round to one of 8 cardinal directions
                        
                        double a = angle( dir );
                        
                        a = 2 * M_PI * 
                            lrint( ( a / ( 2 * M_PI ) ) * 8 ) / 
                            8.0;
                        
                        dir.x = 1;
                        dir.y = 0;
                        dir = rotate( dir, a );
                        }
                    if( t->move == 2 ) {
                        // flee
                        stayInBiome = true;
                        dir = mult( dir, -1 );
                        }
                    }
                else if( t->move > 3 ) {
                    // NSEW

                    switch( t->move ) {
                        case 4:
                            dir.y = 1;
                            break;
                        case 5:
                            dir.y = -1;
                            break;
                        case 6:
                            dir.x = 1;
                            break;
                        case 7:
                            dir.x = -1;
                            break;
                        }


                    if( desiredMoveDist == 1 ) {
                        // make sure something else isn't moving out of
                        // destination
                        int destPosX = inX + dir.x;
                        int destPosY = inY + dir.y ;
                        
                        
                        int numMoving = liveMovements.size();
                        
                        for( int i=0; i<numMoving; i++ ) {
                            MovementRecord *m = liveMovements.getElement( i );
                            
                            if( m->sourceX == destPosX &&
                                m->sourceY == destPosY ) {
                                // found something leaving where we're landing
                                // wait for it to finish
                                setEtaDecay( inX, inY, MAP_TIMESEC + 1, t );
                                return inID;
                                }
                            }
                        }

                    }
                
                // round to 1000ths to avoid rounding errors
                // that can separate our values from zero

                dir.x = lrint( dir.x * 1000 ) / 1000.0;
                dir.y = lrint( dir.y * 1000 ) / 1000.0;
                

                if( dir.x == 0 && dir.y == 0 ) {
                    // random instead
                    
                    stayInBiome = true;
                    
                    dir.x = 1;
                    dir.y = 0;
                    
                    // 8 cardinal directions
                    dir = rotate( 
                        dir,
                        2 * M_PI * 
                        randSource.getRandomBoundedInt( 0, 7 ) / 8.0 );

                    // clean up rounding errors
                    if( fabs( dir.x ) < 0.1 ) {
                        dir.x = 0;
                        }
                    if( fabs( dir.y ) < 0.1 ) {
                        dir.y = 0;
                        }
                    if( dir.x > 0.9 ) {
                        dir.x = 1;
                        }
                    if( dir.x < - 0.9 ) {
                        dir.x = -1;
                        }
                    if( dir.y > 0.9 ) {
                        dir.y = 1;
                        }
                    if( dir.y < - 0.9 ) {
                        dir.y = -1;
                        }
                    }
                
                if( dir.x != 0 && dir.y != 0 ) {
                    // diag

                    // push both up to full step
                    
                    if( dir.x < 0 ) {
                        dir.x = -1;
                        }
                    else if( dir.x > 0 ) {
                        dir.x = 1;
                        }

                    if( dir.y < 0 ) {
                        dir.y = -1;
                        }
                    else if( dir.y > 0 ) {
                        dir.y = 1;
                        }
                    }
                
                // now we have the dir we want to go in    


                int tryDist = desiredMoveDist;
                
                if( tryDist < 1 ) {
                    tryDist = 1;
                    }
                
                int tryRadius = 4;

                if( t->move > 3 && tryDist == 1 ) {
                    // single-step NSEW moves never go beyond 
                    // their intended distance
                    tryRadius = 0;
                    }

                // try again and again with smaller distances until we
                // find an empty spot
                while( newX == inX && newY == inY && tryDist > 0 ) {

                    // walk up to 4 steps past our dist in that direction,
                    // looking for non-blocking objects or an empty spot
                
                    for( int i=1; i<=tryDist + tryRadius; i++ ) {
                        int testX = lrint( inX + dir.x * i );
                        int testY = lrint( inY + dir.y * i );
                    
                        int oID = getMapObjectRaw( testX, testY );
                        
                        // does trans exist for this object used on destination 
                        // obj?
                        TransRecord *trans = NULL;
                        
                        if( oID > 0 ) {
                            trans = getPTrans( inID, oID );
                            
                            if( trans == NULL ) {
                                // does trans exist for newID applied to
                                // destination?
                                trans = getPTrans( newID, oID );
                                }
                            }
                        else if( oID == 0 ) {
                            // check for bare ground trans
                            trans = getPTrans( inID, -1 );
                            
                            if( trans == NULL ) {
                                // does trans exist for newID applied to
                                // bare ground
                                trans = getPTrans( newID, -1 );
                                }
                            else {
                                // what about trans onto floor?
                                int fID = getMapFloor( testX, testY );
                                if( fID > 0 ) {
                                    trans = getPTrans( inID, fID );
                                    if( trans == NULL ) {
                                        // does exist for newID applied to
                                        // floor?
                                        trans = getPTrans( newID, fID );
                                        }
                                    }
                                }
                            }
                        
                        
                        char blockedByFloor = false;
                        
                        if( oID == 0 &&
                            avoidFloor ) {
                            int floorID = getMapFloor( testX, testY );
                        
                            if( floorID > 0 ) {
                                blockedByFloor = true;
                                }
                            }


                        if( i >= tryDist && oID == 0 ) {
                            if( ! blockedByFloor ) {
                                // found a bare ground spot for it to move
                                newX = testX;
                                newY = testY;
                                // keep any bare ground transition (or NULL)
                                destTrans = trans;
                                break;
                                }
                            }
                        else if( i >= tryDist && trans != NULL ) {
                            newX = testX;
                            newY = testY;
                            destTrans = trans;
                            break;
                            }
                        else if( oID > 0 && getObject( oID ) != NULL &&
                                 getObject( oID )->blocksMoving ) {
                            // blocked, stop now
                            break;
                            }
                        // else walk through it
                        }
                    
                    tryDist--;
                    
                    if( tryRadius != 0 ) {
                        // 4 on first try, but then 1 on remaining tries to
                        // avoid overlap with previous tries
                        tryRadius = 1;
                        }
                    }
                
                
                int curBiome = -1;
                if( stayInBiome ) {
                    curBiome = getMapBiome( inX, inY );
                    
                    if( newX != inX || newY != inY ) {
                        int newBiome = getMapBiome( newX, newY );
                        
                        if( newBiome != curBiome ) {
                            // block move
                            newX = inX;
                            newY = inY;
                            
                            // forget about trans that we found above
                            // it crosses biome boundary
                            // (and this fixes the infamous sliding
                            //  penguin ice-hole bug)
                            destTrans = NULL;
                            }
                        }
                    }
                


                if( newX == inX && newY == inY &&
                    t->move <= 3 ) {
                    // can't move where we want to go in flee/chase/random

                    // pick some random spot to go instead

                    int possibleX[8];
                    int possibleY[8];
                    int numPossibleDirs = 0;
                    
                    for( int d=0; d<8; d++ ) {
                        
                        dir.x = 1;
                        dir.y = 0;
                    
                        // 8 cardinal directions
                        dir = rotate( 
                            dir,
                            2 * M_PI * d / 8.0 );
                        

                        tryDist = t->desiredMoveDist;
                        
                        if( tryDist < 1 ) {
                            tryDist = 1;
                            }
                
                        tryRadius = 4;

                        // try again and again with smaller distances until we
                        // find an empty spot
                        char stopCheckingDir = false;

                        while( !stopCheckingDir && tryDist > 0 ) {

                            // walk up to 4 steps in that direction, looking
                            // for non-blocking objects or an empty spot
                        
                            for( int i=0; i<tryDist + tryRadius; i++ ) {
                                int testX = lrint( inX + dir.x * i );
                                int testY = lrint( inY + dir.y * i );
                            
                                int oID = getMapObjectRaw( testX, testY );
                                               
                    
                                if( i >= tryDist && oID == 0 ) {
                                    // found a spot for it to move
                                    
                                    if( stayInBiome &&
                                        curBiome !=
                                        getMapBiome( testX, testY ) ) {
                                        
                                        continue;
                                        }
                                    if( avoidFloor ) {
                                        int floorID = 
                                            getMapFloor( testX, testY );
                        
                                        if( floorID > 0 ) {
                                            // blocked by floor
                                            continue;
                                            }
                                        }

                                    possibleX[ numPossibleDirs ] = testX;
                                    possibleY[ numPossibleDirs ] = testY;
                                    numPossibleDirs++;
                                    stopCheckingDir = true;
                                    break;
                                    }
                                else if( oID > 0 && getObject( oID ) != NULL &&
                                         getObject( oID )->blocksMoving ) {
                                    // blocked, stop now
                                    break;
                                    }
                                // else walk through it
                                }

                            tryDist --;
                            // 1 on remaining tries to avoid overlap
                            tryRadius = 1;
                            }
                        }
                    
                    if( numPossibleDirs > 0 ) {
                        int pick = 
                            randSource.getRandomBoundedInt( 
                                0, numPossibleDirs - 1 );
                        
                        newX = possibleX[ pick ];
                        newY = possibleY[ pick ];
                        }
                    }
                
                

                if( newX != inX || newY != inY ) {
                    // a reall move!
                    
                    printf( "Object moving from (%d,%d) to (%d,%d)\n",
                            inX, inY, newX, newY );
                    
                    // move object
                    
                    if( destTrans != NULL ) {
                        newID = destTrans->newTarget;
                        }
                    
                    dbPut( newX, newY, 0, newID );
                    

                    // update old spot
                    // do this second, so that it is reported to client
                    // after move is reported
                    
                    if( destTrans == NULL || destTrans->newActor == 0 ) {
                        // try bare ground trans
                        destTrans = getPTrans( inID, -1 );

                        if( destTrans == NULL ) {
                            // another attempt at bare ground transition
                            destTrans = getPTrans( movingObjID, -1 );
                            }
                        
                        if( destTrans != NULL &&
                            destTrans->newTarget != newID &&
                            destTrans->newTarget != movingObjID ) {
                            // for bare ground, make sure newTarget
                            // matches newTarget of our orginal move transition
                            destTrans = NULL;
                            }
                        }
                    
                        
                    
                    if( destTrans != NULL &&
                        destTrans->newActor > 0 ) {
                        // leave new actor behind
                        
                        leftBehindID = destTrans->newActor;
                        
                        ObjectRecord *leftBehindObj = getObject( leftBehindID );
                        
                        char leftFloor = false;
                        if( leftBehindObj->floor ) {
                            leftFloor = true;
                            }
                        
                        if( leftFloor ) {
                            dbFloorPut( inX, inY, leftBehindID );
                            }
                        else {
                            dbPut( inX, inY, 0, leftBehindID );
                            }

                        
                        TransRecord *leftDecayT = 
                            getMetaTrans( -1, leftBehindID );

                        double leftMapETA = 0;
                        
                        if( leftDecayT != NULL ) {

                            // add some random variation to avoid lock-step
                            // especially after a server restart
                            int tweakedSeconds = 
                                randSource.getRandomBoundedInt( 
                                    lrint( 
                                        leftDecayT->autoDecaySeconds * 0.9 ), 
                                    leftDecayT->autoDecaySeconds );
                            
                            if( tweakedSeconds < 1 ) {
                                tweakedSeconds = 1;
                                }
                            leftMapETA = MAP_TIMESEC + tweakedSeconds;
                            }
                        else {
                            // no further decay
                            leftMapETA = 0;
                            }

                        if( leftFloor ) {
                            setFloorEtaDecay( inX, inY, leftMapETA );
                            
                            // don't leave anything behind except for floor
                            dbPut( inX, inY, 0, 0 );
                            leftBehindID = 0;
                            }
                        else {
                            setEtaDecay( inX, inY, leftMapETA );
                            }
                        }
                    else {
                        // leave empty spot behind
                        dbPut( inX, inY, 0, 0 );
                        leftBehindID = 0;
                        }
                    

                    // move contained
                    int numCont;
                    int *cont = getContained( inX, inY, &numCont );
                    timeSec_t *contEta = 
                        getContainedEtaDecay( inX, inY, &numCont );
                    
                    if( numCont > 0 ) {
                        setContained( newX, newY, numCont, cont );
                        setContainedEtaDecay( newX, newY, numCont, contEta );
                        
                        for( int c=0; c<numCont; c++ ) {
                            if( cont[c] < 0 ) {
                                // sub cont
                                int numSub;
                                int *subCont = getContained( inX, inY, 
                                                             &numSub,
                                                             c + 1 );
                                timeSec_t *subContEta = getContainedEtaDecay( 
                                    inX, inY, &numSub, c + 1 );
                                
                                if( numSub > 0 ) {
                                    setContained( newX, newY, numSub,
                                                  subCont, c + 1 );
                                    setContainedEtaDecay( 
                                        newX, newY, numSub, subContEta, c + 1 );
                                    }
                                delete [] subCont;
                                delete [] subContEta;
                                }
                            }
                        

                        clearAllContained( inX, inY );
                        
                        delete [] cont;
                        delete [] contEta;
                        }
                    
                    double moveDist = sqrt( (newX - inX) * (newX - inX) +
                                            (newY - inY) * (newY - inY) );
                    
                    double speed = 4.0f;
                    
                    char deadly = false;
                    if( newID > 0 ) {
                        ObjectRecord *newObj = getObject( newID );
                        
                        if( newObj != NULL ) {
                            speed *= newObj->speedMult;
                            
                            deadly = ( newObj->deadlyDistance > 0 );
                            }
                        }
                    
                    double moveTime = moveDist / speed;
                    
                    double etaTime = Time::getCurrentTime() + moveTime;
                    
                    MovementRecord moveRec = { newX, newY, inX, inY, 
                                               newID,
                                               deadly, 
                                               etaTime,
                                               moveTime };
                    
                    liveMovementEtaTimes.insert( newX, newY, 0, 0, etaTime );
                    
                    liveMovements.insert( moveRec, etaTime );
                    

                    // now patch up change record marking this as a move
                    
                    for( int i=0; i<mapChangePosSinceLastStep.size(); i++ ) {
                        
                        ChangePosition *p = 
                            mapChangePosSinceLastStep.getElement( i );
                        
                        if( p->x == newX && p->y == newY ) {
                            
                            // update it
                            p->oldX = inX;
                            p->oldY = inY;
                            p->speed = (float)speed;
                            
                            break;
                            }
                        }
                    }
                else {
                    // failed to find a spot to move

                    // default to applying bare-ground transition, if any
                    TransRecord *trans = getPTrans( inID, -1 );
                    
                    int currentMovingID = newID;

                    if( trans == NULL ) {
                        // does trans exist for newID applied to
                        // bare ground
                        trans = getPTrans( currentMovingID, -1 );
                        }
                    if( trans != NULL ) {
                        newID = trans->newTarget;
                        
                        // what was SUPPOSED to be left behind on ground
                        // that object moved away from?
                        if( trans->newActor > 0 ) {
                            
                            // see if there's anything defined for when
                            // the new object moves ONTO this thing
                            
                            // (object is standing still in same spot, 
                            //  effectively on top of what it was supposed
                            //  to leave behind)
                            
                            TransRecord *inPlaceTrans = 
                                getPTrans( newID, trans->newActor );

                            if( inPlaceTrans == NULL ) {
                                // see if there's anything for moving ID
                                // applied directly to what's left on ground
                                // allowing moving item to remain in place
                                inPlaceTrans = getPTrans( currentMovingID,
                                                          trans->newActor );
                                }
                            
                            if( inPlaceTrans != NULL &&
                                inPlaceTrans->newTarget > 0 ) {
                                
                                newID = inPlaceTrans->newTarget;
                                }
                            }
                        }
                    }
                }
            
                
            if( newX == inX && newY == inY ) {
                // no move happened

                // just set change in DB
                setMapObjectRaw( inX, inY, newID );
                }
            
                
            TransRecord *newDecayT = getMetaTrans( -1, newID );

            if( newDecayT != NULL ) {

                // add some random variation to avoid lock-step
                // especially after a server restart
                int tweakedSeconds = 
                    randSource.getRandomBoundedInt( 
                        lrint( newDecayT->autoDecaySeconds * 0.9 ), 
                        newDecayT->autoDecaySeconds );
                
                if( tweakedSeconds < 1 ) {
                    tweakedSeconds = 1;
                    }
                mapETA = MAP_TIMESEC + tweakedSeconds;
                }
            else {
                // no further decay
                mapETA = 0;
                }
            
            if( mapETA != 0 && 
                ( newX != inX ||
                  newY != inY ) ) {
                
                // copy old last look time from where we came from
                char foundInOldSpot;
                
                timeSec_t lastLookTime =
                    liveDecayRecordLastLookTimeHashTable.lookup( 
                        inX, inY, 0, 0, &foundInOldSpot );
                
                if( foundInOldSpot ) {
                    
                    char foundInNewSpot;
                    liveDecayRecordLastLookTimeHashTable.
                        lookup( newX, newY, 0, 0, &foundInNewSpot );
    
                    if( ! foundInNewSpot ) {
                        // we're not tracking decay for this new cell yet
                        // but leave a look time here to affect
                        // the tracking that we're about to setup
                        
                        liveDecayRecordLastLookTimeHashTable.
                            insert( newX, newY, 0, 0, lastLookTime );
                        }
                    }
                }            

            setEtaDecay( newX, newY, mapETA, newDecayT );
            }

        }
    else {
        // an object on the map that has never been seen by anyone before
        // not decaying yet
        
        // update map with decay for the applicable transition
        
        // randomize it so that every same object on map
        // doesn't cycle at same time
        int decayTime = 
            randSource.getRandomBoundedInt( t->autoDecaySeconds / 2 , 
                                            t->autoDecaySeconds );
        if( decayTime < 1 ) {
            decayTime = 1;
            }
        
        mapETA = MAP_TIMESEC + decayTime;
            
        setEtaDecay( inX, inY, mapETA );
        }
    

    if( newX != inX ||
        newY != inY ) {
        // object moved and is gone
        return leftBehindID;
        }
    else {
        return newID;
        }
    }



void checkDecayContained( int inX, int inY, int inSubCont ) {
    
    if( getNumContained( inX, inY, inSubCont ) == 0 ) {
        return;
        }
    
    int numContained;
    int *contained = getContainedRaw( inX, inY, &numContained, inSubCont );
    
    SimpleVector<int> newContained;
    SimpleVector<timeSec_t> newDecayEta;
    
    SimpleVector< SimpleVector<int> > newSubCont;
    SimpleVector< SimpleVector<timeSec_t> > newSubContDecay;
    
        
    char change = false;

    // track last ID we saw with no decay, so we don't have to keep
    // looking it up over and over.
    int lastIDWithNoDecay = 0;
    
    
    for( int i=0; i<numContained; i++ ) {
        int oldID = contained[i];
        
        if( oldID == lastIDWithNoDecay ) {
            // same ID we've already seen before
            newContained.push_back( oldID );
            newDecayEta.push_back( 0 );
            continue;
            }
        

        char isSubCont = false;
        
        if( oldID < 0 ) {
            // negative ID means this is a sub-container
            isSubCont = true;
            oldID *= -1;
            }
    
        TransRecord *t = getPTrans( -1, oldID );

        if( t == NULL ) {
            // no auto-decay for this object
            if( isSubCont ) {
                oldID *= -1;
                }
            lastIDWithNoDecay = oldID;
            
            newContained.push_back( oldID );
            newDecayEta.push_back( 0 );
            continue;
            }
    
    
        // else decay exists for this object
    
        int newID = oldID;

        // is eta stored in map?
        timeSec_t mapETA = getSlotEtaDecay( inX, inY, i, inSubCont );
    
        if( mapETA != 0 ) {
        
            if( (int)mapETA <= MAP_TIMESEC ) {
            
                // object in container slot has decayed (eta expired)
                
                // apply the transition
                newID = t->newTarget;
                
                if( newID != oldID ) {
                    change = true;
                    }
    
                if( newID != 0 ) {
                    
                    TransRecord *newDecayT = getMetaTrans( -1, newID );

                    if( newDecayT != NULL ) {
                        
                        // add some random variation to avoid lock-step
                        // especially after a server restart
                        int tweakedSeconds = 
                            randSource.getRandomBoundedInt( 
                                lrint( newDecayT->autoDecaySeconds * 0.9 ), 
                                newDecayT->autoDecaySeconds );

                        if( tweakedSeconds < 1 ) {
                            tweakedSeconds = 1;
                            }
                        
                        mapETA = 
                            MAP_TIMESEC +
                            tweakedSeconds / 
                            getMapContainerTimeStretch( inX, inY, inSubCont );
                        }
                    else {
                        // no further decay
                        mapETA = 0;
                        }
                    }
                }
            }
        
        if( newID != 0 ) {    
            if( isSubCont ) {
                
                int oldSlots = getNumContainerSlots( oldID );

                int newSlots = getNumContainerSlots( newID );
            
                if( newSlots < oldSlots ) {
                    shrinkContainer( inX, inY, newSlots, i + 1 );
                    }
                if( newSlots > 0 ) {    
                    restretchMapContainedDecays( inX, inY, 
                                                 oldID, newID, i + 1 );
                    
                    // negative IDs indicate sub-containment 
                    newID *= -1;
                    }
                }
            newContained.push_back( newID );
            newDecayEta.push_back( mapETA );
            }
        }
    
    
    if( change ) {
        int *containedArray = newContained.getElementArray();
        int numContained = newContained.size();

        setContained( inX, inY, newContained.size(), containedArray, 
                      inSubCont );
        delete [] containedArray;
        
        for( int i=0; i<numContained; i++ ) {
            timeSec_t mapETA = newDecayEta.getElementDirect( i );
            
            if( mapETA != 0 ) {
                trackETA( inX, inY, 1 + i, mapETA, inSubCont );
                }
            
            setSlotEtaDecay( inX, inY, i, mapETA, inSubCont );
            }
        }

    if( contained != NULL ) {
        delete [] contained;
        }
    }




int getTweakedBaseMap( int inX, int inY ) {
    
    // nothing in map
    char wasGridPlacement = false;
        
    int result = getBaseMap( inX, inY, &wasGridPlacement );

    if( result > 0 ) {
        ObjectRecord *o = getObject( result );
            
        if( o->wide ) {
            // make sure there's not possibly another wide object too close
            int maxR = getMaxWideRadius();
                
            for( int dx = -( o->leftBlockingRadius + maxR );
                 dx <= ( o->rightBlockingRadius + maxR ); dx++ ) {
                    
                if( dx != 0 ) {
                    int nID = getBaseMap( inX + dx, inY );
                        
                    if( nID > 0 ) {
                        ObjectRecord *nO = getObject( nID );
                            
                        if( nO->wide ) {
                                
                            int minDist;
                            int dist;
                                
                            if( dx < 0 ) {
                                minDist = nO->rightBlockingRadius +
                                    o->leftBlockingRadius;
                                dist = -dx;
                                }
                            else {
                                minDist = nO->leftBlockingRadius +
                                    o->rightBlockingRadius;
                                dist = dx;
                                }

                            if( dist <= minDist ) {
                                // collision
                                // don't allow this wide object here
                                return 0;
                                }
                            }
                        }
                    }
                }
            }
        else if( !wasGridPlacement && getObjectHeight( result ) < CELL_D ) {
            // a short object should be here
            // and it wasn't forced by a grid placement

            // make sure there's not any semi-short objects below already

            // this avoids vertical stacking of short objects
            // and ensures that the map is sparse with short object
            // clusters, even in very dense map areas
            // (we don't want the floor tiled with berry bushes)

            // this used to be an unintentional bug, but it was in place
            // for a year, and we got used to it.

            // when the bug was fixed, the map became way too dense
            // in short-object areas
                
            // actually, fully replicate the bug for now
            // only block short objects with objects to the south
            // that extend above the tile midline

            // So we can have clusters of very short objects, like stones
            // but not less-short ones like berry bushes, rabbit holes, etc.

            // use the old buggy "2 pixels" and "3 pixels" above the
            // midline measure just to keep the map the same

            // south
            int sID = getBaseMap( inX, inY - 1 );
                        
            if( sID > 0 && getObjectHeight( sID ) >= 2 ) {
                return 0;
                }
                
            int s2ID = getBaseMap( inX, inY - 2 );
                        
            if( s2ID > 0 && getObjectHeight( s2ID ) >= 3 ) {
                return 0;
                }                
            }

        if( o->forceBiome != -1 &&
            biomeDBGet( inX, inY ) == -1 &&
            getBiomeIndex( o->forceBiome ) != -1 ) {
            
            // naturally-occurring object that forces a biome
            // stick into floorDB            
            biomeDBPut( inX, inY, o->forceBiome, o->forceBiome, 0.5 );

            if( lastCheckedBiome != -1 &&
                lastCheckedBiomeX == inX &&
                lastCheckedBiomeY == inY ) {
                // replace last checked with this
                lastCheckedBiome = o->forceBiome;
                }

            // we also need to force-set the object itself into the DB
            // otherwise, the next time we gen this square, we might not
            // generate this object, because the underlying biome has changed
            setMapObjectRaw( inX, inY, o->id );
            }
        }
    return result;
    }



static int getPossibleBarrier( int inX, int inY ) {
    if( barrierOn )
    if( inX == barrierRadius ||
        inX == - barrierRadius ||
        inY == barrierRadius ||
        inY == - barrierRadius ) {

        // along barrier line
        
        // now make sure that we don't stick out beyond square

        if( inX <= barrierRadius &&
            inX >= -barrierRadius &&
            inY <= barrierRadius &&
            inY >= -barrierRadius ) {
            
        
            setXYRandomSeed( 9238597 );
            
            int numOptions = barrierItemList.size();
            
            if( numOptions > 0 ) {
                
                // random doesn't always look good
                int pick =
                    floor( numOptions * getXYRandom( inX * 10, inY * 10 ) );

                if( pick >= numOptions ) {
                    pick = numOptions - 1;
                    }
                
                int barrierID = barrierItemList.getElementDirect( pick );

                if( getObject( barrierID ) != NULL ) {
                    // actually exists
                    return barrierID;
                    }
                }
            }
        }

    return 0;
    }



int getMapObjectRaw( int inX, int inY ) {
    
    int barrier = getPossibleBarrier( inX, inY );

    if( barrier != 0 ) {
        return barrier;
        }

    int result = dbGet( inX, inY, 0 );
    
    if( result == -1 ) {
        result = getTweakedBaseMap( inX, inY );
        }
    
    return result;
    }




void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd ) {
    timeSec_t currentTime = MAP_TIMESEC;
    
    for( int y=inYStart; y<=inYEnd; y++ ) {
        for( int x=inXStart; x<=inXEnd; x++ ) {
        
            
            if( ! lookTimeTracking.checkExists( x, y, currentTime ) ) {
                
                // we haven't looked at this spot in a while
                
                // see if any decays apply
                // if so, get that part of the tile once to re-trigger
                // live tracking

                timeSec_t floorEtaDecay = getFloorEtaDecay( x, y );
                
                if( floorEtaDecay != 0 &&
                    floorEtaDecay < 
                    currentTime + maxSecondsForActiveDecayTracking ) {
                
                    getMapFloor( x, y );
                    }
                
                    

                timeSec_t etaDecay = getEtaDecay( x, y );
                
                int objID = 0;
                
                if( etaDecay != 0 &&
                    etaDecay < 
                    currentTime + maxSecondsForActiveDecayTracking ) {
                
                    objID = getMapObject( x, y );
                    }

                // also check all contained items to trigger
                // live tracking of their decays too
                if( objID != 0 ) {
                    
                    int numCont = getNumContained( x, y );
                    
                    for( int c=0; c<numCont; c++ ) {
                        int contID = getContained( x, y, c );
                        
                        if( contID < 0 ) {
                            // sub cont
                            int numSubCont = getNumContained( x, y, c + 1 );
                            
                            for( int s=0; s<numSubCont; s++ ) {
                                getContained( x, y, c, s + 1 );
                                }
                            }
                        }
                    }
                }
            

            timeSec_t *oldLookTime = 
                liveDecayRecordLastLookTimeHashTable.lookupPointer( x, y, 
                                                                    0, 0 );
    
            if( oldLookTime != NULL ) {
                // we're tracking decay for this cell
                *oldLookTime = currentTime;
                }

            ContRecord *contRec = 
                liveDecayRecordLastLookTimeMaxContainedHashTable.
                lookupPointer( x, y, 0, 0 );
            
            if( contRec != NULL ) {

                for( int c=1; c<= contRec->maxSlots; c++ ) {
                
                    oldLookTime =
                        liveDecayRecordLastLookTimeHashTable.lookupPointer( 
                            x, y, c, 0 );
                    if( oldLookTime != NULL ) {
                        // look at it now
                        *oldLookTime = currentTime;
                        }            

                    for( int s=1; s<= contRec->maxSubSlots; s++ ) {
                        
                        oldLookTime =
                            liveDecayRecordLastLookTimeHashTable.lookupPointer( 
                                x, y, c, s );
                        if( oldLookTime != NULL ) {
                            // look at it now
                            *oldLookTime = currentTime;
                            }
                        }
                    }
                }
            }
        }
    }



int getMapObject( int inX, int inY ) {

    // look at this map cell
    timeSec_t *oldLookTime = 
        liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY, 0, 0 );
    
    timeSec_t curTime = MAP_TIMESEC;

    if( oldLookTime != NULL ) {
        // we're tracking decay for this cell
        *oldLookTime = curTime;
        }

    // apply any decay that should have happened by now
    return checkDecayObject( inX, inY, getMapObjectRaw( inX, inY ) );
    }


int getMapObjectNoLook( int inX, int inY ) {

    // apply any decay that should have happened by now
    return checkDecayObject( inX, inY, getMapObjectRaw( inX, inY ) );
    }



char isMapObjectInTransit( int inX, int inY ) {
    char found;
    
    double etaTime = 
        liveMovementEtaTimes.lookup( inX, inY, 0, 0, &found );
    
    if( found ) {
        if( etaTime > Time::getCurrentTime() ) {
            return true;
            }
        }
    
    return false;
    }



int getMapBiome( int inX, int inY ) {
    return biomes[getMapBiomeIndex( inX, inY )];
    }




// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inStartX, int inStartY, 
                                int inWidth, int inHeight,
                                GridPos inRelativeToPos,
                                int *outMessageLength ) {
    
    int chunkCells = inWidth * inHeight;
    
    int *chunk = new int[chunkCells];

    int *chunkBiomes = new int[chunkCells];
    int *chunkFloors = new int[chunkCells];
    
    int *containedStackSizes = new int[ chunkCells ];
    int **containedStacks = new int*[ chunkCells ];

    int **subContainedStackSizes = new int*[chunkCells];
    int ***subContainedStacks = new int**[chunkCells];
    

    int endY = inStartY + inHeight;
    int endX = inStartX + inWidth;

    if( endY < inStartY ) {
        // wrapped around in integer space
        // pull inStartY back from edge
        inStartY -= inHeight;
        endY = inStartY + inHeight;
        }
    if( endX < inStartX ) {
        // wrapped around in integer space
        // pull inStartY back from edge
        inStartX -= inWidth;
        endX = inStartX + inWidth;
        }
    


    timeSec_t curTime = MAP_TIMESEC;

    // look at four corners of chunk whenever we fetch one
    dbLookTimePut( inStartX, inStartY, curTime );
    dbLookTimePut( inStartX, endY, curTime );
    dbLookTimePut( endX, inStartY, curTime );
    dbLookTimePut( endX, endY, curTime );
    
    for( int y=inStartY; y<endY; y++ ) {
        int chunkY = y - inStartY;
        

        for( int x=inStartX; x<endX; x++ ) {
            int chunkX = x - inStartX;
            
            int cI = chunkY * inWidth + chunkX;
            
            lastCheckedBiome = -1;
            
            chunk[cI] = getMapObject( x, y );

            if( lastCheckedBiome == -1 ||
                lastCheckedBiomeX != x ||
                lastCheckedBiomeY != y ) {
                // biome wasn't checked in order to compute
                // getMapObject

                // get it ourselves
                
                lastCheckedBiome = biomes[getMapBiomeIndex( x, y )];
                }
            chunkBiomes[ cI ] = lastCheckedBiome;

            chunkFloors[cI] = getMapFloor( x, y );
            

            int numContained;
            int *contained = NULL;

            if( chunk[cI] > 0 && getObject( chunk[cI] )->numSlots > 0 ) {
                contained = getContained( x, y, &numContained );
                }
            
            if( contained != NULL ) {
                containedStackSizes[cI] = numContained;
                containedStacks[cI] = contained;
                
                subContainedStackSizes[cI] = new int[numContained];
                subContainedStacks[cI] = new int*[numContained];

                for( int i=0; i<numContained; i++ ) {
                    subContainedStackSizes[cI][i] = 0;
                    subContainedStacks[cI][i] = NULL;
                    
                    if( containedStacks[cI][i] < 0 ) {
                        // a sub container
                        containedStacks[cI][i] *= -1;
                        
                        int numSubContained;
                        int *subContained = getContained( x, y, 
                                                          &numSubContained,
                                                          i + 1 );
                        if( subContained != NULL ) {
                            subContainedStackSizes[cI][i] = numSubContained;
                            subContainedStacks[cI][i] = subContained;
                            }
                        }
                    }
                }
            else {
                containedStackSizes[cI] = 0;
                containedStacks[cI] = NULL;
                subContainedStackSizes[cI] = NULL;
                subContainedStacks[cI] = NULL;
                }
            }
        
        }



    SimpleVector<unsigned char> chunkDataBuffer;

    for( int i=0; i<chunkCells; i++ ) {
        
        if( i > 0 ) {
            chunkDataBuffer.appendArray( (unsigned char*)" ", 1 );
            }
        

        char *cell = autoSprintf( "%d:%d:%d", chunkBiomes[i],
                                  hideIDForClient( chunkFloors[i] ), 
                                  hideIDForClient( chunk[i] ) );
        
        chunkDataBuffer.appendArray( (unsigned char*)cell, strlen(cell) );
        delete [] cell;

        if( containedStacks[i] != NULL ) {
            for( int c=0; c<containedStackSizes[i]; c++ ) {
                char *containedString = 
                    autoSprintf( ",%d", 
                                 hideIDForClient( containedStacks[i][c] ) );
        
                chunkDataBuffer.appendArray( (unsigned char*)containedString, 
                                             strlen( containedString ) );
                delete [] containedString;

                if( subContainedStacks[i][c] != NULL ) {
                    
                    for( int s=0; s<subContainedStackSizes[i][c]; s++ ) {
                        
                        char *subContainedString = 
                            autoSprintf( ":%d", 
                                         hideIDForClient( 
                                             subContainedStacks[i][c][s] ) );
        
                        chunkDataBuffer.appendArray( 
                            (unsigned char*)subContainedString, 
                            strlen( subContainedString ) );
                        delete [] subContainedString;
                        }
                    delete [] subContainedStacks[i][c];
                    }
                }

            delete [] subContainedStackSizes[i];
            delete [] subContainedStacks[i];

            delete [] containedStacks[i];
            }
        }
    
    delete [] chunk;
    delete [] chunkBiomes;
    delete [] chunkFloors;

    delete [] containedStackSizes;
    delete [] containedStacks;

    delete [] subContainedStackSizes;
    delete [] subContainedStacks;
    
    

    unsigned char *chunkData = chunkDataBuffer.getElementArray();
    
    int compressedSize;
    unsigned char *compressedChunkData =
        zipCompress( chunkData, chunkDataBuffer.size(),
                     &compressedSize );



    char *header = autoSprintf( "MC\n%d %d %d %d\n%d %d\n#", 
                                inWidth, inHeight,
                                inStartX - inRelativeToPos.x, 
                                inStartY - inRelativeToPos.y, 
                                chunkDataBuffer.size(),
                                compressedSize );
    
    SimpleVector<unsigned char> buffer;
    buffer.appendArray( (unsigned char*)header, strlen( header ) );
    delete [] header;

    
    buffer.appendArray( compressedChunkData, compressedSize );
    
    delete [] chunkData;
    delete [] compressedChunkData;
    

    
    *outMessageLength = buffer.size();
    return buffer.getElementArray();
    }










char isMapSpotBlocking( int inX, int inY ) {
    
    signed char cachedVal = blockingGetCached( inX, inY );
    if( cachedVal != -1 ) {
        
        return cachedVal;
        }


    int target = getMapObject( inX, inY );

    if( target != 0 ) {
        ObjectRecord *obj = getObject( target );
    
        if( obj->blocksWalking ) {
            // only cache direct hits
            // wide objects that block are difficult to clear from cache
            // when map cell changes
            blockingPutCached( inX, inY, 1 );
            return true;
            }
        }
    
    // not directly blocked
    // need to check for wide objects to left and right
    int maxR = getMaxWideRadius();
    
    for( int dx = -maxR; dx <= maxR; dx++ ) {
        
        if( dx != 0 ) {
            
            int nX = inX + dx;
        
            int nID = getMapObject( nX, inY );
            
            if( nID != 0 ) {
                ObjectRecord *nO = getObject( nID );
                
                if( nO->wide ) {
                    
                    int dist;
                    int minDist;
                    
                    if( dx < 0 ) {
                        dist = -dx;
                        minDist = nO->rightBlockingRadius;
                        }
                    else {
                        dist = dx;
                        minDist = nO->leftBlockingRadius;
                        }
                    
                    if( dist <= minDist ) {
                        // don't cache results from wide objects
                        return true;
                        }
                    }
                }
            }
        }
    
    // cache non-blocking results
    blockingPutCached( inX, inY, 0 );
    return false;
    }





static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }



static char tooClose( GridPos inA, GridPos inB, int inMinComponentDistance ) {
    double xDist = (double)inA.x - (double)inB.x;
    if( xDist < 0 ) {
        xDist = -xDist;
        }
    double yDist = (double)inA.y - (double)inB.y;
    if( yDist < 0 ) {
        yDist = -yDist;
        }
    
    if( xDist < inMinComponentDistance &&
        yDist < inMinComponentDistance ) {
        return true;
        }
    return false;
    }
    



static int findGridPos( SimpleVector<GridPos> *inList, GridPos inP ) {
    for( int i=0; i<inList->size(); i++ ) {
        GridPos q = inList->getElementDirect( i );
        if( equal( inP, q ) ) {
            return i;
            }
        }
    return -1;
    }




// inSetO can be NULL
// returns new ID at inX, inY
static int neighborWallAgree( int inX, int inY, ObjectRecord *inSetO,
                              char inRecurse ) {

    // make sure this agrees with all neighbors
    
    int nX[4] = {-1, 1,  0, 0};
    int nY[4] = { 0, 0, -1, 1};
    char nSet[4] = { false, false, false, false };
    int nID[4] = { -1, -1, -1, -1 };
    
    for( int n=0; n<4; n++ ) {
        int oID = getMapObjectRaw( inX + nX[n], inY + nY[n] );
        
        if( oID > 0 ) {
            ObjectRecord *nO = getObject( oID );
            
            if( nO->isAutoOrienting ) {

                if( // watch for E/W neighbors that aren't supposed to affect
                    // us
                    ( n < 2 && ! nO->causeAutoOrientVOnly ) 
                    ||
                    // watch for N/S neighbors that aren't supposed to
                    // affect us
                    ( n >= 2 && ! nO->causeAutoOrientHOnly ) ) {
                    
                    nSet[n] = true;
                    nID[n] = oID;
                    }
                }
            }
        }

    
    int returnID = 0;

    if( inSetO != NULL ) {
        returnID = inSetO->id;
        }
    

    if( inSetO != NULL &&
        inSetO->horizontalVersionID != -1 &&
        inSetO->verticalVersionID != -1 &&
        inSetO->cornerVersionID != -1 ) {
        
        // this object can react to its neighbors
        
        if( inSetO->id != inSetO->verticalVersionID &&
            ( nSet[2] || nSet[3] ) 
            &&
            ! ( nSet[0] || nSet[1] ) ) {
            // should be vert
            
            returnID = inSetO->verticalVersionID;
            }
        else if( inSetO->id != inSetO->horizontalVersionID &&
                 ! ( nSet[2] || nSet[3] ) 
                 &&
                 ( nSet[0] || nSet[1] ) ) {
            // should be horizontal
            
            returnID = inSetO->horizontalVersionID;
            }
        else if( inSetO->id != inSetO->cornerVersionID &&
                 ( nSet[2] || nSet[3] ) 
                 &&
                 ( nSet[0] || nSet[1] ) ) {
            // should be corner
            
            returnID = inSetO->cornerVersionID;
            }
    
        if( returnID != inSetO->id ) {
            dbPut( inX, inY, 0, returnID );
            }
        }
    
    
    if( inRecurse ) {
        // recurse once for each matching neighbor that has orientations
        for( int n=0; n<4; n++ ) {
            if( nSet[n] ) {
                ObjectRecord *nO = getObject( nID[n] );
                
                // need to check this, because nSet is true if
                // it is auto-orienting, but not all auto-orienting
                // objects have all three orientations defined
                if( nO->horizontalVersionID != -1 &&
                    nO->verticalVersionID != -1 &&
                    nO->cornerVersionID != -1 ) {
                 
                    neighborWallAgree( inX + nX[n], inY + nY[n], 
                                       nO, false );
                    }
                }
            }
        }
    
    return returnID;
    }
    

static int applyTapoutGradientRotate( int inX, int inY,
                                      int inTargetX, int inTargetY,
                                      int inEastwardGradientID ) {
    // apply result to itself to flip it 
    // and point gradient in other direction
                
    // eastward + eastward = westward, etc

    // order:  e, w, s, n, ne, se, sw, nw
    
    int numRepeat = 0;
    
    int curObjectID = inEastwardGradientID;                        

    if( inX > inTargetX && inY == inTargetY ) {
        numRepeat = 0;
        }
    else if( inX < inTargetX && inY == inTargetY ) {
        numRepeat = 1;
        }
    else if( inX == inTargetX && inY < inTargetY ) {
        numRepeat = 2;
        }
    else if( inX == inTargetX && inY > inTargetY ) {
        numRepeat = 3;
        }
    else if( inX > inTargetX && inY > inTargetY ) {
        numRepeat = 4;
        }
    else if( inX > inTargetX && inY < inTargetY ) {
        numRepeat = 5;
        }
    else if( inX < inTargetX && inY < inTargetY ) {
        numRepeat = 6;
        }
    else if( inX < inTargetX && inY > inTargetY ) {
        numRepeat = 7;
        }


    
    for( int i=0; i<numRepeat; i++ ) {
        if( curObjectID == 0 ) {
            break;
            }
        TransRecord *flipTrans = getPTrans( curObjectID, curObjectID );
        
        if( flipTrans != NULL ) {
            curObjectID = flipTrans->newTarget;
            }
        }

    if( curObjectID == 0 ) {
        return -1;
        }

    return curObjectID;
    }




// returns true if tapout-triggered a +primaryHomeland object
static char runTapoutOperation( int inX, int inY, 
                                int inRadiusX, int inRadiusY,
                                int inSpacingX, int inSpacingY,
                                int inTriggerID,
                                char inPlayerHasPrimaryHomeland,
                                char inIsPost = false ) {

    char returnVal = false;
    
    for( int y =  inY - inRadiusY; 
         y <= inY + inRadiusY; 
         y += inSpacingY ) {
    
        for( int x =  inX - inRadiusX; 
             x <= inX + inRadiusX; 
             x += inSpacingX ) {
            
            if( inX == x && inY == y ) {
                // skip center
                continue;
                }

            int id = getMapObjectRaw( x, y );
                    
            // change triggered by tapout represented by 
            // tapoutTrigger object getting used as actor
            // on tapoutTarget
            TransRecord *t = NULL;
            
            int newTarget = -1;

            if( ! inIsPost ) {
                // last use target signifies what happens in 
                // same row or column as inX, inY
                
                // get eastward
                t = getPTrans( inTriggerID, id, false, true );

                if( t != NULL ) {
                    newTarget = t->newTarget;
                    }
                
                if( newTarget > 0 ) {
                    newTarget = applyTapoutGradientRotate( inX, inY,
                                                           x, y,
                                                           newTarget );
                    }
                }

            if( newTarget == -1 ) {
                // not same row or post or last-use-target trans undefined
                t = getPTrans( inTriggerID, id );
                
                if( t != NULL ) {
                    newTarget = t->newTarget;
                    }
                }

            if( newTarget != -1 ) {
                ObjectRecord *nt = getObject( newTarget );
                
                if( strstr( nt->description, "+primaryHomeland" ) != NULL ) {
                    if( inPlayerHasPrimaryHomeland ) {
                        // block creation of objects that require 
                        // +primaryHomeland
                        // player already has a primary homeland
                    
                        newTarget = -1;
                        }
                    else {
                        // created a +primaryHomeland object
                        returnVal = true;
                        }
                    }
                }
            
            if( newTarget != -1 ) {
                setMapObjectRaw( x, y, newTarget );
                }
            }
        }
    
    return returnVal;
    }



// from server.cpp
extern int getPlayerLineage( int inID );
extern char isPlayerIgnoredForEvePlacement( int inID );

    


void setMapObjectRaw( int inX, int inY, int inID ) {
    int oldID = dbGet( inX, inY, 0 );
    
    dbPut( inX, inY, 0, inID );
    

    
    // allow un-trigger for auto-orient even if tile becomes empty
    
    ObjectRecord *o = NULL;

    if( inID > 0 ) {
        o = getObject( inID );
        }


    if( o != NULL && o->isAutoOrienting ) {
        
        // recurse one step
        inID = neighborWallAgree( inX, inY, o, true );
        
        o = getObject( inID );
        }
    else if( oldID > 0 ) {
        ObjectRecord *oldO = getObject( oldID );
        
        if( oldO->isAutoOrienting ) {
            // WAS auto-orienting, but not anymore
            // recurse once to let neighbors un-react to it
            neighborWallAgree( inX, inY, o, true );
            }
        }
    


    // global trigger and speech pipe stuff
    // skip if tile is now empty

    if( o == NULL ) {
        return;
        }

        
    
    char tappedOutPrimaryHomeland = false;
    

    if( o->isFlightLanding ) {
        GridPos p = { inX, inY };

        char found = false;

        for( int i=0; i<flightLandingPos.size(); i++ ) {
            GridPos otherP = flightLandingPos.getElementDirect( i );
            
            if( equal( p, otherP ) ) {
                
                // make sure this other strip really still exists
                int oID = getMapObject( otherP.x, otherP.y );
            
                if( oID <=0 ||
                    ! getObject( oID )->isFlightLanding ) {
                
                    // not even a valid landing pos anymore
                    flightLandingPos.deleteElement( i );
                    i--;
                    }
                else {
                    found = true;
                    break;
                    }
                }
            }
        
        if( !found ) {
            flightLandingPos.push_back( p );
            }
        }
    


    if( o->speechPipeIn ) {
        GridPos p = { inX, inY };
        
        int foundIndex = 
            findGridPos( &( speechPipesIn[ o->speechPipeIndex ] ), p );
        
        if( foundIndex == -1 ) {
            speechPipesIn[ o->speechPipeIndex ].push_back( p );
            }        
        }
    else if( o->speechPipeOut ) {
        GridPos p = { inX, inY };
        
        int foundIndex = 
            findGridPos( &( speechPipesOut[ o->speechPipeIndex ] ), p );
        
        if( foundIndex == -1 ) {
            speechPipesOut[ o->speechPipeIndex ].push_back( p );
            }
        }
    else if( o->isGlobalTriggerOn ) {
        GlobalTriggerState *s = globalTriggerStates.getElement(
            o->globalTriggerIndex );
        
        GridPos p = { inX, inY };
        
        int foundIndex = findGridPos( &( s->triggerOnLocations ), p );
        
        if( foundIndex == -1 ) {
            s->triggerOnLocations.push_back( p );
            
            if( s->triggerOnLocations.size() == 1 ) {
                // just turned on globally

                /// process all receivers
                for( int i=0; i<s->receiverLocations.size(); i++ ) {
                    GridPos q = s->receiverLocations.getElementDirect( i );

                    int id = getMapObjectRaw( q.x, q.y );
                    
                    if( id <= 0 ) {
                        // receiver no longer here
                        s->receiverLocations.deleteElement( i );
                        i--;
                        continue;
                        }

                    ObjectRecord *oR = getObject( id );
                    
                    if( oR->isGlobalReceiver &&
                        oR->globalTriggerIndex == o->globalTriggerIndex ) {
                        // match
                        
                        int metaID = getMetaTriggerObject( 
                            o->globalTriggerIndex );
                        
                        if( metaID > 0 ) {
                            TransRecord *tR = getPTrans( metaID, id );
                            
                            if( tR != NULL ) {
                                
                                dbPut( q.x, q.y, 0, tR->newTarget );
                            
                                // save this to our "triggered" list
                                int foundIndex = findGridPos(
                                    &( s->triggeredLocations ), q );
                                
                                if( foundIndex != -1 ) {
                                    // already exists
                                    // replace
                                    *( s->triggeredIDs.getElement( 
                                           foundIndex ) ) =
                                        tR->newTarget;
                                    *( s->triggeredRevertIDs.getElement( 
                                           foundIndex ) ) =
                                        tR->target;
                                    }
                                else {
                                    s->triggeredLocations.push_back( q );
                                    s->triggeredIDs.push_back( tR->newTarget );
                                    s->triggeredRevertIDs.push_back( 
                                        tR->target );
                                    }
                                }
                            }
                        }
                    // receiver no longer here
                    // (either wasn't here anymore for other reasons,
                    //  or we just changed it into its triggered state)
                    // remove it
                    s->receiverLocations.deleteElement( i );
                    i--;
                    }
                }
            }
        }
    else if( o->isGlobalTriggerOff ) {
        GlobalTriggerState *s = globalTriggerStates.getElement(
            o->globalTriggerIndex );
        
        GridPos p = { inX, inY };

        int foundIndex = findGridPos( &( s->triggerOnLocations ), p );
        
        if( foundIndex != -1 ) {
            s->triggerOnLocations.deleteElement( foundIndex );
            
            if( s->triggerOnLocations.size() == 0 ) {
                // just turned off globally, no on triggers left on map

                /// process all triggered elements back to off

                for( int i=0; i<s->triggeredLocations.size(); i++ ) {
                    GridPos q = s->triggeredLocations.getElementDirect( i );

                    int curID = getMapObjectRaw( q.x, q.y );

                    int triggeredID = s->triggeredIDs.getElementDirect( i );
                    
                    if( curID == triggeredID ) {
                        // cell still in triggered state

                        // revert it
                        int revertID = 
                            s->triggeredRevertIDs.getElementDirect( i );
                        
                        dbPut( q.x, q.y, 0, revertID );
                        
                        // no longer triggered, remove it
                        s->triggeredLocations.deleteElement( i );
                        s->triggeredIDs.deleteElement( i );
                        s->triggeredRevertIDs.deleteElement( i );
                        i--;
                        
                        // remember it as a reciever (it has gone back
                        // to being a receiver again)
                        s->receiverLocations.push_back( q );
                        }
                    }
                }
            }
        }
    else if( o->isGlobalReceiver ) {
        GlobalTriggerState *s = globalTriggerStates.getElement(
            o->globalTriggerIndex );
        
        GridPos p = { inX, inY };

        int foundIndex = findGridPos( &( s->receiverLocations ), p );
        
        if( foundIndex == -1 ) {
            s->receiverLocations.push_back( p );
            }
        
        if( s->triggerOnLocations.size() > 0 ) {
            // this receiver is currently triggered
            
            // trigger it now, right away, as soon as it is placed on map
                                    
            int metaID = getMetaTriggerObject( o->globalTriggerIndex );
                        
            if( metaID > 0 ) {
                TransRecord *tR = getPTrans( metaID, inID );

                if( tR != NULL ) {
                    
                    dbPut( inX, inY, 0, tR->newTarget );
                        
                    GridPos q = { inX, inY };
                                  
    
                    // save this to our "triggered" list
                    int foundIndex = findGridPos( 
                        &( s->triggeredLocations ), q );
                    
                    if( foundIndex != -1 ) {
                        // already exists
                        // replace
                        *( s->triggeredIDs.getElement( foundIndex ) ) =
                            tR->newTarget;
                        *( s->triggeredRevertIDs.getElement( 
                               foundIndex ) ) =
                            tR->target;
                        }
                    else {
                        s->triggeredLocations.push_back( q );
                        s->triggeredIDs.push_back( tR->newTarget );
                        s->triggeredRevertIDs.push_back( tR->target );
                        }
                    }
                }
            }
        }
    else if( o->isTapOutTrigger ) {
        // this object, when created, taps out other objects in grid around

        char playerHasPrimaryHomeland = false;
        
        if( currentResponsiblePlayer != -1 ) {
            int pID = currentResponsiblePlayer;
            if( pID < 0 ) {
                pID = -pID;
                }
            int lineage = getPlayerLineage( pID );
            
            if( lineage != -1 ) {
                playerHasPrimaryHomeland = hasPrimaryHomeland( lineage );
                }
            }
        
        // don't make current player responsible for all these changes
        int restoreResponsiblePlayer = currentResponsiblePlayer;
        currentResponsiblePlayer = -1;        
        
        TapoutRecord *r = getTapoutRecord( inID );
        
        if( r != NULL ) {

            tappedOutPrimaryHomeland = 
            runTapoutOperation( inX, inY, 
                                r->limitX, r->limitY,
                                r->gridSpacingX, r->gridSpacingY, 
                                inID,
                                playerHasPrimaryHomeland );
            
            
            r->buildCount++;
            
            if( r->buildCountLimit != -1 &&
                r->buildCount >= r->buildCountLimit ) {
                // hit limit!
                // tapout a larger radius now
                tappedOutPrimaryHomeland =
                runTapoutOperation( inX, inY, 
                                    r->postBuildLimitX, r->postBuildLimitY,
                                    r->gridSpacingX, r->gridSpacingY, 
                                    inID, 
                                    playerHasPrimaryHomeland, true );
                }
            }
        
        currentResponsiblePlayer = restoreResponsiblePlayer;
        }
    
    
    if( o->famUseDist > 0  && currentResponsiblePlayer != -1 ) {
        
        int p = currentResponsiblePlayer;
        if( p < 0 ) {
            p = - p;
            }
        
        int lineage = getPlayerLineage( p );
        
        if( lineage != -1 ) {

            // include expired, and update
            Homeland *h = getHomeland( inX, inY, true );

            double t = Time::getCurrentTime();
                                  
            if( h == NULL ) {

                // is this the family's first homeland?
                char firstHomelandForFamily = true;
                
                for( int i=0; i<homelands.size(); i++ ) {
                    Homeland *h = homelands.getElement( i );
                    
                    // watch for stale
                    if( ! h->expired &&
                        h->lineageEveID == lineage ) {
                        firstHomelandForFamily = false;
                        break;
                        }
                    }
                
                Homeland newH = { inX, inY, o->famUseDist,
                                  lineage,
                                  t,
                                  false,
                                  // changed
                                  true,
                                  tappedOutPrimaryHomeland,
                                  isPlayerIgnoredForEvePlacement( p ),
                                  firstHomelandForFamily };
                homelands.push_back( newH );
                }
            else if( h->expired ) {
                // update expired record
                h->x = inX;
                h->y = inY;
                h->radius = o->famUseDist;
                h->lineageEveID = lineage;
                h->lastBabyBirthTime = t;
                h->expired = false;
                h->changed = true;
                }
            }
        }
    }



static void logMapChange( int inX, int inY, int inID ) {
    // log it?
    if( mapChangeLogFile != NULL ) {
        
        double timeDelta = Time::getCurrentTime() - mapChangeLogTimeStart;

        if( timeDelta > 3600 * 24 ) {
            // break logs int 24-hour chunks
            setupMapChangeLogFile();
            timeDelta = Time::getCurrentTime() - mapChangeLogTimeStart;
            }
        
        

        ObjectRecord *o = getObject( inID );
        
        const char *extraFlag = "";
        
        if( o != NULL && o->floor ) {
            extraFlag = "f";
            }
        
        int respPlayer = currentResponsiblePlayer;
        
        if( respPlayer != -1 && respPlayer < 0 ) {
            respPlayer = - respPlayer;
            }

        if( o != NULL && o->isUseDummy ) {
            fprintf( mapChangeLogFile, 
                     "%.2f %d %d %s%du%d %d\n",
                     timeDelta,
                     inX, inY,
                     extraFlag,
                     o->useDummyParent,
                     o->thisUseDummyIndex,
                     respPlayer );
            }
        else if( o != NULL && o->isVariableDummy ) {
            fprintf( mapChangeLogFile, 
                     "%.2f %d %d %s%dv%d %d\n", 
                     timeDelta,
                     inX, inY,
                     extraFlag,
                     o->variableDummyParent,
                     o->thisVariableDummyIndex,
                     respPlayer );
            }
        else {        
            fprintf( mapChangeLogFile, 
                     "%.2f %d %d %s%d %d\n", 
                     timeDelta,
                     inX, inY,
                     extraFlag,
                     inID,
                     respPlayer );
            }
        }
    }



void setMapObject( int inX, int inY, int inID ) {
    
    if( inID > 0 ) {
        ObjectRecord *o = getObject( inID );
        
        if( o != NULL && o->useVarSerialNumbers ) {
            
            inID = getNextVarSerialNumberChild( o );
            }
        }
    

    logMapChange( inX, inY, inID );

    setMapObjectRaw( inX, inY, inID );


    // actually need to set decay here
    // otherwise, if we wait until getObject, it will assume that
    // this is a never-before-seen object and randomize the decay.
    TransRecord *newDecayT = getMetaTrans( -1, inID );
    
    timeSec_t mapETA = 0;
    
    if( newDecayT != NULL && newDecayT->autoDecaySeconds > 0 ) {
        
        mapETA = MAP_TIMESEC + newDecayT->autoDecaySeconds;
        }
    
    setEtaDecay( inX, inY, mapETA );


    // note that we also potentially set up decay for objects on get
    // if they have no decay set already
    // We do this because there are loads
    // of gets that have no set (for example, getting a map chunk)
    // Those decay times get randomized to avoid lock-step in newly-seen
    // objects
    
    if( inID > 0 ) {

        char found = false;        
        
        for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
            
            if( inX == recentPlacements[i].pos.x 
                &&
                inY == recentPlacements[i].pos.y ) {
                
                found = true;
                // update depth
                int newDepth = getObjectDepth( inID );
                
                if( newDepth != UNREACHABLE ) {
                    recentPlacements[i].depth = getObjectDepth( inID );
                    }
                break;
                }
            }
        

        if( !found ) {
            
            int newDepth = getObjectDepth( inID );
            if( newDepth != UNREACHABLE ) {

                recentPlacements[nextPlacementIndex].pos.x = inX;
                recentPlacements[nextPlacementIndex].pos.y = inY;
                recentPlacements[nextPlacementIndex].depth = newDepth;
                
                nextPlacementIndex++;

                if( nextPlacementIndex >= NUM_RECENT_PLACEMENTS ) {
                    nextPlacementIndex = 0;
                
                    // write again every time we have a fresh 100
                    writeRecentPlacements();
                    }
                }
            }
        
        }
        
    }




void setEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds,
                  TransRecord *inApplicableTrans ) {
    dbTimePut( inX, inY, DECAY_SLOT, inAbsoluteTimeInSeconds );
    if( inAbsoluteTimeInSeconds != 0 ) {
        trackETA( inX, inY, 0, inAbsoluteTimeInSeconds, 0, inApplicableTrans );
        }
    }




timeSec_t getEtaDecay( int inX, int inY ) {
    // 0 if not found
    return dbTimeGet( inX, inY, DECAY_SLOT );
    }







void setSlotEtaDecay( int inX, int inY, int inSlot,
                      timeSec_t inAbsoluteTimeInSeconds, int inSubCont ) {
    dbTimePut( inX, inY, getContainerDecaySlot( inX, inY, inSlot, inSubCont ),
               inAbsoluteTimeInSeconds, inSubCont );
    if( inAbsoluteTimeInSeconds != 0 ) {
        setSlotItemsNoDecay( inX, inY, inSubCont, false );

        trackETA( inX, inY, inSlot + 1, inAbsoluteTimeInSeconds,
                  inSubCont );
        }
    }


timeSec_t getSlotEtaDecay( int inX, int inY, int inSlot, int inSubCont ) {
    // 0 if not found
    return dbTimeGet( inX, inY, getContainerDecaySlot( inX, inY, inSlot, 
                                                       inSubCont ),
                      inSubCont );
    }






void addContained( int inX, int inY, int inContainedID, 
                   timeSec_t inEtaDecay, int inSubCont ) {
    int oldNum;
    

    timeSec_t curTime = MAP_TIMESEC;

    if( inEtaDecay != 0 ) {    
        timeSec_t etaOffset = inEtaDecay - curTime;
        
        inEtaDecay = curTime + 
            etaOffset / getMapContainerTimeStretch( inX, inY, inSubCont );
        }
    
    int *oldContained = getContained( inX, inY, &oldNum, inSubCont );

    timeSec_t *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum,
                                                       inSubCont );

    int *newContained = new int[ oldNum + 1 ];
    
    if( oldNum != 0 ) {
        memcpy( newContained, oldContained, oldNum * sizeof( int ) );
        }
    
    newContained[ oldNum ] = inContainedID;
    
    if( oldContained != NULL ) {
        delete [] oldContained;
        }
    
    timeSec_t *newContainedETA = new timeSec_t[ oldNum + 1 ];
    
    if( oldNum != 0 ) {    
        memcpy( newContainedETA, 
                oldContainedETA, oldNum * sizeof( timeSec_t ) );
        }
    
    newContainedETA[ oldNum ] = inEtaDecay;
    
    if( oldContainedETA != NULL ) {
        delete [] oldContainedETA;
        }
    
    int newNum = oldNum + 1;
    
    setContained( inX, inY, newNum, newContained, inSubCont );
    setContainedEtaDecay( inX, inY, newNum, newContainedETA, inSubCont );
    
    delete [] newContained;
    delete [] newContainedETA;
    }


int getNumContained( int inX, int inY, int inSubCont ) {
    int result = dbGet( inX, inY, NUM_CONT_SLOT, inSubCont );
    
    if( result != -1 ) {
        // found
        return result;
        }
    else {
        // default, empty container
        return 0;
        }
    }






void setContained( int inX, int inY, int inNumContained, int *inContained,
                   int inSubCont ) {
    dbPut( inX, inY, NUM_CONT_SLOT, inNumContained, inSubCont );
    for( int i=0; i<inNumContained; i++ ) {
        changeContained( inX, inY, i, inSubCont, inContained[i] );
        }
    }


void setContainedEtaDecay( int inX, int inY, int inNumContained, 
                           timeSec_t *inContainedEtaDecay, int inSubCont ) {
    char someDecay = false;
    for( int i=0; i<inNumContained; i++ ) {
        dbTimePut( inX, inY, 
                   getContainerDecaySlot( inX, inY, i, inSubCont,
                                          inNumContained ),
                   inContainedEtaDecay[i], inSubCont );
        
        if( inContainedEtaDecay[i] != 0 ) {
            someDecay = true;
            trackETA( inX, inY, i + 1, inContainedEtaDecay[i], inSubCont );
            }
        }
    setSlotItemsNoDecay( inX, inY, inSubCont, !someDecay );
    }





int getContained( int inX, int inY, int inSlot, int inSubCont ) {
    int num = getNumContained( inX, inY, inSubCont );
    
    if( num == 0 ) {
        return 0;
        }

    if( inSlot == -1 || inSlot > num - 1 ) {
        inSlot = num - 1;
        }
    
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + inSlot, inSubCont );
    
    if( result != -1 ) {    
        return result;
        }
    else {
        // nothing in that slot
        return 0;
        }
    }


    

// removes from top of stack
int removeContained( int inX, int inY, int inSlot, timeSec_t *outEtaDecay,
                     int inSubCont ) {
    int num = getNumContained( inX, inY, inSubCont );
    
    if( num == 0 ) {
        return 0;
        }

    if( inSlot == -1 || inSlot > num - 1 ) {
        inSlot = num - 1;
        }
    
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + inSlot, inSubCont );

    timeSec_t curTime = MAP_TIMESEC;
    
    timeSec_t resultEta = dbTimeGet( inX, inY, 
                                     getContainerDecaySlot( 
                                         inX, inY, inSlot, inSubCont, num ),
                                     inSubCont );

    if( resultEta != 0 ) {    
        timeSec_t etaOffset = resultEta - curTime;
        
        etaOffset = etaOffset * getMapContainerTimeStretch( inX, inY );
        
        resultEta = curTime + etaOffset;
        }
    
    *outEtaDecay = resultEta;
    
    int oldNum;
    int *oldContained = getContained( inX, inY, &oldNum, inSubCont );

    timeSec_t *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum,
                                                       inSubCont );

    SimpleVector<int> newContainedList;
    SimpleVector<timeSec_t> newContainedETAList;

    SimpleVector<int> newSubContainedNumList;
    SimpleVector<int*> newSubContainedList;
    SimpleVector<timeSec_t*> newSubContainedEtaList;
        
    
    for( int i=0; i<oldNum; i++ ) {
        if( i != inSlot ) {
            newContainedList.push_back( oldContained[i] );
            newContainedETAList.push_back( oldContainedETA[i] );
            
            if( inSubCont == 0 ) {
                int num;
                
                newSubContainedList.push_back(
                    getContained( inX, inY, &num, i + 1 ) );
                newSubContainedNumList.push_back( num );

                newSubContainedEtaList.push_back(
                    getContainedEtaDecay( inX, inY, &num, i + 1 ) );
                }
            }
        }
    clearAllContained( inX, inY );
    
    int *newContained = newContainedList.getElementArray();
    timeSec_t *newContainedETA = newContainedETAList.getElementArray();

    int newNum = oldNum - 1;

    setContained( inX, inY, newNum, newContained, inSubCont );
    setContainedEtaDecay( inX, inY, newNum, newContainedETA, inSubCont );

    if( inSubCont == 0 ) {
        for( int i=0; i<newNum; i++ ) {
            int *idList = newSubContainedList.getElementDirect( i );
            timeSec_t *etaList = newSubContainedEtaList.getElementDirect( i );
            
            if( idList != NULL ) {
                int num = newSubContainedNumList.getElementDirect( i );
                
                setContained( inX, inY, num, idList, i + 1 );
                setContainedEtaDecay( inX, inY, num, etaList, i + 1 );
                
                delete [] idList;
                delete [] etaList;
                }
            }
        }
    

    
    delete [] oldContained;
    delete [] oldContainedETA;
    delete [] newContained;
    delete [] newContainedETA;

    if( result != -1 ) {    
        return result;
        }
    else {
        // nothing in that slot
        return 0;
        }
    }



void clearAllContained( int inX, int inY, int inSubCont ) {
    int oldNum = getNumContained( inX, inY, inSubCont );

    if( inSubCont == 0 ) {
        // clear sub container slots too, if any
    
        for( int i=0; i<oldNum; i++ ) {
            if( getNumContained( inX, inY, i + 1 ) > 0 ) {
                dbPut( inX, inY, NUM_CONT_SLOT, 0, i + 1 );
                }
            }
        }
    
    if( oldNum != 0 ) {
        dbPut( inX, inY, NUM_CONT_SLOT, 0, inSubCont );
        }
    }




#include "spiral.h"


void shrinkContainer( int inX, int inY, int inNumNewSlots, int inSubCont ) {
    int oldNum = getNumContained( inX, inY, inSubCont );
    
    if( oldNum > inNumNewSlots ) {
        
        // first, scatter extra contents into empty nearby spots.
        int nextSprialIndex = 1;
        
        for( int i=inNumNewSlots; i<oldNum; i++ ) {
            
            int contID = getContained( inX, inY, i, inSubCont );

            char subCont = false;
            
            if( contID < 0 ) {
                contID *= -1;
                subCont = true;
                }
            
            int emptyX, emptyY;
            char foundEmpty = false;
            
            GridPos center = { inX, inY };

            while( !foundEmpty ) {
                GridPos sprialPoint = getSpriralPoint( center, 
                                                       nextSprialIndex );
                if( getMapObjectRaw( sprialPoint.x, sprialPoint.y ) == 0 ) {
                    emptyX = sprialPoint.x;
                    emptyY = sprialPoint.y;
                    foundEmpty = true;
                    }
                nextSprialIndex ++;
                }
            
            if( foundEmpty ) {
                setMapObject( emptyX, emptyY, contID );
                
                if( subCont ) {
                    int numSub = getNumContained( inX, inY, i + 1 );
                    
                    for( int s=0; s<numSub; s++ ) {
                        addContained( emptyX, emptyY, 
                                      getContained( inX, inY, s, i + 1 ),
                                      getSlotEtaDecay( inX, inY, 
                                                       s, i + 1 ) );
                        }
                    }
                }
            }
        

        // now clear old extra contents from original spot
        dbPut( inX, inY, NUM_CONT_SLOT, inNumNewSlots, inSubCont );
        
        if( inSubCont == 0 ) {    
            // clear sub cont slots too
            for( int i=inNumNewSlots; i<oldNum; i++ ) {
                dbPut( inX, inY, NUM_CONT_SLOT, 0, i + 1 );
                }
            }
        
        }
    }




MapChangeRecord getMapChangeRecord( ChangePosition inPos ) {

    MapChangeRecord r;
    r.absoluteX = inPos.x;
    r.absoluteY = inPos.y;
    r.oldCoordsUsed = false;

    // compose format string
    SimpleVector<char> buffer;
    

    char *header = autoSprintf( "%%d %%d %d ", 
                                hideIDForClient( 
                                    getMapFloor( inPos.x, inPos.y ) ) );
    
    buffer.appendElementString( header );
    
    delete [] header;
    

    char *idString = autoSprintf( "%d", 
                                  hideIDForClient( 
                                      getMapObjectNoLook( 
                                          inPos.x, inPos.y ) ) );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    int numContained;
    int *contained = getContainedNoLook( inPos.x, inPos.y, &numContained );

    for( int i=0; i<numContained; i++ ) {

        char subCont = false;
        
        if( contained[i] < 0 ) {
            subCont = true;
            contained[i] *= -1;
            
            }
        
        char *idString = autoSprintf( ",%d", hideIDForClient( contained[i] ) );
        
        buffer.appendElementString( idString );
        
        delete [] idString;

        if( subCont ) {
            
            int numSubContained;
            int *subContained = getContainedNoLook( inPos.x, inPos.y, 
                                                    &numSubContained,
                                                    i + 1 );
            for( int s=0; s<numSubContained; s++ ) {

                idString = autoSprintf( ":%d", 
                                        hideIDForClient( subContained[s] ) );
        
                buffer.appendElementString( idString );
        
                delete [] idString;
                }
            if( subContained != NULL ) {
                delete [] subContained;
                }
            }
        
        }
    
    if( contained != NULL ) {
        delete [] contained;
        }
    

    char *player = autoSprintf( " %d", inPos.responsiblePlayerID );
    
    buffer.appendElementString( player );
    
    delete [] player;

    
    if( inPos.speed > 0 ) {
        r.absoluteOldX = inPos.oldX;
        r.absoluteOldY = inPos.oldY;
        r.oldCoordsUsed = true;

        char *moveString = autoSprintf( " %%d %%d %f", 
                                        inPos.speed );
        
        buffer.appendElementString( moveString );
    
        delete [] moveString;
        }

    buffer.appendElementString( "\n" );

    r.formatString = buffer.getElementString();

    return r;
    }





char *getMapChangeLineString( ChangePosition inPos ) {
    MapChangeRecord r = getMapChangeRecord( inPos );

    char *lineString = getMapChangeLineString( &r, 0, 0 );
    
    delete [] r.formatString;
    
    return lineString;
    }




char *getMapChangeLineString( MapChangeRecord *inRecord,
                              int inRelativeToX, int inRelativeToY ) {
    
    char *lineString;
    
    if( inRecord->oldCoordsUsed ) {
        lineString = autoSprintf( inRecord->formatString, 
                                  inRecord->absoluteX - inRelativeToX, 
                                  inRecord->absoluteY - inRelativeToY,
                                  inRecord->absoluteOldX - inRelativeToX, 
                                  inRecord->absoluteOldY - inRelativeToY );
        }
    else {
        lineString = autoSprintf( inRecord->formatString, 
                                  inRecord->absoluteX - inRelativeToX, 
                                  inRecord->absoluteY - inRelativeToY );
        }
    
    return lineString;
    }





int getMapFloor( int inX, int inY ) {
    int id = dbFloorGet( inX, inY );
    
    if( id <= 0 ) {
        return 0;
        }
    
    TransRecord *t = getPTrans( -1, id );

    if( t == NULL ) {
        // no auto-decay for this floor
        return id;
        }

    timeSec_t etaTime = getFloorEtaDecay( inX, inY );
    
    timeSec_t curTime = MAP_TIMESEC;
    

    if( etaTime == 0 ) {
        // not set
        // start decay now for future

        setFloorEtaDecay( inX, inY, curTime + t->autoDecaySeconds );
        
        return id;
        }
    
        
    if( etaTime > curTime ) {
        return id;
        }
    
    // else eta expired, apply decay
    
    int newID = t->newTarget;
    
    setMapFloor( inX, inY, newID );

    return newID;
    }



void setMapFloor( int inX, int inY, int inID ) {
    
    logMapChange( inX, inY, inID );


    dbFloorPut( inX, inY, inID );


    // further decay from here
    TransRecord *newT = getMetaTrans( -1, inID );

    timeSec_t newEta = 0;

    if( newT != NULL ) {
        timeSec_t curTime = MAP_TIMESEC;
        newEta = curTime + newT->autoDecaySeconds;
        }

    setFloorEtaDecay( inX, inY, newEta );
    }



void setFloorEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds ) {
    dbFloorTimePut( inX, inY, inAbsoluteTimeInSeconds );
    }


timeSec_t getFloorEtaDecay( int inX, int inY ) {
    return dbFloorTimeGet( inX, inY );
    }





int getNextDecayDelta() {
    if( liveDecayQueue.size() == 0 ) {
        return -1;
        }
    
    timeSec_t curTime = MAP_TIMESEC;

    timeSec_t minTime = liveDecayQueue.checkMinPriority();
    
    
    if( minTime <= curTime ) {
        return 0;
        }
    
    return minTime - curTime;
    }




static void cleanMaxContainedHashTable( int inX, int inY ) {
    
    ContRecord *oldCount = 
        liveDecayRecordLastLookTimeMaxContainedHashTable.
        lookupPointer( inX, inY, 0, 0 );
                
    if( oldCount != NULL ) {
    
        int maxFoundSlot = 0;
        int maxFoundSubSlot = 0;
        
        for( int c=1; c<= oldCount->maxSlots; c++ ) {

            for( int s=0; s<= oldCount->maxSubSlots; s++ ) {
                timeSec_t *val =
                    liveDecayRecordLastLookTimeHashTable.lookupPointer(
                        inX, inY, c, s );
                
                if( val != NULL ) {
                    maxFoundSlot = c;
                    maxFoundSubSlot = s;
                    }
                }
            }
        

        if( maxFoundSlot == 0 && maxFoundSubSlot == 0 ) {
            liveDecayRecordLastLookTimeMaxContainedHashTable.
                remove( inX, inY, 0, 0 );
            }
        else {
            if( maxFoundSlot < oldCount->maxSlots ) {
                oldCount->maxSlots = maxFoundSlot;
                }
            if( maxFoundSubSlot < oldCount->maxSubSlots ) {
                oldCount->maxSubSlots = maxFoundSubSlot;
                }            
            }
        
        
        }
    }




void stepMap( SimpleVector<MapChangeRecord> *inMapChanges, 
              SimpleVector<ChangePosition> *inChangePosList ) {
    
    timeSec_t curTime = MAP_TIMESEC;

    
    lookTimeTracking.cleanStale( curTime - noLookCountAsStaleSeconds );


    while( liveDecayQueue.size() > 0 && 
           liveDecayQueue.checkMinPriority() <= curTime ) {
        
        // another expired

        LiveDecayRecord r = liveDecayQueue.removeMin();        

        char storedFound;
        timeSec_t storedETA =
            liveDecayRecordPresentHashTable.lookup( r.x, r.y, r.slot,
                                                    r.subCont,
                                                    &storedFound );
        
        if( storedFound && storedETA == r.etaTimeSeconds ) {
            
            liveDecayRecordPresentHashTable.remove( r.x, r.y, r.slot,
                                                    r.subCont );

                    
            timeSec_t lastLookTime =
                liveDecayRecordLastLookTimeHashTable.lookup( r.x, r.y, r.slot,
                                                             r.subCont,
                                                             &storedFound );

            if( storedFound ) {

                if( MAP_TIMESEC - lastLookTime > 
                    maxSecondsNoLookDecayTracking 
                    &&
                    ! isDecayTransAlwaysLiveTracked( r.applicableTrans ) ) {
                    
                    // this cell or slot hasn't been looked at in too long
                    // AND it's not a trans that's live tracked even when
                    // not watched

                    // don't even apply this decay now
                    liveDecayRecordLastLookTimeHashTable.remove( 
                        r.x, r.y, r.slot, r.subCont );
                    cleanMaxContainedHashTable( r.x, r.y );
                    continue;
                    }
                // else keep lastlook time around in case
                // this cell will decay further and we're still tracking it
                // (but maybe delete it if cell is no longer tracked, below)
                }
            }

        if( r.slot == 0 ) {
            

            int oldID = getMapObjectRaw( r.x, r.y );

            // apply real eta from map (to ignore stale duplicates in live list)
            // and update live list if new object is decaying too
        

            // this call will append changes to our global lists, which
            // we process below
            checkDecayObject( r.x, r.y, oldID );
            }
        else {
            if( ! getSlotItemsNoDecay( r.x, r.y, r.subCont ) ) {
                checkDecayContained( r.x, r.y, r.subCont );
                }
            }
        
        
        char stillExists;
        liveDecayRecordPresentHashTable.lookup( r.x, r.y, r.slot, r.subCont,
                                                &stillExists );
        
        if( !stillExists ) {
            // cell or slot no longer tracked
            // forget last look time
            liveDecayRecordLastLookTimeHashTable.remove( 
                r.x, r.y, r.slot, r.subCont );
            
            cleanMaxContainedHashTable( r.x, r.y );
            }
        }
    

    while( liveMovements.size() > 0 && 
           liveMovements.checkMinPriority() <= curTime ) {
        MovementRecord r = liveMovements.removeMin();    
        liveMovementEtaTimes.remove( r.x, r.y, 0, 0 );
        }
    
        
    // all of them, including these new ones and others acculuated since
    // last step are accumulated in these global vectors

    int numPos = mapChangePosSinceLastStep.size();

    for( int i=0; i<numPos; i++ ) {
        ChangePosition p = mapChangePosSinceLastStep.getElementDirect(i);
        
        inChangePosList->push_back( p );
        
        MapChangeRecord changeRecord = getMapChangeRecord( p );
        inMapChanges->push_back( changeRecord );
        }

    
    mapChangePosSinceLastStep.deleteAll();
    }




void restretchDecays( int inNumDecays, timeSec_t *inDecayEtas,
                      int inOldContainerID, int inNewContainerID ) {
    
    float oldStrech = getObject( inOldContainerID )->slotTimeStretch;
    float newStetch = getObject( inNewContainerID )->slotTimeStretch;
            
    if( oldStrech != newStetch ) {
        timeSec_t curTime = MAP_TIMESEC;

        for( int i=0; i<inNumDecays; i++ ) {
            if( inDecayEtas[i] != 0 ) {
                int offset = inDecayEtas[i] - curTime;
                        
                offset = offset * oldStrech;
                offset = offset / newStetch;
                inDecayEtas[i] = curTime + offset;
                }
            }    
        }
    }



void restretchMapContainedDecays( int inX, int inY,
                                  int inOldContainerID, 
                                  int inNewContainerID, int inSubCont ) {
    
    float oldStrech = getObject( inOldContainerID )->slotTimeStretch;
    float newStetch = getObject( inNewContainerID )->slotTimeStretch;
            
    if( oldStrech != newStetch ) {
                
        int oldNum;
        timeSec_t *oldContDecay =
            getContainedEtaDecay( inX, inY, &oldNum, inSubCont );
                
        restretchDecays( oldNum, oldContDecay, 
                         inOldContainerID, inNewContainerID );        
        
        setContainedEtaDecay( inX, inY, oldNum, oldContDecay, inSubCont );
        delete [] oldContDecay;
        }
    }




doublePair computeRecentCampAve( int *outNumPosFound ) {
    SimpleVector<doublePair> pos;
    SimpleVector<double> weight;
    
    doublePair sum = {0,0};
    
    double weightSum = 0;

    // the exponent that we raise depth to in order to squash
    // down higher values
    double depthFactor = 0.5;

    for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
        if( recentPlacements[i].pos.x != 0 ||
            recentPlacements[i].pos.y != 0 ) {
            
            doublePair p = { (double)( recentPlacements[i].pos.x ), 
                             (double)( recentPlacements[i].pos.y ) };
            
            pos.push_back( p );

            // natural objects can be moved around, and they have depth 0
            // this can result in a total weight sum of 0, causing NAN
            // push all depths up to 1 or greater
            int d = recentPlacements[i].depth + 1;

            double w = pow( d, depthFactor );
            
            weight.push_back( w );
            
            // weighted sum, with deeper objects weighing more
            sum = add( sum, mult( p, w ) );
            
            weightSum += w;
            }
        }
    

    *outNumPosFound = pos.size();
    
    if( pos.size() == 0 ) {
        doublePair zeroPos = { 0, 0 };    
        pos.push_back( zeroPos );
        weight.push_back( 1 );
        weightSum += 1;
        }
    
    
    doublePair ave = mult( sum, 1.0 / weightSum );
    
    double maxDist = 2.0 * campRadius;
    
    while( maxDist > campRadius ) {
        
        maxDist = 0;
        int maxI = -1;
        
        for( int i=0; i<pos.size(); i++ ) {
            
            double d = distance( pos.getElementDirect( i ), ave );
            
            if( d > maxDist ) {
                maxDist = d;
                maxI = i;
                }
            }
        
        if( maxDist > campRadius ) {
            
            double w = weight.getElementDirect( maxI );
            
            sum = sub( sum, mult( pos.getElementDirect( maxI ), w ) );
            
            pos.deleteElement( maxI );
            weight.deleteElement( maxI );
            
            weightSum -= w;
            
            ave = mult( sum, 1.0 / weightSum );
            }
        }
    
    printf( "Found an existing camp at (%f,%f) with %d placements "
            "and %f max radius\n",
            ave.x, ave.y, pos.size(), maxDist );

    
    // ave is now center of camp
    return ave;
    }




extern char doesEveLineExist( int inEveID );



void getEvePosition( const char *inEmail, int inID, int *outX, int *outY, 
                     SimpleVector<GridPos> *inOtherPeoplePos,
                     char inAllowRespawn,
                     char inIncrementPosition ) {

    int currentEveRadius = eveRadius;

    char forceEveToBorder = false;

    doublePair ave = { 0, 0 };

    printf( "Placing new Eve...\n" );
    
    
    int pX, pY, pR;
    
    int result = eveDBGet( inEmail, &pX, &pY, &pR );
    
    if( inAllowRespawn && result == 1 && pR > 0 ) {
        printf( "Placing new Eve:  "
                "Found camp center (%d,%d) r=%d in db for %s\n",
                pX, pY, pR, inEmail );
        
        ave.x = pX;
        ave.y = pY;
        currentEveRadius = pR;
        }
    else if( SettingsManager::getIntSetting( "useEveMovingGrid", 0 ) ) {
        printf( "Placing new Eve:  "
                "using Eve moving grid method\n" );
        
        int gridX = eveLocation.x;
        int gridY = eveLocation.y;
        

        getEveMovingGridPosition( & gridX, & gridY, inIncrementPosition );
        
        ave.x = gridX;
        ave.y = gridY;
        
        forceEveToBorder = true;
        currentEveRadius = 50;

        if( inIncrementPosition ) {
            // update advancing position
            eveLocation.x = gridX;
            eveLocation.y = gridY;
            }

        if( SettingsManager::getIntSetting( "eveToWestOfHomelands", 0 ) ) {
            // we've placed Eve based on walking grid
            // now move her farther west, to avoid plopping her down
            // in middle of active homelands
            
            FILE *tempLog = fopen( "evePlacementHomelandLog.txt", "a" );
            
            fprintf( tempLog, "Placing Eve for %s at time %.f:\n",
                     inEmail, Time::timeSec() );


            fprintf( tempLog, "    First homeland list:  " );


            int homelandXSum = 0;

            SimpleVector<Homeland*> consideredHomelands;

            for( int i=0; i<homelands.size(); i++ ) {
                Homeland *h = homelands.getElement( i );
                
                // only first homelands are considered
                // any d-town or tutorial homelands are ignored
                if( h->firstHomelandForFamily && 
                    ! h->expired && ! h->ignoredForEve ) {
                    homelandXSum += h->x;
                    consideredHomelands.push_back( h );
                    
                    fprintf( tempLog, "(%d,%d)  ", h->x, h->y );
                    }
                }            

            fprintf( tempLog, "\n" );


            int homelandXAve = 0;
            int maxAveDistance = 9999999;

            int outlierDist = 500;
            
            
            if( consideredHomelands.size() > 0 ) {
                homelandXAve = homelandXSum / consideredHomelands.size();
                }
            
            fprintf( tempLog, "    Found %d first homelands with average "
                     "x position %d\n",
                     consideredHomelands.size(), homelandXAve );

            
            // keep discarding the homeland that is max distance from the ave
            // until all we have left is homelands that are within 500 from ave
            // and keep adjusting ave as we go along
            // (Essentially, we discard farthest outlier repeatedly, until
            //  there are no far outliers left).
            while( consideredHomelands.size() > 0 && 
                   maxAveDistance > outlierDist ) {
                
                homelandXAve = homelandXSum / consideredHomelands.size();
                
                int maxDist = 0;
                int maxIndex = -1;
                for( int i=0; i<consideredHomelands.size(); i++ ) {
                    Homeland *h = consideredHomelands.getElementDirect( i );
                    
                    int dist = abs( h->x - homelandXAve );
                    if( dist > maxDist ) {
                        maxDist = dist;
                        maxIndex = i;
                        }
                    }
                if( maxDist > outlierDist ) {
                    Homeland *h = 
                        consideredHomelands.getElementDirect( maxIndex );
                    
                    homelandXSum -= h->x;
                    
                    fprintf( tempLog, "    Discarding outlier (%d,%d)\n",
                             h->x, h->y );

                    consideredHomelands.deleteElement( maxIndex );
                    }
                maxAveDistance = maxDist;
                }

            
            fprintf( tempLog, "    After discarding outliers, "
                     "have %d first homelands with average x position %d\n",
                     consideredHomelands.size(), homelandXAve );



            if( consideredHomelands.size() > 0 ) {
                for( int i=0; i<consideredHomelands.size(); i++ ) {
                    
                    Homeland *h = consideredHomelands.getElementDirect( i );
                    
                    int xBoundary = h->x - 2 * h->radius;
                        
                    if( xBoundary < ave.x ) {
                        ave.x = xBoundary;
                        
                        AppLog::infoF( 
                            "Pushing Eve to west of homeland %d at x=%d\n",
                            i, h->x );

                        fprintf( 
                            tempLog, 
                            "    Pushing Eve to west of homeland %d at x=%d\n",
                            i, h->x );
                        }
                    }
                }

            fclose( tempLog );
            }
        }
    else {
        // player has never been an Eve that survived to old age before
        // or such repawning forbidden by caller

        maxEveLocationUsage = 
            SettingsManager::getIntSetting( "maxEveStartupLocationUsage", 10 );


        printf( "Placing new Eve:  "
                "using Eve spiral method\n" );

        
        // first try new grid placement method

        
        // actually skip this for now and go back to normal Eve spiral
        if( false )
        if( eveLocationUsage >= maxEveLocationUsage
            && evePrimaryLocObjectID > 0 ) {
            
            GridPos centerP = lastEvePrimaryLocation;
            
            if( inOtherPeoplePos->size() > 0 ) {
                
                centerP = inOtherPeoplePos->getElementDirect( 
                    randSource.getRandomBoundedInt(
                        0, inOtherPeoplePos->size() - 1 ) );
                
                // round to nearest whole spacing multiple
                centerP.x /= evePrimaryLocSpacingX;
                centerP.y /= evePrimaryLocSpacingY;
                
                centerP.x *= evePrimaryLocSpacingX;
                centerP.y *= evePrimaryLocSpacingY;
                }
            

            GridPos tryP = centerP;
            char found = false;
            GridPos foundP = tryP;
            
            double curTime = Time::getCurrentTime();
            
            int r;
            
            int maxSearchRadius = 10;


            // first, clean any that have timed out
            // or gone extinct
            for( int p=0; p<recentlyUsedPrimaryEvePositions.size();
                 p++ ) {

                char reusePos = false;
                
                if( curTime -
                    recentlyUsedPrimaryEvePositionTimes.
                    getElementDirect( p )
                    > recentlyUsedPrimaryEvePositionTimeout ) {
                    // timed out
                    reusePos = true;
                    }
                else if( ! doesEveLineExist( 
                             recentlyUsedPrimaryEvePositionPlayerIDs.
                             getElementDirect( p ) ) ) {
                    // eve line extinct
                    reusePos = true;
                    }

                if( reusePos ) {
                    recentlyUsedPrimaryEvePositions.
                        deleteElement( p );
                    recentlyUsedPrimaryEvePositionTimes.
                        deleteElement( p );
                    recentlyUsedPrimaryEvePositionPlayerIDs.
                        deleteElement( p );
                    p--;
                    }
                }


            for( r=1; r<maxSearchRadius; r++ ) {
                
                for( int y=-r; y<=r; y++ ) {
                    for( int x=-r; x<=r; x++ ) {
                        tryP = centerP;
                        
                        tryP.x += x * evePrimaryLocSpacingX;
                        tryP.y += y * evePrimaryLocSpacingY;
                        
                        char existsAlready = false;

                        for( int p=0; p<recentlyUsedPrimaryEvePositions.size();
                             p++ ) {

                            GridPos pos =
                                recentlyUsedPrimaryEvePositions.
                                getElementDirect( p );
                            
                            if( equal( pos, tryP ) ) {
                                existsAlready = true;
                                break;
                                }
                            }
                        
                        if( existsAlready ) {
                            continue;
                            }
                        else {
                            }
                        
                                     
                        int mapID = getMapObject( tryP.x, tryP.y );

                        if( mapID == evePrimaryLocObjectID ) {
                            printf( "Found primary Eve object at %d,%d\n",
                                    tryP.x, tryP.y );
                            found = true;
                            foundP = tryP;
                            }
                        else if( eveSecondaryLocObjectIDs.getElementIndex( 
                                     mapID ) != -1 ) {
                            // a secondary ID, allowed
                            printf( "Found secondary Eve object at %d,%d\n",
                                    tryP.x, tryP.y );
                            found = true;
                            foundP = tryP;
                            }
                        }
                
                    if( found ) break;
                    }
                if( found ) break;
                }

            if( found ) {

                if( r >= maxSearchRadius / 2 ) {
                    // exhausted central window around last eve center
                    // save this as the new eve center
                    // next time, we'll search a window around that

                    AppLog::infoF( "Eve pos %d,%d not in center of "
                                   "grid window, recentering window for "
                                   "next time", foundP.x, foundP.y );

                    lastEvePrimaryLocation = foundP;
                    }

                AppLog::infoF( "Sticking Eve at unused primary grid pos "
                               "of %d,%d\n",
                               foundP.x, foundP.y );
                
                recentlyUsedPrimaryEvePositions.push_back( foundP );
                recentlyUsedPrimaryEvePositionTimes.push_back( curTime );
                recentlyUsedPrimaryEvePositionPlayerIDs.push_back( inID );
                
                // stick Eve directly to south
                *outX = foundP.x;
                *outY = foundP.y - 1;
                
                if( eveHomeMarkerObjectID > 0 ) {
                    // stick home marker there
                    setMapObject( *outX, *outY, eveHomeMarkerObjectID );
                    }
                else {
                    // make it empty
                    setMapObject( *outX, *outY, 0 );
                    }
                
                // clear a few more objects to the south, to make
                // sure Eve's spring doesn't spawn behind a tree
                setMapObject( *outX, *outY - 1, 0 );
                setMapObject( *outX, *outY - 2, 0 );
                setMapObject( *outX, *outY - 3, 0 );
                
                
                // finally, prevent Eve entrapment by sticking
                // her at a random location around the spring

                doublePair v = { 14, 0 };
                v = rotate( v, randSource.getRandomBoundedInt( 0, 2 * M_PI ) );
                
                *outX += v.x;
                *outY += v.y;
                
                return;
                }
            else {
                AppLog::info( "Unable to find location for Eve "
                              "on primary grid." );
                }
            }
        

        // Spiral method:
        GridPos eveLocToUse = eveLocation;
        
        int jumpUsed = 0;

        if( eveLocationUsage < maxEveLocationUsage ) {
            eveLocationUsage++;
            // keep using same location

            printf( "Reusing same eve start-up location "
                    "of %d,%d for %dth time\n",
                    eveLocation.x, eveLocation.y, eveLocationUsage );
            

            // remember it for when we exhaust it
            if( evePrimaryLocObjectID > 0 &&
                ( evePrimaryLocSpacingX > 0 || evePrimaryLocSpacingY > 0 ) ) {

                lastEvePrimaryLocation = eveLocation;
                // round to nearest whole spacing multiple
                lastEvePrimaryLocation.x /= evePrimaryLocSpacingX;
                lastEvePrimaryLocation.y /= evePrimaryLocSpacingY;
                
                lastEvePrimaryLocation.x *= evePrimaryLocSpacingX;
                lastEvePrimaryLocation.y *= evePrimaryLocSpacingY;
            
                printf( "Saving eve start-up location close grid pos "
                        "of %d,%d for later\n",
                        lastEvePrimaryLocation.x, lastEvePrimaryLocation.y );
                }

            }
        else {
            // post-startup eve location has been used too many times
            // place eves on spiral instead

            if( abs( eveLocToUse.x ) > 100000 ||
                abs( eveLocToUse.y ) > 100000 ) {
                // we've gotten to far away from center over time
                
                // re-center spiral on center to rein things in

                // we'll end up saving a position on the arm of this new
                // centered spiral for future start-ups, so the eve
                // location can move out from here
                eveLocToUse.x = 0;
                eveLocToUse.y = 0;

                eveStartSpiralPosSet = false;
                }



            if( eveStartSpiralPosSet &&
                longTermCullEnabled ) {
                
                int longTermCullingSeconds = 
                    SettingsManager::getIntSetting( 
                        "longTermNoLookCullSeconds", 3600 * 12 );
                
                // see how long center has not been seen
                // if it's old enough, we can reset Eve angle and restart
                // spiral there again
                // this will bring Eves closer together again, after
                // rim of spiral gets too far away
                
                timeSec_t lastLookTime = 
                    dbLookTimeGet( eveStartSpiralPos.x,
                                   eveStartSpiralPos.y );
                
                if( Time::getCurrentTime() - lastLookTime > 
                    longTermCullingSeconds * 2 ) {
                    // double cull start time
                    // that should be enough for the center to actually have
                    // started getting culled, and then some
                    
                    // restart the spiral
                    eveAngle = 2 * M_PI;
                    eveLocToUse = eveLocation;
                    
                    eveStartSpiralPosSet = false;
                    }
                }

            
            int jump = SettingsManager::getIntSetting( "nextEveJump", 2000 );
            jumpUsed = jump;
            
            // advance eve angle along spiral
            // approximate recursive form
            eveAngle = eveAngle + ( 2 * M_PI ) / eveAngle;
            
            // exact formula for radius along spiral from angle
            double radius = ( jump * eveAngle ) / ( 2 * M_PI );
            


            doublePair delta = { radius, 0 };
            delta = rotate( delta, eveAngle );
            
            // but don't update the post-startup location
            // keep jumping away from startup-location as center of spiral
            eveLocToUse.x += lrint( delta.x );
            eveLocToUse.y += lrint( delta.y );
            
            
            if( barrierOn &&
                // we use jumpUsed / 3 as randomizing radius below
                // so jumpUsed / 2 is safe here
                ( abs( eveLocToUse.x ) > barrierRadius - jumpUsed / 2 ||
                  abs( eveLocToUse.y ) > barrierRadius - jumpUsed / 2 ) ) {
                
                // Eve has gotten too close to the barrier
                
                // hard reset of location back to (0,0)-centered spiral
                eveAngle = 2 * M_PI;

                eveLocation.x = 0;
                eveLocation.y = 0;
                eveLocToUse = eveLocation;

                eveStartSpiralPosSet = false;
                }
            
                  


            // but do save it as a possible post-startup location for next time
            File eveLocFile( NULL, "lastEveLocation.txt" );
            char *locString = 
                autoSprintf( "%d,%d", eveLocToUse.x, eveLocToUse.y );
            eveLocFile.writeToFile( locString );
            delete [] locString;
            }

        ave.x = eveLocToUse.x;
        ave.y = eveLocToUse.y;

        
        

        // put Eve in radius 50 around this location
        forceEveToBorder = true;
        currentEveRadius = 50;

        if( jumpUsed > 3 && currentEveRadius > jumpUsed / 3 ) {
            currentEveRadius = jumpUsed / 3;
            }
        }
    




    // pick point in box according to eve radius

    
    char found = 0;

    if( currentEveRadius < 1 ) {
        currentEveRadius = 1;
        }
    
    while( !found ) {
        printf( "Placing new Eve:  "
                "trying radius of %d from camp\n", currentEveRadius );

        int tryCount = 0;
        
        while( !found && tryCount < 100 ) {
            
            doublePair p = { 
                randSource.getRandomBoundedDouble(-currentEveRadius,
                                                  +currentEveRadius ),
                randSource.getRandomBoundedDouble(-currentEveRadius,
                                                  +currentEveRadius ) };

            
            if( forceEveToBorder ) {
                // or pick ap point on the circle instead
                p.x = currentEveRadius;
                p.y = 0;
                
                double a = randSource.getRandomBoundedDouble( 0, 2 * M_PI );
                p = rotate( p, a );
                }
            

            p = add( p, ave );
            
            GridPos pInt = { (int)lrint( p.x ), (int)lrint( p.y ) };
            
            if( getMapObjectRaw( pInt.x, pInt.y ) == 0 ) {
                
                *outX = pInt.x;
                *outY = pInt.y;

                if( ! eveStartSpiralPosSet ) {
                    eveStartSpiralPos = pInt;
                    eveStartSpiralPosSet = true;
                    }

                found = true;
                }

            tryCount++;
            }

        // tried too many times, expand radius
        currentEveRadius *= 2;
        
        }


    // final sanity check:
    // make sure Eves spawn outside of barrier    
    if( barrierOn ) {
        
        if( abs( *outY ) >= barrierRadius ) {
            if( *outY > 0 ) {
                *outY = barrierRadius - 3;
                }
            else {
                *outY = - barrierRadius + 3;
                }
            }
        if( abs( *outX ) >= barrierRadius ) {
            if( *outX > 0 ) {
                *outX = barrierRadius - 3;
                }
            else {
                *outX = - barrierRadius + 3;
                }
            }
        }

    printf( "Placing new Eve:  "
            "Final location (%d,%d)\n", *outX, *outY );


    
    // clear recent placements after placing a new Eve
    // let her make new placements in her life which we will remember
    // later

    clearRecentPlacements();
    }




void mapEveDeath( const char *inEmail, double inAge, GridPos inDeathMapPos ) {
    
    // record exists?

    int pX, pY, pR;

    pR = eveRadius;
    
    printf( "Logging Eve death:   " );
    

    if( inAge < minEveCampRespawnAge ) {
        printf( "Eve died too young (age=%f, min=%f), "
                "not remembering her camp, and clearing any old camp memory\n",
                inAge, minEveCampRespawnAge );
        
        // 0 for radius means not set
        eveDBPut( inEmail, 0, 0, 0 );

        return;
        }
    

    
    int result = eveDBGet( inEmail, &pX, &pY, &pR );
    
    if( result == 1 && pR > 0 ) {
        
        // don't keep growing radius after it gets too big
        // if one player is dying young over and over, they will
        // eventually overflow 32-bit integers

        if( inAge < 16 && pR < 1024 ) {
            pR *= 2;
            }
        else if( inAge > 20 ) {
            pR = eveRadiusStart;
            }
        }
    else {
        // not found in DB
        
        // must overwrite no matter what
        pR = eveRadiusStart;
        }


    // their next camp will start where they last died
    pX = inDeathMapPos.x;
    pY = inDeathMapPos.y;
    

    printf( "Remembering Eve's camp in database (%d,%d) r=%d for %s\n",
            pX, pY, pR, inEmail );
    
    eveDBPut( inEmail, pX, pY, pR );
    }




static unsigned int nextLoadID = 0;


char loadTutorialStart( TutorialLoadProgress *inTutorialLoad, 
                        const char *inMapFileName, int inX, int inY ) {

    // don't open file yet, because we don't want to have the same
    // file open in parallel
    
    // save info to open file on first step, which is called one player at a 
    // time
    inTutorialLoad->uniqueLoadID = nextLoadID++;
    inTutorialLoad->fileOpened = false;
    inTutorialLoad->file = NULL;
    inTutorialLoad->mapFileName = stringDuplicate( inMapFileName );
    inTutorialLoad->x = inX;
    inTutorialLoad->y = inY;
    inTutorialLoad->startTime = Time::getCurrentTime();
    inTutorialLoad->stepCount = 0;

    return true;
    }




char loadTutorialStep( TutorialLoadProgress *inTutorialLoad,
                       double inTimeLimitSec ) {

    if( ! inTutorialLoad->fileOpened ) {
        // first step, open file
        
        char returnVal = false;
        
        // only try opening it once
        inTutorialLoad->fileOpened = true;
        
        File tutorialFolder( NULL, "tutorialMaps" );

        if( tutorialFolder.exists() && tutorialFolder.isDirectory() ) {
        
            File *mapFile = tutorialFolder.getChildFile( 
                inTutorialLoad->mapFileName );
            
            if( mapFile->exists() &&  ! mapFile->isDirectory() ) {
                char *fileName = mapFile->getFullFileName();
                
                FILE *file = fopen( fileName, "r" );
                
                if( file != NULL ) {
                    inTutorialLoad->file = file;
                    
                    returnVal = true;
                    }
                
                delete [] fileName;
                }
            delete mapFile;
            }
        
        delete [] inTutorialLoad->mapFileName;
        
        return returnVal;
        }
    

    // else file already open

    if( inTutorialLoad->file == NULL ) {
        // none left
        return false;
        }

    char moreLeft = loadIntoMapFromFile( inTutorialLoad->file, 
                                         inTutorialLoad->x, inTutorialLoad->y,
                                         inTimeLimitSec );

    inTutorialLoad->stepCount++;
    

    if( ! moreLeft ) {
        fclose( inTutorialLoad->file );
        inTutorialLoad->file = NULL;
        }
    return moreLeft;
    }








char getMetadata( int inMapID, unsigned char *inBuffer ) {
    int metaID = extractMetadataID( inMapID );
    
    if( metaID == 0 ) {
        return false;
        }

    // look up in metadata DB
    unsigned char key[4];    
    intToValue( metaID, key );
    int result = DB_get( &metaDB, key, inBuffer );

    if( result == 0 ) {
        return true;
        }

    return false;
    }

    


// returns full map ID with embedded metadata ID for new metadata record
int addMetadata( int inObjectID, unsigned char *inBuffer ) {
    int metaID = getNewMetadataID();
    
    int mapID = packMetadataID( inObjectID, metaID );
    
    // insert into metadata DB
    unsigned char key[4];    
    intToValue( metaID, key );
    DB_put( &metaDB, key, inBuffer );

    
    return mapID;
    }




void removeLandingPos( GridPos inPos ) {
    for( int i=0; i<flightLandingPos.size(); i++ ) {
        if( equal( inPos, flightLandingPos.getElementDirect( i ) ) ) {
            flightLandingPos.deleteElement( i );
            return;
            }
        }
    }


char isInDir( GridPos inPos, GridPos inOtherPos, doublePair inDir ) {
    double dX = (double)inOtherPos.x - (double)inPos.x;
    double dY = (double)inOtherPos.y - (double)inPos.y;
    
    if( inDir.x > 0 && dX > 0 ) {
        return true;
        }
    if( inDir.x < 0 && dX < 0 ) {
        return true;
        }
    if( inDir.y > 0 && dY > 0 ) {
        return true;
        }
    if( inDir.y < 0 && dY < 0 ) {
        return true;
        }
    return false;
    }



GridPos getNextCloseLandingPos( GridPos inCurPos, 
                                doublePair inDir, 
                                char *outFound ) {
    
    int closestIndex = -1;
    GridPos closestPos;
    double closestDist = DBL_MAX;

    double maxDist = SettingsManager::getDoubleSetting( "maxFlightDistance",
                                                        10000 );
    
    for( int i=0; i<flightLandingPos.size(); i++ ) {
        GridPos thisPos = flightLandingPos.getElementDirect( i );

        if( tooClose( inCurPos, thisPos, 250 ) ) {
            // don't consider landing at spots closer than 250,250 manhattan
            // to takeoff spot
            continue;
            }

        
        if( isInDir( inCurPos, thisPos, inDir ) ) {
            double dist = distance( inCurPos, thisPos );
            
            if( dist > maxDist ) {
                continue;
                }

            if( dist < closestDist ) {
                // check if this is still a valid landing pos
                int oID = getMapObject( thisPos.x, thisPos.y );
                
                if( oID <=0 ||
                    ! getObject( oID )->isFlightLanding ) {
                    
                    // not even a valid landing pos anymore
                    flightLandingPos.deleteElement( i );
                    i--;
                    continue;
                    }
                closestDist = dist;
                closestPos = thisPos;
                closestIndex = i;
                }
            }
        }
    
    if( closestIndex == -1 ) {
        *outFound = false;
        }
    else {
        *outFound = true;
        }
    
    return closestPos;
    }




GridPos getClosestLandingPos( GridPos inTargetPos, char *outFound ) {

    int closestIndex = -1;
    GridPos closestPos;
    double closestDist = DBL_MAX;

    double maxDist = SettingsManager::getDoubleSetting( "maxFlightDistance",
                                                        10000 );
    
    for( int i=0; i<flightLandingPos.size(); i++ ) {
        GridPos thisPos = flightLandingPos.getElementDirect( i );

        
        double dist = distance( inTargetPos, thisPos );
        
        if( dist > maxDist ) {
            continue;
            }

        if( dist < closestDist ) {
            // check if this is still a valid landing pos
            int oID = getMapObject( thisPos.x, thisPos.y );
            
            if( oID <=0 ||
                ! getObject( oID )->isFlightLanding ) {
                
                // not even a valid landing pos anymore
                flightLandingPos.deleteElement( i );
                i--;
                continue;
                }
            closestDist = dist;
            closestPos = thisPos;
            closestIndex = i;
            }
        }
    
    if( closestIndex == -1 ) {
        *outFound = false;
        }
    else {
        *outFound = true;
        }
    
    return closestPos;
    }

                



GridPos getNextFlightLandingPos( int inCurrentX, int inCurrentY, 
                                 doublePair inDir, 
                                 int inRadiusLimit ) {
    int closestIndex = -1;
    GridPos closestPos;
    double closestDist = DBL_MAX;

    double maxDist = SettingsManager::getDoubleSetting( "maxFlightDistance",
                                                        10000 );

    GridPos curPos = { inCurrentX, inCurrentY };

    char useLimit = false;
    
    if( abs( inCurrentX ) <= inRadiusLimit &&
        abs( inCurrentY ) <= inRadiusLimit ) {
        useLimit = true;
        }
        
        

    for( int i=0; i<flightLandingPos.size(); i++ ) {
        GridPos thisPos = flightLandingPos.getElementDirect( i );
        
        if( useLimit &&
            ( abs( thisPos.x ) > inRadiusLimit ||
              abs( thisPos.x ) > inRadiusLimit ) ) {
            // out of bounds destination
            continue;
            }
        
              
        double dist = distance( curPos, thisPos );

        if( dist > maxDist ) {
            continue;
            }
        
        if( dist < closestDist ) {
            
            // check if this is still a valid landing pos
            int oID = getMapObject( thisPos.x, thisPos.y );
            
            if( oID <=0 ||
                ! getObject( oID )->isFlightLanding ) {
                
                // not even a valid landing pos anymore
                flightLandingPos.deleteElement( i );
                i--;
                continue;
                }
            closestDist = dist;
            closestPos = thisPos;
            closestIndex = i;
            }
        }

    
    if( closestIndex != -1 && flightLandingPos.size() > 1 ) {
        // found closest, and there's more than one
        // look for next valid position in chosen direction

        
        char found = false;
        
        GridPos nextPos = getNextCloseLandingPos( curPos, inDir, &found );
        
        if( found ) {
            return nextPos;
            }

        // if we got here, we never found a nextPos that was valid
        // closestPos is only option
        return closestPos;
        }
    else if( closestIndex != -1 && flightLandingPos.size() == 1 ) {
        // land at closest, only option
        return closestPos;
        }
    
    // got here, no place to land

    // crash them at next Eve location
    
    int eveX, eveY;

    SimpleVector<GridPos> otherPeoplePos;
    
    getEvePosition( "dummyPlaneCrashEmail@test.com", 0, &eveX, &eveY, 
                    &otherPeoplePos, false, false );
    
    GridPos returnVal = { eveX, eveY };
    
    if( inRadiusLimit > 0 &&
        ( abs( eveX ) >= inRadiusLimit ||
          abs( eveY ) >= inRadiusLimit ) ) {
        // even Eve pos is out of bounds
        // stick them in center
        returnVal.x = 0;
        returnVal.y = 0;
        }
    
          


    return returnVal;
    }



int getGravePlayerID( int inX, int inY ) {
    unsigned char key[9];
    unsigned char value[4];

    // look for changes to default in database
    intPairToKey( inX, inY, key );
    
    int result = DB_get( &graveDB, key, value );
    
    if( result == 0 ) {
        // found
        int returnVal = valueToInt( value );
        
        return returnVal;
        }
    else {
        return 0;
        }
    }


void setGravePlayerID( int inX, int inY, int inPlayerID ) {
    unsigned char key[8];
    unsigned char value[4];
    

    intPairToKey( inX, inY, key );
    intToValue( inPlayerID, value );
            
    
    DB_put( &graveDB, key, value );
    }






static char tileCullingIteratorSet = false;
static DB_Iterator tileCullingIterator;

static char floorCullingIteratorSet = false;
static DB_Iterator floorCullingIterator;

static double lastSettingsLoadTime = 0;
static double settingsLoadInterval = 5 * 60;

static int numTilesExaminedPerCullStep = 10;
static int longTermCullingSeconds = 3600 * 12;

static int minActivePlayersForLongTermCulling = 15;



static SimpleVector<int> noCullItemList;


static int numTilesSeenByIterator = 0;
static int numFloorsSeenByIterator = 0;

void stepMapLongTermCulling( int inNumCurrentPlayers ) {

    double curTime = Time::getCurrentTime();
    
    if( curTime - lastSettingsLoadTime > settingsLoadInterval ) {
        
        lastSettingsLoadTime = curTime;
        
        numTilesExaminedPerCullStep = 
            SettingsManager::getIntSetting( 
                "numTilesExaminedPerCullStep", 10 );
        longTermCullingSeconds = 
            SettingsManager::getIntSetting( 
                "longTermNoLookCullSeconds", 3600 * 12 );
        minActivePlayersForLongTermCulling = 
            SettingsManager::getIntSetting( 
                "minActivePlayersForLongTermCulling", 15 );
        
        longTermCullEnabled = 
            SettingsManager::getIntSetting( 
                "longTermNoLookCullEnabled", 1 );
        

        SimpleVector<int> *list = 
            SettingsManager::getIntSettingMulti( "noCullItemList" );
        
        noCullItemList.deleteAll();
        noCullItemList.push_back_other( list );
        delete list;

        barrierRadius = SettingsManager::getIntSetting( "barrierRadius", 250 );
        barrierOn = SettingsManager::getIntSetting( "barrierOn", 1 );
        }


    if( ! longTermCullEnabled ||
        minActivePlayersForLongTermCulling > inNumCurrentPlayers ) {
        return;
        }

    
    if( !tileCullingIteratorSet ) {
        DB_Iterator_init( &db, &tileCullingIterator );
        tileCullingIteratorSet = true;
        numTilesSeenByIterator = 0;
        }

    unsigned char tileKey[16];
    unsigned char floorKey[8];
    unsigned char value[4];


    for( int i=0; i<numTilesExaminedPerCullStep; i++ ) {        
        int result = 
            DB_Iterator_next( &tileCullingIterator, tileKey, value );

        if( result <= 0 ) {
            // restart the iterator back at the beginning
            DB_Iterator_init( &db, &tileCullingIterator );
            if( numTilesSeenByIterator != 0 ) {
                AppLog::infoF( "Map cull iterated through %d tile db entries.",
                               numTilesSeenByIterator );
                }
            numTilesSeenByIterator = 0;
            // end loop when we reach end of list, so we don't cycle through
            // a short iterator list too quickly.
            break;
            }
        else {
            numTilesSeenByIterator ++;
            }

        
        int tileID = valueToInt( value );
        
        // consider 0-values too, where map has been cleared by players, but
        // a natural object should be there
        if( tileID >= 0 ) {
            // next value

            int s = valueToInt( &( tileKey[8] ) );
            int b = valueToInt( &( tileKey[12] ) );
       
            if( s == 0 && b == 0 ) {
                // main object
                int x = valueToInt( tileKey );
                int y = valueToInt( &( tileKey[4] ) );
                
                int wildTile = getTweakedBaseMap( x, y );
                
                if( wildTile != tileID ) {
                    // tile differs from natural tile
                    // don't keep checking/resetting tiles that are already
                    // in wild state
                    
                    // NOTE that we don't check/clear container slots for 
                    // already-wild tiles.  So a natural container 
                    // (if one is ever
                    // added to the game, like a hidey-hole cave) will
                    // keep its items even after that part of the map
                    // is culled.  Seems like okay behavior.

                    timeSec_t lastLookTime = dbLookTimeGet( x, y );

                    if( curTime - lastLookTime > longTermCullingSeconds ) {
                        // stale
                    
                        if( noCullItemList.getElementIndex( tileID ) == -1 ) {
                            // not on our no-cull list
                            clearAllContained( x, y );
                            
                            // put proc-genned map value in there
                            setMapObject( x, y, wildTile );

                            if( wildTile != 0 &&
                                getObject( wildTile )->permanent ) {
                                // something nautural occurs here
                                // this "breaks" any remaining floor
                                // (which may be cull-proof on its own below).
                                // this will effectively leave gaps in roads
                                // with trees growing through, etc.
                                setMapFloor( x, y, 0 );
                                }                            
                            }
                        }
                    }
                }
            }
        }
    
    

    if( !floorCullingIteratorSet ) {
        DB_Iterator_init( &floorDB, &floorCullingIterator );
        floorCullingIteratorSet = true;
        numFloorsSeenByIterator = 0;
        }
    

    for( int i=0; i<numTilesExaminedPerCullStep; i++ ) {        
        int result = 
            DB_Iterator_next( &floorCullingIterator, floorKey, value );

        if( result <= 0 ) {
            // restart the iterator back at the beginning
            DB_Iterator_init( &floorDB, &floorCullingIterator );
            if( numFloorsSeenByIterator != 0 ) {
                AppLog::infoF( "Map cull iterated through %d floor db entries.",
                               numFloorsSeenByIterator );
                }
            numFloorsSeenByIterator = 0;
            // end loop now, avoid re-cycling through a short list
            // in same step
            break;
            }
        else {
            numFloorsSeenByIterator ++;
            }
        
        
        int floorID = valueToInt( value );
        
        if( floorID > 0 ) {
            // next value
            
            int x = valueToInt( floorKey );
            int y = valueToInt( &( floorKey[4] ) );
                
            timeSec_t lastLookTime = dbLookTimeGet( x, y );

            if( curTime - lastLookTime > longTermCullingSeconds ) {
                // stale

                if( noCullItemList.getElementIndex( floorID ) == -1 ) {
                    // not on our no-cull list
                    
                    setMapFloor( x, y, 0 );
                    }
                }
            }
        }
    }




int isHomeland( int inX, int inY, int inLineageEveID ) {
    Homeland *h = getHomeland( inX, inY );
    
    if( h != NULL ) {
        if( h->lineageEveID == inLineageEveID ) {
            return 1;
            }
        else {
            return -1;
            }
        }

    // else check if they even have one
    
    for( int i=0; i<homelands.size(); i++ ) {
        h = homelands.getElement( i );
        
        if( h->lineageEveID == inLineageEveID ) {
            // they are outside of their homeland
            return -1;
            }
        }
    
    // never found one, and they aren't inside someone else's
    // report that they don't have one
    return 0;
    }




#include "specialBiomes.h"


int isBirthland( int inX, int inY, int inLineageEveID, int inDisplayID ) {
    if( specialBiomeBandMode && 
        getNumPlayers() >= minActivePlayersForBirthlands ) {
        
        char outOfBand = false;
        
        int pickedBiome = getSpecialBiomeIndexForYBand( inY, &outOfBand );
        
        if( pickedBiome == -1 || outOfBand ) {
            return -1;
            }
        
        int biomeNumber = biomes[ pickedBiome ];
        
        int personRace = getObject( inDisplayID )->race;

        int specialistRace = getSpecialistRace( biomeNumber );
        
        if( specialistRace != -1 ) {
            if( personRace == specialistRace ) {
                return 1;
                }
            else {
                return -1;
                }
            }
        else {
            // in-band, but no specialist race defined
            // "language expert" band?
            if( personRace == getPolylingualRace( true ) ) {
                return 1;
                }
            else {
                return -1;
                }
            }

        }
    else {
        return isHomeland( inX, inY, inLineageEveID );
        }
    }




int getSpecialBiomeBandYCenterForRace( int inRace ) {
    int bandIndex = -1;
    
    for( int i=0; i<specialBiomeBandOrder.size(); i++ ) {
        
        int biomeNumber = specialBiomeBandOrder.getElementDirect( i );
        
        if( getSpecialistRace( biomeNumber ) == inRace ) {
            // hit
            bandIndex = i;
            break;
            }
        }
    
    if( bandIndex == -1 ) {
        // no hit...
        // treat as polylingual
        for( int i=0; i<specialBiomeBandOrder.size(); i++ ) {
        
            int biomeNumber = specialBiomeBandOrder.getElementDirect( i );
            
            // find non-specialist specialBiomeBand for polylingual race
            if( getSpecialistRace( biomeNumber ) == -1 ) {
                bandIndex = i;
                break;
                }
            }
        }
    

    if( bandIndex == -1 ) {
        AppLog::errorF( "Could not find biome band for race %d", inRace );
        return 0;
        }
    
    return specialBiomeBandYCenter.getElementDirect( bandIndex );
    }




void logHomelandBirth( int inX, int inY, int inLineageEveID ) {
    Homeland *h = getHomeland( inX, inY );
    
    if( h != NULL ) {
        if( h->lineageEveID == inLineageEveID ) {
            h->lastBabyBirthTime = Time::getCurrentTime();
            }
        }
    }



void homelandsDead( int inLineageEveID ) {
    for( int i=0; i<homelands.size(); i++ ) {
        Homeland *h = homelands.getElement( i );
        
        if( ! h->expired && h->lineageEveID == inLineageEveID ) {
            expireHomeland( h );
            }
        }
    }



char getHomelandCenter( int inX, int inY, 
                        GridPos *outCenter, int *outLineageEveID ) {
    
    // include expired
    Homeland *h = getHomeland( inX, inY, true );

    if( h != NULL ) {
        outCenter->x = h->x;
        outCenter->y = h->y;
        
        if( h->expired ) {
            *outLineageEveID = -1;
            }
        else {
            *outLineageEveID = h->lineageEveID;
            }
        
        return true;
        }
    else {
        return false;
        }
    }



SimpleVector<HomelandInfo> getHomelandChanges() {
    SimpleVector<HomelandInfo> list;
    
    for( int i=0; i<homelands.size(); i++ ) {
        Homeland *h = homelands.getElement( i );
        
        if( h->changed ) {
            GridPos p = { h->x, h->y };
            
            HomelandInfo hi = { p, h->radius, h->lineageEveID };
            
            if( h->expired ) {
                hi.lineageEveID = -1;
                }

            list.push_back( hi );
            
            h->changed = false;

            if( h->expired ) {
                // now that we've reported that it expired, remove it
                homelands.deleteElement( i );
                i--;
                }
            }
        }
    return list;
    }




int getDeadlyMovingMapObject( int inPosX, int inPosY,
                              int *outMovingDestX, int *outMovingDestY ) {
    
    double curTime = Time::getCurrentTime();
    
    int numMoving = liveMovements.size();
    
    for( int i=0; i<numMoving; i++ ) {
        MovementRecord *m = liveMovements.getElement( i );
        
        if( ! m->deadly ) {
            continue;
            }
        double progress = 
            ( m->totalTime - ( m->etaTime - curTime )  )
            / m->totalTime;
        
        if( progress < 0 ||
            progress > 1 ) {
            continue;
            }
        int curPosX = lrint( ( m->x - m->sourceX ) * progress + m->sourceX );
        int curPosY = lrint( ( m->y - m->sourceY ) * progress + m->sourceY );
        
        if( curPosX != inPosX ||
            curPosY != inPosY ) {
            continue;
            }
        
        // hit position
        *outMovingDestX = m->x;
        *outMovingDestY = m->y;
        return m->id;
        }
    

    return 0;
    }
