#include "minorGems/game/doublePair.h"


extern float lifespan_multiplier;
extern int age_baby;
extern int age_fertile;
extern int age_mature;
extern int age_old;
extern int age_death;


void initAgeControl();


// returns 0,0 if inAge is -1
doublePair getAgeHeadOffset( double inAge, doublePair inHeadSpritePos,
                             doublePair inBodySpritePos,
                             doublePair inFrontFootSpritePos );

doublePair getAgeBodyOffset( double inAge, doublePair inBodySpritePos );
