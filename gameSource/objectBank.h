#ifndef OBJECT_BANK_INCLUDED
#define OBJECT_BANK_INCLUDED


#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"


typedef struct FloatRGB {
        float r, g, b;
    } FloatRGB;


char equal( FloatRGB inA, FloatRGB inB );


#include "SoundUsage.h"


void setDrawColor( FloatRGB inColor );



typedef struct ObjectRecord {
        int id;
        
        char *description;

        // can it go into a container
        char containable;
        // how big of a slot is needed to contain it
        int containSize;

        // by default, when placed in a vertical container slot,
        // objects rotate 90 deg clockwise
        // this is an offset from that angle (default 0)
        double vertContainRotationOffset;
        
        
        // can it not be picked up
        char permanent;

        // age you have to be to to pick something up
        int minPickupAge;
        

        // true for smaller objects that have heldOffsets relative to
        // front, moving hand
        // non-handheld objects held relative to body
        char heldInHand;

        // true for huge objects that are ridden when held (horses, cars, etc.)
        // held offset is not relative to any body part, but relative to
        // ground under body
        // note that objects cannot be BOTH heldInHand and rideable
        // (rideable overrides heldInHand)
        char rideable;

        
        
        // true for objects that cannot be walked through
        char blocksWalking;
        
        // true if sticks out and blocks on left or right of center tile
        char wide;
        
        int leftBlockingRadius, rightBlockingRadius;
        

        // true for objects that are forced behind player
        // wide objects have this set to true automatically
        char drawBehindPlayer;
        
        
        // biome numbers where this object will naturally occur according
        // to mapChance below
        int numBiomes;
        int *biomes;
        

        // chance of occurrence naturally on map
        // value between 0 and 1 inclusive
        // Note that there's an overall chance-of-anything that is applied
        // first (controls density of map), so even if an object's value is
        // 1, it will not appear everywhere.
        // Furthermore, this value is a weight that is a fraction of the
        // total sum weight of all objects.
        float mapChance;
        
        int heatValue;
        
        // between 0 and 1, how much heat is transmitted
        float rValue;

        char person;
        // true if this person should never spawn
        // (a person for testing, a template, etc.)
        char personNoSpawn;
        
        char male;
        
        // if a person, what race number?  1, 2, 3, ....
        // 0 if not a person
        int race;

        // true if this object can be placed by server to mark a death
        char deathMarker;
        
        // true if this object can serve as a home marker
        // (remembered by client when a player makes it, and client points
        //  HOME arrow back toward it).
        char homeMarker;
        
        
        // floor objects are drawn under everything else
        // and can have other objects in the same cell
        char floor;
        
        // for vertical walls, neighboring floors auto-extended to meet
        // them
        char floorHugging;
        
        
        int foodValue;
        
        // multiplier on walking speed when holding
        float speedMult;

        // how far to move object off center when held
        // (for right-facing hold)
        // if 0, held dead center on person center
        doublePair heldOffset;
        
        // n = not wearable
        // s = shoe
        // t = tunic
        // h = hat
        // b = bottom
        // p = backpack
        char clothing;
        
        // offset of clothing from it's default location
        // (hats is slightly above head, shoes is centered on feet,
        //  tunics is centered on body)
        doublePair clothingOffset;
        
        // how many cells away this object can kill
        // 0 for non-deadly objects
        int deadlyDistance;

        // for non-deadly uses of this object, how far away can it reach?
        // (example:  lasso an animal, but has no effect on a person)
        int useDistance;
        

        SoundUsage creationSound;
        SoundUsage usingSound;
        SoundUsage eatingSound;
        SoundUsage decaySound;

        // true if creation sound should only be triggered
        // on player-caused creation of this object (not automatic,
        // decay-caused creation).
        char creationSoundInitialOnly;
        

        // if it is a container, how many slots?
        // 0 if not a container
        int numSlots;
        
        // how big of a containable can fit in each slot?
        int slotSize;
        
        doublePair *slotPos;

        // should objects be flipped vertically?
        char *slotVert;

        // index of this slot's parent in the sprite list
        // or -1 if this slot doesn't follow the motion of a sprite
        int *slotParent;
        

        // does being contained in one of this object's slots
        // adjust the passage of decay time?
        // 1.0 means normal time rate
        // > 1.0 means decay time passes faster
        // < 1.0 means longer decay times
        // must be larger than 0.0001
        float slotTimeStretch;
        

        int numSprites;
        
        int *sprites;
        
        doublePair *spritePos;

        double *spriteRot;
        
        char *spriteHFlip;

        FloatRGB *spriteColor;
        
        // -1,-1 if sprite present whole life
        double *spriteAgeStart;
        double *spriteAgeEnd;
        
        // index in this sprite list of sprite that is motion parent of this 
        // sprite, or -1 if this sprite doesn't follow the motion of another
        int *spriteParent;

        // for person objects, is this sprite a hand?
        // (the name is left over from older implementations that made the
        //  entire arm disappear when holding something large.  This name
        //  persists in the object data files, so it's best to keep it
        //  matching in the code as well)
        char *spriteInvisibleWhenHolding;
        
        // 1 for parts of clothing that disappear when clothing put on
        // 2 for parts of clothing that disappear when clothing taken off
        // all 0 for non-clothing objects
        int *spriteInvisibleWhenWorn;
        
        char *spriteBehindSlots;
        
        
        // flags for sprites that are special body parts
        char *spriteIsHead;
        char *spriteIsBody;
        char *spriteIsBackFoot;
        char *spriteIsFrontFoot;
        
        
        // number of times this object can be used before
        // something different happens
        int numUses;

        // flags for sprites that vanish with additional
        // use of this object
        // (example:  berries getting picked)
        char *spriteUseVanish;
        
        // sprites that appear with use
        // (example:  wear marks on an axe head)
        char *spriteUseAppear;
        

        // NULL unless we are auto-populating use dummy objects
        // then contains ( numUses - 1 ) ids for auto-generated dummy objects
        // with dummy_1 at index 0, dummy_2 at index 1, etc.
        int *useDummyIDs;
        
        // flags to manipulate which sprites of an object should be drawn
        // not saved to disk.  Defaults to all false for an object.
        char *spriteSkipDrawing;

        // dummy objects should not be left permanently in map database
        // because they can become invalid after a data update
        char isUseDummy;
        
        int useDummyParent;
        
        // -1 if not set
        // used to avoid recomputing height repeatedly at client/server runtime
        int cachedHeight;
        
        char apocalypseTrigger;

        char monumentStep;
        char monumentDone;
        char monumentCall;

        
        
        // NULL unless we are auto-populating variable objects
        // then contains ( N ) ids for auto-generated variable dummy objects
        // with dummy_1 at index 0, dummy_2 at index 1, etc.
        int numVariableDummyIDs;
        int *variableDummyIDs;
        
        char isVariableDummy;


    } ObjectRecord;


#define NUM_CLOTHING_PIECES 6


// can be applied to a person object when drawing it 
// note that these should be pointers to records managed elsewhere
// null pointers mean no clothing of that type
typedef struct ClothingSet {
        // drawn above top layer
        ObjectRecord *hat;

        // drawn behind top of back arm
        ObjectRecord *tunic;


        // drawn over front foot
        ObjectRecord *frontShoe;

        // drawn over back foot
        ObjectRecord *backShoe;

        // drawn under tunic
        ObjectRecord *bottom;
        
        // drawn on top of tunic
        ObjectRecord *backpack;
    } ClothingSet;


ClothingSet getEmptyClothingSet();

// 0 hat, 1, tunic, 2, front shoe, 3 back shoe, 4 bottom, 5 backpack
ObjectRecord *clothingByIndex( ClothingSet inSet, int inIndex );

void setClothingByIndex( ClothingSet *inSet, int inIndex, 
                         ObjectRecord *inClothing );

// gets the piece of clothing that was added to make inNewSet from inOldSet
// returns NULL if nothing added
ObjectRecord *getClothingAdded( ClothingSet *inOldSet, ClothingSet *inNewSet );



// enable index for string-based object searching
// call before init to index objects on load
// defaults to off
void enableObjectSearch( char inEnable );


// loads from objects folder
// returns number of objects that need to be loaded
//
// if inAutoGenerateUsedObjects is true, (n-1) dummy objects are generated
// for each object that has n uses.  These are not saved to disk
//
// Same for variable objects that contain the string $N in their discription
// (objects 1 - N are generated)
int initObjectBankStart( char *outRebuildingCache, 
                         char inAutoGenerateUsedObjects = false,
                         char inAutoGenerateVariableObjects = false );

// returns progress... ready for Finish when progress == 1.0
float initObjectBankStep();
void initObjectBankFinish();


// can only be called after bank init is complete
int getMaxObjectID();


void freeObjectBank();


// useful during development when new object property added
void resaveAll();


// returns ID of object
int reAddObject( ObjectRecord *inObject, 
                 char *inNewDescription = NULL,
                 char inNoWriteToFile = false, int inReplaceID = -1 );



ObjectRecord *getObject( int inID );


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


int addObject( const char *inDescription,
               char inContainable,
               int inContainSize,
               double inVertContainRotationOffset,
               char inPermanent,
               int inMinPickupAge,
               char inHeldInHand,
               char inRideable,
               char inBlocksWalking,
               int inLeftBlockingRadius, int inRightBlockingRadius,
               char inDrawBehindPlayer,
               char *inBiomes,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               char inPersonNoSpawn,
               char inMale,
               int inRace,
               char inDeathMarker,
               char inHomeMarker,
               char inFloor,
               char inFloorHugging,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inUseDistance,
               SoundUsage inCreationSound,
               SoundUsage inUsingSound,
               SoundUsage inEatingSound,
               SoundUsage inDecaySound,
               char inCreationSoundInitialOnly,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
               char *inSlotVert,
               int *inSlotParent,
               float inSlotTimeStretch,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               double *inSpriteRot,
               char *inSpriteHFlip,
               FloatRGB *inSpriteColor,
               double *inSpriteAgeStart,
               double *inSpriteAgeEnd,
               int *inSpriteParent,
               char *inSpriteInvisibleWhenHolding,
               int *inSpriteInvisibleWhenWorn,
               char *inSpriteBehindSlots,
               char *inSpriteIsHead,
               char *inSpriteIsBody,
               char *inSpriteIsBackFoot,
               char *inSpriteIsFrontFoot,
               int inNumUses,
               char *inSpriteUseVanish,
               char *inSpriteUseAppear,
               char inNoWriteToFile = false,
               int inReplaceID = -1 );




typedef struct HoldingPos {
        char valid;
        doublePair pos;
        double rot;
    } HoldingPos;


// sets max index of layers to draw on next draw call
// (can be used to draw lower layers only)
// Defaults to -1 and resets to -1 after every call (draw all layers)
void setObjectDrawLayerCutoff( int inCutoff );



// inAge -1 for no age modifier
//
// note that inScale, which is only used by the object picker, to draw objects
// so that they fit in the picker, is not applied to clothing
//
// returns the position used to hold something 
//
// inDrawBehindSlots = 0 for back layer, 1 for front layer, 2 for both
HoldingPos drawObject( ObjectRecord *inObject, int inDrawBehindSlots,
                       doublePair inPos, 
                       double inRot, char inWorn, char inFlipH, double inAge,
                       // 1 for front arm, -1 for back arm, 0 for no hiding
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       double inScale = 1.0 );


HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos,
                       double inRot, char inWorn, char inFlipH, double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       int inNumContained, int *inContainedIDs,
                       SimpleVector<int> *inSubContained );



void deleteObjectFromBank( int inID );


char isSpriteUsed( int inSpriteID );


char isSoundUsedByObject( int inSoundID );


int getNumContainerSlots( int inID );

char isContainable( int inID );

char isApocalypseTrigger( int inID );


// 0 for nothing
// 1 for monumentStep
// 2 for monumentDone
// 3 for monumentCall
int getMonumentStatus( int inID );


// return vector NOT destroyed by caller
SimpleVector<int> *getMonumentCallObjects();




// -1 if no person object exists
int getRandomPersonObject();


// -1 if no female
int getRandomFemalePersonObject();

// get a list of all races for which we have at least one person
// these are returned in order by race number
int *getRaces( int *outNumRaces );


// -1 if no person of this race exists
int getRandomPersonObjectOfRace( int inRace );

// gets a family member near inMotherID with max distance away in family
// spectrum 
int getRandomFamilyMember( int inRace, int inMotherID, int inFamilySpan );



int getNextPersonObject( int inCurrentPersonObjectID );
int getPrevPersonObject( int inCurrentPersonObjectID );


// -1 if no death marker object exists
int getRandomDeathMarker();


// return array destroyed by caller
ObjectRecord **getAllObjects( int *outNumResults );



// returns true if sprite inPossibleAncestor is an ancestor of inChild
char checkSpriteAncestor( ObjectRecord *inRecord, int inChildIndex,
                          int inPossibleAncestorIndex );




// get the maximum diameter of an object based on 
// sprite position and dimensions
int getMaxDiameter( ObjectRecord *inObject );


// gets estimate of object height from cell center
int getObjectHeight( int inObjectID );



// picked layer can be -1 if nothing is picked
double getClosestObjectPart( ObjectRecord *inObject,
                             // can be NULL
                             ClothingSet *inClothing,
                             // can be NULL
                             SimpleVector<int> *inContained,
                             // array of vectors, one for each clothing slot
                             // can be NULL
                             SimpleVector<int> *inClothingContained,
                             // true if inObject is currently being worn
                             // controls visibility of worn/unworn layers
                             // in clothing objects
                             char inWorn,
                             double inAge,
                             int inPickedLayer,
                             char inFlip,
                             float inXCenterOffset, float inYCenterOffset,
                             int *outSprite,
                             // 0, 1, 2, 3 if clothing hit
                             int *outClothing,
                             int *outSlot,
                             // whether sprites marked as multiplicative
                             // blend-mode should be considered clickable
                             char inConsiderTransparent = true );


char isSpriteVisibleAtAge( ObjectRecord *inObject,
                           int inSpriteIndex,
                           double inAge );


int getBackHandIndex( ObjectRecord *inObject,
                      double inAge );

int getFrontHandIndex( ObjectRecord *inObject,
                       double inAge );


int getHeadIndex( ObjectRecord *inObject, double inAge );

int getBodyIndex( ObjectRecord *inObject, double inAge );

int getBackFootIndex( ObjectRecord *inObject, double inAge );

int getFrontFootIndex( ObjectRecord *inObject, double inAge );


void getFrontArmIndices( ObjectRecord *inObject, double inAge, 
                         SimpleVector<int> *outList );

void getBackArmIndices( ObjectRecord *inObject, double inAge, 
                        SimpleVector<int> *outList );

int getBackArmTopIndex( ObjectRecord *inObject, double inAge );


void getAllLegIndices( ObjectRecord *inObject, double inAge, 
                       SimpleVector<int> *outList );



char *getBiomesString( ObjectRecord *inObject );


void getAllBiomes( SimpleVector<int> *inVectorToFill );


float getBiomeHeatValue( int inBiome );



// offset of object pixel center from 0,0
// Note that this is computed as the center of centers, 
// which is the only the approximate pixel center of the whole object.  
// Long sprites that stick
// out far from their centers, mixed with short sprites, will make 
// it somewhat inaccurate, but good enough.
doublePair getObjectCenterOffset( ObjectRecord *inObject );


// gets the largest possible radius of all wide objects
int getMaxWideRadius();


// returns true if inSubObjectID's sprites are all part of inSuperObjectID
char isSpriteSubset( int inSuperObjectID, int inSubObjectID );



// gets arm parameters for a given held object
void getArmHoldingParameters( ObjectRecord *inHeldObject,
                              int *outHideClosestArm,
                              char *outHideAllLimbs );


void computeHeldDrawPos( HoldingPos inHoldingPos, doublePair inPos,
                         ObjectRecord *inHeldObject,
                         char inFlipH,
                         doublePair *outHeldDrawPos, double *outHeldDrawRot );



// sets vis flags in inSpriteVis based on inUsesRemaining
void setupSpriteUseVis( ObjectRecord *inObject, int inUsesRemaining,
                        char *inSpriteVis );



char bothSameUseParent( int inAObjectID, int inBObjectID );


#endif
