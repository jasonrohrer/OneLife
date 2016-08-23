#include "minorGems/game/doublePair.h"


void initAgeControl();


// returns 0,0 if inAge is -1
doublePair getAgeHeadOffset( double inAge, doublePair inHeadSpritePos,
                             doublePair inBodySpritePos,
                             doublePair inFrontFootSpritePos );

doublePair getAgeBodyOffset( double inAge, doublePair inBodySpritePos );
