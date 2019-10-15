#include "minorGems/system/Time.h"


// one int to a 4-byte value
void intToValue( int inV, unsigned char *outValue );


int valueToInt( unsigned char *inValue );


// converts any length email to a 50-byte key
// outKey must be pre-allocated to 50 bytes
void emailToKey( const char *inEmail, unsigned char *outKey );


// one timeSec_t to an 8-byte double value
void timeToValue( timeSec_t inT, unsigned char *outValue );


// inValue is 8 bytes
timeSec_t valueToTime( unsigned char *inValue );



// four ints to a 16-byte key
void intQuadToKey( int inX, int inY, int inSlot, int inB, 
                   unsigned char *outKey );


// two ints to an 8-byte key
void intPairToKey( int inX, int inY, unsigned char *outKey );

