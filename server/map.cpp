#include "map.h"
#include "HashTable.h"


#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"


#include "kissdb.h"

#include <stdarg.h>
#include <math.h>
#include <values.h>


#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"




static int chunkDimension = 22;


static int currentResponsiblePlayer = -1;


void setResponsiblePlayer( int inPlayerID ) {
    currentResponsiblePlayer = inPlayerID;
    }




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


static int randSeed = 124567;
//static JenkinsRandomSource randSource( randSeed );
static CustomRandomSource randSource( randSeed );



#define DECAY_SLOT 1
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3

// decay slots for contained items start after container slots


// 15 minutes
static unsigned int maxSecondsForActiveDecayTracking = 900;

// 1 minute
static unsigned int maxSecondsNoLookDecayTracking = 10;


typedef struct LiveDecayRecord {
        int x, y;
        
        // 0 means main object decay
        // 1 - NUM_CONT_SLOT means contained object decay
        int slot;
        
        unsigned int etaTimeSeconds;
    } LiveDecayRecord;



#include "minorGems/util/MinPriorityQueue.h"

static MinPriorityQueue<LiveDecayRecord> liveDecayQueue;


// for quick lookup of existing records in liveDecayQueue
// store the eta time here
// before storing a new record in the queue, we can check this hash
// table to see whether it already exists
static HashTable<unsigned int> liveDecayRecordPresentHashTable( 1024 );

// times in seconds that a tracked live decay map cell or slot
// was last looked at
static HashTable<unsigned int> liveDecayRecordLastLookTimeHashTable( 1024 );



// track all map changes that happened since the last
// call to stepMap
static SimpleVector<ChangePosition> mapChangePosSinceLastStep;






#include "../commonSource/fractalNoise.h"







int getChunkDimension() {
    return chunkDimension;
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
    
    
    return pickedBiome;
    }




// gets procedurally-generated base map at a given spot
// player modifications are overlayed on top of this

// SIDE EFFECT:
// if biome at x,y needed to be determined in order to compute map
// at this spot, it is saved into lastCheckedBiome

static int lastCheckedBiome = -1;

static int getBaseMap( int inX, int inY ) {
    // first step:  save rest of work if density tells us that
    // nothing is here anyway
    double density = getXYFractal( inX, inY, 0.1, 1 );
    
    // correction
    density = sigmoid( density, 0.1 );
    
    // scale
    density *= .4;
    //density = 1;


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





// three ints to an 12-byte key
void intTripleToKey( int inX, int inY, int inSlot, unsigned char *outKey ) {
    for( int i=0; i<4; i++ ) {
        int offset = i * 8;
        outKey[i] = ( inX >> offset ) & 0xFF;
        outKey[i+4] = ( inY >> offset ) & 0xFF;
        outKey[i+8] = ( inSlot >> offset ) & 0xFF;
        }    
    }

// one int to an 4-byte value
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



void initMap() {


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
                             4 // one int, object ID at x,y in slot s
                               // OR contained count if s=1
                             );
    
    if( error ) {
        AppLog::errorF( "Error %d opening KissDB", error );
        return;
        }
    
    dbOpen = true;

    

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
            unsigned int *decay = getContainedEtaDecay( x, y, &numCont );
            
            SimpleVector<int> newCont;
            SimpleVector<unsigned int> newDecay;
            
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
            unsigned int *newDecayArray = newDecay.getElementArray();
            
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
        KISSDB_close( &db );
        }
    
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



static void dbPut( int inX, int inY, int inSlot, int inValue ) {
    char found = false;
    for( int i=0; i<mapChangePosSinceLastStep.size(); i++ ) {
        
        ChangePosition *p = mapChangePosSinceLastStep.getElement( i );
        
        if( p->x == inX && p->y == inY ) {
            found = true;
            
            // update it
            p->responsiblePlayerID = currentResponsiblePlayer;
            }
        }
    
    if( ! found ) {
        ChangePosition p = { inX, inY, false, currentResponsiblePlayer };
        mapChangePosSinceLastStep.push_back( p );
        }
    
    unsigned char key[12];
    unsigned char value[4];
    

    intTripleToKey( inX, inY, inSlot, key );
    intToValue( inValue, value );
            
    
    KISSDB_put( &db, key, value );
    }



// slot is 0 for main map cell, or higher for container slots
static void trackETA( int inX, int inY, int inSlot, unsigned int inETA ) {
    unsigned int timeLeft = inETA - time( NULL );
        
    if( timeLeft < maxSecondsForActiveDecayTracking ) {
        // track it live
            
        // duplicates okay
        // we'll deal with them when they ripen
        // (we check the true ETA stored in map before acting
        //   on one stored in this queue)
        LiveDecayRecord r = { inX, inY, inSlot, inETA };
            
        char exists;
        unsigned int existingETA =
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
                                                             time( NULL ) );
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
    unsigned int currentTime = time( NULL );
    
    for( int i=0; i<*outNumContained; i++ ) {
        char found;
        unsigned int *oldLookTime =
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



unsigned int *getContainedEtaDecay( int inX, int inY, int *outNumContained ) {
    int num = getNumContained( inX, inY );

    *outNumContained = num;
    
    if( num == 0 ) {
        return NULL;
        }
   
    unsigned int *containedEta = new unsigned int[ num ];

    for( int i=0; i<num; i++ ) {
        int result = dbGet( inX, inY, FIRST_CONT_SLOT + num + i );
        if( result != -1 ) {
            containedEta[i] = (unsigned int)result;
            }
        else {
            containedEta[i] = 0;
            }
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

    // is eta stored in map?
    unsigned int mapETA = getEtaDecay( inX, inY );
    
    if( mapETA != 0 ) {
        
        if( (int)mapETA <= time( NULL ) ) {
            
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
            
            // set it in DB
            dbPut( inX, inY, 0, newID );
            




            TransRecord *newDecayT = getTrans( -1, newID );

            if( newDecayT != NULL ) {

                // add some random variation to avoid lock-step
                // especially after a server restart
                int tweakedSeconds = 
                    randSource.getRandomBoundedInt( 
                        newDecayT->autoDecaySeconds * 0.9, 
                        newDecayT->autoDecaySeconds );
                
                if( tweakedSeconds < 1 ) {
                    tweakedSeconds = 1;
                    }
                mapETA = time(NULL) + tweakedSeconds;
                }
            else {
                // no further decay
                mapETA = 0;
                }

            setEtaDecay( inX, inY, mapETA );
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
        
        mapETA = time( NULL ) + decayTime;
            
        setEtaDecay( inX, inY, mapETA );
        }
    

    if( mapETA != 0 ) {
        trackETA( inX, inY, 0, mapETA );
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
    SimpleVector<unsigned int> newDecayEta;
    
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
        unsigned int mapETA = getSlotEtaDecay( inX, inY, i );
    
        if( mapETA != 0 ) {
        
            if( (int)mapETA <= time( NULL ) ) {
            
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
                                newDecayT->autoDecaySeconds * 0.9, 
                                newDecayT->autoDecaySeconds );

                        if( tweakedSeconds < 1 ) {
                            tweakedSeconds = 1;
                            }
                        
                        mapETA = 
                            time(NULL) +
                            lrint( tweakedSeconds / 
                                   getMapContainerTimeStretch( inX, inY ) );
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
            unsigned int mapETA = newDecayEta.getElementDirect( i );
            
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




int getMapObject( int inX, int inY ) {

    // look at this map cell
    char found;
    unsigned int *oldLookTime = 
        liveDecayRecordLastLookTimeHashTable.lookupPointer( inX, inY, 0,
                                                            &found );
    
    if( oldLookTime != NULL ) {
        // we're tracking decay for this cell
        *oldLookTime = time( NULL );
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
    
    int chunkCells = chunkDimension * chunkDimension;
    
    int *chunk = new int[chunkCells];

    int *chunkBiomes = new int[chunkCells];
    
    int *containedStackSizes = new int[ chunkCells ];
    int **containedStacks = new int*[ chunkCells ];

    // 0,0 is center of map
    
    int halfChunk = chunkDimension /2;
    

    int startY = inCenterY - halfChunk;
    int startX = inCenterX - halfChunk;
    
    int endY = startY + chunkDimension;
    int endX = startX + chunkDimension;

    
    
    for( int y=startY; y<endY; y++ ) {
        int chunkY = y - startY;
        

        for( int x=startX; x<endX; x++ ) {
            int chunkX = x - startX;
            
            int cI = chunkY * chunkDimension + chunkX;
            
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



    char *header = autoSprintf( "MC\n%d %d %d\n%d %d\n#", chunkDimension,
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

    // disable any old decay
    setEtaDecay( inX, inY, 0 );
    
    // don't set up new decay here
    // only set it up on the next get
    // (if no one is looking at the object currently, it doesn't decay)
    // decay is only set up when someone looks at it
    //
    // NOTE that after setting a new map object, a get almost always follows
    // so we have "decay setup on get only" to avoid duplicate decay setups

    // also, when chosing between setting up decay on a set and setting
    // it up on a get, we have to chose get, because there are loads
    // of gets that have no set (for example, getting a map chunk)
    }




void setEtaDecay( int inX, int inY, unsigned int inAbsoluteTimeInSeconds ) {
    dbPut( inX, inY, DECAY_SLOT, (int)inAbsoluteTimeInSeconds );
    if( inAbsoluteTimeInSeconds != 0 ) {
        trackETA( inX, inY, 0, inAbsoluteTimeInSeconds );
        }
    }




unsigned int getEtaDecay( int inX, int inY ) {
    int value = dbGet( inX, inY, DECAY_SLOT );
    
    if( value != -1 ) {
        return (unsigned int)value;
        }
    else {
        return 0;
        }
    
    }



// gets DB slot number where a given container slot's decay time is stored
static int getContainerDecaySlot( int inX, int inY, int inSlot ) {
    int numSlots = getNumContained( inX, inY );

    return FIRST_CONT_SLOT + numSlots + inSlot;
    }



void setSlotEtaDecay( int inX, int inY, int inSlot,
                      unsigned int inAbsoluteTimeInSeconds ) {

    dbPut( inX, inY, getContainerDecaySlot( inX, inY, inSlot ), 
           (int)inAbsoluteTimeInSeconds );
    if( inAbsoluteTimeInSeconds != 0 ) {
        trackETA( inX, inY, inSlot + 1, inAbsoluteTimeInSeconds );
        }
    }


unsigned int getSlotEtaDecay( int inX, int inY, int inSlot ) {
    int value = dbGet( inX, inY, getContainerDecaySlot( inX, inY, inSlot ) );
    
    if( value != -1 ) {
        return (unsigned int)value;
        }
    else {
        return 0;
        }
    }






void addContained( int inX, int inY, int inContainedID, 
                   unsigned int inEtaDecay ) {
    int oldNum;
    

    unsigned int curTime = time( NULL );

    if( inEtaDecay != 0 ) {    
        int etaOffset = inEtaDecay - curTime;
        
        inEtaDecay = curTime + 
            lrint( etaOffset / getMapContainerTimeStretch( inX, inY ) );
        }
    
    int *oldContained = getContained( inX, inY, &oldNum );

    unsigned int *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum );

    int *newContained = new int[ oldNum + 1 ];
    
    if( oldNum != 0 ) {
        memcpy( newContained, oldContained, oldNum * sizeof( int ) );
        }
    
    newContained[ oldNum ] = inContainedID;
    
    if( oldContained != NULL ) {
        delete [] oldContained;
        }
    
    unsigned int *newContainedETA = new unsigned int[ oldNum + 1 ];
    
    if( oldNum != 0 ) {    
        memcpy( newContainedETA, 
                oldContainedETA, oldNum * sizeof( unsigned int ) );
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
                           unsigned int *inContainedEtaDecay ) {
    for( int i=0; i<inNumContained; i++ ) {
        dbPut( inX, inY, FIRST_CONT_SLOT + inNumContained + i, 
               (int)( inContainedEtaDecay[i] ) );
        
        if( inContainedEtaDecay[i] != 0 ) {
            trackETA( inX, inY, i + 1, inContainedEtaDecay[i] );
            }
        }
    }


    

// removes from top of stack
int removeContained( int inX, int inY, int inSlot, unsigned int *outEtaDecay ) {
    int num = getNumContained( inX, inY );
    
    if( num == 0 ) {
        return 0;
        }

    if( inSlot == -1 || inSlot > num - 1 ) {
        inSlot = num - 1;
        }
    
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + inSlot );

    unsigned int curTime = time(NULL);
    
    unsigned int resultEta = dbGet( inX, inY, FIRST_CONT_SLOT + num + inSlot );

    if( resultEta != 0 ) {    
        int etaOffset = resultEta - curTime;
        
        etaOffset = lrint( etaOffset * getMapContainerTimeStretch( inX, inY ) );
        
        resultEta = curTime + etaOffset;
        }
    
    *outEtaDecay = resultEta;
    
    int oldNum;
    int *oldContained = getContained( inX, inY, &oldNum );

    unsigned int *oldContainedETA = getContainedEtaDecay( inX, inY, &oldNum );

    SimpleVector<int> newContainedList;
    SimpleVector<unsigned int> newContainedETAList;
    
    for( int i=0; i<oldNum; i++ ) {
        if( i != inSlot ) {
            newContainedList.push_back( oldContained[i] );
            newContainedETAList.push_back( oldContainedETA[i] );
            }
        }
    
    int *newContained = newContainedList.getElementArray();
    unsigned int *newContainedETA = newContainedETAList.getElementArray();

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




char *getMapChangeLineString( int inX, int inY,
                              int inResponsiblePlayerID ) {
    

    SimpleVector<char> buffer;
    

    char *header = autoSprintf( "%d %d ", inX, inY );
    
    buffer.appendElementString( header );
    
    delete [] header;
    

    char *idString = autoSprintf( "%d", getMapObjectNoLook( inX, inY ) );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    int numContained;
    int *contained = getContainedNoLook( inX, inY, &numContained );

    for( int i=0; i<numContained; i++ ) {
        
        char *idString = autoSprintf( ",%d", contained[i] );
        
        buffer.appendElementString( idString );
        
        delete [] idString;
        }
    
    if( contained != NULL ) {
        delete [] contained;
        }
    

    char *player = autoSprintf( " %d", inResponsiblePlayerID );
    
    buffer.appendElementString( player );
    
    delete [] player;

    
    buffer.appendElementString( "\n" );

    return buffer.getElementString();
    }



int getNextDecayDelta() {
    if( liveDecayQueue.size() == 0 ) {
        return -1;
        }
    
    unsigned int curTime = time( NULL );

    unsigned int minTime = liveDecayQueue.checkMinPriority();
    
    
    if( minTime <= curTime ) {
        return 0;
        }
    
    return minTime - curTime;
    }




void stepMap( SimpleVector<char> *inMapChanges, 
              SimpleVector<ChangePosition> *inChangePosList ) {
    
    unsigned int curTime = time( NULL );

    while( liveDecayQueue.size() > 0 && 
           liveDecayQueue.checkMinPriority() <= curTime ) {
        
        // another expired

        LiveDecayRecord r = liveDecayQueue.removeMin();        

        char storedFound;
        unsigned int storedETA =
            liveDecayRecordPresentHashTable.lookup( r.x, r.y, r.slot,
                                                    &storedFound );
        
        if( storedFound && storedETA == r.etaTimeSeconds ) {
            
            liveDecayRecordPresentHashTable.remove( r.x, r.y, r.slot );

                    
            unsigned int lastLookTime =
                liveDecayRecordLastLookTimeHashTable.lookup( r.x, r.y, r.slot,
                                                             &storedFound );

            if( storedFound ) {

                if( time(NULL) - lastLookTime > 
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
        
        char *changeString = getMapChangeLineString( p.x, p.y,
                                                     p.responsiblePlayerID );
        inMapChanges->appendElementString( changeString );
        delete [] changeString;
        }

    
    mapChangePosSinceLastStep.deleteAll();
    }




void restretchDecays( int inNumDecays, unsigned int *inDecayEtas,
                      int inOldContainerID, int inNewContainerID ) {
    
    float oldStrech = getObject( inOldContainerID )->slotTimeStretch;
    float newStetch = getObject( inNewContainerID )->slotTimeStretch;
            
    if( oldStrech != newStetch ) {
        unsigned int curTime = time( NULL );

        for( int i=0; i<inNumDecays; i++ ) {
            if( inDecayEtas[i] != 0 ) {
                int offset = inDecayEtas[i] - curTime;
                        
                offset = lrint( offset * oldStrech );
                offset = lrint( offset / newStetch );
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
        unsigned int *oldContDecay =
            getContainedEtaDecay( inX, inY, &oldNum );
                
        restretchDecays( oldNum, oldContDecay, 
                         inOldContainerID, inNewContainerID );        
        
        setContainedEtaDecay( inX, inY, oldNum, oldContDecay );
        delete [] oldContDecay;
        }
    }



