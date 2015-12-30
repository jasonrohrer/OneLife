#include "map.h"
#include "HashTable.h"


#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"


#include "kissdb.h"

#include <stdarg.h>
#include <math.h>


#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"




static int chunkDimension = 16;




// object ids that occur naturally on map at random
static SimpleVector<int> naturalMapIDs;
static SimpleVector<float> naturalMapChances;

static float totalChanceWeight;




static KISSDB db;
static char dbOpen = false;


static int randSeed = 124567;
//static JenkinsRandomSource randSource( randSeed );
static CustomRandomSource randSource( randSeed );



#define DECAY_SLOT 1
#define NUM_CONT_SLOT 2
#define FIRST_CONT_SLOT 3


// 15 minutes
static int maxSecondsForActiveDecayTracking = 900;


typedef struct LiveDecayRecord {
        int x, y;
        unsigned int etaTimeSeconds;
    } LiveDecayRecord;



#include "minorGems/util/MinPriorityQueue.h"

static MinPriorityQueue<LiveDecayRecord> liveDecayQueue;


// for quick lookup of existing records in liveDecayQueue
// store the eta time here
// before storing a new record in the queue, we can check this hash
// table to see whether it already exists
static HashTable<unsigned int> liveDecayRecordPresentHashTable( 1024 );



// track decay-caused map transitions that happened since the last
// call to stepMap
static SimpleVector<char> mapChangesSinceLastStep;

static SimpleVector<ChangePosition> mapChangePosSinceLastStep;




// in 0..1
static double getXYRandom( int inX, int inY ) {
    
    unsigned int fullSeed = inX ^ (inY * 57) ^ ( randSeed * 131 );
    
    randSource.reseed( fullSeed );
    
    return randSource.getRandomDouble();
    }


// in -1..1
static double getXYRandomN( int inX, int inY ) {
    
    unsigned int fullSeed = inX ^ (inY * 57) ^ ( randSeed * 131 );
    
    randSource.reseed( fullSeed );
    
    return 2 * randSource.getRandomDouble() - 1;
    }



// in -1..1
// interpolated for inX,inY that aren't integers
static double getXYRandomBN( double inX, double inY ) {
    
    int floorX = (int)floor(inX);
    int ceilX = (int)ceil(inX);
    int floorY = (int)floor(inY);
    int ceilY = (int)ceil(inY);
    

    double cornerA1 = getXYRandomN( floorX, floorY );
    double cornerA2 = getXYRandomN( ceilX, floorY );

    double cornerB1 = getXYRandomN( floorX, ceilY );
    double cornerB2 = getXYRandomN( ceilX, ceilY );
    

    double xOffset = inX - floorX;
    double yOffset = inY - floorY;
    
    
    double topBlend = cornerA2 * xOffset + (1-xOffset) * cornerA1;
    
    double bottomBlend = cornerB2 * xOffset + (1-xOffset) * cornerB1;
    

    return bottomBlend * yOffset + (1-yOffset) * topBlend;
    }






int getChunkDimension() {
    return chunkDimension;
    }



double getXYFractal( int inX, int inY, double inRoughness, int inScale ) {

    double b = inRoughness;
    double a = 1 - b;

    double sum =
        a * getXYRandomN( inX / (32 * inScale), inY / (32 * inScale) )
        +
        b * (
            a * getXYRandomN( inX / (16 * inScale), inY / (16 * inScale) )
            +
            b * (
                a * getXYRandomN( inX / (8 * inScale), 
                                   inY / (8 * inScale) )
                +
                b * (
                    a * getXYRandomN( inX / (4 * inScale), 
                                       inY / (4 * inScale) )
                    +
                    b * (
                        a * getXYRandomN( inX / (2 * inScale), 
                                           inY / (2 * inScale) )
                        +
                        b * (
                            getXYRandomN( inX / inScale, inY / inScale )
                            ) ) ) ) );
    
    return ( sum + 1 ) * 0.5;
    }




double getXYFractalB( int inX, int inY, double inRoughness, double inScale ) {

    double b = inRoughness;
    double a = 1 - b;

    double sum =
        a * getXYRandomBN( inX / (32 * inScale), inY / (32 * inScale) )
        +
        b * (
            a * getXYRandomBN( inX / (16 * inScale), inY / (16 * inScale) )
            +
            b * (
                a * getXYRandomBN( inX / (8 * inScale), 
                                   inY / (8 * inScale) )
                +
                b * (
                    a * getXYRandomBN( inX / (4 * inScale), 
                                       inY / (4 * inScale) )
                    +
                    b * (
                        a * getXYRandomBN( inX / (2 * inScale), 
                                           inY / (2 * inScale) )
                        +
                        b * (
                            getXYRandomBN( inX / inScale, inY / inScale )
                            ) ) ) ) );
    
    return ( sum + 1 ) * 0.5;
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




// gets procedurally-generated base map at a given spot
// player modifications are overlayed on top of this
static int getBaseMap( int inX, int inY ) {

    double density = getXYFractalB( inX, inY, 0.1, 1 );
    
    // correction
    density = sigmoid( density, 0.1 );
    
    // scale
    density *= .4;
    //density = 1;


    int numObjects = naturalMapIDs.size();

    if( numObjects > 0 && 
        getXYRandom( 287 + inX, 383 + inY ) < density ) {
    
  
        // something present here

        
        // special object in this region is 10x more common than it 
        // would be otherwise

        double randVal = getXYFractalB(  123 + inX, 753 + inY, 0.3, 1 );
        

        
        
        

        // expand range around 0.5
        double expandFactor = 4;
        
        randVal = ( randVal - 0.5 ) * expandFactor + 0.5;
        
        if( randVal < 0 ) {
            randVal = 0;
            }
        else if( randVal > 1 ) {
            randVal = 1;
            }
        

        int specialObjectIndex =
            lrint( ( numObjects - 1 ) * randVal );
        


        // jump randomly amoung objects to avoid consistent banding
        
        // if we pick our seed for jumping using a fractal that is low
        // frequency across the map, we'll get a jump that changes 
        // infrequently,
        // but with discontinuities
        int randJumpSeed = (int)( 5 * 
                                  getXYFractalB(  3123 + inX, 
                                                  9753 + inY, 0.3, 6 ) );

        randSource.reseed( 1457 + 327 * randJumpSeed + 
                           293 * specialObjectIndex );
    
        // pick a large value and wrap around
        int jump = randSource.getRandomBoundedInt( 0, numObjects * 10 );

        specialObjectIndex += jump;
        
        specialObjectIndex = specialObjectIndex % numObjects;




        float oldSpecialChance = 
            naturalMapChances.getElementDirect( specialObjectIndex );
        
        float newSpecialChance = oldSpecialChance * 10;
        
        *( naturalMapChances.getElement( specialObjectIndex ) )
            = newSpecialChance;
        
        float oldTotalChanceWeight = totalChanceWeight;
        
        totalChanceWeight -= oldSpecialChance;
        totalChanceWeight += newSpecialChance;
        

        // pick one of our natural objects at random

        // pick value between 0 and total weight

        double randValue = totalChanceWeight * getXYRandom( inX, inY );

        // walk through objects, summing weights, until one crosses threshold
        int i = 0;
        float weightSum = 0;        
        
        while( weightSum < randValue && i < numObjects ) {
            weightSum += naturalMapChances.getElementDirect( i );
            i++;
            }
        
        i--;
        

        // restore chance of special object
        *( naturalMapChances.getElement( specialObjectIndex ) )
            = oldSpecialChance;

        totalChanceWeight = oldTotalChanceWeight;

        if( i >= 0 ) {
            return naturalMapIDs.getElementDirect( i );
            }
        else {
            return 0;
            }
        }
    else {
        return 0;
        }
    
    }





// two ints to an 8-byte key
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
    for( int i=0; i<naturalMapIDs.size(); i++ ) {
        randSource.getRandomFloat();
        Color *c = Color::makeColorFromHSV( (float)i / naturalMapIDs.size(),
                                            1, 1 );
        objColors.push_back( *c );
        delete c;
        }

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

    for( int y = 0; y<h; y++ ) {
        
        for( int x = 0; x<w; x++ ) {
            int id = getBaseMap( x, y );
    
            if( id > 0 ) {
                for( int i=0; i<naturalMapIDs.size(); i++ ) {
                    if( naturalMapIDs.getElementDirect(i) == id ) {
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
    }






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
        printf( "Error %d opening KissDB\n", error );
        return;
        }
    
    dbOpen = true;

    

    int numObjects;
    ObjectRecord **allObjects = getAllObjects( &numObjects );
    
    totalChanceWeight = 0;
    
    for( int i=0; i<numObjects; i++ ) {
        ObjectRecord *o = allObjects[i];
        
        float p = o->mapChance;
        if( p > 0 ) {
            
            naturalMapIDs.push_back( o->id );
            naturalMapChances.push_back( p );
            
            totalChanceWeight += p;
            }
        }
    printf( "Found %d natural objects with total weight %f\n",
            naturalMapIDs.size(), totalChanceWeight );

    delete [] allObjects;



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
    unsigned char key[12];
    unsigned char value[4];
    

    intTripleToKey( inX, inY, inSlot, key );
    intToValue( inValue, value );
            
    
    KISSDB_put( &db, key, value );
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
        
        if( (int)mapETA < time( NULL ) ) {
            
            // object in map has decayed (eta expired)

            // apply the transition
            newID = t->newTarget;
            

            int oldSlots = getNumContainerSlots( inID );

            int newSlots = getNumContainerSlots( newID );
            
            if( newSlots < oldSlots ) {
                shrinkContainer( inX, inY, newSlots );
                }
            
            // set it in DB
            dbPut( inX, inY, 0, newID );
            

            char *changeString = getMapChangeLineString( inX, inY );
            
            mapChangesSinceLastStep.appendElementString( changeString );

            delete [] changeString;
            

            ChangePosition p = { inX, inY, false };
            
            mapChangePosSinceLastStep.push_back( p );



            TransRecord *newDecayT = getTrans( -1, newID );

            if( newDecayT != NULL ) {
                mapETA = time(NULL) + newDecayT->autoDecaySeconds;
                }
            else {
                // no further decay
                mapETA = 0;
                }

            setEtaDecay( inX, inY, mapETA );
            }

        }
    else {
        // update map with decay for the applicable transition
        mapETA = time( NULL ) + t->autoDecaySeconds;
        
        setEtaDecay( inX, inY, mapETA );
        }
    

    if( mapETA != 0 ) {
        int timeLeft = mapETA - time( NULL );
        
        if( timeLeft < maxSecondsForActiveDecayTracking ) {
            // track it live
            
            // duplicates okay
            // we'll deal with them when they ripen
            // (we check the true ETA stored in map before acting
            //   on one stored in this queue)
            LiveDecayRecord r = { inX, inY, mapETA };
            
            char exists;
            unsigned int existingETA =
                liveDecayRecordPresentHashTable.lookup( inX, inY,
                                                        &exists );

            if( !exists || existingETA != mapETA ) {
                
                liveDecayQueue.insert( r, mapETA );
                
                liveDecayRecordPresentHashTable.insert( inX, inY, mapETA );
                }
            }
        
        }
    
    
    return newID;
    }



int getMapObjectRaw( int inX, int inY ) {
    int result = dbGet( inX, inY, 0 );
    
    if( result == -1 ) {
        // nothing in map
        result = getBaseMap( inX, inY );
        }
    
    return result;
    }




int getMapObject( int inX, int inY ) {

    // apply any decay that should have happened by now
    return checkDecayObject( inX, inY, getMapObjectRaw( inX, inY ) );
    }



// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength ) {
    
    int chunkCells = chunkDimension * chunkDimension;
    
    int *chunk = new int[chunkCells];
    
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
            
            chunk[cI] = getMapObject( x, y );

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
        

        char *cell = autoSprintf( "%d", chunk[i] );
        
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





void addContained( int inX, int inY, int inContainedID ) {
    int oldNum = getNumContained( inX, inY );
    
    int newNum = oldNum + 1;
    

    dbPut( inX, inY, FIRST_CONT_SLOT + newNum - 1, inContainedID );
    
    dbPut( inX, inY, NUM_CONT_SLOT, newNum );
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



int *getContained( int inX, int inY, int *outNumContained ) {
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

    

// removes from top of stack
int removeContained( int inX, int inY ) {
    int num = getNumContained( inX, inY );
    
    if( num == 0 ) {
        return 0;
        }
    
    int result = dbGet( inX, inY, FIRST_CONT_SLOT + num - 1 );
    
    // shrink number of slots
    num -= 1;
    dbPut( inX, inY, NUM_CONT_SLOT, num );
        

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
    

    char *idString = autoSprintf( "%d", getMapObject( inX, inY ) );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    int numContained;
    int *contained = getContained(  inX, inY, &numContained );

    if( numContained > 0 ) {
        for( int i=0; i<numContained; i++ ) {
            
            char *idString = autoSprintf( ",%d", contained[i] );
    
            buffer.appendElementString( idString );
    
            delete [] idString;
            }
        
        delete [] contained;
        }
    

    char *player = autoSprintf( " %d", inResponsiblePlayerID );
    
    buffer.appendElementString( player );
    
    delete [] player;

    
    buffer.appendElementString( "\n" );

    return buffer.getElementString();
    }




void stepMap( SimpleVector<char> *inMapChanges, 
              SimpleVector<ChangePosition> *inChangePosList ) {
    
    unsigned int curTime = time( NULL );

    while( liveDecayQueue.size() > 0 && 
           liveDecayQueue.checkMinPriority() < curTime ) {
        
        // another expired

        LiveDecayRecord r = liveDecayQueue.removeMin();        

        char storedFound;
        unsigned int storedETA =
            liveDecayRecordPresentHashTable.lookup( r.x, r.y, &storedFound );
        
        if( storedFound && storedETA == r.etaTimeSeconds ) {
            
            liveDecayRecordPresentHashTable.remove( r.x, r.y );
            }
        

        int oldID = getMapObjectRaw( r.x, r.y );

        // apply real eta from map (to ignore stale duplicates in live list)
        // and update live list if new object is decaying too
        

        // this call will append changes to our global lists, which
        // we process below
        checkDecayObject( r.x, r.y, oldID );
        }
    

    // all of them, including these new ones and others acculuated since
    // last step are accumulated in these global vectors
    
    int numChars = mapChangesSinceLastStep.size();
    
    for( int i=0; i<numChars; i++ ) {
        inMapChanges->push_back( mapChangesSinceLastStep.getElementDirect(i) );
        }
    
    int numPos = mapChangePosSinceLastStep.size();

    for( int i=0; i<numPos; i++ ) {
        inChangePosList->push_back( 
            mapChangePosSinceLastStep.getElementDirect(i) );
        }

    
    mapChangesSinceLastStep.deleteAll();
    mapChangePosSinceLastStep.deleteAll();
    }


