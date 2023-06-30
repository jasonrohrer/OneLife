

void initCurseLog();


void freeCurseLog();


void logCurse( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail );

void logTrust( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail );


void logCurseScore( char *inPlayerEmail, int inCurseScore );
