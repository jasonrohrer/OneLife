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
        // if -1, decays according to server epoch time setting
        int autoDecaySeconds;
        
        // flag that this decay time is epoch time
        // 0 of not epoch, or N for the number of epochs
        int epochAutoDecay;

        // specially flagged last-use transitions
        char lastUseActor;
        char lastUseTarget;
        
        // true if this transition undoes a use
        char reverseUseActor;
        char reverseUseTarget;
        

        // defaults to 0, which means that any transition on thje main
        // object with numUses can apply to generated useDummy objects
        // Higher values specify a cut-off point when the object becomes
        // "too used" to participate in the transition.
        // A value of 1.0f means that only the main object can participate
        // not the use dummy objects.
        float actorMinUseFraction;
        float targetMinUseFraction;

        // 0 for normal, non-moving transition
        // 1 for follow closest player
        // 2 for flee closest player
        // 3 for random movement
        // 4 for N
        // 5 for S
        // 6 for E
        // 7 for W
        int move;

        // for things that move longer distances per move
        int desiredMoveDist;

        
    } TransRecord;




// if inAutoGenerateCategoryTransitions is true, we auto-generate concrete
// transitions based on categories and do not actually insert abstract category 
// transitions explicitly.
// 
// if false, we do nothing special with category transitions, and they end
// up in the bank in their abstract form.
//
// Category bank MUST be inited before transition bank.
// Object bank MUST be inited before transition bank.
//
// returns number of transitions that need to be loaded
int initTransBankStart( char *outRebuildingCache,
                        char inAutoGenerateCategoryTransitions = false,
                        char inAutoGenerateUsedObjectTransitions = false,
                        char inAutoGenerateGenericUseTransitions = false,
                        char inAutoGenerateVariableTransitions = false );

float initTransBankStep();
void initTransBankFinish();


void freeTransBank();





// returns NULL if no trans defined
TransRecord *getTrans( int inActor, int inTarget, 
                       char inLastUseActor = false,
                       char inLastUseTarget = false );


// might not be unique
// Returns first transition producing inNewActor and inNewTarget
// returns NULL if no such trans exists
TransRecord *getTransProducing( int inNewActor, int inNewTarget );



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


SimpleVector<TransRecord*> *getAllProduces( int inUsesID );



// returns true if inPossibleAncestorID is an ancestor of inTargetID
// (used to make an ancestor of inTargetID no more than inStepLimit steps
//  up the transition tree).
char isAncestor( int inTargetID, int inPossibleAncestorID, int inStepLimit );



// inTarget can never be 0, except in the case of generic on-person transitions
// (to define what happens to an actor object when it is used on any person)

// inTarget can be -1 if inActor is > 0 and inNewTarget is 0
// (to define what's left in hand if inActor is eaten, if it's a food, or
//  to define a generic useage result for a one-time-use object)
// OR 
// if inActor > 0 and inNewTarget > 0
// (to define use-on-bare-ground transition)

// inActor can be 0 (this is the bare-hands action on the target)

// inActor can be -1 if inAutoDecaySeconds != 0 and inNewActor is 0
// (this signals an auto-decay from inTarget to inNewTarget after X seconds
//   or after server-epoch seconds if inAutoDecaySeconds = -1 )

// inActor can be -2 if inNewActor is 0 and inTarget is not 0
// (this signals default transition to execute on target when no other 
//  transition is defined)

// only one trans allowed per actor/target pair, so replacement is automatic

// 0 for inNewActor or inNewTarget means actor or target consumed
void addTrans( int inActor, int inTarget,
               int inNewActor, int inNewTarget,
               char inLastUseActor,
               char inLastUseTarget,
               char inReverseUseActor,
               char inReverseUseTarget,
               int inAutoDecaySeconds,
               float inActorMinUseFraction,
               float inTargetMinUseFraction,
               int inMove,
               int inDesiredMoveDist,
               char inNoWriteToFile = false );



void deleteTransFromBank( int inActor, int inTarget,
                          char inLastUseActor,
                          char inLastUseTarget,
                          char inNoWriteToFile = false );



char isObjectUsed( int inObjectID );


// is this object a death marker OR descended from a death marker
char isGrave( int inObjectID );



void printTrans( TransRecord *inTrans );


#define UNREACHABLE 999999999

int getObjectDepth( int inObjectID );


// true for objects that are not naturally occurring
// or for natural objects that can be moved by humans
char isHumanMade( int inObjectID );



// walks through all transitions and replaces any epoch-based autoDecay
// transition times with inEpocSeconds
// Intended to be called at server startup or when epoch time changes
void setTransitionEpoch( int inEpocSeconds );




#endif
