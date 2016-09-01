#ifndef OBJECT_BANK_INCLUDED
#define OBJECT_BANK_INCLUDED


#include "minorGems/game/doublePair.h"
#include "minorGems/util/SimpleVector.h"


typedef struct FloatRGB {
        float r, g, b;
    } FloatRGB;




void setDrawColor( FloatRGB inColor );



typedef struct ObjectRecord {
        int id;
        
        char *description;

        // can it go into a container
        char containable;
        // how big of a slot is needed to contain it
        int containSize;
        
        // can it not be picked up
        char permanent;


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
        
        char male;
        
        // if a person, what race number?  1, 2, 3, ....
        // 0 if not a person
        int race;

        // true if this object can be placed by server to mark a death
        char deathMarker;
        

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


        // if it is a container, how many slots?
        // 0 if not a container
        int numSlots;
        
        // how big of a containable can fit in each slot?
        int slotSize;
        
        doublePair *slotPos;
        
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

        // for person objects, should this sprite vanish when
        // the person is holding something?
        char *spriteInvisibleWhenHolding;
        

        // flags for sprites that are special body parts
        char *spriteIsHead;
        char *spriteIsBody;
        char *spriteIsBackFoot;
        char *spriteIsFrontFoot;
                
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



// loads from objects folder
// returns number of objects that need to be loaded
int initObjectBankStart( char *outRebuildingCache );

// returns progress... ready for Finish when progress == 1.0
float initObjectBankStep();
void initObjectBankFinish();


// can only be called after bank init is complete
int getMaxObjectID();


void freeObjectBank();


// useful during development when new object property added
void resaveAll();



ObjectRecord *getObject( int inID );


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


int addObject( const char *inDescription,
               char inContainable,
               int inContainSize,
               char inPermanent,
               char inHeldInHand,
               char inRideable,
               char inBlocksWalking,
               char *inBiomes,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               char inMale,
               int inRace,
               char inDeathMarker,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
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
               char *inSpriteIsHead,
               char *inSpriteIsBody,
               char *inSpriteIsBackFoot,
               char *inSpriteIsFrontFoot,
               int inReplaceID = -1 );




typedef struct HoldingPos {
        char valid;
        doublePair pos;
        double rot;
    } HoldingPos;


// inAge -1 for no age modifier
//
// note that inScale, which is only used by the object picker, to draw objects
// so that they fit in the picker, is not applied to clothing
//
// returns the position used to hold something 
HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos, 
                       double inRot, char inFlipH, double inAge,
                       // 1 for front arm, -1 for back arm, 0 for no hiding
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       double inScale = 1.0 );


HoldingPos drawObject( ObjectRecord *inObject, doublePair inPos,
                       double inRot, char inFlipH,
                       double inAge,
                       int inHideClosestArm,
                       char inHideAllLimbs,
                       char inHeldNotInPlaceYet,
                       ClothingSet inClothing,
                       int inNumContained, int *inContainedIDs );



void deleteObjectFromBank( int inID );


char isSpriteUsed( int inSpriteID );



int getNumContainerSlots( int inID );

char isContainable( int inID );



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



// picked layer can be -1 if nothing is picked
double getClosestObjectPart( ObjectRecord *inObject,
                             // can be NULL
                             ClothingSet *inClothing,
                             // can be NULL
                             SimpleVector<int> *inContained,
                             // array of vectors, one for each clothing slot
                             // can be NULL
                             SimpleVector<int> *inClothingContained,
                             double inAge,
                             int inPickedLayer,
                             char inFlip,
                             float inXCenterOffset, float inYCenterOffset,
                             int *outSprite,
                             // 0, 1, 2, 3 if clothing hit
                             int *outClothing,
                             int *outSlot );


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



// offset of object pixel center from 0,0
// Note that this is computed as the center of centers, 
// which is the only the approximate pixel center of the whole object.  
// Long sprites that stick
// out far from their centers, mixed with short sprites, will make 
// it somewhat inaccurate, but good enough.
doublePair getObjectCenterOffset( ObjectRecord *inObject );


#endif
