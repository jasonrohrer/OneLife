
#include "minorGems/game/doublePair.h"


void initLifeTokens();

void freeLifeTokens();


void triggerLifeTokenUpdate();


// returns -1 if still updating
int getNumLifeTokens();

// these calls below won't work unless getNumLifeTokens returns value >= 0


// if we have > 0, this string is all that needs to be shown
char *getLifeTokenString();

// if we have less than the cap, this is time until we get a new one
void getLifeTokenTime( int *outHours, int *outMinutes, int *outSeconds );



void drawTokenMessage( doublePair inPos );
