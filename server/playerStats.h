

void initPlayerStats();


void freePlayerStats();

void stepPlayerStats();


void recordPlayerLifeStats( char *inEmail, int inNumSecondsLived );


// returns -1 on failure, 1 on success
// if using stats server, returns 0 when lookup pending
int getPlayerLifeStats( char *inEmail, int *outNumLives, 
                        int *outTotalSeconds );


char isUsingStatsServer();

