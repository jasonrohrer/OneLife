#include "fitnessScore.h"
#include "serverCalls.h"


#include "minorGems/util/SettingsManager.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"


typedef struct FitnessFitnessOpRequest {
        char *email;
        // separated by &, should end with & too
        const char *extraParams;
        char isScoreRequest;
        float scoreResult;
        WebRequest *seqW;
        WebRequest *mainW;
        char isDeathRequest;
        int lastStepResult;
    } FitnessOpRequest;

static SimpleVector<FitnessOpRequest> allRequests;

static char *serverID = NULL;



void initFitnessScore() {
    serverID = SettingsManager::getStringSetting( "serverID", "testServer" );
    }



static void freeRequest( FitnessOpRequest *inR ) {
    
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
    for( int i=0; i<allRequests.size(); i++ ) {
        FitnessOpRequest *r = allRequests.getElement( i );
        
        freeRequest( r );
        }
    allRequests.deleteAll();

    if( serverID != NULL ) {
        delete [] serverID;
        serverID = NULL;
        }
    }



// returns 1 if op succeeded (or connection error preventing op entirely),
// 0 if in progress, 
// -1 if denied
// request is freed if return value not 0 
static int stepOpRequest( FitnessOpRequest *inR ) {
    FitnessOpRequest *r = inR;
    
    if( r->mainW != NULL ) {             
        
        int result = r->mainW->step();
                
        if( result == 0 ) {
            return 0;
            }
        else if( result == -1 ) {
            // error
            // maybe server is down
            // let player through anyway
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
                "fitnessServerURL", "" );

        char *encodedName = URLUtils::urlEncode( serverID );
    
        char *url = autoSprintf( 
            "%s?action=get_server_sequence_number"
            "&server_name=%s",
            serverURL,
            encodedName );
    
        delete [] encodedName;
        delete [] serverURL;

        r->seqW = defaultTimeoutWebRequest( url );
        printf( "Starting new web request for %s\n", url );

        delete [] url;
        
        return 0;
        }
    }




void stepFitnessScore() {

    // step oldest request here that's not done/error
    
    // do NOT interleave them
    
    for( int i=0; i<allRequests.size(); i++ ) {
        FitnessOpRequest *r = allRequests.getElement( i );

        if( r->lastStepResult == 0 ) {
            
            r->lastStepResult = stepOpRequest( r );

            if( r->isDeathRequest && r->lastStepResult != 0 ) {
                // no one is waiting for results from death request
                // either error or done, stop either way

                freeRequest( r );
                allRequests.deleteElement( i );
                }
            // only process latest un-done request
            break;
            }
        }
    }



// return value:
// 0 still pending
// -1 DENIED
// 1 score ready (and returned in outScore)
int getFitnessScore( char *inEmail, float *outScore ) {

    stepFitnessScore();
    
    
    for( int i=0; i<allRequests.size(); i++ ) {
        FitnessOpRequest *r = allRequests.getElement( i );
        
        if( ! r->isDeathRequest && 
            strcmp( r->email, inEmail ) == 0 ) {
            // match
            
            int result = r->lastStepResult;
            
            if( result == 1 ) {
                *outScore = r->scoreResult;
                }
            
            
            if( result != 0 ) {
                // done
                freeRequest( r );
                allRequests.deleteElement( i );
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
    
    FitnessOpRequest r;
    
    r.email = stringDuplicate( inEmail );
    r.extraParams = stringDuplicate( "action=get_score&" );
    r.isScoreRequest = true;
    
    r.seqW = NULL;
    r.mainW = NULL;

    r.isDeathRequest = false;
    r.lastStepResult = 0;
    
    allRequests.push_back( r );

    return 0;
    }



void logFitnessDeath( int inNumLivePlayers,
                      char *inEmail, char *inName, int inDisplayID,
                      double inAge,
                      SimpleVector<char*> *inAncestorEmails,
                      SimpleVector<char*> *inAncestorRelNames ) {

    if( SettingsManager::getIntSetting( "useFitnessServer", 0 ) == 0 ||
        SettingsManager::getIntSetting( "remoteReport", 0 ) == 0 ) {
        // not using server
        return;
        }

    if( inNumLivePlayers < 
        SettingsManager::getIntSetting( "minActivePlayersForFitness", 15 ) ) {
        // not enough players for this to count
        return;
        }
    
        

    
    // else using server, start a new death request
    
    FitnessOpRequest r;
    
    r.email = stringDuplicate( inEmail );
    

    if( inName == NULL ) {
        inName = (char*)"NAMELESS";
        }

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
            "action=report_death&"
            "name=%s&"
            "display_id=%d&"
            "self_rel_name=You&"
            "age=%f&"
            "ancestor_list=%s",
            encodedName,
            inDisplayID,
            inAge,
            encodedList );

    delete [] encodedName;
    delete [] encodedList;

    r.isScoreRequest = false;

    r.seqW = NULL;
    r.mainW = NULL;
    
    r.isDeathRequest = true;
    r.lastStepResult = 0;
    

    allRequests.push_back( r );
    }

