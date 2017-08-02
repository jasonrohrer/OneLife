#include "map.h"
#include "HashTable.h"


#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"


#include "kissdb.h"

#include <stdarg.h>
#include <math.h>
#include <values.h>
#include <stdint.h>


#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"
#include "../gameSource/GridPos.h"

#include "../gameSource/GridPos.h"


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

// what human-placed stuff, together, counts as a camp
static int campRadius = 20;



static int chunkDimensionX = 32;
static int chunkDimensionY = 30;


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


static KISSDB db;
static char dbOpen = false;


static KISSDB timeDB;
static char timeDBOpen = false;


static KISSDB biomeDB;
static char biomeDBOpen = false;


static int randSeed = 124567;
//static JenkinsRandomSource randSource( randSeed );
static CustomRandomSource randSource( randSeed );



#define DECAY_SLOT 1
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

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



// track all map changes that happened since the last
// call to stepMap
static SimpleVector<ChangePosition> mapChangePosSinceLastStep;






#include "../commonSource/fractalNoise.h"







int getMaxChunkDimension() {
    return chunkDimensionX;
    }




// three ints to an 12-byte key
void intTripleToKey( int inX, int inY, int inSlot, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        outKey[i+8] = ( inSlot >> offset ) & 0xFF;
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


// one int to a 4-byte value
void intToValue( int inV, unsigned char *outValue ) {
    for( int i=0; i<4; i++ ) {
        outValue[i] = ( inV >> (i * 8) ) & 0xFF;
        }    
    }


int valueToInt( unsigned char *inValue ) {
    return 
        inValue[3] << 24 | inValue[2] << 16 | 
        inValue[1] << 8 | inValue[0];
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





// returns -1 if not found
static int biomeDBGet( int inX, int inY, 
                       int *outSecondPlaceBiome = NULL,
                       double *outSecondPlaceGap = NULL ) {
    unsigned char key[8];
    unsigned char value[12];

    // look for changes to default in database
    intPairToKey( inX, inY, key );
    
    int result = KISSDB_get( &biomeDB, key, value );
    
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
            
    
    KISSDB_put( &biomeDB, key, value );
    }
    




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




static int getMapBiomeIndex( int inX, int inY, 
                             int *outSecondPlaceIndex = NULL,
                             double *outSecondPlaceGap = NULL ) {
    
    int secondPlaceBiome;
    
    int dbBiome = biomeDBGet( inX, inY,
                              &secondPlaceBiome,
                              outSecondPlaceGap );
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
        
        // else a biome or second place in biome.db that isn't in game anymore
        // ignore it
        }
    

    int pickedBiome = -1;
        
    double maxValue = -DBL_MAX;
        
    int secondPlace = -1;
    
    double secondPlaceGap = 0;

    
    for( int i=0; i<numBiomes; i++ ) {
        int biome = biomes[i];
        
        double randVal = getXYFractal(  723 + inX + 263 * biome, 
                                        1553 + inY + 187 * biome, 
                                        0.55, 
                                        1.5 + 0.16666 * numBiomes );
        
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
    


    if( outSecondPlaceIndex != NULL ) {
        *outSecondPlaceIndex = secondPlace;
        }
    if( outSecondPlaceGap != NULL ) {
        *outSecondPlaceGap = secondPlaceGap;
        }
    

    if( dbBiome == -1 ) {
        // not stored, store it

        secondPlaceBiome = 0;
        if( secondPlace != -1 ) {
            secondPlaceBiome = biomes[ secondPlace ];
            }
        
        biomeDBPut( inX, inY, biomes[pickedBiome], 
                    secondPlaceBiome, secondPlaceGap );
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


static int getBaseMap( int inX, int inY ) {
    
    if( inX > xLimit || inX < -xLimit ||
        inY > yLimit || inY < -yLimit ) {
    
        return edgeObjectID;
        }
    
    
    // first step:  save rest of work if density tells us that
    // nothing is here anyway
    double density = getXYFractal( inX, inY, 0.1, 1 );
    
    // correction
    density = sigmoid( density, 0.1 );
    
    // scale
    density *= .4;
    // good for zoom in to map for teaser
    //density = .70;

    if( getXYRandom( 287 + inX, 383 + inY ) < density ) {




        // next step, pick top two biomes
        int secondPlace;
        double secondPlaceGap;
        
        int pickedBiome = getMapBiomeIndex( inX, inY, &secondPlace,
                                            &secondPlaceGap );
        
        if( pickedBiome == -1 ) {
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
        
        if( getXYRandom( 2087 + inX, 793 + inY ) > 
            .5 + secondPlaceReduction * secondPlaceGap ) {
        
            // note that lastCheckedBiome is NOT changed, so ground
            // shows the true, first-place biome, but object placement
            // follows the second place biome
            pickedBiome = secondPlace;
            }
        

        int numObjects = naturalMapIDs[pickedBiome].size();

        if( numObjects == 0  ) {
            return 0;
            }

    
  
        // something present here

        
        // special object in this region is 10x more common than it 
        // would be otherwise


        int specialObjectIndex = -1;
        double maxValue = -DBL_MAX;
        

        for( int i=0; i<numObjects; i++ ) {
            
        
            double randVal = getXYFractal(  123 + inX + 263 * i, 
                                            753 + inY + 187 * i, 
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
            return naturalMapIDs[pickedBiome].getElementDirect( i );
            }
        else {
            return 0;
            }
        }
    else {
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
    
    Image im( w, h, 3, true );
    
    SimpleVector<Color> objColors;
    for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
        randSource.getRandomFloat();
        Color *c = Color::makeColorFromHSV( (float)i / allNaturalMapIDs.size(),
                                            1, 1 );
        objColors.push_back( *c );
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
            
            
            int id = getBaseMap( x, y );
    
            if( id > 0 ) {
                for( int i=0; i<allNaturalMapIDs.size(); i++ ) {
                    if( allNaturalMapIDs.getElementDirect(i) == id ) {
                        im.setColor( y * w + x,
                                     objColors.getElementDirect( i ) );
                        break;
                        }
                    }
                
                }
            }
        }
    
    File tgaFile( NULL, "mapOut.tga" );
    FileOutputStream tgaStream( &tgaFile );
    
    TGAImageConverter converter;
    
    converter.formatImage( &im, &tgaStream );
    exit(0);
    }



int getMapObjectRaw( int inX, int inY );
int *getContainedRaw( int inX, int inY, int *outNumContained );



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
    eveRadius *= 2;
    writeEveRadius();
    }



void resetEveRadius() {
    eveRadius = eveRadiusStart;
    writeEveRadius();
    }



void initMap() {

    edgeObjectID = SettingsManager::getIntSetting( "edgeObject", 0 );
    

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
            
            if( recentPlacements[i].pos.x != 0 ||
                recentPlacements[i].pos.y != 0 ) {
                }
            }
        fscanf( placeFile, "\nnextPlacementIndex=%d", &nextPlacementIndex );
        
        fclose( placeFile );
        }
    

    FILE *eveRadFile = fopen( "eveRadius.txt", "r" );
    if( eveRadFile != NULL ) {
        
        fscanf( eveRadFile, "%d", &eveRadius );

        fclose( eveRadFile );
        }
    
            

    // note that the various decay ETA slots in map.db 
    // are define but unused, because we store times separately
    // in mapTime.db
    int error = KISSDB_open( &db, 
                             "map.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             80000,
                             12, // three 32-bit ints, xys
                                 // s is the slot number 
                                 // s=0 for base object
                                 // s=1 decay ETA seconds (wall clock time)
                                 // s=2 for count of contained objects
                                 // s=3 first contained object
                                 // s=4 second contained object
                                 // s=... remaining contained objects
                                 // Then decay ETA for each slot, in order,
                                 //   after that.
                             4 // one int, object ID at x,y in slot s
                               // OR contained count if s=1
                             );
    
    if( error ) {
        AppLog::errorF( "Error %d opening map KissDB", error );
        return;
        }
    
    dbOpen = true;



    // this DB uses the same slot numbers as the map.db
    // however, only times are stored here, because they require 8 bytes
    // so, slot 0 and 2 are never used, for example
    error = KISSDB_open( &timeDB, 
                         "mapTime.db", 
                         KISSDB_OPEN_MODE_RWCREAT,
                         80000,
                         12, // three 32-bit ints, xys
                         // s is the slot number 
                         // s=0 for base object
                         // s=1 decay ETA seconds (wall clock time)
                         // s=2 for count of contained objects
                         // s=3 first contained object
                         // s=4 second contained object
                         // s=... remaining contained objects
                         // Then decay ETA for each slot, in order,
                         //   after that.
                         8 // one 64-bit double, representing an ETA time
                           // in whatever binary format and byte order
                           // "double" on the server platform uses
                         );
    
    if( error ) {
        AppLog::errorF( "Error %d opening map time KissDB", error );
        return;
        }
    
    timeDBOpen = true;






    error = KISSDB_open( &biomeDB, 
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
    


    AppLog::info( "\nCleaning map of objects that have been removed..." );
    

    KISSDB_Iterator dbi;
    
    
    KISSDB_Iterator_init( &db, &dbi );
    
    unsigned char key[12];
    
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
    
    while( KISSDB_Iterator_next( &dbi, key, value ) > 0 ) {
        
        int s = valueToInt( &( key[8] ) );
       
        if( s == 0 ) {
            int id = valueToInt( value );
            
            if( id > 0 ) {
                totalSetCount++;
                
                if( getObject( id ) == NULL ) {
                    // id doesn't exist anymore
                    
                    numClearedCount++;
                    
                    int x = valueToInt( key );
                    int y = valueToInt( &( key[4] ) );
                    
                    xToClear.push_back( x );
                    yToClear.push_back( y );
                    }
                }
            }
        if( s == 2 ) {
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
            
            for( int c=0; c<numCont; c++ ) {
                if( getObject( cont[c] ) != NULL ) {
                    newCont.push_back( cont[c] );
                    newDecay.push_back( decay[c] );
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
    
    
    if( totalSetCount == 0 ) {
        // map has been cleared

        // ignore old value for placements
        for( int i=0; i<NUM_RECENT_PLACEMENTS; i++ ) {
            recentPlacements[i].pos.x = 0;
            recentPlacements[i].pos.y = 0;
            recentPlacements[i].depth = 0;
            }

        writeRecentPlacements();
        }
    
    // for debugging the map
    //outputMapImage();
    }





void freeAndNullString( char **inStringPointer ) {
    if( *inStringPointer != NULL ) {
        delete [] *inStringPointer;
        *inStringPointer = NULL;
        }
    }



void freeMap() {
    if( dbOpen ) {
        
        AppLog::infoF( "Cleaning up map database on server shutdown." );
        
        // iterate through DB and look for useDummy objects
        KISSDB_Iterator dbi;
    
    
        KISSDB_Iterator_init( &db, &dbi );
    
        unsigned char key[12];
    
        unsigned char value[4];


        // keep list of x,y coordinates in map that need replacing
        SimpleVector<int> xToPlace;
        SimpleVector<int> yToPlace;

        SimpleVector<int> idToPlace;
        

        // container slots that need replacing
        SimpleVector<int> xContToCheck;
        SimpleVector<int> yContToCheck;
        
        
        while( KISSDB_Iterator_next( &dbi, key, value ) > 0 ) {
        
            int s = valueToInt( &( key[8] ) );
       
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
        
            if( getMapObjectRaw( x, y ) != 0 ) {

                int numCont;
                int *cont = getContainedRaw( x, y, &numCont );

                for( int c=0; c<numCont; c++ ) {
                    ObjectRecord *contObj = getObject( cont[c] );
                    
                    if( contObj != NULL && contObj->isUseDummy ) {
                        cont[c] = contObj->useDummyParent;
                        numContChanged ++;
                        }
                    }
                
                setContained( x, y, numCont, cont );
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
        
        KISSDB_close( &db );
        }

    if( timeDBOpen ) {
        KISSDB_close( &timeDB );
        }

    if( biomeDBOpen ) {
        KISSDB_close( &biomeDB );
        }
    
    writeEveRadius();
    writeRecentPlacements();

    delete [] biomes;
    
    delete [] naturalMapIDs;
    delete [] naturalMapChances;
    delete [] totalChanceWeight;
    }






// returns -1 if not found
static int dbGet( int inX, int inY, int inSlot ) {
    unsigned char key[12];
    unsigned char value[4];

    // look for changes to default in database
    intTripleToKey( inX, inY, inSlot, key );
    
    int result = KISSDB_get( &db, key, value );
    
    if( result == 0 ) {
        // found
        return valueToInt( value );
        }
    else {
        return -1;
        }
    }




// returns 0 if not found
static timeSec_t dbTimeGet( int inX, int inY, int inSlot ) {
    unsigned char key[12];
    unsigned char value[8];

    // look for changes to default in database
    intTripleToKey( inX, inY, inSlot, key );
    
    int result = KISSDB_get( &timeDB, key, value );
    
    if( result == 0 ) {
        // found
        return valueToTime( value );
        }
    else {
        return 0;
        }
    }



static void dbPut( int inX, int inY, int inSlot, int inValue ) {
    
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
    
    
    unsigned char key[12];
    unsigned char value[4];
    

    intTripleToKey( inX, inY, inSlot, key );
    intToValue( inValue, value );
            
    
    KISSDB_put( &db, key, value );
    }





static void dbTimePut( int inX, int inY, int inSlot, timeSec_t inTime ) {
    // ETA decay changes don't get reported as map changes    
    
    unsigned char key[12];
    unsigned char value[8];
    

    intTripleToKey( inX, inY, inSlot, key );
    timeToValue( inTime, value );
            
    
    KISSDB_put( &timeDB, key, value );
    }






// slot is 0 for main map cell, or higher for container slots
static void trackETA( int inX, int inY, int inSlot, timeSec_t inETA ) {
    timeSec_t timeLeft = inETA - Time::timeSec();
        
    if( timeLeft < maxSecondsForActiveDecayTracking ) {
        // track it live
            
        // duplicates okay
        // we'll deal with them when they ripen
        // (we check the true ETA stored in map before acting
        //   on one stored in this queue)
        LiveDecayRecord r = { inX, inY, inSlot, inETA };
            
        char exists;
        timeSec_t existingETA =
            liveDecayRecordPresentHashTable.lookup( inX, inY, inSlot,
                                                    &exists );

        if( !exists || existingETA != inETA ) {
            
            liveDecayQueue.insert( r, inETA );
            
            liveDecayRecordPresentHashTable.insert( inX, inY, inSlot, inETA );

            char exists;
            
            liveDecayRecordLastLookTimeHashTable.lookup( inX, inY, inSlot,
                                                         &exists );
            
            if( !exists ) {
                // don't overwrite old one
                liveDecayRecordLastLookTimeHashTable.insert( inX, inY, inSlot, 
                                                             Time::timeSec() );
                }
            }
        }
    }





float getMapContainerTimeStretch( int inX, int inY ) {
    
    float stretch = 1.0f;
                        
    int containerID = getMapObjectRaw( inX, inY );
                        
    if( containerID != 0 ) {
        stretch = getObject( containerID )->slotTimeStretch;
        }
    return stretch;
    }                        



void checkDecayContained( int inX, int inY );







int *getContainedRaw( int inX, int inY, int *outNumContained ) {
    int num = getNumContained( inX, inY );

    *outNumContained = num;
    
    if( num == 0 ) {
        return NULL;
        }
   
    int *contained = new int[ num ];

    for( int i=0; i<num; i++ ) {
        int result = dbGet( inX, inY, FIRST_CONT_SLOT + i );
        if( result != -1 ) {
            contained[i] = result;
            }
        else {
            contained[i] = 0;
            }
        }
    return contained;
    }



int *getContained( int inX, int inY, int *outNumContained ) {
    checkDecayContained( inX, inY );
    int *result = getContainedRaw( inX, inY, outNumContained );
    
    // look at these slots if they are subject to live decay
    timeSec_t currentTime = Time::timeSec();
    
    for( int i=0; i<*outNumContained; i++ ) {
        char found;
        timeSec_t *oldLookTime =
            liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY,
                                                                i + 1,
                                                                &found );
        if( oldLookTime != NULL ) {
            // look at it now
            *oldLookTime = currentTime;
            }
        }
    
    return result;
    }


int *getContainedNoLook( int inX, int inY, int *outNumContained ) {
    checkDecayContained( inX, inY );
    return getContainedRaw( inX, inY, outNumContained );
    }



timeSec_t *getContainedEtaDecay( int inX, int inY, int *outNumContained ) {
    int num = getNumContained( inX, inY );

    *outNumContained = num;
    
    if( num == 0 ) {
        return NULL;
        }
   
    timeSec_t *containedEta = new timeSec_t[ num ];

    for( int i=0; i<num; i++ ) {
        // can be 0 if not found, which is okay
        containedEta[i] = dbTimeGet( inX, inY, FIRST_CONT_SLOT + num + i );
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
    
    // in case of movement
    int newX = inX;
    int newY = inY;
    

    // is eta stored in map?
    timeSec_t mapETA = getEtaDecay( inX, inY );
    
    if( mapETA != 0 ) {
        
        if( (int)mapETA <= Time::timeSec() ) {
            
            // object in map has decayed (eta expired)

            // apply the transition
            newID = t->newTarget;
            

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
                
                if( t->move < 3 ) {
                    
                    GridPos p = getClosestPlayerPos( inX, inY );
                    
                    if( p.x != 0 || p.y != 0 ) {
                        
                        dir.x = p.x - inX;
                        dir.y = p.y - inY;
                        dir = normalize( dir );
                        
                        // round to one of 8 cardinal directions
                        
                        double a = angle( dir );
                        
                        a = lrint( a * 8 ) / 8.0;
                        
                        dir.x = 1;
                        dir.y = 0;
                        dir = rotate( dir, a );
                        }
                    if( t->move == 2 ) {
                        // flee
                        dir = mult( dir, -1 );
                        }
                    }

                if( dir.x == 0 && dir.y == 0 ) {
                    // random instead
                    
                    dir.x = 1;
                    dir.y = 0;
                    
                    // 8 cardinal directions
                    dir = rotate( 
                        dir,
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

                // walk up to 4 steps in that direction, looking
                // for non-blocking objects or an empty spot
                
                for( int i=0; i<4; i++ ) {
                    int testX = lrint( inX + dir.x * i );
                    int testY = lrint( inY + dir.y * i );
                    
                    int oID = getMapObjectRaw( testX, testY );
                                               
                    
                    if( oID == 0 ) {
                        // found a spot for it to move
                        newX = testX;
                        newY = testY;
                        break;
                        }
                    else if( oID > 0 && getObject( oID ) != NULL &&
                             getObject( oID )->blocksWalking ) {
                        // blocked, stop now
                        break;
                        }
                    // else walk through it
                    }
                

                if( newX != inX || newY != inY ) {
                    // a reall move!

                    // move object
                    // leave empty spot behind
                    dbPut( inX, inY, 0, 0 );
                    
                    dbPut( newX, newY, 0, newID );


                    // move contained
                    int numCont;
                    int *cont = getContained( inX, inY, &numCont );
                    timeSec_t *contEta = 
                        getContainedEtaDecay( inX, inY, &numCont );
                    
                    if( numCont > 0 ) {
                        setContained( newX, newY, numCont, cont );
                        setContainedEtaDecay( newX, newY, numCont, contEta );
                        
                        clearAllContained( inX, inY );
                        
                        delete [] cont;
                        delete [] contEta;
                        }

                    // now patch up change record marking this as a move
                    
                    for( int i=0; i<mapChangePosSinceLastStep.size(); i++ ) {
                        
                        ChangePosition *p = 
                            mapChangePosSinceLastStep.getElement( i );
                        
                        if( p->x == newX && p->y == newY ) {
                            
                            // update it
                            p->oldX = inX;
                            p->oldY = inY;
                            p->speed = 4.0f;
                            
                            if( newID > 0 ) {
                                ObjectRecord *newObj = getObject( newID );
                                
                                if( newObj != NULL ) {
                                    p->speed *= newObj->speedMult;
                                    }
                                }
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
                mapETA = Time::timeSec() + tweakedSeconds;
                }
            else {
                // no further decay
                mapETA = 0;
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
        
        mapETA = Time::timeSec() + decayTime;
            
        setEtaDecay( inX, inY, mapETA );
        }
    

    if( mapETA != 0 ) {
        trackETA( newX, newY, 0, mapETA );
        }
    
    
    return newID;
    }



void checkDecayContained( int inX, int inY ) {
    
    if( getNumContained( inX, inY ) == 0 ) {
        return;
        }
    
    int numContained;
    int *contained = getContainedRaw( inX, inY, &numContained );
    
    SimpleVector<int> newContained;
    SimpleVector<timeSec_t> newDecayEta;
    
    char change = false;
    
    for( int i=0; i<numContained; i++ ) {
        int oldID = contained[i];
        
        TransRecord *t = getTrans( -1, oldID );

        if( t == NULL ) {
            // no auto-decay for this object
            newContained.push_back( oldID );
            newDecayEta.push_back( 0 );
            continue;
            }
    
    
        // else decay exists for this object
    
        int newID = oldID;

        // is eta stored in map?
        timeSec_t mapETA = getSlotEtaDecay( inX, inY, i );
    
        if( mapETA != 0 ) {
        
            if( (int)mapETA <= Time::timeSec() ) {
            
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
                            Time::timeSec() +
                            tweakedSeconds / 
                            getMapContainerTimeStretch( inX, inY );
                        }
                    else {
                        // no further decay
                        mapETA = 0;
                        }
                    }
                }
            }
        
        if( newID != 0 ) {    
            newContained.push_back( newID );
            newDecayEta.push_back( mapETA );
            }
        }
    
    
    if( change ) {
        int *containedArray = newContained.getElementArray();
        int numContained = newContained.size();

        setContained( inX, inY, newContained.size(), containedArray );
        delete [] containedArray;
        
        for( int i=0; i<numContained; i++ ) {
            timeSec_t mapETA = newDecayEta.getElementDirect( i );
            
            if( mapETA != 0 ) {
                trackETA( inX, inY, 1 + i, mapETA );
                }
            
            setSlotEtaDecay( inX, inY, i, mapETA );
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
            }
        
        }
    
    return result;
    }




void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd ) {
    timeSec_t currentTime = Time::timeSec();
    
    for( int y=inYStart; y<=inYEnd; y++ ) {
        for( int x=inXStart; x<=inXEnd; x++ ) {
        
            char found;
            timeSec_t *oldLookTime = 
                liveDecayRecordLastLookTimeHashTable.lookupPointer( x, y, 0,
                                                                    &found );
    
            if( oldLookTime != NULL ) {
                // we're tracking decay for this cell
                *oldLookTime = currentTime;
                }

            int numContained = getNumContained( x, y );
            
            for( int c=0; c<numContained; c++ ) {
                
                char found;
                oldLookTime =
                    liveDecayRecordLastLookTimeHashTable.lookupPointer( 
                        x, y, c + 1, &found );
                if( oldLookTime != NULL ) {
                    // look at it now
                    *oldLookTime = currentTime;
                    }
                }
            }
        }
    }



int getMapObject( int inX, int inY ) {

    // look at this map cell
    char found;
    timeSec_t *oldLookTime = 
        liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY, 0,
                                                            &found );
    
    if( oldLookTime != NULL ) {
        // we're tracking decay for this cell
        *oldLookTime = Time::timeSec();
        }


    // apply any decay that should have happened by now
    return checkDecayObject( inX, inY, getMapObjectRaw( inX, inY ) );
    }


int getMapObjectNoLook( int inX, int inY ) {

    // apply any decay that should have happened by now
    return checkDecayObject( inX, inY, getMapObjectRaw( inX, inY ) );
    }



// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength ) {
    
    int chunkCells = chunkDimensionX * chunkDimensionY;
    
    int *chunk = new int[chunkCells];

    int *chunkBiomes = new int[chunkCells];
    
    int *containedStackSizes = new int[ chunkCells ];
    int **containedStacks = new int*[ chunkCells ];

    // 0,0 is center of map
    
    int halfChunkX = chunkDimensionX /2;
    int halfChunkY = chunkDimensionY /2;
    

    int startY = inCenterY - halfChunkY;
    int startX = inCenterX - halfChunkX;
    
    int endY = startY + chunkDimensionY;
    int endX = startX + chunkDimensionX;

    
    
    for( int y=startY; y<endY; y++ ) {
        int chunkY = y - startY;
        

        for( int x=startX; x<endX; x++ ) {
            int chunkX = x - startX;
            
            int cI = chunkY * chunkDimensionX + chunkX;
            
            lastCheckedBiome = -1;
            
            chunk[cI] = getMapObject( x, y );

            if( lastCheckedBiome == -1 ) {
                // biome wasn't checked in order to compute
                // getMapObject

                // get it ourselves
                
                lastCheckedBiome = biomes[getMapBiomeIndex( x, y )];
                }
            chunkBiomes[ cI ] = lastCheckedBiome;
            

            int numContained;
            int *contained = getContained( x, y, &numContained );

            if( contained != NULL ) {
                containedStackSizes[cI] = numContained;
                containedStacks[cI] = contained;
                }
            else {
                containedStackSizes[cI] = 0;
                containedStacks[cI] = NULL;
                }
            }
        
        }



    SimpleVector<unsigned char> chunkDataBuffer;

    for( int i=0; i<chunkCells; i++ ) {
        
        if( i > 0 ) {
            chunkDataBuffer.appendArray( (unsigned char*)" ", 1 );
            }
        

        char *cell = autoSprintf( "%d:%d", chunkBiomes[i], chunk[i] );
        
        chunkDataBuffer.appendArray( (unsigned char*)cell, strlen(cell) );
        delete [] cell;

        if( containedStacks[i] != NULL ) {
            for( int c=0; c<containedStackSizes[i]; c++ ) {
                char *containedString = autoSprintf( ",%d", 
                                                     containedStacks[i][c] );
        
                chunkDataBuffer.appendArray( (unsigned char*)containedString, 
                                             strlen( containedString ) );
                delete [] containedString;
                }
            delete [] containedStacks[i];
            }
        }
    
    delete [] chunk;
    delete [] chunkBiomes;

    delete [] containedStackSizes;
    delete [] containedStacks;
    

    unsigned char *chunkData = chunkDataBuffer.getElementArray();
    
    int compressedSize;
    unsigned char *compressedChunkData =
        zipCompress( chunkData, chunkDataBuffer.size(),
                     &compressedSize );



    char *header = autoSprintf( "MC\n%d %d %d %d\n%d %d\n#", 
                                chunkDimensionX, chunkDimensionY,
                                startX, startY, chunkDataBuffer.size(),
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
        
        mapETA = Time::timeSec() + newDecayT->autoDecaySeconds;
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



// gets DB slot number where a given container slot's decay time is stored
static int getContainerDecaySlot( int inX, int inY, int inSlot ) {
    int numSlots = getNumContained( inX, inY );

    return FIRST_CONT_SLOT + numSlots + inSlot;
    }



void setSlotEtaDecay( int inX, int inY, int inSlot,
                      timeSec_t inAbsoluteTimeInSeconds ) {
    dbTimePut( inX, inY, getContainerDecaySlot( inX, inY, inSlot ), 
               inAbsoluteTimeInSeconds );
    if( inAbsoluteTimeInSeconds != 0 ) {
        trackETA( inX, inY, inSlot + 1, inAbsoluteTimeInSeconds );
        }
    }


timeSec_t getSlotEtaDecay( int inX, int inY, int inSlot ) {
    // 0 if not found
    return dbTimeGet( inX, inY, getContainerDecaySlot( inX, inY, inSlot ) );
    }






void addContained( int inX, int inY, int inContainedID, 
                   timeSec_t inEtaDecay ) {
    int oldNum;
    

    timeSec_t curTime = Time::timeSec();

    if( inEtaDecay != 0 ) {    
        timeSec_t etaOffset = inEtaDecay - curTime;
        
        inEtaDecay = curTime + 
            etaOffset / getMapContainerTimeStretch( inX, inY );
        }
    
    int *oldContained = getContained( inX, inY, &oldNum );

    timeSec_t *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum );

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
    
    setContained( inX, inY, newNum, newContained );
    setContainedEtaDecay( inX, inY, newNum, newContainedETA );
    
    delete [] newContained;
    delete [] newContainedETA;
    }


int getNumContained( int inX, int inY ) {
    int result = dbGet( inX, inY, NUM_CONT_SLOT );
    
    if( result != -1 ) {
        // found
        return result;
        }
    else {
        // default, empty container
        return 0;
        }
    }






void setContained( int inX, int inY, int inNumContained, int *inContained ) {
    dbPut( inX, inY, NUM_CONT_SLOT, inNumContained );
    for( int i=0; i<inNumContained; i++ ) {
        dbPut( inX, inY, FIRST_CONT_SLOT + i, inContained[i] );
        }
    }


void setContainedEtaDecay( int inX, int inY, int inNumContained, 
                           timeSec_t *inContainedEtaDecay ) {
    for( int i=0; i<inNumContained; i++ ) {
        dbTimePut( inX, inY, FIRST_CONT_SLOT + inNumContained + i, 
                   inContainedEtaDecay[i] );
        
        if( inContainedEtaDecay[i] != 0 ) {
            trackETA( inX, inY, i + 1, inContainedEtaDecay[i] );
            }
        }
    }





int getContained( int inX, int inY, int inSlot ) {
    int num = getNumContained( inX, inY );
    
    if( num == 0 ) {
        return 0;
        }

    if( inSlot == -1 || inSlot > num - 1 ) {
        inSlot = num - 1;
        }
    
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + inSlot );
    
    if( result != -1 ) {    
        return result;
        }
    else {
        // nothing in that slot
        return 0;
        }
    }


    

// removes from top of stack
int removeContained( int inX, int inY, int inSlot, timeSec_t *outEtaDecay ) {
    int num = getNumContained( inX, inY );
    
    if( num == 0 ) {
        return 0;
        }

    if( inSlot == -1 || inSlot > num - 1 ) {
        inSlot = num - 1;
        }
    
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + inSlot );

    timeSec_t curTime = Time::timeSec();
    
    timeSec_t resultEta = dbTimeGet( inX, inY, FIRST_CONT_SLOT + num + inSlot );

    if( resultEta != 0 ) {    
        timeSec_t etaOffset = resultEta - curTime;
        
        etaOffset = etaOffset * getMapContainerTimeStretch( inX, inY );
        
        resultEta = curTime + etaOffset;
        }
    
    *outEtaDecay = resultEta;
    
    int oldNum;
    int *oldContained = getContained( inX, inY, &oldNum );

    timeSec_t *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum );

    SimpleVector<int> newContainedList;
    SimpleVector<timeSec_t> newContainedETAList;
    
    for( int i=0; i<oldNum; i++ ) {
        if( i != inSlot ) {
            newContainedList.push_back( oldContained[i] );
            newContainedETAList.push_back( oldContainedETA[i] );
            }
        }
    
    int *newContained = newContainedList.getElementArray();
    timeSec_t *newContainedETA = newContainedETAList.getElementArray();

    setContained( inX, inY, oldNum - 1, newContained );
    setContainedEtaDecay( inX, inY, oldNum - 1, newContainedETA );
    
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



void clearAllContained( int inX, int inY ) {
    dbPut( inX, inY, NUM_CONT_SLOT, 0 );
    }



void shrinkContainer( int inX, int inY, int inNumNewSlots ) {
    int oldNum = getNumContained( inX, inY );
    
    if( oldNum > inNumNewSlots ) {
        dbPut( inX, inY, NUM_CONT_SLOT, inNumNewSlots );
        }
    }




char *getMapChangeLineString( ChangePosition inPos ) {
    

    SimpleVector<char> buffer;
    

    char *header = autoSprintf( "%d %d ", inPos.x, inPos.y );
    
    buffer.appendElementString( header );
    
    delete [] header;
    

    char *idString = autoSprintf( "%d", getMapObjectNoLook( inPos.x,
                                                            inPos.y ) );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    int numContained;
    int *contained = getContainedNoLook( inPos.x, inPos.y, &numContained );

    for( int i=0; i<numContained; i++ ) {
        
        char *idString = autoSprintf( ",%d", contained[i] );
        
        buffer.appendElementString( idString );
        
        delete [] idString;
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



int getNextDecayDelta() {
    if( liveDecayQueue.size() == 0 ) {
        return -1;
        }
    
    timeSec_t curTime = Time::timeSec();

    timeSec_t minTime = liveDecayQueue.checkMinPriority();
    
    
    if( minTime <= curTime ) {
        return 0;
        }
    
    return minTime - curTime;
    }




void stepMap( SimpleVector<char> *inMapChanges, 
              SimpleVector<ChangePosition> *inChangePosList ) {
    
    timeSec_t curTime = Time::timeSec();

    while( liveDecayQueue.size() > 0 && 
           liveDecayQueue.checkMinPriority() <= curTime ) {
        
        // another expired

        LiveDecayRecord r = liveDecayQueue.removeMin();        

        char storedFound;
        timeSec_t storedETA =
            liveDecayRecordPresentHashTable.lookup( r.x, r.y, r.slot,
                                                    &storedFound );
        
        if( storedFound && storedETA == r.etaTimeSeconds ) {
            
            liveDecayRecordPresentHashTable.remove( r.x, r.y, r.slot );

                    
            timeSec_t lastLookTime =
                liveDecayRecordLastLookTimeHashTable.lookup( r.x, r.y, r.slot,
                                                             &storedFound );

            if( storedFound ) {

                if( Time::timeSec() - lastLookTime > 
                    maxSecondsNoLookDecayTracking ) {
                    
                    // this cell or slot hasn't been looked at in too long
                    // don't even apply this decay now
                    liveDecayRecordLastLookTimeHashTable.remove( 
                        r.x, r.y, r.slot );
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
            checkDecayContained( r.x, r.y );
            }
        
        
        char stillExists;
        liveDecayRecordPresentHashTable.lookup( r.x, r.y, r.slot,
                                                &stillExists );
        
        if( !stillExists ) {
            // cell or slot no longer tracked
            // forget last look time
            liveDecayRecordLastLookTimeHashTable.remove( 
                r.x, r.y, r.slot );
            }
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
        timeSec_t curTime = Time::timeSec();

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
                                  int inNewContainerID ) {
    
    float oldStrech = getObject( inOldContainerID )->slotTimeStretch;
    float newStetch = getObject( inNewContainerID )->slotTimeStretch;
            
    if( oldStrech != newStetch ) {
                
        int oldNum;
        timeSec_t *oldContDecay =
            getContainedEtaDecay( inX, inY, &oldNum );
                
        restretchDecays( oldNum, oldContDecay, 
                         inOldContainerID, inNewContainerID );        
        
        setContainedEtaDecay( inX, inY, oldNum, oldContDecay );
        delete [] oldContDecay;
        }
    }



void getEvePosition( int *outX, int *outY ) {
    
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

            int d = recentPlacements[i].depth;

            double w = pow( d, depthFactor );
            
            weight.push_back( w );
            
            // weighted sum, with deeper objects weighing more
            sum = add( sum, mult( p, w ) );
            
            weightSum += w;
            }
        }

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
    
    printf( "Placing new Eve:  "
            "found an existing camp with %d placements and %f max radius\n",
            pos.size(), maxDist );

    // ave is now center of camp

    // pick point in box according to eve radius

    int currentEveRadius = eveRadius;
    
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

    }




