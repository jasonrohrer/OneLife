

void initMap();


void freeMap();


int getChunkDimension();


// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength );



int getMapObject( int inX, int inY );


void setMapObject( int inX, int inY, int inID );


// adds to top of stack
void addContained( int inX, int inY, int inContainedID );

int getNumContained( int inX, int inY );

// destroyed by caller, returns NULL if empty
int *getContained( int inX, int inY, int *outNumContained );

// removes from top of stack
int removeContained( int inX, int inY );

void clearAllContained( int inX, int inY );

// if inNumNewSlots less than number contained, the excess are discarded
void shrinkContainer( int inX, int inY, int inNumNewSlots );


// line for a map change message
char *getMapChangeLineString( int inX, int inY  );
