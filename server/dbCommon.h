// one int to a 4-byte value
void intToValue( int inV, unsigned char *outValue );


int valueToInt( unsigned char *inValue );


// converts any length email to a 50-byte key
// outKey must be pre-allocated to 50 bytes
void emailToKey( char *inEmail, unsigned char *outKey );
