#include "playerStats.h"

#include "kissdb.h"
#include "dbCommon.h"

#include "minorGems/util/log/AppLog.h"


static KISSDB db;
static char dbOpen = false;


void initPlayerStats() {
    int error = KISSDB_open( &db, 
                             "playerStats.db", 
                             KISSDB_OPEN_MODE_RWCREAT,
                             80000,
                             50, // first 50 characters of email address
                                 // append spaces to the end if needed 
                             8   // two ints,  num_lives, total_seconds
                             );
    
    if( error ) {
        AppLog::errorF( "Error %d opening eve KissDB", error );
        return;
        }
    
    dbOpen = true;
    }



void freePlayerStats() {
    if( dbOpen ) {
        KISSDB_close( &db );
        dbOpen = false;
        }    
    }



// returns -1 on failure, 1 on success
int getPlayerLifeStats( char *inEmail, int *outNumLives, 
                        int *outTotalSeconds ) {
    
    unsigned char key[50];
    
    unsigned char value[8];

    emailToKey( inEmail, key );
    
    int result = KISSDB_get( &db, key, value );
    
    if( result == 0 ) {
        // found
        *outNumLives = valueToInt( &( value[0] ) );
        *outTotalSeconds = valueToInt( &( value[4] ) );
        
        return 1;
        }
    else {
        return -1;
        }
    }
        


void recordPlayerLifeStats( char *inEmail, int inNumSecondsLived ) {
    int numLives = 0;
    int numSec = 0;
    
    getPlayerLifeStats( inEmail, &numLives, &numSec );
    
    numLives++;
    numSec += inNumSecondsLived;
    
    
    unsigned char key[50];
    unsigned char value[8];
    

    emailToKey( inEmail, key );
    
    intToValue( numLives, &( value[0] ) );
    intToValue( numSec, &( value[4] ) );
            
    
    KISSDB_put( &db, key, value );
    }

