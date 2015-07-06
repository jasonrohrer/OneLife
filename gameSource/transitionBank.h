#ifndef TRANSITION_BANK_INCLUDED
#define TRANSITION_BANK_INCLUDED



typedef struct TransRecord {
        
        int actor;
        int target;
        
        // if either are blank, they are consumed by transition
        int newActor;
        int newTarget;
        
    } TransRecord;





void initTransBank();


void freeTransBank();



// returns NULL if no trans defined
TransRecord *getTrans( int inActor, int inTarget );


// return array destroyed by caller, NULL if none found
TransRecord **searchUses( int inUsesID, 
                          int inNumToSkip, 
                          int inNumToGet, 
                          int *outNumResults, int *outNumRemaining );

TransRecord **searchProduces( int inProducesID, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


// only one trans allowed per actor/target pair, so replacement is automatic
// -1 for inNewActor or inNewTarget means actor or target consumed
void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget );


#endif
