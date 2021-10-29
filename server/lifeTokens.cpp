#include "serverCalls.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"


typedef struct OpRequest {
        char *email;
        const char *action;
        WebRequest *seqW;
        WebRequest *mainW;
    } OpRequest;

static SimpleVector<OpRequest> spendRequests;

static SimpleVector<OpRequest> refundRequests;



void initLifeTokens() {
    }



static void freeRequest( OpRequest *inR ) {
    
    if( inR->email != NULL ) {
        delete [] inR->email;
        inR->email = NULL;
        }
    if( inR->seqW != NULL ) {
        delete inR->seqW;
        inR->seqW = NULL;
        }
    if( inR->mainW != NULL ) {
        delete inR->mainW;
        inR->mainW = NULL;
        }
    }



void freeLifeTokens() {
    for( int i=0; i<spendRequests.size(); i++ ) {
        OpRequest *r = spendRequests.getElement( i );
        
        freeRequest( r );
        }
    spendRequests.deleteAll();

    for( int i=0; i<refundRequests.size(); i++ ) {
        OpRequest *r = refundRequests.getElement( i );
        
        freeRequest( r );
        }
    refundRequests.deleteAll();
    }



// returns 1 if op succeeded (or connection error preventing op entirely),
// 0 if in progress, 
// -1 if denied
// request is freed if return value not 0 
static int stepOpRequest( OpRequest *inR ) {
    OpRequest *r = inR;
    
    if( r->mainW != NULL ) {             
        
        int result = r->mainW->step();
                
        if( result == 0 ) {
            return 0;
            }
        else if( result == -1 ) {
            // error
            // maybe server is down
            // let player through anyway
                    
            freeRequest( r );
            return 1;
            }
        else if( result == 1 ) {
            // got result
            // make sure op is permitted
                    
            int returnVal = 1;
                    
            char *text = r->mainW->getResult();
                    
            if( strstr( text, "DENIED" ) != NULL ) {
                returnVal = -1;
                }
            delete [] text;
                    
            freeRequest( r );
            return returnVal;
            }
        
        return 0;
        }
    else if( r->seqW != NULL ) {
        // still waiting for result from sequence request

        int result = r->seqW->step();
        
        if( result == 0 ) {
            return 0;
            }
        else if( result == -1 ) {
            // error
            // maybe server is down
            // let player through anyway
            
            freeRequest( r );
            
            return 1;
            }
        else if( result == 1 ) {
            // got seq result
                    
            int sequenceNumber = 0;
            
            char *text = r->seqW->getResult();
            
            sscanf( text, "%d\nOK", &sequenceNumber );
            
            delete [] text;
                    

            char *sharedSecret = 
                SettingsManager::getStringSetting( 
                    "lifeTokenServerSharedSecret", 
                    "secret_phrase" );
        
            char *seqString = autoSprintf( "%d", sequenceNumber );

            char *hash = hmac_sha1( sharedSecret, seqString );
            
            delete [] seqString;
            
            delete [] sharedSecret;
            
            char *encodedEmail = URLUtils::urlEncode( r->email );
            
            char *serverURL = 
                SettingsManager::getStringSetting( 
                    "lifeTokenServerURL", "" );

            
            char *url = autoSprintf( 
                "%s?action=%s"
                "&email=%s"
                "&sequence_number=%d"
                "&hash_value=%s",
                serverURL,
                r->action,
                encodedEmail,
                sequenceNumber,
                hash );
            
            delete [] encodedEmail;
            delete [] hash;
            delete [] serverURL;
            
            r->mainW = defaultTimeoutWebRequest( url );
            printf( "Starting new web request for %s\n", url );
                    
            delete [] url;
            }

        return 0;
        }
    else {
        // need to send seq request
        char *serverURL = 
            SettingsManager::getStringSetting( 
                "lifeTokenServerURL", "" );

        char *encodedEmail = URLUtils::urlEncode( r->email );
    
        char *url = autoSprintf( 
            "%s?action=get_sequence_number"
            "&email=%s",
            serverURL,
            encodedEmail );
    
        delete [] encodedEmail;
        delete [] serverURL;

        r->seqW = defaultTimeoutWebRequest( url );
        printf( "Starting new web request for %s\n", url );

        delete [] url;
        
        return 0;
        }
    }




void stepLifeTokens() {
    // only need to step refund requests here, because no one is checking
    // them
    
    // spend requests are checked over and over (and thus stepped)
    // with repeated calls to spendLifeToken


    for( int i=0; i<refundRequests.size(); i++ ) {
        OpRequest *r = refundRequests.getElement( i );

        int result = stepOpRequest( r );
        if( result != 0 ) {
            // either error or done, stop either way
            refundRequests.deleteElement( i );
            i--;
            }
        }
    }



// return value:
// 0 still pending
// -1 DENIED
// 1 spent  (or not using life token server)
int spendLifeToken( char *inEmail ) {
    
    for( int i=0; i<spendRequests.size(); i++ ) {
        OpRequest *r = spendRequests.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            // match
            
            int result = stepOpRequest( r );
            
            
            if( result != 0 ) {
                // done, deleted already
                spendRequests.deleteElement( i );
                }
            return result;
            }
        }



    // didn't find a match in existing requests

    if( SettingsManager::getIntSetting( "useLifeTokenServer", 0 ) == 0 ||
        SettingsManager::getIntSetting( "remoteReport", 0 ) == 0 ) {
        // not using server, allow all
        return 1;
        }

    
    // else using server, start a new spend request
    
    OpRequest r;
    
    r.email = stringDuplicate( inEmail );
    r.action = "spend_token";
    
    r.seqW = NULL;
    r.mainW = NULL;

    spendRequests.push_back( r );

    return 0;
    }



void refundLifeToken( char *inEmail ) {
    if( SettingsManager::getIntSetting( "useLifeTokenServer", 0 ) == 0 ||
        SettingsManager::getIntSetting( "remoteReport", 0 ) == 0 ) {
        // not using server
        return;
        }

    
    // else using server, start a new refund request
    
    OpRequest r;
    
    r.email = stringDuplicate( inEmail );
    r.action = "refund_token";
    
    r.seqW = NULL;
    r.mainW = NULL;

    refundRequests.push_back( r );
    }

