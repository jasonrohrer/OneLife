#include "map.h"


#include "minorGems/util/random/JenkinsRandomSource.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/formats/encodingUtils.h"

#include "kissdb.h"

#include <stdarg.h>






static int chunkDimension = 32;


static int startingObjectID = 26;



static KISSDB db;
static char dbOpen = false;


static int randSeed = 10;
static JenkinsRandomSource randSource;



static float getXYRandom( int inX, int inY ) {
    
    unsigned int fullSeed = inX ^ (inY * 57) ^ ( randSeed * 131 );
    
    randSource.reseed( fullSeed );
    
    return randSource.getRandomFloat();
    }


int getChunkDimension() {
    return chunkDimension;
    }







// gets procedurally-generated base map at a given spot
// player modifications are overlayed on top of this
static int getBaseMap( int inX, int inY ) {
    
    float randValue = getXYRandom( inX, inY );
    
    if( randValue < 0.1 ) {
        return startingObjectID;
        }
    return 0;
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





void initMap() {


    int error = KISSDB_open( &db, 
                             "map.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             80000,
                             12, // three 32-bit ints, xys
                                 // s is the slot number 
                                 // s=0 for base object
                                 // s=1 for count of contained objects
                                 // s=2 first contained object
                                 // s=3 second contained object
                                 // s=... remaining contained objects
                             4 // one int, object ID at x,y in slot s
                               // OR contained count if s=1
                             );
    
    if( error ) {
        printf( "Error %d opening KissDB\n", error );
        return;
        }
    
    dbOpen = true;
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



int getMapObject( int inX, int inY ) {    

    int result = dbGet( inX, inY, 0 );
    
    if( result != -1 ) {
        // found
        return result;
        }
    else {
        return getBaseMap( inX, inY );
        }
    }



// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength ) {
    
    int chunkCells = chunkDimension * chunkDimension;
    
    int *chunk = new int[chunkCells];
    

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
        }
    delete [] chunk;

    
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
    }





void addContained( int inX, int inY, int inContainedID ) {
    int oldNum = getNumContained( inX, inY );
    
    int newNum = oldNum + 1;
    

    dbPut( inX, inY, 1 + newNum, inContainedID );
    
    dbPut( inX, inY, 1, newNum );
    }


int getNumContained( int inX, int inY ) {
    int result = dbGet( inX, inY, 1 );
    
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
        int result = dbGet( inX, inY, 2 + i );
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
    
    int result = dbGet( inX, inY, 1 + num );
    
    // shrink number of slots
    num -= 1;
    dbPut( inX, inY, 1, num );
        

    if( result != -1 ) {    
        return result;
        }
    else {
        // nothing in that slot
        return 0;
        }
    }



