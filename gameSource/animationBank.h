#ifndef ANIMATION_BANK_INCLUDED
#define ANIMATION_BANK_INCLUDED


#include "minorGems/game/doublePair.h"


typedef enum AnimType {
    ground = 0,
    held,
    moving,
    endAnimType
    } AnimType;


typedef struct SpriteAnimationRecord {
        double xOscPerSec;
        double xAmp;
        double xPhase;

        double yOscPerSec;
        double yAmp;
        double yPhase;

        double rotPerSec;
        double rotPhase;
        
    } SpriteAnimationRecord;


void zeroRecord( SpriteAnimationRecord *inRecord );



typedef struct AnimationRecord {
        int objectID;
        
        AnimType type;
        
        int numSprites;
        int numSlots;
        
        SpriteAnimationRecord *spriteAnim;
        SpriteAnimationRecord *slotAnim;
        
    } AnimationRecord;



AnimationRecord *copyRecord( AnimationRecord *inRecord );


void initAnimationBank();


void freeAnimationBank();



// return value not destroyed by caller
// should not be modifed by caller
// change an animation by re-adding it using addAnimation below
AnimationRecord *getAnimation( int inID, AnimType inType );


// record destroyed by caller
void addAnimation( AnimationRecord *inRecord );


void clearAnimation( int inObjectID, AnimType inType );



void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     doublePair inPos );


void drawObjectAnim( int inObjectID, AnimationRecord *inAnim, 
                     double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     doublePair inPos );


void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime, 
                     double inRotFrameTime,
                     double inAnimFade,
                     doublePair inPos,
                     int inNumContained, int *inContainedIDs );


void drawObjectAnim( int inObjectID, AnimationRecord *inAnim,
                     double inFrameTime,
                     double inRotFrameTime,
                     double inAnimFade,
                     doublePair inPos,
                     int inNumContained, int *inContainedIDs );


#endif
