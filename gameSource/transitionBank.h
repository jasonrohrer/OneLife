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




// if inAutoGenerateCategoryTransitions is true, we auto-generate concrete
// transitions based on categories and do not actually insert abstract category 
// transitions explicitly.
// 
// if false, we do nothing special with category transitions, and they end
// up in the bank in their abstract form.
//
// Category bank MUST be inited before transition bank.
//
// returns number of transitions that need to be loaded
int initTransBankStart( char *outRebuildingCache,
                        char inAutoGenerateCategoryTransitions = false );

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



// returns true if inPossibleAncestorID is an ancestor of inTargetID
// (used to make an ancestor of inTargetID no more than inStepLimit steps
//  up the transition tree).
char isAncestor( int inTargetID, int inPossibleAncestorID, int inStepLimit );



// inTarget can never be 0, except in the case of generic on-person transitions
// (to define what happens to an actor object when it is used on any person)

// inTarget can be -1 if inActor is > 0 and inNewTarget is 0
// (to define what's left in hand if inActor is eaten)
// OR 
// if inActor > 0 and inNewTarget > 0
// (to define use-on-bare-ground transition)

// inActor can be 0 (this is the bare-hands action on the target)

// inActor can be -1 if inAutoDecaySeconds > 0 and inNewActor is 0
// (this signals an auto-decay from inTarget to inNewTarget after X seconds)

// inActor can be -2 if inNewActor is 0 and inTarget is not 0
// (this signals default transition to execute on target when no other 
//  transition is defined)

// only one trans allowed per actor/target pair, so replacement is automatic

// 0 for inNewActor or inNewTarget means actor or target consumed
void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget,
               int inAutoDecaySeconds = 0,
               char inNoWriteToFile = false );



void deleteTransFromBank( int inActor, int inTarget,
                          char inNoWriteToFile = false );



char isObjectUsed( int inObjectID );


// is this object a death marker OR descended from a death marker
char isGrave( int inObjectID );


#endif
