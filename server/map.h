#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED



#include "minorGems/system/Time.h"

#include "../gameSource/GridPos.h"
#include "../gameSource/transitionBank.h"

#include "minorGems/game/doublePair.h"



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



// returns true on success
char initMap();


void freeMap( char inSkipCleanup = false );


// loads seed from file, or generates a new one and saves it to file
void reseedMap( char inForceFresh );


// can only be called before initMap or after freeMap
// deletes the underlying .db files for the map 
void wipeMapFiles();



// make Eve placement radius bigger
void doubleEveRadius();

// return Eve placement radius to starting value
void resetEveRadius();

// return eve placement back to 0,0
void resetEveLocation();


// gets new Eve position on outskirts of civilization
// if inAllowRespawn, this player's last Eve old-age-death will be
// considered.
//
// Returns true if this is an Eve respawning in their own camp
// (low-pop solo server mode)
char getEvePosition( const char *inEmail, int inID, int *outX, int *outY,
                     SimpleVector<GridPos> *inOtherPeoplePos,
                     char inAllowRespawn = true,
                     // true if we should increment position for advancing
                     // eve grid or spiral placement
                     // false to just sample the current position with no update
                     char inIncrementPosition = true );


// save recent placements on Eve's death so that this player can spawn
// near them if they are ever Eve again
void mapEveDeath( const char *inEmail, double inAge, GridPos inDeathMapPos );





// returns properly formatted chunk message for chunk in rectangle shape
// with bottom-left corner at x,y
// coordinates in message will be relative to inRelativeToPos
// note that inStartX,Y are absolute world coordinates
unsigned char *getChunkMessage( int inStartX, int inStartY, 
                                int inWidth, int inHeight,
                                GridPos inRelativeToPos,
                                int *outMessageLength );


// sets the player responsible for subsequent map changes
// meant to track who set down an object
// should be set to -1 (default) except for object set-down
void setResponsiblePlayer( int inPlayerID );


int getMapObject( int inX, int inY );

char isMapSpotBlocking( int inX, int inY );


// is the object returned by getMapObject still in motion with
// destination inX, inY
char isMapObjectInTransit( int inX, int inY );


void setMapObject( int inX, int inY, int inID );


void setEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds,
                  TransRecord *inApplicableTrans = NULL );


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
// negative elements indicate sub-containers
void addContained( int inX, int inY, int inContainedID, 
                   timeSec_t inEtaDecay, int inSubCont = 0 );

int getNumContained( int inX, int inY, int inSubCont = 0 );

// destroyed by caller, returns NULL if empty
// negative elements indicate sub-containers
int *getContained( int inX, int inY, int *outNumContained, int inSubCont = 0 );
timeSec_t *getContainedEtaDecay( int inX, int inY, int *outNumContained,
                                 int inSubCont = 0 );

// gets contained item from specified slot, or from top of stack
// if inSlot is -1
// negative elements indicate sub-containers
int getContained( int inX, int inY, int inSlot, int inSubCont = 0 );


// setting negative elements indicates sub containers
void setContained( int inX, int inY, int inNumContained, int *inContained,
                   int inSubCont = 0 );
void setContainedEtaDecay( int inX, int inY, int inNumContained, 
                           timeSec_t *inContainedEtaDecay,
                           int inSubCont = 0 );

// removes contained item from specified slot, or remove from top of stack
// if inSlot is -1
// if inSubCont = 0, then sub-container in that slot is cleared by this call
int removeContained( int inX, int inY, int inSlot, timeSec_t *outEtaDecay,
                     int inSubCont = 0 );


// if inSubCont is 0, container and all sub-containers are cleared
// otherwise, clears only a specific sub-container
void clearAllContained( int inX, int inY, int inSubCont = 0 );

// if inNumNewSlots less than number contained, the excess are discarded
void shrinkContainer( int inX, int inY, int inNumNewSlots, int inSubCont = 0 );



// unlike normal objects, there is no live tracking of floors and when
// they will decay in stepMap
// Decay of floors only applied on next call to getMapFloor
// Thus, short-term floor decay isn't supported (e.g., burning floor that
// finishes burning while player still has it on the screen).
int getMapFloor( int inX, int inY );

void setMapFloor( int inX, int inY, int inID );

void setFloorEtaDecay( int inX, int inY, timeSec_t inAbsoluteTimeInSeconds );

timeSec_t getFloorEtaDecay( int inX, int inY );


typedef struct MapChangeRecord {
        char *formatString;
        int absoluteX, absoluteY;
        
        char oldCoordsUsed;
        int absoluteOldX, absoluteOldY;
    } MapChangeRecord;



// formatString in returned record destroyed by caller
MapChangeRecord getMapChangeRecord( ChangePosition inPos );


// line for a map change message
char *getMapChangeLineString( ChangePosition inPos );


char *getMapChangeLineString( MapChangeRecord *inRecord,
                              int inRelativeToX, int inRelativeToY );



// returns number of seconds from now until when next decay is supposed
// to happen
// returns -1 if no decay pending
int getNextDecayDelta();


// marks region as looked at, so that live decay tracking continues
// there
void lookAtRegion( int inXStart, int inYStart, int inXEnd, int inYEnd );



// any change lines resulting from step are appended to inMapChanges
// any change positions are added to end of inChangePosList
void stepMap( SimpleVector<MapChangeRecord> *inMapChanges, 
               SimpleVector<ChangePosition> *inChangePosList );



void restretchDecays( int inNumDecays, timeSec_t *inDecayEtas,
                      int inOldContainerID, int inNewContainerID );


void restretchMapContainedDecays( int inX, int inY,
                                  int inOldContainerID, int inNewContainerID,
                                  int inSubCont = 0 );



int getMapBiome( int inX, int inY );



typedef struct {
        unsigned int uniqueLoadID;
        char *mapFileName;
        char fileOpened;
        FILE *file;
        int x, y;
        double startTime;
        int stepCount;
    } TutorialLoadProgress;

    


// returns true on success
// example:
// loadTutorial( newPlayer.tutorialLoad, "tutorialA.txt", 10000, 10000 )
char loadTutorialStart( TutorialLoadProgress *inTutorialLoad,
                        const char *inMapFileName, int inX, int inY );


// returns true if more steps are needed
// false if done
char loadTutorialStep( TutorialLoadProgress *inTutorialLoad,
                       double inTimeLimitSec );




#define MAP_METADATA_LENGTH 128

// inBuffer must be at least MAP_METADATA_LENGTH bytes
// returns true if metadata found
char getMetadata( int inMapID, unsigned char *inBuffer );


// returns full map ID with embedded metadata ID for new metadata record
int addMetadata( int inObjectID, unsigned char *inBuffer );
    



// ascii string data, \0 terminated
// they can say 88 chars max
// their name is 31 chars long, max
// 10 chars per id, 7 of them, 70 chars max
// age is 10 chars max
// 10 separators
// 209 chars max, plus \0 termination
// 210 is perfect
//
// displayID|age|name|
//           hat|tunic|frontShoe|backShoe|bottom|backpack|
//           final words
#define MAP_STATUE_DATA_LENGTH 210

// inBuffer must be at least MAP_STATUE_DATA_LENGTH bytes
// returns true if statue data found
char getStatueData( int inX, int inY,
                    timeSec_t *outStatueTime, char *inBuffer );


// inDataString can be at most MAP_STATUE_DATA_LENGTH long, including \0
// termination byte
void addStatueData( int inX, int inY,
                    timeSec_t inStatueTime,
                    const char *inDataString );







// gets speech pipe indices for IN pipes at or adjacent to inX,inY
// vector passed in through outIndicies will be filled with indices
void getSpeechPipesIn( int inX, int inY, 
                       SimpleVector<int> *outIndicies,
                       SimpleVector<GridPos> *outPositions );


// returned vector NOT destroyed or modified by caller
SimpleVector<GridPos> *getSpeechPipesOut( int inIndex );



// for performance reasons, when the true decayed version of the object
// doesn't matter, this skips some expensive steps
int getMapObjectRaw( int inX, int inY );



// next landing strip in line, in round-the-world circuit across all
// landing positions
// radius limit limits flights from inside that square radius
// from leaving (though flights from outside are unrestriced)
GridPos getNextFlightLandingPos( int inCurrentX, int inCurrentY,
                                 doublePair inDir,
                                 int inRadiusLimit = -1 );



GridPos getClosestLandingPos( GridPos inTargetPos, char *outFound );




// get and set player ID for grave on map

// returns 0 if not found
int getGravePlayerID( int inX, int inY );

void setGravePlayerID( int inX, int inY, int inPlayerID );



// culling regions of map that haven't been seen in a long time
void stepMapLongTermCulling( int inNumCurrentPlayers );


// 1 if homeland
// 0 if fam has no homeland
// -1 if outside of homeland (or in someone else's)
int isHomeland( int inX, int inY, int inLineageEveID );

void logHomelandBirth( int inX, int inY, int inLineageEveID );

// get position of homeland center that  (x,y) is within the radius of
// also get the lineage corresponding to this homeland
// returns -1 for outLineageEveID if this WAS a homeland, but is now expired.
char getHomelandCenter( int inX, int inY, 
                        GridPos *outCenter, int *outLineageEveID );


// when last member of a family dies
void homelandsDead( int inLineageEveID );


typedef struct HomelandInfo {
        GridPos center;
        int radius;
        // -1 if abandonned
        int lineageEveID;
    } HomelandInfo;
    

// gets changes since last call
SimpleVector<HomelandInfo> getHomelandChanges();


// if birthland bands are enabled, returns 1 if in birthland band, 
// or -1 otherwise
// if birthland bands are disabled, returns same result as isHomeland
// 1 if birthland
// -1 if outside of birthland
int isBirthland( int inX, int inY, int inLineageEveID, int inDisplayID );



// looks for deadly object that is crossing inPos
// passes out moving object's destination
int getDeadlyMovingMapObject( int inPosX, int inPosY,
                              int *outMovingDestX, int *outMovingDestY );



int getSpecialBiomeBandYCenterForRace( int inRace );


#endif
