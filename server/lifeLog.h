

void initLifeLog();


void freeLifeLog();



void logBirth( int inPlayerID, char *inPlayerEmail,
               int inMapX, int inMapY );

// killer email NULL if died of natural causes
void logDeath( int inPlayerID, char *inPlayerEmail,
               int inMapX, int inMapY,  int inKillerID = -1, 
               char *inKillerEmail = NULL );


