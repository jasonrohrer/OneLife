#include "map.h"
#include "HashTable.h"
#include "monument.h"

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
#include "stackdb.h"


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
*/

/**/
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
/**/





#include "dbCommon.h"


#include <stdarg.h>
#include <math.h>
#include <values.h>
#include <stdint.h>


#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/GridPos.h"

#include "../gameSource/GridPos.h"



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


// can replace with frozenTime to freeze time
// or slowTime to slow it down
#define MAP_TIMESEC Time::timeSec()
//#define MAP_TIMESEC frozenTime()
//#define MAP_TIMESEC fastTime()


extern GridPos getClosestPlayerPos( int inX, int inY );



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
                            


// what human-placed stuff, together, counts as a camp
static int campRadius = 20;

static float minEveCampRespawnAge = 60.0;


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

// one vector per biome
static SimpleVector<int> *naturalMapIDs;
static SimpleVector<float> *naturalMapChances;

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


// per-player memory of where they should spawn as eve
static DB eveDB;
static char eveDBOpen = false;





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

// 10 seconds
static int maxSecondsNoLookDecayTracking = 10;


typedef struct LiveDecayRecord {
        int x, y;
        
        // 0 means main object decay
        // 1 - NUM_CONT_SLOT means contained object decay
        int slot;
        
        timeSec_t etaTimeSeconds;

        // 0 means main object
        // >0 indexs sub containers of object
        int subCont;

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



// track currently in-process movements so that we can be queried
// about whether arrival has happened or not
typedef struct MovementRecord {
        int x, y;
        double etaTime;
    } MovementRecord;


// clock time in fractional seconds of destination ETA
// indexed as x, y, 0
static HashTable<double> liveMovementEtaTimes( 1024, 0 );

static MinPriorityQueue<MovementRecord> liveMovements;



    

// track all map changes that happened since the last
// call to stepMap
static SimpleVector<ChangePosition> mapChangePosSinceLastStep;


static char anyBiomesInDB = false;




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






#include "../commonSource/fractalNoise.h"





// four ints to a 16-byte key
void intQuadToKey( int inX, int inY, int inSlot, int inB, 
                   unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        outKey[i+8] = ( inSlot >> offset ) & 0xFF;
        outKey[i+12] = ( inB >> offset ) & 0xFF;
        }    
    }


// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        }    
    }







// one timeSec_t to an 8-byte double value
void timeToValue( timeSec_t inT, unsigned char *outValue ) {
    

    // pack double time into 8 bytes in whatever endian order the
    // double is stored on this platform

    union{ timeSec_t doubleTime; uint64_t intTime; };

    doubleTime = inT;
    
    for( int i=0; i<8; i++ ) {
        outValue[i] = ( intTime >> (i * 8) ) & 0xFF;
        }    
    }


timeSec_t valueToTime( unsigned char *inValue ) {

    union{ timeSec_t doubleTime; uint64_t intTime; };

    // get bytes back out in same order they were put in
    intTime = 
        (uint64_t)inValue[7] << 56 | (uint64_t)inValue[6] << 48 | 
        (uint64_t)inValue[5] << 40 | (uint64_t)inValue[4] << 32 | 
        (uint64_t)inValue[3] << 24 | (uint64_t)inValue[2] << 16 | 
        (uint64_t)inValue[1] << 8  | (uint64_t)inValue[0];
    
    // caste back to timeSec_t
    return doubleTime;
    }




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
    DB_put( &biomeDB, key, value );

    dbLookTimePut( inX, inY, MAP_TIMESEC );
    }
    



// returns -1 on failure, 1 on success
static int eveDBGet( char *inEmail, int *outX, int *outY, int *outRadius ) {
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



static void eveDBPut( char *inEmail, int inX, int inY, int inRadius ) {
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

static int computeBiomeCacheHash( int inKeyA, int inKeyB ) {
    
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
        biomeCache[ computeBiomeCacheHash( inX, inY ) ];

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
    
    biomeCache[ computeBiomeCacheHash( inX, inY ) ] = r;
    }









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


    double maxValue = -DBL_MAX;

    
    for( int i=0; i<numBiomes; i++ ) {
        int biome = biomes[i];
        
        setXYRandomSeed( biome * 263 + 723 );

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
    
    if( anyBiomesInDB ) {
        // don't bother with this call unless biome DB has
        // something in it
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

// 1671 shy of int max
static int xLimit = 2147481977;
static int yLimit = 2147481977;





typedef struct BaseMapCacheRecord {
        int x, y;
        int id;
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
static int mapCacheLookup( int inX, int inY ) {
    BaseMapCacheRecord *r = mapCacheRecordLookup( inX, inY );
    
    if( r->x == inX && r->y == inY ) {
        return r->id;
        }

    return -1;
    }



static void mapCacheInsert( int inX, int inY, int inID ) {
    BaseMapCacheRecord *r = mapCacheRecordLookup( inX, inY );
    
    r->x = inX;
    r->y = inY;
    r->id = inID;
    }

    

static int getBaseMapCallCount = 0;


static int getBaseMap( int inX, int inY ) {
    
    if( inX > xLimit || inX < -xLimit ||
        inY > yLimit || inY < -yLimit ) {
    
        return edgeObjectID;
        }
    
    int cachedID = mapCacheLookup( inX, inY );
    
    if( cachedID != -1 ) {
        return cachedID;
        }
    
    getBaseMapCallCount ++;

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
        
        lastCheckedBiome = biomes[pickedBiome];
        

        
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

    int w =  500;
    int h = 500;
    
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

    for( int j=0; j<numBiomes; j++ ) {        
        Color *c = Color::makeColorFromHSV( (float)j / numBiomes, 1, 1 );
        
        biomeColors.push_back( *c );
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
            
            
            int id = getBaseMap( x, y );
            
            int biomeInd = getMapBiomeIndex( x, y );

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
            }
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






int getMapObjectRaw( int inX, int inY );
int *getContainedRaw( int inX, int inY, int *outNumContained, 
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





// optimization:
// cache dbGet results in RAM

// 2.6 MB of RAM for this.
#define DB_CACHE_SIZE 131072

typedef struct DBCacheRecord {
        int x, y, slot, subCont;
        int value;
    } DBCacheRecord;
    
static DBCacheRecord dbCache[ DB_CACHE_SIZE ];


#define CACHE_PRIME_A 776509273
#define CACHE_PRIME_B 904124281
#define CACHE_PRIME_C 528383237
#define CACHE_PRIME_D 148497157

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



static void initDBCache() {
    DBCacheRecord blankRecord = { 0, 0, 0, 0, -2 };
    for( int i=0; i<DB_CACHE_SIZE; i++ ) {
        dbCache[i] = blankRecord;
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





char lookTimeDBEmpty = false;

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
    
    if( ! dbFile.exists() || lookTimeDBEmpty ) {

        if( lookTimeDBEmpty ) {
            AppLog::infoF( "No lookTimes present, not cleaning %s", path );
            }
        
        int error = DB_open( db, 
                                 path, 
                                 mode,
                                 hash_table_size,
                                 key_size,
                                 value_size );

        if( ! error ) {
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

    DB tempDB;
    
    error = DB_open( &tempDB, 
                         dbTempName, 
                         mode,
                         hash_table_size,
                         key_size,
                         value_size );
    if( error ) {
        AppLog::errorF( "Failed to open DB file %s in DB_open_timeShrunk",
                        dbTempName );
        delete [] dbTempName;
        DB_close( &oldDB );
        return error;
        }


    

    
    DB_Iterator dbi;
    
    
    DB_Iterator_init( &oldDB, &dbi );
    
    // key and value size that are big enough to handle all of our DB
    unsigned char key[16];
    
    unsigned char value[12];
    
    int total = 0;
    int stale = 0;

    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        total++;

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
            stale++;
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






// returns num set after
int cleanMap() {
    AppLog::info( "\nCleaning map of objects that have been removed and "
                  "variable objects..." );

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
    
    
    int totalSetCount = 0;
    int numClearedCount = 0;
    int totalNumContained = 0;
    int numContainedCleared = 0;
    
    while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        
        int s = valueToInt( &( key[8] ) );
        int b = valueToInt( &( key[12] ) );
       
        if( s == 0 ) {
            int id = valueToInt( value );
            
            if( id > 0 ) {
                totalSetCount++;
                
                ObjectRecord *o = getObject( id );
                
                if( o == NULL || o->isVariableDummy ) {
                    // id doesn't exist anymore or is variable dummy
                    
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
            
            for( int c=0; c<numCont; c++ ) {
                
                SimpleVector<int> subCont;
                SimpleVector<timeSec_t> subContDecay;
                

                char thisKept = false;
                
                if( cont[c] < 0 ) {
                    
                    ObjectRecord *o = getObject( - cont[c] );
                    
                    if( o != NULL && ! o->isVariableDummy ) {
                        
                        thisKept = true;
                        
                        newCont.push_back( cont[c] );
                        newDecay.push_back( decay[c] );
                        
                        int numSub;
                        
                        int *contSub = 
                            getContainedRaw( x, y, &numSub, c + 1 );
                        timeSec_t *decaySub = 
                            getContainedEtaDecay( x, y, &numSub, c + 1 );

                        for( int s=0; s<numSub; s++ ) {
                            
                            if( getObject( contSub[s] ) != NULL ) {
                                subCont.push_back( contSub[s] );
                                subContDecay.push_back( decaySub[s] );
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
                    if( o != NULL && ! o->isVariableDummy ) {
                        
                        thisKept = true;
                        newCont.push_back( cont[c] );
                        newDecay.push_back( decay[c] );
                        }
                    }

                if( thisKept ) {        
                    newSubCont.push_back( subCont );
                    newSubContDecay.push_back( subContDecay );
                    }
                }
            


            delete [] cont;
            delete [] decay;
            
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
                        newSubContDecay.getElementDirect( c ).getElementArray();
                    
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
    

    AppLog::infoF( "...%d map cells were set, and %d needed to be cleared.",
                   totalSetCount, numClearedCount );
    AppLog::infoF( 
        "...%d contained objects present, and %d needed to be cleared.",
        totalNumContained, numContainedCleared );

    printf( "\n" );
    return totalSetCount;
    }




void initMap() {
    initDBCache();
    initBiomeCache();

    mapCacheClear();
    
    edgeObjectID = SettingsManager::getIntSetting( "edgeObject", 0 );
    
    minEveCampRespawnAge = 
        SettingsManager::getFloatSetting( "minEveCampRespawnAge", 60.0f );
    

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

            int secInDay = 3600 * 24 * 1000;
            
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
    
    lookTimeDBExists = lookTimeDBFile.exists();

    if( ! lookTimeDBExists ) {
        lookTimeDBEmpty = true;
        }

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
        return;
        }
    
    lookTimeDBOpen = true;

    

    int staleSec = SettingsManager::getIntSetting( "mapCellForgottenSeconds", 
                                                   0 );
    
    if( lookTimeDBExists && staleSec > 0 ) {
        AppLog::info( "\nCleaning stale look times from map..." );


        static DB lookTimeDB_temp;
        
        const char *lookTimeDBName_temp = "lookTime_temp.db";

        File tempDBFile( NULL, lookTimeDBName_temp );
        
        if( tempDBFile.exists() ) {
            tempDBFile.remove();
            }
        

        error = DB_open( &lookTimeDB_temp, 
                             lookTimeDBName_temp, 
                             KISSDB_OPEN_MODE_RWCREAT,
                             80000,
                             8, // two 32-bit ints, xy
                             8 // one 64-bit double, representing an ETA time
                             // in whatever binary format and byte order
                             // "double" on the server platform uses
                             );
    
        if( error ) {
            AppLog::errorF( "Error %d opening look time temp KissDB", error );
            return;
            }
        
        DB_Iterator dbi;
        
        
        DB_Iterator_init( &lookTimeDB_old, &dbi );
        
    
        timeSec_t curTime = MAP_TIMESEC;
        
        unsigned char key[8];
        unsigned char value[8];

        int total = 0;
        int stale = 0;

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
    


    error = DB_open( &lookTimeDB, 
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
        return;
        }

    


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
        return;
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
        return;
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
        return;
        }
    
    biomeDBOpen = true;



    // see if any biomes are listed in DB
    // if not, we don't even need to check it when generating map
    DB_Iterator biomeDBi;
    DB_Iterator_init( &biomeDB, &biomeDBi );
    
    unsigned char *biomeKey[8];
    unsigned char *biomeValue[12];
    
    if( DB_Iterator_next( &biomeDBi, biomeKey, biomeValue ) ) {
        // only check for the first one
        anyBiomesInDB = true;
        }
    


    error = DB_open_timeShrunk( &floorDB, 
                         "floor.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         8, // two 32-bit ints, xy
                         4 // one int, the floor object ID at x,y 
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening floor KissDB", error );
        return;
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
        return;
        }
    
    floorTimeDBOpen = true;






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
        return;
        }
    
    eveDBOpen = true;


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

    
    numBiomes = biomeList.size();
    biomes = biomeList.getElementArray();
    
    naturalMapIDs = new SimpleVector<int>[ numBiomes ];
    naturalMapChances = new SimpleVector<float>[ numBiomes ];
    totalChanceWeight = new float[ numBiomes ];

    for( int j=0; j<numBiomes; j++ ) {
        totalChanceWeight[j] = 0;
        }
    

    
    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = allObjects[i];

        float p = o->mapChance;
        if( p > 0 ) {
            int id = o->id;
            
            allNaturalMapIDs.push_back( id );
            
            for( int j=0; j< o->numBiomes; j++ ) {
                int b = o->biomes[j];

                int bIndex = getBiomeIndex( b );
                
            
                naturalMapIDs[bIndex].push_back( id );
                naturalMapChances[bIndex].push_back( p );
            
                totalChanceWeight[bIndex] += p;
                }
            }
        }


    for( int j=0; j<numBiomes; j++ ) {    
        AppLog::infoF( 
            "Biome %d:  Found %d natural objects with total weight %f",
            biomes[j], naturalMapIDs[j].size(), totalChanceWeight[j] );
        }
    
    delete [] allObjects;


    int totalSetCount = cleanMap();
    
    
    if( totalSetCount == 0 ) {
        // map has been cleared

        // ignore old value for placements
        clearRecentPlacements();
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
            
            // break out when read fails
            while( true ) {
                TestMapRecord r;
                
                char stringBuff[1000];
                
                int numRead = fscanf( testMapFile, "%d %d %d %d %999s", 
                                      &(r.x), &(r.y), &(r.biome),
                                      &(r.floor),
                                      stringBuff );
                
                if( numRead != 5 ) {
                    break;
                    }
                
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



    
    
    

    
    // for debugging the map
    // printBiomeSamples();
    //outputMapImage();

    //outputBiomeFractals();
    }







void freeAndNullString( char **inStringPointer ) {
    if( *inStringPointer != NULL ) {
        delete [] *inStringPointer;
        *inStringPointer = NULL;
        }
    }



void freeMap() {
    printf( "%d calls to getBaseMap\n", getBaseMapCallCount );

    
    if( lookTimeDBOpen ) {
        DB_close( &lookTimeDB );
        lookTimeDBOpen = false;
        }


    if( dbOpen ) {
        
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
        
        
        while( DB_Iterator_next( &dbi, key, value ) > 0 ) {
        
            int s = valueToInt( &( key[8] ) );
            int b = valueToInt( &( key[12] ) );
       
            if( s == 0 ) {
                int id = valueToInt( value );
            
                if( id > 0 ) {
                    
                    ObjectRecord *mapO = getObject( id );
                    
                    
                    if( mapO != NULL && mapO->isUseDummy ) {
                        int x = valueToInt( key );
                        int y = valueToInt( &( key[4] ) );
                    
                        xToPlace.push_back( x );
                        yToPlace.push_back( y );
                        idToPlace.push_back( mapO->useDummyParent );
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
            
            setMapObject( x, y, idToPlace.getElementDirect( i ) );
            }

        
        int numContChanged = 0;
        
        for( int i=0; i<xContToCheck.size(); i++ ) {
            int x = xContToCheck.getElementDirect( i );
            int y = yContToCheck.getElementDirect( i );
            int b = bContToCheck.getElementDirect( i );
        
            if( getMapObjectRaw( x, y ) != 0 ) {

                int numCont;
                int *cont = getContainedRaw( x, y, &numCont, b );

                for( int c=0; c<numCont; c++ ) {

                    char subCont = false;
                    
                    if( cont[c] < 0 ) {
                        cont[c] *= -1;
                        subCont = true;
                        }
                    
                    ObjectRecord *contObj = getObject( cont[c] );
                    
                    if( contObj != NULL && contObj->isUseDummy ) {
                        cont[c] = contObj->useDummyParent;
                        numContChanged ++;
                        }
                   
                    if( subCont ) {
                        cont[c] *= -1;
                        }
                    }
                
                setContained( x, y, numCont, cont, b );
                delete [] cont;
                }
            }


        AppLog::infoF( "...%d useDummy objects present that were changed "
                       "back into their unused parent.",
                       xToPlace.size() );
        AppLog::infoF( 
            "...%d contained useDummy objects present and changed "
            "back to usused parent.",
            numContChanged );

        printf( "\n" );

        AppLog::info( "Now running normal map clean..." );
        cleanMap();

        
        DB_close( &db );
        }

    if( timeDBOpen ) {
        DB_close( &timeDB );
        }

    if( biomeDBOpen ) {
        DB_close( &biomeDB );
        }


    if( floorDBOpen ) {
        DB_close( &floorDB );
        }

    if( floorTimeDBOpen ) {
        DB_close( &floorTimeDB );
        }


    if( eveDBOpen ) {
        DB_close( &eveDB );
        }
    
    writeEveRadius();
    writeRecentPlacements();

    delete [] biomes;
    
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
    }




static void deleteFileByName( const char *inFileName ) {
    File f( NULL, inFileName );
    
    if( f.exists() ) {
        f.remove();
        }
    }



void wipeMapFiles() {
    deleteFileByName( "biome.db" );
    deleteFileByName( "eve.db" );
    deleteFileByName( "floor.db" );
    deleteFileByName( "floorTime.db" );
    deleteFileByName( "lookTime.db" );
    deleteFileByName( "map.db" );
    deleteFileByName( "mapTime.db" );
    deleteFileByName( "playerStats.db" );
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
    unsigned char key[16];
    unsigned char value[8];

    // look for changes to default in database
    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    
    int result = DB_get( &timeDB, key, value );
    
    if( result == 0 ) {
        // found
        return valueToTime( value );
        }
    else {
        return 0;
        }
    }



static int dbFloorGet( int inX, int inY ) {
    unsigned char key[9];
    unsigned char value[4];

    // look for changes to default in database
    intPairToKey( inX, inY, key );
    
    int result = DB_get( &floorDB, key, value );
    
    if( result == 0 ) {
        // found
        return valueToInt( value );
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

    intPairToKey( inX, inY, key );
    
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
                   int inSubCont = 0 ) {
    
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
    
    

    if( apocalypsePossible && inValue > 0 && inSlot == 0 && inSubCont == 0 ) {
        // a primary tile put
        // check if this triggers the apocalypse
        if( isApocalypseTrigger( inValue ) ) {
            apocalypseTriggered = true;
            apocalypseLocation.x = inX;
            apocalypseLocation.y = inY;
            }
        }
    else if( inValue > 0 && inSlot == 0 && inSubCont == 0  ) {
        
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
    dbLookTimePut( inX, inY, MAP_TIMESEC );
    }





static void dbTimePut( int inX, int inY, int inSlot, timeSec_t inTime,
                       int inSubCont = 0 ) {
    // ETA decay changes don't get reported as map changes    
    
    unsigned char key[16];
    unsigned char value[8];
    

    intQuadToKey( inX, inY, inSlot, inSubCont, key );
    timeToValue( inTime, value );
            
    
    DB_put( &timeDB, key, value );
    }




static void dbFloorPut( int inX, int inY, int inValue ) {
    
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
    
    
    unsigned char key[8];
    unsigned char value[4];
    

    intPairToKey( inX, inY, key );
    intToValue( inValue, value );
            
    
    DB_put( &floorDB, key, value );
    dbLookTimePut( inX, inY, MAP_TIMESEC );
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
    

    intPairToKey( inX, inY, key );
    timeToValue( inTime, value );
            
    
    DB_put( &lookTimeDB, key, value );
    }






// slot is 0 for main map cell, or higher for container slots
static void trackETA( int inX, int inY, int inSlot, timeSec_t inETA,
                      int inSubCont = 0 ) {
    
    timeSec_t timeLeft = inETA - MAP_TIMESEC;
        
    if( timeLeft < maxSecondsForActiveDecayTracking ) {
        // track it live
            
        // duplicates okay
        // we'll deal with them when they ripen
        // (we check the true ETA stored in map before acting
        //   on one stored in this queue)
        LiveDecayRecord r = { inX, inY, inSlot, inETA, inSubCont };
            
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
        char found;
        timeSec_t *oldLookTime =
            liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY,
                                                                i + 1,
                                                                inSubCont,
                                                                &found );
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
    TransRecord *t = getTrans( -1, inID );

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
                

                if( t->move < 3 ) {
                    
                    GridPos p = getClosestPlayerPos( inX, inY );
                    
                    double dist = 
                        sqrt( ( p.x - inX ) * ( p.x - inX ) + 
                              ( p.y - inY ) * ( p.y - inY ) );

                    if( dist <= 7 &&
                        ( p.x != 0 || p.y != 0 ) ) {
                        
                        if( t->move == 1 && dist <= desiredMoveDist ) {
                            // chase.  Try to land exactly on them
                            // if they're close enough to do it in one move
                            desiredMoveDist = lrint( dist );
                            }

                        dir.x = p.x - inX;
                        dir.y = p.y - inY;
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

                // try again and again with smaller distances until we
                // find an empty spot
                while( newX == inX && newY == inY && tryDist > 0 ) {

                    // walk up to 4 steps past our dist in that direction,
                    // looking for non-blocking objects or an empty spot
                
                    for( int i=0; i<tryDist + tryRadius; i++ ) {
                        int testX = lrint( inX + dir.x * i );
                        int testY = lrint( inY + dir.y * i );
                    
                        int oID = getMapObjectRaw( testX, testY );
                        
                        // does trans exist for this object used on destination 
                        // obj?
                        TransRecord *trans = NULL;
                        
                        if( oID > 0 ) {
                            trans = getTrans( inID, oID );
                            
                            if( trans == NULL ) {
                                // does trans exist for newID applied to
                                // destination?
                                trans = getTrans( newID, oID );
                                }
                            }
                        
                        if( i >= tryDist && oID == 0 ) {
                            // found a spot for it to move
                            newX = testX;
                            newY = testY;
                            break;
                            }
                        else if( i >= tryDist && trans != NULL ) {
                            newX = testX;
                            newY = testY;
                            destTrans = trans;
                            break;
                            }
                        else if( oID > 0 && getObject( oID ) != NULL &&
                                 getObject( oID )->blocksWalking ) {
                            // blocked, stop now
                            break;
                            }
                        // else walk through it
                        }
                    
                    tryDist--;
                    // 4 on first try, but then 1 on remaining tries to
                    // avoid overlap with previous tries
                    tryRadius = 1;
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

                                    possibleX[ numPossibleDirs ] = testX;
                                    possibleY[ numPossibleDirs ] = testY;
                                    numPossibleDirs++;
                                    stopCheckingDir = true;
                                    break;
                                    }
                                else if( oID > 0 && getObject( oID ) != NULL &&
                                         getObject( oID )->blocksWalking ) {
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
                        destTrans = getTrans( inID, -1 );

                        if( destTrans == NULL ) {
                            // another attempt at bare ground transition
                            destTrans = getTrans( movingObjID, -1 );
                            }
                        
                        if( destTrans != NULL &&
                            destTrans->newTarget != newID &&
                            destTrans->newTarget != movingObjID ) {
                            // for bare ground, make sure newTarget
                            // matches newTarget of our orginal move transition
                            destTrans = NULL;
                            }
                        }
                    
                        
                    
                    if( destTrans != NULL ) {
                        // leave new actor behind
                        
                        leftBehindID = destTrans->newActor;
                        
                        dbPut( inX, inY, 0, leftBehindID );
                        

                        
                        TransRecord *leftDecayT = getTrans( -1, leftBehindID );

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
                        setEtaDecay( inX, inY, leftMapETA );
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
                    
                    
                    if( newID > 0 ) {
                        ObjectRecord *newObj = getObject( newID );
                        
                        if( newObj != NULL ) {
                            speed *= newObj->speedMult;
                            }
                        }
                    
                    double moveTime = moveDist / speed;
                    
                    double etaTime = Time::getCurrentTime() + moveTime;
                    
                    MovementRecord moveRec = { newX, newY, etaTime };
                    
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
                }
            
                
            if( newX == inX && newY == inY ) {
                // no move happened

                // just set change in DB
                dbPut( inX, inY, 0, newID );
                }
            
                
            TransRecord *newDecayT = getTrans( -1, newID );

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

            setEtaDecay( newX, newY, mapETA );
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
    
        TransRecord *t = getTrans( -1, oldID );

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
                    
                    TransRecord *newDecayT = getTrans( -1, newID );

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


int getMapObjectRaw( int inX, int inY ) {
    int result = dbGet( inX, inY, 0 );
    
    if( result == -1 ) {
        // nothing in map
        result = getBaseMap( inX, inY );
        
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
            else if( getObjectHeight( result ) < CELL_D ) {
                // a short object should be here
                // make sure there's not a tall object below already

                
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
            
            }
        
        }
    
    return result;
    }




void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd ) {
    timeSec_t currentTime = MAP_TIMESEC;
    
    for( int y=inYStart; y<=inYEnd; y++ ) {
        for( int x=inXStart; x<=inXEnd; x++ ) {
        
            char found;
            timeSec_t *oldLookTime = 
                liveDecayRecordLastLookTimeHashTable.lookupPointer( x, y, 
                                                                    0, 0,
                                                                    &found );
    
            if( oldLookTime != NULL ) {
                // we're tracking decay for this cell
                *oldLookTime = currentTime;
                }

            int numContained;
            
            int *contained = getContained( x, y, &numContained );

            for( int c=0; c<numContained; c++ ) {
                
                char found;
                oldLookTime =
                    liveDecayRecordLastLookTimeHashTable.lookupPointer( 
                        x, y, c + 1, 0, &found );
                if( oldLookTime != NULL ) {
                    // look at it now
                    *oldLookTime = currentTime;
                    }
                
                if( contained[c] < 0 ) {
                    // sub contained
                    int numSub = getNumContained( x, y, c + 1 );
                    
                    for( int s=0; s<numSub; s++ ) {
                        
                        oldLookTime =
                            liveDecayRecordLastLookTimeHashTable.lookupPointer( 
                                x, y, s + 1, c+1, &found );
                        if( oldLookTime != NULL ) {
                            // look at it now
                            *oldLookTime = currentTime;
                            }
                        }
                    }
                }
            
            if( contained != NULL ) {
                delete [] contained;
                }
            }
        }
    }



int getMapObject( int inX, int inY ) {

    // look at this map cell
    char found;
    timeSec_t *oldLookTime = 
        liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY, 0, 0,
                                                            &found );
    
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

    
    
    for( int y=inStartY; y<endY; y++ ) {
        int chunkY = y - inStartY;
        

        for( int x=inStartX; x<endX; x++ ) {
            int chunkX = x - inStartX;
            
            int cI = chunkY * inWidth + chunkX;
            
            lastCheckedBiome = -1;
            
            chunk[cI] = getMapObject( x, y );

            if( lastCheckedBiome == -1 ) {
                // biome wasn't checked in order to compute
                // getMapObject

                // get it ourselves
                
                lastCheckedBiome = biomes[getMapBiomeIndex( x, y )];
                }
            chunkBiomes[ cI ] = lastCheckedBiome;

            chunkFloors[cI] = getMapFloor( x, y );
            

            int numContained;
            int *contained = getContained( x, y, &numContained );

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
                                  chunkFloors[i], chunk[i] );
        
        chunkDataBuffer.appendArray( (unsigned char*)cell, strlen(cell) );
        delete [] cell;

        if( containedStacks[i] != NULL ) {
            for( int c=0; c<containedStackSizes[i]; c++ ) {
                char *containedString = autoSprintf( ",%d", 
                                                     containedStacks[i][c] );
        
                chunkDataBuffer.appendArray( (unsigned char*)containedString, 
                                             strlen( containedString ) );
                delete [] containedString;

                if( subContainedStacks[i][c] != NULL ) {
                    
                    for( int s=0; s<subContainedStackSizes[i][c]; s++ ) {
                        
                        char *subContainedString = 
                            autoSprintf( ":%d", 
                                         subContainedStacks[i][c][s] );
        
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
                                inStartX, inStartY, chunkDataBuffer.size(),
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













void setMapObject( int inX, int inY, int inID ) {
    dbPut( inX, inY, 0, inID );


    // actually need to set decay here
    // otherwise, if we wait until getObject, it will assume that
    // this is a never-before-seen object and randomize the decay.
    TransRecord *newDecayT = getTrans( -1, inID );
    
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




void setEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds ) {
    dbTimePut( inX, inY, DECAY_SLOT, inAbsoluteTimeInSeconds );
    if( inAbsoluteTimeInSeconds != 0 ) {
        trackETA( inX, inY, 0, inAbsoluteTimeInSeconds );
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
        dbPut( inX, inY, FIRST_CONT_SLOT + i, inContained[i], inSubCont );
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
    if( inSubCont == 0 ) {
        // clear sub container slots too
        int oldNum = getNumContained( inX, inY, inSubCont );
    
        for( int i=0; i<oldNum; i++ ) {
            dbPut( inX, inY, NUM_CONT_SLOT, 0, i + 1 );
            }
        }
    
    dbPut( inX, inY, NUM_CONT_SLOT, 0, inSubCont );
    }



void shrinkContainer( int inX, int inY, int inNumNewSlots, int inSubCont ) {
    int oldNum = getNumContained( inX, inY, inSubCont );
    
    if( oldNum > inNumNewSlots ) {
        
        // first, scatter extra contents into empty nearby spots.
        for( int i=inNumNewSlots; i<oldNum; i++ ) {
            
            int contID = getContained( inX, inY, i, inSubCont );

            char subCont = false;
            
            if( contID < 0 ) {
                contID *= -1;
                subCont = true;
                }
            
            int emptyX, emptyY;
            char foundEmpty = false;

            int r = 1;
            
            while( !foundEmpty && r < 3 ) {    
                for( int y = inY - r; y <= inY + r; y++ ) {
                    for( int x = inX - r; x <= inX + r; x++ ) {
                        
                        if( getMapObjectRaw( x, y ) == 0 ) {
                            emptyX = x;
                            emptyY = y;
                            foundEmpty = true;
                            break;
                            }
                        }
                    if( foundEmpty ) {
                        break;
                        }
                    }
                r++;
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




char *getMapChangeLineString( ChangePosition inPos ) {
    

    SimpleVector<char> buffer;
    

    char *header = autoSprintf( "%d %d %d ", inPos.x, inPos.y,
                                getMapFloor( inPos.x, inPos.y ) );
    
    buffer.appendElementString( header );
    
    delete [] header;
    

    char *idString = autoSprintf( "%d", getMapObjectNoLook( inPos.x,
                                                            inPos.y ) );
    
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
        
        char *idString = autoSprintf( ",%d", contained[i] );
        
        buffer.appendElementString( idString );
        
        delete [] idString;

        if( subCont ) {
            
            int numSubContained;
            int *subContained = getContainedNoLook( inPos.x, inPos.y, 
                                                    &numSubContained,
                                                    i + 1 );
            for( int s=0; s<numSubContained; s++ ) {
                idString = autoSprintf( ":%d", subContained[s] );
        
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
        char *moveString = autoSprintf( " %d %d %f", 
                                        inPos.oldX, inPos.oldY, inPos.speed );
        
        buffer.appendElementString( moveString );
    
        delete [] moveString;
        }

    buffer.appendElementString( "\n" );

    return buffer.getElementString();
    }





int getMapFloor( int inX, int inY ) {
    int id = dbFloorGet( inX, inY );
    
    if( id <= 0 ) {
        return 0;
        }
    
    TransRecord *t = getTrans( -1, id );

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
    dbFloorPut( inX, inY, inID );


    // further decay from here
    TransRecord *newT = getTrans( -1, inID );

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




void stepMap( SimpleVector<char> *inMapChanges, 
              SimpleVector<ChangePosition> *inChangePosList ) {
    
    timeSec_t curTime = MAP_TIMESEC;

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
                    maxSecondsNoLookDecayTracking ) {
                    
                    // this cell or slot hasn't been looked at in too long
                    // don't even apply this decay now
                    liveDecayRecordLastLookTimeHashTable.remove( 
                        r.x, r.y, r.slot, r.subCont );
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
        
        char *changeString = getMapChangeLineString( p );
        inMapChanges->appendElementString( changeString );
        delete [] changeString;
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




void getEvePosition( char *inEmail, int *outX, int *outY ) {

    int currentEveRadius = eveRadius;

    char forceEveToBorder = false;

    doublePair ave = { 0, 0 };

    printf( "Placing new Eve:  " );
    
    
    int pX, pY, pR;
    
    int result = eveDBGet( inEmail, &pX, &pY, &pR );
    
    if( result == 1 && pR > 0 ) {
        printf( "Found camp center (%d,%d) r=%d in db for %s\n",
                pX, pY, pR, inEmail );
        
        ave.x = pX;
        ave.y = pY;
        currentEveRadius = pR;
        }
    else {
        // player has never been an Eve that survived to old age before
        

        // New method:
        GridPos eveLocToUse = eveLocation;
        
        if( eveLocationUsage < maxEveLocationUsage ) {
            eveLocationUsage++;
            // keep using same location
            }
        else {
            // post-startup eve location has been used too many times
            // jump away in random direction

            int jump = SettingsManager::getIntSetting( "nextEveJump", 2000 );
            
            doublePair delta = { (double)jump, 0 };
            delta = rotate( delta,
                            randSource.getRandomBoundedDouble( 0, 2 * M_PI ) );
            
            // but don't update the post-startup location
            // keep jumping away from startup-location as center of
            // a circle
            eveLocToUse.x += lrint( delta.x );
            eveLocToUse.y += lrint( delta.y );
            
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
        }
    




    // pick point in box according to eve radius

    
    char found = 0;
    
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
                found = true;
                }

            tryCount++;
            }

        // tried too many times, expand radius
        currentEveRadius *= 2;
        
        }

    // clear recent placements after placing a new Eve
    // let her make new placements in her life which we will remember
    // later

    clearRecentPlacements();
    }




void mapEveDeath( char *inEmail, double inAge ) {
    
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
    

    int num = 0;
    
    doublePair ave = computeRecentCampAve( &num );
    
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
        pX = lrint( ave.x );
        pY = lrint( ave.y );

        pR = eveRadiusStart;
        }
    
    
    if( num > 0 ) {
        // overwrite middle from last life with new middle of placements
        // from this life
        pX = lrint( ave.x );
        pY = lrint( ave.y );
        }
    else {
        // otherwise, leave last life's average alone
        printf( "Logging Eve death:   "
                "Keeping camp average (%d,%d) from last life\n",
                pX, pY );
        }
    

    printf( "Remembering Eve's camp in database (%d,%d) r=%d for %s\n",
            pX, pY, pR, inEmail );
    
    eveDBPut( inEmail, pX, pY, pR );
    }





