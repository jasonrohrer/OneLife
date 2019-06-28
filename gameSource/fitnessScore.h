#include "minorGems/game/doublePair.h"


void initFitnessScore();

void freeFitnessScore();


void triggerFitnessScoreUpdate();

void triggerFitnessScoreDetailsUpdate();



// These draw nothing if latest data (after last trigger) not ready yet

void drawFitnessScore( doublePair inPos );

void drawFitnessScoreDetails( doublePair inPos );


// returns true if using
char usingFitnessServer();
