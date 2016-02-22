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
        
        // 1 if sprite should move along with head as it ages
        char *spriteAgesWithHead;

        // count of sprites that span entire life
        int numNonAgingSprites;
        
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
void initObjectBankStart();
// returns progress... ready for Finish when progress == 1.0
float initObjectBankStep();
void initObjectBankFinish();


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
               float inMapChance,
               int inHeatValue,
               float inRValue,
               char inPerson,
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
               char *inSpriteAgesWithHead,
               int inReplaceID = -1 );




// inAge -1 for no age modifier
//
// note that inScale, which is only used by the object picker, to draw objects
// so that they fit in the picker, is not applied to clothing
void drawObject( ObjectRecord *inObject, doublePair inPos, 
                 char inFlipH, double inAge, ClothingSet inClothing,
                 double inScale = 1.0 );


void drawObject( ObjectRecord *inObject, doublePair inPos,
                 char inFlipH,
                 double inAge,
                 ClothingSet inClothing,
                 int inNumContained, int *inContainedIDs );



void deleteObjectFromBank( int inID );


char isSpriteUsed( int inSpriteID );



int getNumContainerSlots( int inID );

char isContainable( int inID );



// -1 if no person object exists
int getRandomPersonObject();


// return array destroyed by caller
ObjectRecord **getAllObjects( int *outNumResults );


#endif
