#include "ahapGate.h"
#include "serverCalls.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/log/AppLog.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"




typedef struct GrantActionRecord {
        char *email;

        WebRequest *request;
        
        int sequenceNumber;
        
    } GrantActionRecord;



static SimpleVector<GrantActionRecord> grantRecords;



typedef struct VoteActionRecord {
        char *voterEmail;
        char *voteForEmail;

        WebRequest *request;
        
        int sequenceNumber;
        
    } VoteActionRecord;



static SimpleVector<VoteActionRecord> voteRecords;



void initAHAPGate() {
    }


static void freeRecord( GrantActionRecord *inR ) {
    delete [] inR->email;
    delete inR->request;
    }

static void freeRecord( VoteActionRecord *inR ) {
    delete [] inR->voterEmail;
    delete [] inR->voteForEmail;
    delete inR->request;
    }

    


void freeAHAPGate() {
    for( int i=0; i < grantRecords.size(); i++ ) {
        GrantActionRecord *r = grantRecords.getElement( i );
        freeRecord( r );
        }
    
    grantRecords.deleteAll();

    for( int i=0; i < voteRecords.size(); i++ ) {
        VoteActionRecord *r = voteRecords.getElement( i );
        freeRecord( r );
        }
    
    voteRecords.deleteAll();
    }





static AHAPGrantResult *stepGrantRecords() {
    if( grantRecords.size() == 0 ) {
        return NULL;
        }

    // process grantRecords in order
    // we can only work on one per step, since we return the result
    GrantActionRecord *r = grantRecords.getElement( 0 );
    
    int result = r->request->step();
    
    if( result == -1 ) {
        AppLog::info( "Request to ahapGate server failed." );
        
        freeRecord( r );
        grantRecords.deleteElement( 0 );
        return NULL;
        }
    else if( result == 1 ) {
        // done, have result

        char *webResult = r->request->getResult();
        
        printf( "ahapGate got web result:\n%s\n\n", webResult );

        if( r->sequenceNumber == -1 ) {
            // still waiting for sequence number response

            int numRead = sscanf( webResult, "%d", &( r->sequenceNumber ) );

            delete [] webResult;

            if( numRead != 1 ) {
                AppLog::info( "Failed to read sequence number "
                              "from ahapGate server response." );
                
                freeRecord( r );
                grantRecords.deleteElement( 0 );
                return NULL;
                }
            else {
                delete r->request;
                r->request = NULL;
                
                // start grant request
                        
                /*
                  server.php
                  ?action=grant
                  &email=[email address]
                  &sequence_number=[int]
                  &hash_value=[hash value]
                */
                char *ahapGateURL =
                    SettingsManager::
                    getStringSetting( 
                        "ahapGateURL", 
                        "http://localhost/jcr13/ahapGate/server.php" );
                
                char *ahapGateSharedSecret =
                    SettingsManager::
                    getStringSetting( 
                        "ahapGateSharedSecret", 
                        "secret_phrase" );
                
                char *encodedEmail = URLUtils::urlEncode( r->email );
                
                //  HMAC_SHA1( $shared_secret, $sequence_number$email )
                
                char *stringToHash = autoSprintf( "%d%s", r->sequenceNumber,
                                                  r->email );
                
                char *hash = hmac_sha1( ahapGateSharedSecret,
                                        stringToHash );
                
                delete [] ahapGateSharedSecret;
                delete [] stringToHash;

                
                char *url = autoSprintf( 
                    "%s?action=grant"
                    "&email=%s"
                    "&sequence_number=%d"
                    "&hash_value=%s",
                    ahapGateURL,
                    encodedEmail,
                    r->sequenceNumber,
                    hash );
                
                delete [] encodedEmail;
                delete [] ahapGateURL;
                delete [] hash;
                
                r->request = defaultTimeoutWebRequest( url );
                printf( "ahapGate starting new web request for %s\n", url );
                
                delete [] url;
                
                // still processing
                return NULL;
                }
            }
        else {
            // waiting for grant response

            /*
              Return:
              assigned_steam_key
              view_account_url
              OK
            */
            
            if( strstr( webResult, "DENIED" ) != NULL ||
                strstr( webResult, "OK" ) == NULL ) {
                delete [] webResult;
                
                AppLog::info( "grant action DENIED by ahapGate server." );
                
                freeRecord( r );
                grantRecords.deleteElement( 0 );
                return NULL;
                }

            char steamKeyBuffer[50];
            char urlBuffer[200];
            
            steamKeyBuffer[0] = '\0';
            urlBuffer[0] = '\0';
            
            int numRead = sscanf( webResult, "%s\n%s\nOK", 
                                  steamKeyBuffer, urlBuffer );
            delete [] webResult;
            
            if( numRead != 2 ) {
                AppLog::info( "Bad grant response from ahapGate server." );
                
                freeRecord( r );
                grantRecords.deleteElement( 0 );
                return NULL;
                }
            
            AHAPGrantResult *g = new AHAPGrantResult;
            
            g->email = stringDuplicate( r->email );
            g->steamKey = stringDuplicate( steamKeyBuffer );
            g->accountURL = stringDuplicate( urlBuffer );
            
            freeRecord( r );
            grantRecords.deleteElement( 0 );
            
            return g;
            }
        
        }

    // else still waiting for result from next request on list
    return NULL;
    }


static void stepVoteRecords() {
    if( voteRecords.size() == 0 ) {
        return;
        }
    
    for( int i=0; i<voteRecords.size(); i++ ) {
        
        VoteActionRecord *r = voteRecords.getElement( i );
        
        int result = r->request->step();
    
        if( result == -1 ) {
            AppLog::info( "Request to ahapGate server failed." );
        
            freeRecord( r );
            grantRecords.deleteElement( 0 );
            i--;
            }
        else if( result == 1 ) {
            // done, have result

            char *webResult = r->request->getResult();
        
            printf( "ahapGate got web result:\n%s\n\n", webResult );

            if( r->sequenceNumber == -1 ) {
                // still waiting for sequence number response

                int numRead = sscanf( webResult, "%d", &( r->sequenceNumber ) );

                delete [] webResult;

                if( numRead != 1 ) {
                    AppLog::info( "Failed to read sequence number "
                                  "from ahapGate server response." );
                
                    freeRecord( r );
                    voteRecords.deleteElement( i );
                    i--;
                    }
                else {
                    delete r->request;
                    r->request = NULL;
                
                    // start vote request
                        
                    /*
                      server.php
                      ?action=register_vote
                      &email=[email address]
                      &leader_email=[email address]
                      &sequence_number=[int]
                      &hash_value=[hash value]
                    */
                    char *ahapGateURL =
                        SettingsManager::
                        getStringSetting( 
                            "ahapGateURL", 
                            "http://localhost/jcr13/ahapGate/server.php" );
                
                    char *ahapGateSharedSecret =
                        SettingsManager::
                        getStringSetting( 
                            "ahapGateSharedSecret", 
                            "secret_phrase" );
                
                    char *encodedVoterEmail = 
                        URLUtils::urlEncode( r->voterEmail );
                    
                    char *encodedVoteForEmail = 
                        URLUtils::urlEncode( r->voteForEmail );
                
                    //  HMAC_SHA1( $shared_secret, 
                    //             $sequence_number$email$leaderEmaiil )
                
                    char *stringToHash = autoSprintf( "%d%s%s", 
                                                      r->sequenceNumber,
                                                      r->voterEmail,
                                                      r->voteForEmail );
                
                    char *hash = hmac_sha1( ahapGateSharedSecret,
                                            stringToHash );
                
                    delete [] ahapGateSharedSecret;
                    delete [] stringToHash;

                
                    char *url = autoSprintf( 
                        "%s?action=register_vote"
                        "&email=%s"
                        "&leader_email=%s"
                        "&sequence_number=%d"
                        "&hash_value=%s",
                        ahapGateURL,
                        encodedVoterEmail,
                        encodedVoteForEmail,
                        r->sequenceNumber,
                        hash );
                
                    delete [] encodedVoterEmail;
                    delete [] encodedVoteForEmail;
                    delete [] ahapGateURL;
                    delete [] hash;
                
                    r->request = defaultTimeoutWebRequest( url );
                    printf( "ahapGate starting new web request for %s\n", url );
                
                    delete [] url;
                    }
                }
            else {
                // waiting for register_vote response

                /*
                  Return:
                  OK
                */
            
                if( strstr( webResult, "DENIED" ) != NULL ||
                    strstr( webResult, "OK" ) == NULL ) {
                    AppLog::info( "register_vote "
                                  "action DENIED by ahapGate server." );
                    }
                
                delete [] webResult;
                    
                freeRecord( r );
                voteRecords.deleteElement( i );
                i--;
                }
            }
        }
    }





AHAPGrantResult *stepAHAPGate() {
    
    stepVoteRecords();
    
    return stepGrantRecords();
    }



WebRequest *startSequenceNumberRequest( const char *inEmail ) {
    char *ahapGateURL =
        SettingsManager::
        getStringSetting( "ahapGateURL", 
                          "http://localhost/jcr13/ahapGate/server.php" );
    
    char *encodedEmail = URLUtils::urlEncode( (char*)inEmail );

    char *url = autoSprintf( 
        "%s?action=get_sequence_number"
        "&email=%s",
        ahapGateURL,
        encodedEmail );
        
    delete [] encodedEmail;
    delete [] ahapGateURL;
    
    WebRequest *r = defaultTimeoutWebRequest( url );
    printf( "ahapGate starting new web request for %s\n", url );
        
    delete [] url;

    return r;
    }




void triggerAHAPGrant( const char *inEmail ) {
    GrantActionRecord r;
    
    r.email = stringDuplicate( inEmail );
    r.sequenceNumber = -1;

    r.request = startSequenceNumberRequest( inEmail );    
    
    grantRecords.push_back( r );    
    }



void triggerAHAPVote( const char *inVoterEmail, const char *inVoteForEmail ) {
    VoteActionRecord r;
        
    r.voterEmail = stringDuplicate( inVoterEmail );
    r.voteForEmail = stringDuplicate( inVoteForEmail );
    
    r.sequenceNumber = -1;
    
    r.request = startSequenceNumberRequest( inVoterEmail );    
    
    voteRecords.push_back( r );
    }




