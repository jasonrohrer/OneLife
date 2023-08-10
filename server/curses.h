#include "minorGems/util/SimpleVector.h"
#include "../gameSource/GridPos.h"


void initCurses();

void freeCurses();


void cursesLogBirth( char *inEmail );

// inName can be NULL if player has no name
void cursesLogDeath( char *inEmail, char *inName,
                     double inAge, GridPos inDeathPos );



// check whether a player has a curse token
// meant to be called at birth
char hasCurseToken( char *inEmail );


// gets a list of email addresses for players who now have curse tokens when
// they didn't the last time hasCurseToken was called
//
// Passed-in vector is filled with emails that are destroyed by caller
void getNewCurseTokenHolders( SimpleVector<char*> *inEmailList );



// spends token and registers a curse
// returns true of curse effective
// enforces inMaxDistance for dead receivers only
// (position of living players isn't tracked internally)
char cursePlayer( int inGiverID, int inGiverLineageEveID, 
                  char *inGiverEmail, GridPos inGiverPos,
                  double inMaxDistance,
                  char *inReceiverName );


// spends a token directly
// returns true if spent
char spendCurseToken( char *inGiverEmail );




void logPlayerNameForCurses( char *inPlayerEmail, char *inPlayerName,
                             int inLineageEveID );



typedef struct CurseStatus {
        int curseLevel;
        int excessPoints;
    } CurseStatus;


// returns curse level in struct, or 0 if not cursed
// if cursed, returned struct will also contain excessPoints
// returns curse level of -1 if pending lookup on remote server
CurseStatus getCurseLevel( char *inPlayerEmail );



// true if name already exists in curse system
char isNameDuplicateForCurses( const char *inPlayerName );


void stepCurseServerRequests();


// check if curse system has lineage information for a given player name
// returns -1 on failure
int getCurseReceiverLineageEveID( char *inReceiverName );



// NOT destroyed by caller
// NULL if not found
char *getCurseReceiverEmail( char *inReceiverName );
