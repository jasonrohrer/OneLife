#include "lineageLog.h"
#include "serverCalls.h"


#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"




static char useLineageServer = false;

static char *lineageServerURL = NULL;

static char *serverID = NULL;


typedef struct LineageRecord {
        char *email;
        double age;
        int playerID, parentID, displayID;
        int killerID;
        char *name;
        char *lastSay;
        char male;
        
        WebRequest *request;
        // -1 until first request gets it
        int sequenceNumber;
    } LineageRecord;


static SimpleVector<LineageRecord> records;



void initLineageLog() {

    useLineageServer = 
        SettingsManager::getIntSetting( "useLineageServer", 0 ) &&
        SettingsManager::getIntSetting( "remoteReport", 0 );    
    
    lineageServerURL = 
        SettingsManager::
        getStringSetting( "lineageServerURL", 
                          "http://localhost/jcr13/reviewServer/server.php" );
    

    serverID = SettingsManager::getStringSetting( "serverID", "testServer" );
    }



void freeLineageLog() {

    if( lineageServerURL != NULL ) {
        delete [] lineageServerURL;
        lineageServerURL = NULL;
        }
    

    if( serverID != NULL ) {
        delete [] serverID;
        serverID = NULL;
        }

    for( int i=0; i<records.size(); i++ ) {
        delete records.getElement(i)->request;
        delete [] records.getElement(i)->email;
        delete [] records.getElement(i)->name;
        delete [] records.getElement(i)->lastSay;
        }
    records.deleteAll();
    }



        


void recordPlayerLineage( char *inEmail, double inAge,
                          int inPlayerID, int inParentID,
                          int inDisplayID, int inKillerID,
                          const char *inName,
                          const char *inLastSay,
                          char inMale ) {

    if( useLineageServer ) {

        if( inName == NULL ) {
            inName = "NAMELESS";
            }
        if( inLastSay == NULL ) {
            inLastSay = "";
            }
        
        
        WebRequest *request;
        
        char *encodedEmail = URLUtils::urlEncode( inEmail );

        char *url = autoSprintf( 
            "%s?action=get_sequence_number"
            "&email=%s",
            lineageServerURL,
            encodedEmail );
        
        delete [] encodedEmail;
        
        request = defaultTimeoutWebRequest( url );
        printf( "Starting new web request for %s\n", url );
        
        delete [] url;

        LineageRecord r = { stringDuplicate( inEmail ), inAge, 
                            inPlayerID, inParentID, inDisplayID,
                            inKillerID,
                            stringDuplicate( inName ),
                            stringDuplicate( inLastSay ),
                            inMale,
                            request, -1 };
        records.push_back( r );
        }
    }



void stepLineageLog() {
    if( ! useLineageServer ) {
        return;
        }
    
    for( int i=0; i<records.size(); i++ ) {
        LineageRecord *r = records.getElement( i );
        
        int result = r->request->step();
            
        char recordDone = false;

        if( result == -1 ) {
            AppLog::info( "Request to lineage server failed." );
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
                                  "from lineage server response." );
                    recordDone = true;
                    }
                else {
                    delete r->request;
                    
                    // start lineage-posting request

                    char *seqString = autoSprintf( "%d", r->sequenceNumber );
                    
                    char *lineageServerSharedSecret = 
                        SettingsManager::getStringSetting( 
                            "lineageServerSharedSecret", 
                            "secret_phrase" );


                    char *hash = hmac_sha1( lineageServerSharedSecret,
                                            seqString );
                    
                    delete [] lineageServerSharedSecret;

                    delete [] seqString;
                    
                    char *encodedEmail = URLUtils::urlEncode( r->email );
                    char *encodedName = URLUtils::urlEncode( r->name );
                    char *encodedLastSay = URLUtils::urlEncode( r->lastSay );
                    
                    int maleInt = 0;
                    if( r->male ) {
                        maleInt = 1;
                        }
                    
                    char *url = autoSprintf( 
                        "%s?action=log_life"
                        "&server=%s"
                        "&email=%s"
                        "&age=%f"
                        "&player_id=%d"
                        "&parent_id=%d"
                        "&display_id=%d"
                        "&killer_id=%d"
                        "&name=%s"
                        "&last_words=%s"
                        "&male=%d"
                        "&sequence_number=%d"
                        "&hash_value=%s",
                        lineageServerURL,
                        serverID,
                        encodedEmail,
                        r->age,
                        r->playerID,
                        r->parentID,
                        r->displayID,
                        r->killerID,
                        encodedName,
                        encodedLastSay,
                        maleInt,
                        r->sequenceNumber,
                        hash );
                    
                    delete [] encodedEmail;
                    delete [] encodedName;
                    delete [] encodedLastSay;
                    delete [] hash;

                    r->request = defaultTimeoutWebRequest( url );
                    printf( "Starting new web request for %s\n", url );
                    
                    delete [] url;
                    }
                }
            else {
                
                if( strstr( webResult, "DENIED" ) != NULL ) {
                    AppLog::info( 
                        "Server log_life request rejected by lineage server" );
                    }
                recordDone = true;
                }
            
            delete [] webResult;
            }

        if( recordDone ) {
            delete r->request;
            delete [] r->email;
            delete [] r->name;
            delete [] r->lastSay;
            
            records.deleteElement( i );
            i--;
            }
        }
    }

