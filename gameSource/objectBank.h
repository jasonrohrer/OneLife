#ifndef OBJECT_BANK_INCLUDED
#define OBJECT_BANK_INCLUDED


#include "minorGems/game/doublePair.h"


typedef struct ObjectRecord {
        int id;
        
        char *description;

        // can it go into a container
        char containable;
        
        // can it not be picked up
        char permanent;
        
        int heatValue;
        
        // between 0 and 1, how much heat is transmitted
        float rValue;
        

        // if it is a container, how many slots?
        // 0 if not a container
        int numSlots;

        doublePair *slotPos;
        

        int numSprites;
        
        int *sprites;
        
        doublePair *spritePos;

                

        
    } ObjectRecord;





// loads from objects folder
void initObjectBank();


void freeObjectBank();



ObjectRecord *getObject( int inID );


// return array destroyed by caller, NULL if none found
ObjectRecord **searchObjects( const char *inSearch, 
                              int inNumToSkip, 
                              int inNumToGet, 
                              int *outNumResults, int *outNumRemaining );


int addObject( const char *inDescription,
               char inContainable,
               char inPermanent,
               int inHeatValue,
               float inRValue,
               int inNumSlots, doublePair *inSlotPos,
               int inNumSprites, int *inSprites, 
               doublePair *inSpritePos,
               int inReplaceID = -1 );




void drawObject( ObjectRecord *inObject, doublePair inPos );


void drawObject( ObjectRecord *inObject, doublePair inPos,
                 int inNumContained, int *inContainedIDs );



void deleteObjectFromBank( int inID );


char isSpriteUsed( int inSpriteID );



int getNumContainerSlots( int inID );

char isContainable( int inID );



#endif
