#include "animationBank.h"
#include "LivingLifePage.h"

// live triggers for video-making


void initLiveTriggers();


void freeLiveTriggers();


char anyLiveTriggersLeft();


// send in a key command received from user that make trigger an animation
void registerTriggerKeyCommand( unsigned char inASCII, LivingLifePage *inPage );


// Returns endAnimType if no trigger present this step
// or the type that should be playing this step
// outNew set to true if this is the first step for this trigger

// calls setExtraIndex in animation bank internally for AnimType extra
AnimType stepLiveTriggers( char *outNew );
