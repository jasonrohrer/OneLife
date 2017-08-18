#include "minorGems/system/Time.h"



typedef struct ChangePosition {
        int x, y;
        
        // true if update should be sent to everyone regardless
        // of distance (like position of a new player in the world,
        // or the removal of a player).
        char global;
        
        int responsiblePlayerID;

        // for movement changes
        int oldX, oldY;
        float speed;
        
    } ChangePosition;


#include "minorGems/util/SimpleVector.h"



void initMap();


void freeMap();


// make Eve placement radius bigger
void doubleEveRadius();

// return Eve placement radius to starting value
void resetEveRadius();


// gets new Eve position on outskirts of civilization
void getEvePosition( int *outX, int *outY );



int getMaxChunkDimension();


// returns properly formatted chunk message for chunk centered
// around x,y
unsigned char *getChunkMessage( int inCenterX, int inCenterY,
                                int *outMessageLength );


// sets the player responsible for subsequent map changes
// meant to track who set down an object
// should be set to -1 (default) except for object set-down
void setResponsiblePlayer( int inPlayerID );


int getMapObject( int inX, int inY );


// is the object returned by getMapObject still in motion with
// destination inX, inY
char isMapObjectInTransit( int inX, int inY );


void setMapObject( int inX, int inY, int inID );


void setEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds );


timeSec_t getEtaDecay( int inX, int inY );


// for all these calls, inSubCont indexes the main container (when 0)
// or sub-containers (when > 0).
// So, if inSubCont=3 and inSlot=2, we get information about the 2nd
// slot in the 3rd sub-container (the 3rd slot in the main container)

// for container slots
void setSlotEtaDecay( int inX, int inY, int inSlot,
                      timeSec_t inAbsoluteTimeInSeconds, int inSubCont = 0 );
timeSec_t getSlotEtaDecay( int inX, int inY, int inSlot, int inSubCont = 0 );



// adds to top of stack
void addContained( int inX, int inY, int inContainedID, 
                   timeSec_t inEtaDecay, int inSubCont = 0 );

int getNumContained( int inX, int inY, int inSubCont = 0 );

// destroyed by caller, returns NULL if empty
int *getContained( int inX, int inY, int *outNumContained, int inSubCont = 0 );
timeSec_t *getContainedEtaDecay( int inX, int inY, int *outNumContained,
                                 int inSubCont = 0 );

// gets contained item from specified slot, or from top of stack
// if inSlot is -1
int getContained( int inX, int inY, int inSlot, int inSubCont = 0 );


void setContained( int inX, int inY, int inNumContained, int *inContained,
                   int inSubCont = 0 );
void setContainedEtaDecay( int inX, int inY, int inNumContained, 
                           timeSec_t *inContainedEtaDecay,
                           int inSubCont = 0 );

// removes contained item from specified slot, or remove from top of stack
// if inSlot is -1
int removeContained( int inX, int inY, int inSlot, timeSec_t *outEtaDecay,
                     int inSubCont = 0 );

void clearAllContained( int inX, int inY, int inSubCont = 0 );

// if inNumNewSlots less than number contained, the excess are discarded
void shrinkContainer( int inX, int inY, int inNumNewSlots, int inSubCont = 0 );


// line for a map change message
char *getMapChangeLineString( ChangePosition inPos );


// returns number of seconds from now until when next decay is supposed
// to happen
// returns -1 if no decay pending
int getNextDecayDelta();


// marks region as looked at, so that live decay tracking continues
// there
void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd );



// any change lines resulting from step are appended to inMapChanges
// any change positions are added to end of inChangePosList
void stepMap( SimpleVector<char> *inMapChanges, 
               SimpleVector<ChangePosition> *inChangePosList );



void restretchDecays( int inNumDecays, timeSec_t *inDecayEtas,
                      int inOldContainerID, int inNewContainerID );


void restretchMapContainedDecays( int inX, int inY,
                                  int inOldContainerID, int inNewContainerID,
                                  int inSubCont = 0 );
