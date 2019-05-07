
#include "minorGems/util/SimpleVector.h"
#include "../gameSource/GridPos.h"

void initObjectSurvey();


void freeObjectSurvey();


void stepObjectSurvey();


// true if a survey has been requested
char shouldRunObjectSurvey();


// inLivingPlayerPositions destroyed by caller, and copied internally
void startObjectSurvey( SimpleVector<GridPos> *inLivingPlayerPositions );
