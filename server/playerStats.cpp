#include "playerStats.h"
#include "serverCalls.h"

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
        // > 0 if this is a stats recording action
        // -1 if this is a lookup action
        int numGameSeconds;
        WebRequest *request;
        // -1 until first request gets it
        int sequenceNumber;

        // filled by lookup requests
        int gameCount;
        int gameTotalSeconds;
        char error;
        
        const char *serverAction;
    } StatRecord;


static SimpleVector<StatRecord> records;

// lookups waiting to be processed
static SimpleVector<StatRecord> doneLookups;



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


    useStatsServer = 
        SettingsManager::getIntSetting( "useStatsServer", 0 ) &&
        SettingsManager::getIntSetting( "remoteReport", 0 );    
    
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

    for( int i=0; i<doneLookups.size(); i++ ) {
        delete doneLookups.getElement(i)->request;
        delete [] doneLookups.getElement(i)->email;
        }
    doneLookups.deleteAll();
    }



// returns -1 on failure, 1 on success
int getPlayerLifeStats( char *inEmail, int *outNumLives, 
                        int *outTotalSeconds ) {
    
    if( useStatsServer ) {
        for( int i=0; i<doneLookups.size(); i++ ) {
            StatRecord *r = doneLookups.getElement( i );
            
            if( strcmp( r->email, inEmail ) == 0 ) {
                // found

                char error = false;
                if( r->error ) {
                    error = true;
                    }
                else {
                    *outNumLives = r->gameCount;
                    *outTotalSeconds = r->gameTotalSeconds;
                    }
                
                delete r->request;
                delete [] r->email;
                
                doneLookups.deleteElement( i );
                
                if( error ) {
                    return -1;
                    }
                else {
                    return 1;
                    }
                }
            }
        
        for( int i=0; i<records.size(); i++ ) {
            StatRecord *r = records.getElement( i );
            
            if( strcmp( r->email, inEmail ) == 0 ) {
                // still in progress
                return 0;
                }
            }
        
        // no request exists

        WebRequest *request;
        
        char *encodedEmail = URLUtils::urlEncode( inEmail );

        char *url = autoSprintf( 
            "%s?action=get_sequence_number"
            "&email=%s",
            statsServerURL,
            encodedEmail );
        
        delete [] encodedEmail;
        
        request = defaultTimeoutWebRequest( url );
        printf( "Starting new web request for %s\n", url );
        
        delete [] url;

        StatRecord r = { stringDuplicate( inEmail ), 
                         // lookup
                         -1, 
                         request, -1,
                         -1, -1, false, "get_sequence_number" };
        records.push_back( r );
        // in progress
        return 0;
        }
    else {
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
    }



void recordPlayerLifeStats( char *inEmail, int inNumSecondsLived ) {

    if( useStatsServer ) {
        
        WebRequest *request;
        
        char *encodedEmail = URLUtils::urlEncode( inEmail );

        char *url = autoSprintf( 
            "%s?action=get_sequence_number"
            "&email=%s",
            statsServerURL,
            encodedEmail );
        
        delete [] encodedEmail;
        
        request = defaultTimeoutWebRequest( url );
        printf( "Starting new web request for %s\n", url );
        
        delete [] url;

        StatRecord r = { stringDuplicate( inEmail ), inNumSecondsLived, 
                         request, -1, 
                         -1, -1, false, "get_sequence_number" };
        records.push_back( r );
        }
    else {
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
                    r->error = true;
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
                    
                    char *url;

                    if( r->numGameSeconds == -1 ) {
                        // lookup
                        url = autoSprintf( 
                            "%s?action=get_stats"
                            "&email=%s"
                            "&sequence_number=%d"
                            "&hash_value=%s",
                            statsServerURL,
                            encodedEmail,
                            r->sequenceNumber,
                            hash );
                        r->serverAction = "get_stats";
                        }
                    else {
                        url = autoSprintf( 
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
                        r->serverAction = "log_game";
                        }
                    
                    
                    delete [] encodedEmail;
                    delete [] hash;

                    r->request = defaultTimeoutWebRequest( url );
                    printf( "Starting new web request for %s\n", url );
                    
                    delete [] url;
                    }
                }
            else {
                
                if( strstr( webResult, "DENIED" ) != NULL ) {
                    AppLog::infoF( 
                        "Server %s request rejected by stats server", 
                        r->serverAction );
                    r->error = true;
                    }
                else if( r->numGameSeconds == -1 && 
                         strstr( webResult, "OK" ) != NULL ) {
                    // lookup result
                    sscanf( webResult, "%d\n%d\n", 
                            &( r->gameCount ), &( r->gameTotalSeconds ) );
                    }
                
                recordDone = true;
                }
            
            delete [] webResult;
            }

        if( recordDone ) {
            if( r->numGameSeconds == -1 ) {
                // done lookup request
                doneLookups.push_back( *r );
                }
            else {
                delete r->request;
                delete [] r->email;
                }
            
            records.deleteElement( i );
            i--;
            }
        }
    }



char isUsingStatsServer() {
    return useStatsServer;
    }
