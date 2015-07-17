

void initMap();


void freeMap();



// returns properly formatted chunk message for chunk centered
// around x,y
char *getChunkMessage( int inCenterX, int inCenterY );



int getMapObject( int inX, int inY );


void setMapObject( int inX, int inY, int inID );
