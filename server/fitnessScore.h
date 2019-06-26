#include "minorGems/util/SimpleVector.h"


void initFitnessScore();


void freeFitnessScore();

void stepFitnessScore();


// return value:
// 0 still pending
// -1 DENIED
// 1 score ready (and returned in outScore)
int getFitnessScore( char *inEmail, int *outScore );

// all string params copied internally
void logFitnessDeath( char *inEmail, char *inName, int inDisplayID,
                      SimpleVector<char*> *inAncestorEmails,
                      SimpleVector<char*> *inAncestorRelNames );
