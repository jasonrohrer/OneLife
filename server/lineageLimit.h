

void initLineageLimit();


void freeLineageLimit();



// call this before a batch of isLinePermitted to configure time
void primeLineageTest( int inNumLivePlayers );

char isLinePermitted( const char *inPlayerEmail, int inLineageEveID );



void recordLineage( const char *inPlayerEmail, int inLineageEveID,
                    double inLivedYears, char inMurdered, 
                    char inCommittedMurder );
