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



static SimpleVector<GrantActionRecord> records;



void initAHAPGate() {
    }


static void freeRecord( GrantActionRecord *inR ) {
    delete [] inR->email;
    delete inR->request;
    }



void freeAHAPGate() {
    for( int i=0; i < records.size(); i++ ) {
        GrantActionRecord *r = records.getElement( i );
        freeRecord( r );
        }
    
    records.deleteAll();
    }



AHAPGrantResult *stepAHAPGate() {

    if( records.size() == 0 ) {
        return NULL;
        }
    
    // process records in order
    // we can only work on one per step, since we return the result
    GrantActionRecord *r = records.getElement( 0 );
    
    int result = r->request->step();
    
    if( result == -1 ) {
        AppLog::info( "Request to ahapGate server failed." );
        
        freeRecord( r );
        records.deleteElement( 0 );
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
                records.deleteElement( 0 );
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
                records.deleteElement( 0 );
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
                records.deleteElement( 0 );
                return NULL;
                }
            
            AHAPGrantResult *g = new AHAPGrantResult;
            
            g->email = stringDuplicate( r->email );
            g->steamKey = stringDuplicate( steamKeyBuffer );
            g->accountURL = stringDuplicate( urlBuffer );
            
            freeRecord( r );
            records.deleteElement( 0 );
            
            return g;
            }
        
        }

    // else still waiting for result from next request on list
    return NULL;
    }



void triggerAHAPGrant( const char *inEmail ) {
    GrantActionRecord r;
    
    r.email = stringDuplicate( inEmail );
    r.sequenceNumber = -1;
    
    
    char *ahapGateURL =
        SettingsManager::
        getStringSetting( "ahapGateURL", 
                          "http://localhost/jcr13/ahapGate/server.php" );
    
    char *encodedEmail = URLUtils::urlEncode( r.email );

    char *url = autoSprintf( 
        "%s?action=get_sequence_number"
        "&email=%s",
        ahapGateURL,
        encodedEmail );
        
    delete [] encodedEmail;
    delete [] ahapGateURL;
    
    r.request = defaultTimeoutWebRequest( url );
    printf( "ahapGate starting new web request for %s\n", url );
        
    delete [] url;

    
    records.push_back( r );
    }


