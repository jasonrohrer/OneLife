#include "dbCommon.h"

#include <string.h>

#include <stdint.h>



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


void emailToKey( const char *inEmail, unsigned char *outKey ) {
    memset( outKey, ' ', 50 );
    
    int len = 50;

    int emailLen = strlen( inEmail );
    
    if( emailLen < len ) {
        len = emailLen;
        }
    
    memcpy( outKey, inEmail, len );
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
