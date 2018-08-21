// for packing metadata ID into top portion of 32-bit object ID


#include "transitionBank.h"

// extracts the bottom portion of a 32-bit map ID as an object ID
int extractObjectID( int inMapID );

// extracts top portion of map ID as a metadata ID 
int extractMetadataID( int inMapID );


// to set at startup so new IDs go up from here
// otherwise, IDs start at 1
void setLastMetadataID( int inMetadataID );


int getNewMetadataID();



int packMetadataID( int inObjectID, int inMetadataID );




// same as getTrans, with metadata preserved
// returned pointer managed internally
// limit of 100 results that can be used by caller simultanously
// (statically allocated internally)
TransRecord *getMetaTrans( int inActor, int inTarget, 
                           char inLastUseActor = false,
                           char inLastUseTarget = false );
