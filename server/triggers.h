#include "../gameSource/GridPos.h"
#include "../gameSource/objectBank.h"


// client-issued server-side triggers

// NOTE:
// some triggers open socket connections to the server as dummy players
// thus, they don't work if settings/requireTicketServerCheck.ini is set to 1
// or if requireClientPassword is set

void initTriggers();

void freeTriggers();

char areTriggersEnabled();


// returns -1 if unknown
int getTriggerPlayerDisplayID( const char *inPlayerEmail );

double getTriggerPlayerAge( const char *inPlayerEmail );

GridPos getTriggerPlayerPos( const char *inPlayerEmail );

// only supports a single held id, and not containment
int getTriggerPlayerHolding( const char *inPlayerEmail );

ClothingSet getTriggerPlayerClothing( const char *inPlayerEmail );



void trigger( int inTriggerNumber );

void stepTriggers();


// amount of time that it's safe to wait before calling stepTriggers()
// returns -1.0 if there are no pending triggers
double getShortestTriggerDelay();
