

typedef struct ChangePosition {
        int x, y;
        
        // true if update should be sent to everyone regardless
        // of distance (like position of a new player in the world,
        // or the removal of a player).
        char global;
        
        int responsiblePlayerID;
    } ChangePosition;


#include "minorGems/util/SimpleVector.h"



void initMap();


void freeMap();


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


void setMapObject( int inX, int inY, int inID );


void setEtaDecay( int inX, int inY, unsigned int inAbsoluteTimeInSeconds );


unsigned int getEtaDecay( int inX, int inY );

// for container slots
void setSlotEtaDecay( int inX, int inY, int inSlot,
                      unsigned int inAbsoluteTimeInSeconds );
unsigned int getSlotEtaDecay( int inX, int inY, int inSlot );



// adds to top of stack
void addContained( int inX, int inY, int inContainedID, 
                   unsigned int inEtaDecay );

int getNumContained( int inX, int inY );

// destroyed by caller, returns NULL if empty
int *getContained( int inX, int inY, int *outNumContained );
unsigned int *getContainedEtaDecay( int inX, int inY, int *outNumContained );

void setContained( int inX, int inY, int inNumContained, int *inContained );
void setContainedEtaDecay( int inX, int inY, int inNumContained, 
                           unsigned int *inContainedEtaDecay );

// removes contained item from specified slot, or remove from top of stack
// if inSlot is -1
int removeContained( int inX, int inY, int inSlot, unsigned int *outEtaDecay );

void clearAllContained( int inX, int inY );

// if inNumNewSlots less than number contained, the excess are discarded
void shrinkContainer( int inX, int inY, int inNumNewSlots );


// line for a map change message
char *getMapChangeLineString( int inX, int inY, 
                              int inResponsiblePlayerID = -1  );


// returns number of seconds from now until when next decay is supposed
// to happen
// returns -1 if no decay pending
int getNextDecayDelta();



// any change lines resulting from step are appended to inMapChanges
// any change positions are added to end of inChangePosList
void stepMap( SimpleVector<char> *inMapChanges, 
               SimpleVector<ChangePosition> *inChangePosList );



void restretchDecays( int inNumDecays, unsigned int *inDecayEtas,
                      int inOldContainerID, int inNewContainerID );


void restretchMapContainedDecays( int inX, int inY,
                                  int inOldContainerID, int inNewContainerID );
