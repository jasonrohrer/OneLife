

void initCurseLog();


void freeCurseLog();


void logCurse( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail );

void logForgive( int inPlayerID, char *inPlayerEmail,
                 char *inTargetPlayerEmail );

// when player 
void logForgiveAll( int inPlayerID, char *inPlayerEmail );

void logForgiveAllEffect( char *inPlayerEmail,
                          char *inReceiverEmail );

void logCurseExpire( char *inPlayerEmail,
                     char *inTargetPlayerEmail );



void logTrust( int inPlayerID, char *inPlayerEmail,
               char *inTargetPlayerEmail );

void logDistrust( int inPlayerID, char *inPlayerEmail,
                  char *inTargetPlayerEmail );


void logCurseScore( char *inPlayerEmail, int inCurseScore );
