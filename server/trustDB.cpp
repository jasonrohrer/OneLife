#include "trustDB.h"



#include "kissdb.h"
#include "lineardb3.h"
#include "dbCommon.h"

#include "curseLog.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/log/AppLog.h"
#include "minorGems/io/file/File.h"



static LINEARDB3 db;
static char dbOpen = false;



void initTrustDB() {
    int error = LINEARDB3_open( &db, 
                                "trust.db", 
                                KISSDB_OPEN_MODE_RWCREAT,
                                10000,
                                // sender email (truncated to 39 chars max)
                                // with receiver email (trucated to 39) 
                                // appended,
                                // space separating them
                                // terminated by a NULL character
                                // append spaces to the end if needed
                                // (after the NULL character) to fill
                                // the full 80 characters consistently
                                80, 
                                // one 64-bit double, representing the time
                                // the trustd was placed
                                // in whatever binary format and byte order
                                // "double" on the server platform uses
                                8 );
    
    if( error ) {
        AppLog::errorF( "Error %d opening trust LinearDB3", error );
        return;
        }
    
    dbOpen = true;
    }




void freeTrustDB() {
    if( dbOpen ) {
        LINEARDB3_close( &db );
        dbOpen = false;
        }
    }





static void getKey( const char *inSenderEmail, const char *inReceiverEmail, 
                    unsigned char *outKey ) {
    memset( outKey, ' ', 80 );

    sprintf( (char*)outKey, "%.39s,%.39s", inSenderEmail, inReceiverEmail );
    }











                    

void setDBTrust( int inSenderID, 
                 const char *inSenderEmail, const char *inReceiverEmail ) {

    unsigned char key[80];
    unsigned char value[8];
    

    getKey( inSenderEmail, inReceiverEmail, key );
    
    
    timeToValue( Time::timeSec(), value );

    
    LINEARDB3_put( &db, key, value );
    printf( "Setting personal trust for %s by %s\n", 
            inReceiverEmail, inSenderEmail );


    logTrust( inSenderID, (char*)inSenderEmail, (char*)inReceiverEmail );
    }




void clearDBTrust( const char *inSenderEmail, const char *inReceiverEmail ) {
    unsigned char key[80];
    unsigned char value[8];
    

    getKey( inSenderEmail, inReceiverEmail, key );
    
    
    timeToValue( 0, value );

    
    LINEARDB3_put( &db, key, value );
    printf( "Clearing personal trust for %s by %s\n", 
            inReceiverEmail, inSenderEmail );
    }




char isTrusted( const char *inSenderEmail, const char *inReceiverEmail ) {
    unsigned char key[80];
    unsigned char value[8];

    getKey( inSenderEmail, inReceiverEmail, key );
    
    int result = LINEARDB3_get( &db, key, value );


    if( result == 0 ) {
        timeSec_t trustTime = valueToTime( value );

        if( trustTime > 0 ) {
            return true;
            }
        else {
            // record found, but 0, means cleared
            return false;
            }
        }
    
    // no record found
    return false;
    }