#include "../gameSource/GridPos.h"


void initLineageLimit();


void freeLineageLimit();



// call this before a batch of isLinePermitted to configure time
void primeLineageTest( int inNumLivePlayers );

char isLinePermitted( const char *inPlayerEmail, GridPos inBirthPos );



void recordLineage( const char *inPlayerEmail, GridPos inBirthPos,
                    double inLivedYears, char inMurdered, 
                    char inCommittedMurderOrSID );
