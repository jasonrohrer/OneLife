#include "minorGems/game/doublePair.h"


void initFitnessScore();

void freeFitnessScore();


void triggerFitnessScoreUpdate();

void triggerFitnessScoreDetailsUpdate();


// false if either have been triggered and result not ready yet
char isFitnessScoreReady();


// These draw nothing if latest data (after last trigger) not ready yet

void drawFitnessScore( doublePair inPos, char inMoreDigits = false );


// inSkip controls paging through list 
void drawFitnessScoreDetails( doublePair inPos, int inSkip );

int getMaxFitnessListSkip();

char canFitnessScroll();



// returns true if using
char usingFitnessServer();
