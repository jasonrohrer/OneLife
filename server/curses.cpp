#include "curses.h"

#include <stdlib.h>



typedef struct CurseRecord {
        char *inEmail;
        int score;
        double lastDecrementTime;
    } CurseRecord;



void initCurses() {
    }

void freeCurses() {
    }

static void stepCurses() {
    // fixme:
    // need decrement time setting, or amount per sec.
    // maybe keep a priority queue
    }

static CurseRecord *findCurseRecord( char *inEmail, char inMakeNew=false ) {
    // fixme:  returns empty record if none found and inMakeNew
    return NULL;
    }




void cursePlayer( char *inGiverEmail, char *inReceiverName ) {
    // setting for curse increment needed
    }


void logPlayerNameForCurses( char *inPlayerEmail, char *inPlayerName ) {
    // structure to track names per player
    }


int getCurseLevel( char *inPlayerEmail ) {
    CurseRecord *r = findCurseRecord( inPlayerEmail );
    
    if( r == NULL ) {
        return 0;
        }
    
    int score = r->score;
    
    if( score == 0 ) {
        return 0;
        }
    
    // FIXME
    // settings curseLevelOne, curseLevelTwo
    }

