#ifndef ANIMATION_BANK_INCLUDED
#define ANIMATION_BANK_INCLUDED


#include "minorGems/game/doublePair.h"

#include "minorGems/util/SimpleVector.h"

#include "objectBank.h"


typedef enum AnimType {
    ground = 0,
    held,
    moving,
    // special case of ground
    // for person who is now holding something
    // forces them to go through an animation fade (ground -> ground)
    // in case they are now holding a rideable object, because the second
    // ground animation will have frozen moving arms
    // NOTE that ground2 is NOT a separate animation stored in the
    // animation bank or modified by the editor, and that it will
    // be automatically replaced by ground when drawn
    ground2,
    // animation that only applies to a person as they eat something
    eating,
    doing,
    endAnimType,
    // arbitrary number of extra animation slots
    // indexed by calling setExtraIndex or setExtraIndexB before calling
    // other animation calls
    extra,
    extraB
    } AnimType;



const char *typeToName( AnimType inAnimType );



typedef struct SpriteAnimationRecord {
        doublePair offset;
        
        double xOscPerSec;
        // in pixels
        double xAmp;
        // between 0 and 1
        double xPhase;

        double yOscPerSec;
        double yAmp;
        double yPhase;

        doublePair rotationCenterOffset;

        // can be positive (CW) or negative (CCW)
        double rotPerSec;
        double rotPhase;

        double rockOscPerSec;
        // between 0 and 1, where 1 is full rotation before coming back
        double rockAmp;
        double rockPhase;
        
        // for animations that run for a while and pause between runs
        // ticking cogs, twitching animal noses, etc.
        double durationSec;
        double pauseSec;
        // the first pause, before the first animation duration,
        // for controling the phase of the duration-pause cycle
        double startPauseSec;
        

        double fadeOscPerSec;
        
        // 0 is sine wave, 1 is square wave, in between is a mix
        double fadeHardness;

        // what fade oscillates between
        double fadeMin;
        double fadeMax;

        // between 0 and 1
        double fadePhase;

        
    } SpriteAnimationRecord;



typedef struct SoundAnimationRecord {
        SoundUsage sound;
        double repeatPerSec;
        // between 0 and 1
        // fraction of repeat period that sound waits before playing
        // the first time
        double repeatPhase;

        double ageStart;
        double ageEnd;

        // default footstep sounds are replaced with floor usage
        // sound when walking across floor
        char footstep;
        
    } SoundAnimationRecord;
    


void zeroRecord( SpriteAnimationRecord *inRecord );
void zeroRecord( SoundAnimationRecord *inRecord );


typedef struct AnimationRecord {
        int objectID;
        
        AnimType type;
        
        // for extra anim types
        // used internally, ignored by addAnimation() in favor
        // of value set by calling setExtraIndex()
        int extraIndex;
        
        // true if start of animation should be randomized in time
        // (animation can start from anywhere along its timeline and
        //  loop from there)
        // false means animation always starts at 0-point on timeline
        char randomStartPhase;

        // this animation should always start back at frame 0 when
        // we switch to it (as opposed to continuing where last animation
        // left off)
        // Used for timed reactions.
        char forceZeroStart;
        

        int numSounds;
        SoundAnimationRecord *soundAnim;

        int numSprites;
        int numSlots;
        
        SpriteAnimationRecord *spriteAnim;
        SpriteAnimationRecord *slotAnim;
        
    } AnimationRecord;




typedef struct LayerSwapRecord {
        int indexA;
        int indexB;
    } LayerSwapRecord;



AnimationRecord *copyRecord( AnimationRecord *inRecord );

// should only be called on results of copyRecord and NOT
// on results of getAnimation
void freeRecord( AnimationRecord *inRecord );



SoundAnimationRecord copyRecord( SoundAnimationRecord inRecord );

// takes pointer, but DOES NOT free on heap
// meant to operate on pointer to a record on the stack or 
// in an array
void freeRecord( SoundAnimationRecord *inRecord );



// returns number of animations that need to be loaded
int initAnimationBankStart( char *outRebuildingCache );

float initAnimationBankStep();
void initAnimationBankFinish();



void freeAnimationBank();


// set index for subsequent animation calls with type = extra
// most calls only pay attention to extra, but draw calls can use
// both for blending between two
void setExtraIndex( int inIndex );
void setExtraIndexB( int inIndex );


int getNumExtraAnim( int inID );


// return value not destroyed by caller
// should not be modifed by caller
// change an animation by re-adding it using addAnimation below
AnimationRecord *getAnimation( int inID, AnimType inType );


// record destroyed by caller
// note that if inRecord->type == extra, inRecord->extraIndex is ignored
// and the value last set by setExtraIndex is used instead
// AnimationRecord.extraIndex is used internally only
void addAnimation( AnimationRecord *inRecord, char inNoWriteToFile = false );


void clearAnimation( int inObjectID, AnimType inType );


// do we need a smooth fade when transitionion from current animation
// to a given target animation?
char isAnimFadeNeeded( int inObjectID, AnimType inCurType, 
                       AnimType inTargetType );

char isAnimFadeNeeded( int inObjectID,
                       AnimationRecord *inCurR, AnimationRecord *inTargetR );


// returns true if animation is all-0 for both motion and phase
char isAnimEmpty( int inObjectID, AnimType inType );



// sets per-sprite/slot alpha fades to use for next drawObjectAnim call
// must match number of sprites and slot of next object drawn
// sprites fades come first, followed by slot fades
// inFades will be destroyed internally and cleared after
// the next drawObjectAnim call
void setAnimLayerFades( float *inFades );

// sets max index of layers to draw on next draw call
// (can be used to draw lower layers only)
// Defaults to -1 and resets to -1 after every call (draw all layers)
void setAnimLayerCutoff( int inCutoff );
    


// packed version of all object animation call parameters
// used for version of call that can be saved to be applied later
typedef struct ObjectAnimPack {
        int inObjectID;
        AnimType inType;
        double inFrameTime;
        double inAnimFade;
        AnimType inFadeTargetType;
        double inFadeTargetFrameTime;
        double inFrozenRotFrameTime;
        char *outFrozenRotFrameTimeUsed;
        // set endAnimType for no frozen arms
        AnimType inFrozenArmType;
        AnimType inFrozenArmFadeTargetType;
        doublePair inPos;
        double inRot;
        char inWorn;
        char inFlipH;
        double inAge;
        int inHideClosestArm;
        char inHideAllLimbs;
        char inHeldNotInPlaceYet;
        ClothingSet inClothing;
        SimpleVector<int> *inClothingContained;
        int inNumContained;
        // can be NULL if there are none contained
        int *inContainedIDs;
        SimpleVector<int> *inSubContained;
    } ObjectAnimPack;



// prepare a packed animation, but don't draw it yet
ObjectAnimPack drawObjectAnimPacked( 
    int inObjectID, AnimType inType, double inFrameTime, 
    double inAnimFade,
    AnimType inFadeTargetType,
    double inFadeTargetFrameTime,
    double inFrozenRotFrameTime,
    char *outFrozenRotFrameTimeUsed,
    // set endAnimType for no frozen arms
    AnimType inFrozenArmType,
    AnimType inFrozenArmFadeTargetType,
    doublePair inPos,
    double inRot,
    char inWorn,
    char inFlipH,
    double inAge,
    int inHideClosestArm,
    char inHideAllLimbs,
    char inHeldNotInPlaceYet,
    ClothingSet inClothing,
    SimpleVector<int> *inClothingContained,
    int inNumContained, 
    // set to NULL if not used
    int *inContainedIDs,
    SimpleVector<int> *inSubContained );


// draw a packed animation now
void drawObjectAnim( ObjectAnimPack inPack );

        


HoldingPos drawObjectAnim( int inObjectID, int inDrawBehindSlots, 
                           AnimType inType, double inFrameTime,
                           double inAnimFade,
                           // if inAnimFade < 1, this is the animation
                           // that we are fading to (fading to the time-0 config
                           // for this animation)
                           AnimType inFadeTargetType,
                           double inFadeTargetFrameTime,
                           // for any zero rotations in current anim or fade
                           // target, if either is 'held',
                           // we apply frozen rotations from the 'moving' 
                           // animation using this frame time
                           double inFrozenRotFrameTime,
                           // or'd with true on return 
                           // if inFrozenRotFrameTime was applied
                           // to some sprite layer (and thus that frame
                           // time should be used as the resume point when
                           // returning to 'moving' animation later)
                           char *outFrozenRotFrameTimeUsed,
                           // set endAnimType for no frozen arms
                           AnimType inFrozenArmType,
                           AnimType inFrozenArmFadeTargetType,
                           doublePair inPos,
                           double inRot,
                           char inWorn,
                           char inFlipH,
                           double inAge, int inHideClosestArm,
                           char inHideAllLimbs,
                           // if true, we draw holding parts anyway
                           char inHeldNotInPlaceYet,
                           ClothingSet inClothing,
                           SimpleVector<int> *inClothingContained,
                           // set to non-NULL properly sized arrays
                           // to receive info about slot rots and positions
                           double *outSlotRots = NULL,
                           doublePair *outSlotOffsets = NULL );


HoldingPos drawObjectAnim( int inObjectID, int inDrawBehindSlots,
                           AnimationRecord *inAnim, 
                           double inFrameTime,
                           double inAnimFade,
                           AnimationRecord *inFadeTargetAnim,
                           double inFadeTargetFrameTime,
                           double inFrozenRotFrameTime,
                           char *outFrozenRotFrameTimeUsed,
                           AnimationRecord *inFrozenRotAnim,
                           // set NULL for no frozen arms
                           AnimationRecord *inFrozenArmAnim,
                           AnimationRecord *inFrozenArmFadeTargetAnim,
                           doublePair inPos,
                           double inRot,
                           char inWorn,
                           char inFlipH,
                           double inAge, int inHideClosestArm,
                           char inHideAllLimbs,
                           char inHeldNotInPlaceYet,
                           ClothingSet inClothing,
                           SimpleVector<int> *inClothingContained,
                           // set to non-NULL properly sized arrays
                           // to receive info about slot rots and positions
                           double *outSlotRots = NULL,
                           doublePair *outSlotOffsets = NULL );


void drawObjectAnim( int inObjectID, AnimType inType, double inFrameTime, 
                     double inAnimFade,
                     AnimType inFadeTargetType,
                     double inFadeTargetFrameTime,
                     double inFrozenRotFrameTime,
                     char *outFrozenRotFrameTimeUsed,
                     // set endAnimType for no frozen arms
                     AnimType inFrozenArmType,
                     AnimType inFrozenArmFadeTargetType,
                     doublePair inPos,
                     double inRot,
                     char inWorn,
                     char inFlipH,
                     double inAge,
                     int inHideClosestArm,
                     char inHideAllLimbs,
                     char inHeldNotInPlaceYet,
                     ClothingSet inClothing,
                     SimpleVector<int> *inClothingContained,
                     int inNumContained, int *inContainedIDs,
                     SimpleVector<int> *inSubContained );
                     


void drawObjectAnim( int inObjectID, AnimationRecord *inAnim,
                     double inFrameTime,
                     double inAnimFade,
                     AnimationRecord *inFadeTargetAnim,
                     double inFadeTargetFrameTime,
                     double inFrozenRotFrameTime,
                     char *outFrozenRotFrameTimeUsed,
                     AnimationRecord *inFrozenRotAnim,
                     // set NULL for no frozen arms
                     AnimationRecord *inFrozenArmAnim,
                     AnimationRecord *inFrozenArmFadeTargetAnim,
                     doublePair inPos,
                     double inRot,
                     char inWorn,
                     char inFlipH,
                     double inAge,
                     int inHideClosestArm,
                     char inHideAllLimbs,
                     char inHeldNotInPlaceYet,
                     ClothingSet inClothing,
                     SimpleVector<int> *inClothingContained,
                     int inNumContained, int *inContainedIDs,
                     SimpleVector<int> *inSubContained );



void performLayerSwaps( int inObjectID, 
                        SimpleVector<LayerSwapRecord> *inSwapList,
                        int inNewNumSprites );



char isSoundUsedByAnim( int inSoundID );



#endif
