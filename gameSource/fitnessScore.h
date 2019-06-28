#include "minorGems/game/doublePair.h"


void initFitnessScore();

void freeFitnessScore();


void triggerFitnessScoreUpdate();

void triggerFitnessScoreDetailsUpdate();


// false if either have been triggered and result not ready yet
char isFitnessScoreReady();


// These draw nothing if latest data (after last trigger) not ready yet

void drawFitnessScore( doublePair inPos );

void drawFitnessScoreDetails( doublePair inPos );


// returns true if using
char usingFitnessServer();
