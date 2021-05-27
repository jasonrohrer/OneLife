
// for tracking the offspring of players that live to old age,
// so that they can be born to their own descendants in the future.



void initOffspringTracker();


void freeOffspringTracker();



// lineageID is player's own life ID if they are female
// or their mother's life ID if they are male.
//
// if female, we look only at their own descendants.
// if male, we look at their sister's decendants.
void trackOffspring( char *inPlayerEmail, int inLineageIDToTrack );


char isOffspringTracked( char *inPlayerEmail );


int getOffspringLineageID( char *inPlayerEmail );


void clearOffspringLineageID( char *inPlayerEmail );
