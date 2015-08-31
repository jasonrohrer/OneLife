#ifndef TRANSITION_BANK_INCLUDED
#define TRANSITION_BANK_INCLUDED



typedef struct TransRecord {
        
        int actor;
        int target;
        
        // if either are blank, they are consumed by transition
        int newActor;
        int newTarget;
    

        // if actor is -1 and autoDecaySeconds is non-zero
        // then target decays to newTarget automatically in autoDecaySeconds
        int autoDecaySeconds;
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



// inTarget can never be 0

// inActor can be 0 (this is the bare-hands action on the target)

// inActor can be -1 if inAutoDecaySeconds > 0 and inNewActor is 0
// (this signals an auto-decay from inTarget to inNewTarget after X seconds)

// only one trans allowed per actor/target pair, so replacement is automatic

// 0 for inNewActor or inNewTarget means actor or target consumed
void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget,
               int inAutoDecaySeconds = 0 );



void deleteTransFromBank( int inActor, int inTarget );



char isObjectUsed( int inObjectID );


#endif
