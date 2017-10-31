#include "dbCommon.h"

#include <string.h>



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


void emailToKey( char *inEmail, unsigned char *outKey ) {
    memset( outKey, ' ', 50 );
    
    int len = 50;

    int emailLen = strlen( inEmail );
    
    if( emailLen < len ) {
        len = emailLen;
        }
    
    memcpy( outKey, inEmail, len );
    }
