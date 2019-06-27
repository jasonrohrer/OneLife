#include "fitnessScore.h"


#include "minorGems/util/SettingsManager.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"


typedef struct OpRequest {
        char *email;
        // separated by &, should end with & too
        const char *extraParams;
        char isScoreRequest;
        float scoreResult;
        WebRequest *seqW;
        WebRequest *mainW;
    } OpRequest;

static SimpleVector<OpRequest> scoreRequests;

static SimpleVector<OpRequest> deathRequests;

static char *serverID = NULL;



void initFitnessScore() {
    serverID = SettingsManager::getStringSetting( "serverID", "testServer" );
    }



static void freeRequest( OpRequest *inR ) {
    
    if( inR->extraParams != NULL ) {
        delete [] inR->extraParams;
        inR->extraParams = NULL;
        }
    
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



void freeFitnessScore() {
    for( int i=0; i<scoreRequests.size(); i++ ) {
        OpRequest *r = scoreRequests.getElement( i );
        
        freeRequest( r );
        }
    scoreRequests.deleteAll();

    for( int i=0; i<deathRequests.size(); i++ ) {
        OpRequest *r = deathRequests.getElement( i );
        
        freeRequest( r );
        }
    deathRequests.deleteAll();

    if( serverID != NULL ) {
        delete [] serverID;
        serverID = NULL;
        }
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
            else if( r->isScoreRequest ) {
                sscanf( text, "%f", &( r->scoreResult ) );
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
                    "fitnessServerSharedSecret", 
                    "secret_phrase" );
        
            char *seqString = autoSprintf( "%d", sequenceNumber );

            char *hash = hmac_sha1( sharedSecret, seqString );
            
            delete [] seqString;
            
            delete [] sharedSecret;
            
            char *encodedEmail = URLUtils::urlEncode( r->email );
            
            char *serverURL = 
                SettingsManager::getStringSetting( 
                    "fitnessServerURL", "" );

            
            char *url = autoSprintf( 
                "%s?%s"
                "&email=%s"
                "&server_name=%s"
                "&sequence_number=%d"
                "&hash_value=%s",
                serverURL,
                r->extraParams,
                encodedEmail,
                serverID,
                sequenceNumber,
                hash );
            
            delete [] encodedEmail;
            delete [] hash;
            delete [] serverURL;
            
            r->mainW = new WebRequest( "GET", url, NULL );
            printf( "Starting new web request for %s\n", url );
                    
            delete [] url;
            }

        return 0;
        }
    else {
        // need to send seq request
        char *serverURL = 
            SettingsManager::getStringSetting( 
                "fitnessServerURL", "" );

        char *encodedName = URLUtils::urlEncode( serverID );
    
        char *url = autoSprintf( 
            "%s?action=get_server_sequence_number"
            "&server_name=%s",
            serverURL,
            encodedName );
    
        delete [] encodedName;
        delete [] serverURL;

        r->seqW = new WebRequest( "GET", url, NULL );
        printf( "Starting new web request for %s\n", url );

        delete [] url;
        
        return 0;
        }
    }




void stepFitnessScore() {
    // only need to step death requests here, because no one is checking
    // them
    
    // score requests are checked over and over (and thus stepped)
    // with repeated calls to getFitnessScore


    for( int i=0; i<deathRequests.size(); i++ ) {
        OpRequest *r = deathRequests.getElement( i );

        int result = stepOpRequest( r );
        if( result != 0 ) {
            // either error or done, stop either way
            deathRequests.deleteElement( i );
            i--;
            }
        }
    }



// return value:
// 0 still pending
// -1 DENIED
// 1 score ready (and returned in outScore)
int getFitnessScore( char *inEmail, float *outScore ) {
    
    for( int i=0; i<scoreRequests.size(); i++ ) {
        OpRequest *r = scoreRequests.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            // match
            
            int result = stepOpRequest( r );
            
            if( result == 1 ) {
                *outScore = r->scoreResult;
                }
            
            
            if( result != 0 ) {
                // done, deleted already
                scoreRequests.deleteElement( i );
                }
            return result;
            }
        }



    // didn't find a match in existing requests

    if( SettingsManager::getIntSetting( "useFitnessServer", 0 ) == 0 ||
        SettingsManager::getIntSetting( "remoteReport", 0 ) == 0 ) {
        // everyone has fitness 0 (default
        *outScore = 0;
        return 1;
        }

    
    // else using server, start a new spend request
    
    OpRequest r;
    
    r.email = stringDuplicate( inEmail );
    r.extraParams = stringDuplicate( "action=get_score&" );
    r.isScoreRequest = true;
    
    r.seqW = NULL;
    r.mainW = NULL;

    scoreRequests.push_back( r );

    return 0;
    }



void logFitnessDeath( char *inEmail, char *inName, int inDisplayID,
                      SimpleVector<char*> *inAncestorEmails,
                      SimpleVector<char*> *inAncestorRelNames ) {

    if( SettingsManager::getIntSetting( "useFitnessServer", 0 ) == 0 ||
        SettingsManager::getIntSetting( "remoteReport", 0 ) == 0 ) {
        // not using server
        return;
        }

    
    // else using server, start a new refund request
    
    OpRequest r;
    
    r.email = stringDuplicate( inEmail );
    

    char *encodedName = URLUtils::urlEncode( inName );


    SimpleVector<char> workingList;
    
    int num = inAncestorEmails->size();
    
    for( int i=0; i<num; i++ ) {
        workingList.appendElementString( 
            inAncestorEmails->getElementDirect( i ) );
        
        workingList.appendElementString( " " );
        
        char *relName = inAncestorRelNames->getElementDirect( i );
        
        char found;
        char *relNameNoSpace =
            replaceAll( relName, " ", "_", &found );
        
        workingList.appendElementString( relNameNoSpace );

        if( i < num-1 ) {    
            workingList.appendElementString( "," );
            }
        
        delete [] relNameNoSpace;
        }


    char *ancestorList = workingList.getElementString();
    
    char *encodedList = URLUtils::urlEncode( ancestorList );

    delete [] ancestorList;


    r.extraParams = 
        autoSprintf(
            "action=refund_token&"
            "name=%s&"
            "display_id=%d&"
            "self_rel_name=You&"
            "ancestor_list=%s",
            encodedName,
            inDisplayID,
            encodedList );

    delete [] encodedName;
    delete [] encodedList;

    r.isScoreRequest = false;

    r.seqW = NULL;
    r.mainW = NULL;

    deathRequests.push_back( r );
    }

