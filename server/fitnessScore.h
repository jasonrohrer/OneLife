#include "minorGems/util/SimpleVector.h"


void initFitnessScore();


void freeFitnessScore();

void stepFitnessScore();


// return value:
// 0 still pending
// -1 DENIED
// 1 score ready (and returned in outScore)
// outScore NOT modified if result not ready
int getFitnessScore( char *inEmail, float *outScore );

// all string params copied internally
void logFitnessDeath( int inNumLivePlayers, 
                      char *inEmail, char *inName, int inDisplayID,
                      double inAge,
                      SimpleVector<char*> *inAncestorEmails,
                      SimpleVector<char*> *inAncestorRelNames,
                      SimpleVector<char*> *inAncestorData );
