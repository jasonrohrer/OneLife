#include "arcReport.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"


extern double secondsPerYear;

// only one request at a time
static WebRequest *webRequest = NULL;
static int sequenceNumber = -1;


void freeArcReport() {
    if( webRequest != NULL ) {
        delete webRequest;
        webRequest = NULL;
        }
    sequenceNumber = -1;
    }



void reportArcEnd() {
    if( webRequest != NULL ) {
        // already have a request pending
        return;
        }
    
    char sendReport = 
        SettingsManager::getIntSetting( "useArcServer", 0 ) &&
        SettingsManager::getIntSetting( "remoteReport", 0 );    
    
    if( !sendReport ) {
        return;
        }
    

    char *serverURL = 
        SettingsManager::getStringSetting( "arcServerURL", "" );
    
    if( strcmp( serverURL, "" ) == 0 ) {
        delete [] serverURL;
        return;
        }
    
    char *url = autoSprintf( "%s?action=get_sequence_number", serverURL );
    delete [] serverURL;

    sequenceNumber = -1;
    
    printf( "Starting new web request for %s\n", url );
    
    webRequest = new WebRequest( "GET", url, NULL );
    delete [] url;
    }




void stepArcReport() {
    if( webRequest == NULL ) {
        return;
        }

    int v = webRequest->step();
    
    if( v == -1 ) {
        // error
        delete webRequest;
        webRequest = NULL;
        return;
        }
    
    if( v == 0 ) {
        return;
        }
    

    // result ready
    char *result = webRequest->getResult();
    delete webRequest;
    webRequest = NULL;
    
    
    if( sequenceNumber == -1 ) {
        sscanf( result, "%d", &sequenceNumber );
        
        if( sequenceNumber != -1 ) {
            // got a valid number
            // start report

            char *arcServerURL = 
                SettingsManager::getStringSetting( "arcServerURL", "" );

            char *sharedSecret = 
                SettingsManager::getStringSetting( 
                    "arcServerSharedSecret", "" );

            char *seqString = autoSprintf( "%d", sequenceNumber );
                    
            char *hash = hmac_sha1( sharedSecret,
                                    seqString );
            delete [] seqString;
            delete [] sharedSecret;

            char *serverName = 
                SettingsManager::getStringSetting( "serverID", "" );

            
            char *url = autoSprintf( 
                "%s?action=report_arc_end"
                "&server_name=%s"
                "&seconds_per_year=%f"
                "&sequence_number=%d"
                "&hash_value=%s",
                arcServerURL,
                serverName,
                secondsPerYear,
                sequenceNumber,
                hash );
                    
            
            delete [] arcServerURL;
            delete [] hash;
            delete [] serverName;

            
            printf( "Starting new web request for %s\n", url );
            
            webRequest = new WebRequest( "GET", url, NULL );
            delete [] url;
            }
        }
    else {
        // this was our main request
        // result doesn't matter
        }
    
    delete [] result;
    }


