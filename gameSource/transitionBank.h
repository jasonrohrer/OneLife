#ifndef TRANSITION_BANK_INCLUDED
#define TRANSITION_BANK_INCLUDED


#include "minorGems/util/SimpleVector.h"



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





// returns number of transitions that need to be loaded
int initTransBankStart( char *outRebuildingCache );

float initTransBankStep();
void initTransBankFinish();


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



// returns internal vector of ALL transitions that use inUsesID as either
// an actor or a target
//
// This vector must NOT be destroyed or altered by caller
// Can return NULL if no transitions use inUsesID
SimpleVector<TransRecord*> *getAllUses( int inUsesID );



// inTarget can never be 0, except in the case of generic on-person transitions
// (to define what happens to an actor object when it is used on any person)

// inTarget can be -1 if inActor is > 0 and inNewTarget is 0
// (to define what's left in hand if inActor is eaten)

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
