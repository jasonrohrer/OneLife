#include "LivingLifePage.h"
#include "objectBank.h"



void initRocketAnimation( LivingLifePage *inPage, 
                          LiveObject *inRidingPlayer, ObjectRecord *inRocket,
                          double inAnimationLengthSeconds );

void freeRocketAnimation();


void stepRocketAnimation();


void drawRocketAnimation();


void addRocketSpeech( int inSpeakerID, const char *inSpeech );


char isRocketAnimationRunning();



