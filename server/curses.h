

void initCurses();

void freeCurses();


void cursesLogBirth( char *inEmail );
void cursesLogDeath( char *inEmail );


// returns true of curse effective
char cursePlayer( char *inGiverEmail, char *inReceiverName );

void logPlayerNameForCurses( char *inPlayerEmail, char *inPlayerName );



// returns curse level, or 0 if not cursed
int getCurseLevel( char *inPlayerEmail );
