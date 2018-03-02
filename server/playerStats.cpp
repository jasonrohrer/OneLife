#include "playerStats.h"

#include "kissdb.h"
#include "dbCommon.h"

#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"



static KISSDB db;
static char dbOpen = false;

static char useStatsServer = false;

static char *statsServerURL = NULL;
static char *statsServerSharedSecret = NULL;


typedef struct StatRecord {
        char *email;
        int numGameSeconds;
        WebRequest *request;
        // -1 until first request gets it
        int sequenceNumber;
    } StatRecord;


static SimpleVector<StatRecord> records;



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


    useStatsServer = SettingsManager::getIntSetting( "useStatsServer", 0 );    
    
    statsServerURL = 
        SettingsManager::
        getStringSetting( "statsServerURL", 
                          "http://localhost/jcr13/reviewServer/server.php" );
    
    statsServerSharedSecret = 
        SettingsManager::getStringSetting( "statsServerSharedSecret", 
                                           "secret_phrase" );
    }



void freePlayerStats() {
    if( dbOpen ) {
        KISSDB_close( &db );
        dbOpen = false;
        }    

    if( statsServerURL != NULL ) {
        delete [] statsServerURL;
        statsServerURL = NULL;
        }
    
    if( statsServerSharedSecret != NULL ) {
        delete [] statsServerSharedSecret;
        statsServerSharedSecret = NULL;
        }

    for( int i=0; i<records.size(); i++ ) {
        delete records.getElement(i)->request;
        delete [] records.getElement(i)->email;
        }
    records.deleteAll();
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

    if( useStatsServer ) {
        
        WebRequest *request;
        
        char *encodedEmail = URLUtils::urlEncode( inEmail );

        char *url = autoSprintf( 
            "%s?action=get_sequence_number"
            "&email=%s",
            statsServerURL,
            encodedEmail );
        
        delete [] encodedEmail;
        
        request = new WebRequest( "GET", url, NULL );
        printf( "Starting new web request for %s\n", url );
        
        delete [] url;

        StatRecord r = { stringDuplicate( inEmail ), inNumSecondsLived, 
                         request, -1 };
        records.push_back( r );
        }
    }



void stepPlayerStats() {
    if( ! useStatsServer ) {
        return;
        }
    
    for( int i=0; i<records.size(); i++ ) {
        StatRecord *r = records.getElement( i );
        
        int result = r->request->step();
            
        char recordDone = false;

        if( result == -1 ) {
            AppLog::info( "Request to stats server failed." );
            recordDone = true;
            }
        else if( result == 1 ) {
            // done, have result

            char *webResult = r->request->getResult();
            
            if( r->sequenceNumber == -1 ) {
                // still waiting for sequence number response

                int numRead = sscanf( webResult, "%d", &( r->sequenceNumber ) );

                if( numRead != 1 ) {
                    AppLog::info( "Failed to read sequence number "
                                  "from stats server response." );
                    recordDone = true;
                    }
                else {
                    delete r->request;
                    
                    // start stats-posting request

                    char *seqString = autoSprintf( "%d", r->sequenceNumber );
                    
                    char *hash = hmac_sha1( statsServerSharedSecret,
                                                seqString );
                    
                    delete [] seqString;
                    
                    char *encodedEmail = URLUtils::urlEncode( r->email );
                    
                    char *url = autoSprintf( 
                        "%s?action=log_game"
                        "&email=%s"
                        "&game_seconds=%d"
                        "&sequence_number=%d"
                        "&hash_value=%s",
                        statsServerURL,
                        encodedEmail,
                        r->numGameSeconds,
                        r->sequenceNumber,
                        hash );
                    
                    delete [] encodedEmail;
                    delete [] hash;

                    r->request = new WebRequest( "GET", url, NULL );
                    printf( "Starting new web request for %s\n", url );
                    
                    delete [] url;
                    }
                }
            else {
                
                if( strstr( webResult, "DENIED" ) != NULL ) {
                    AppLog::info( 
                        "Server log_game request rejected by stats server" );
                    }
                recordDone = true;
                }
            
            delete [] webResult;
            }

        if( recordDone ) {
            delete r->request;
            delete [] r->email;
            
            records.deleteElement( i );
            i--;
            }
        }
    }

