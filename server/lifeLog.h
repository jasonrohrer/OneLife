

double getAgeRate();


void initLifeLog();


void freeLifeLog();



void logBirth( int inPlayerID, char *inPlayerEmail,
               int inParentID, char *inParentEmail,
               char inIsMale,
               int inRace,
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
               // can be negative objectID if death caused by non-player object
               // can be own ID if died from suicide
               int inKillerID = -1, 
               char *inKillerEmail = NULL );


void logName( int inPlayerID, char *inEmail, char *inName, int inLineageEveID );

