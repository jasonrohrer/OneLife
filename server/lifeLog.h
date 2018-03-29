

double getAgeRate();


void initLifeLog();


void freeLifeLog();



void logBirth( int inPlayerID, char *inPlayerEmail,
               int inParentID, char *inParentEmail,
               char inIsMale,
               int inMapX, int inMapY,
               int inTotalPopulation,
               int inParentChainLength );


// killer email NULL if died of natural causes
void logDeath( int inPlayerID, char *inPlayerEmail,
               char inEve,
               double inAge,
               int inSecPlayed,
               char inIsMale,
               int inMapX, int inMapY,  
               int inTotalRemainingPopulation,
               char inDisconnect = false,
               int inKillerID = -1, 
               char *inKillerEmail = NULL );


void logName( int inPlayerID, char *inName );

