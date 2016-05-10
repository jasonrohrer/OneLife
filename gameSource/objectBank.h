#ifndef OBJECT_BANK_INCLUDED
#define OBJECT_BANK_INCLUDED


#include "minorGems/game/doublePair.h"


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
        char heldInHand;

        
        // true for objects that cannot be walked through
        char blocksWalking;
        
        
        
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
        

        // index of special body parts (aging, attaching clothing)
        // these should be non-aging layers
        int headIndex;
        int bodyIndex;
        int backFootIndex;
        int frontFootIndex;
        
    } ObjectRecord;




// can be applied to a person object when drawing it 
// note that these should be pointers to records managed elsewhere
// null pointers mean no clothing of that type
typedef struct ClothingSet {
        // drawn above top layer
        ObjectRecord *hat;

        // drawn to replace second layer
        ObjectRecord *tunic;


        // drawn to replace third layer
        ObjectRecord *frontShoe;

        // drawn to replace bottom layer
        ObjectRecord *backShoe;
    } ClothingSet;


ClothingSet getEmptyClothingSet();




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
               char inBlocksWalking,
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
               char inMale,
               char inDeathMarker,
               int inFoodValue,
               float inSpeedMult,
               doublePair inHeldOffset,
               char inClothing,
               doublePair inClothingOffset,
               int inDeadlyDistance,
               int inNumSlots, int inSlotSize, doublePair *inSlotPos,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               double *inSpriteRot,
               char *inSpriteHFlip,
               FloatRGB *inSpriteColor,
               double *inSpriteAgeStart,
               double *inSpriteAgeEnd,
               int *inSpriteParent,
               char *inSpriteInvisibleWhenHolding,
               int inHeadIndex,
               int inBodyIndex,
               int inBackFootIndex,
               int inFrontFootIndex,
               int inReplaceID = -1 );




typedef struct HandPos {
        char valid;
        doublePair pos;
    } HandPos;


// inAge -1 for no age modifier
//
// note that inScale, which is only used by the object picker, to draw objects
// so that they fit in the picker, is not applied to clothing
HandPos drawObject( ObjectRecord *inObject, doublePair inPos, 
                    double inRot, char inFlipH, double inAge,
                    char inHoldingSomething,
                    ClothingSet inClothing,
                    double inScale = 1.0 );


void drawObject( ObjectRecord *inObject, doublePair inPos,
                 double inRot, char inFlipH,
                 double inAge,
                 char inHoldingSomething,
                 ClothingSet inClothing,
                 int inNumContained, int *inContainedIDs );



void deleteObjectFromBank( int inID );


char isSpriteUsed( int inSpriteID );



int getNumContainerSlots( int inID );

char isContainable( int inID );



// -1 if no person object exists
int getRandomPersonObject();

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
                             double inAge,
                             int inPickedLayer,
                             float inXCenterOffset, float inYCenterOffset,
                             int *outSprite,
                             int *outSlot );


int getBackHandIndex( ObjectRecord *inObject,
                      double inAge );

int getFrontHandIndex( ObjectRecord *inObject,
                       double inAge );


#endif
