

void initMap();


void freeMap();


int getChunkDimension();


// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength );



int getMapObject( int inX, int inY );


void setMapObject( int inX, int inY, int inID );
