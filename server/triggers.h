// client-issued server-side triggers


void initTriggers();

void freeTriggers();

char areTriggersEnabled();


// returns -1 if unknown
int getTriggerPlayerDisplayID( const char *inPlayerEmail );

void trigger( int inTriggerNumber );

void stepTriggers();
