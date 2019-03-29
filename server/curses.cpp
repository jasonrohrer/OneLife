 #include "curses.h"
#include "curseLog.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/system/Time.h"


#include "minorGems/network/web/WebRequest.h"
#include "minorGems/network/web/URLUtils.h"

#include "minorGems/crypto/hashes/sha1.h"



#include <stdlib.h>



typedef struct CurseRecord {
        char *email;
        int tokens;
        int score;

        char alive;

        char bornCursed;

        double aliveStartTimeSinceTokenSpent;
        double aliveStartTimeSinceScoreDecrement;
        
        double livedTimeSinceTokenSpent;
        double livedTimeSinceScoreDecrement;

        double lastBirthTime;
    } CurseRecord;


static SimpleVector<CurseRecord> curseRecords;


typedef struct RemoteUpdateRecord {
        char *email;
        
        // used to compose final URL
        // everything after url ? and before &email=
        // for example:  "action=live_time&seconds=2092.4"
        char *actionString;

        // -1 if not fetched from server yet
        int sequenceNumber;
        WebRequest *request;
    } RemoteUpdateRecord;

    

static SimpleVector<RemoteUpdateRecord> serverPendingUpdates;


typedef struct RemoteCurseRecord {
        char *email;
        // -1 if we have no response yet
        int curseStatus;
        
        int excessCursePoints;
        
        WebRequest *request;
    } RemoteCurseRecord;
    

static SimpleVector<RemoteCurseRecord> serverRemoteRecords;




typedef struct PlayerNameRecord {
        char *name;
        char *email;
        double timeCreated;
    } PlayerNameRecord;



// a full hour after their full life is over, we can clear their name
// no one living remembers them
static double playerNameTimeout = 2 * 3600;

static SimpleVector<PlayerNameRecord> playerNames;


// players that have gotten new tokens recently
static SimpleVector<char*> newTokenEmails;



static int useCurseServer = false;

static char *curseServerURL = NULL;




void initCurses() {
    initCurseLog();

    FILE *f = fopen( "curseSave.txt", "r" );
    
    if( f != NULL ) {
        char email[100];
        int tokens;
        int score;
        
        double livedTimeSinceTokenSpent;
        double livedTimeSinceScoreDecrement;

        int numRead = 5;
        
        while( numRead == 5 ) {
            numRead = fscanf( f, "%99s %d %d %lf %lf", email, &tokens, &score,
                              &livedTimeSinceTokenSpent, 
                              &livedTimeSinceScoreDecrement );
            
            if( numRead == 5 ) {
                CurseRecord r;
                r.email = stringDuplicate( email );
                r.tokens = tokens;
                r.score = score;
                r.alive = false;
                r.bornCursed = false;
                r.aliveStartTimeSinceScoreDecrement = 0;
                r.aliveStartTimeSinceScoreDecrement = 0;
                
                r.livedTimeSinceTokenSpent = livedTimeSinceTokenSpent;
                r.livedTimeSinceScoreDecrement = livedTimeSinceScoreDecrement;
                r.lastBirthTime = Time::getCurrentTime();
                
                curseRecords.push_back( r );
                }
            }
        fclose( f );
        }

    useCurseServer = 
        SettingsManager::getIntSetting( "useCurseServer", 0 ) &&
        SettingsManager::getIntSetting( "remoteReport", 0 );
    
    if( useCurseServer ) {
        AppLog::info( "Using remote curse server." );
        
        curseServerURL = 
            SettingsManager::getStringSetting( "curseServerURL", "" );
        }
    }





void freeCurses() {
    
    FILE *f = fopen( "curseSave.txt", "w" );
    
    if( f != NULL ) {
        for( int i=0; i<curseRecords.size(); i++ ) {
            CurseRecord *r = curseRecords.getElement( i );
            
            fprintf( f, "%s %d %d %.f %.f\n", r->email, r->tokens, r->score,
                     r->livedTimeSinceTokenSpent, 
                     r->livedTimeSinceScoreDecrement );
            }
        fclose( f );
        }


    for( int i=0; i<playerNames.size(); i++ ) {
        PlayerNameRecord *r = playerNames.getElement( i );
        
        delete [] r->name;
        delete [] r->email;
        }
    playerNames.deleteAll();



    for( int i=0; i<curseRecords.size(); i++ ) {
        CurseRecord *r = curseRecords.getElement( i );
        
        delete [] r->email;
        }
    curseRecords.deleteAll();


    newTokenEmails.deallocateStringElements();
    
    freeCurseLog();

    if( curseServerURL != NULL ) {
        delete [] curseServerURL;
        curseServerURL = NULL;
        }

    for( int i=0; i<serverRemoteRecords.size(); i++ ) {
        RemoteCurseRecord r = serverRemoteRecords.getElementDirect( i );
        delete [] r.email;
        delete r.request;
        }
    serverRemoteRecords.deleteAll();

    for( int i=0; i<serverPendingUpdates.size(); i++ ) {
        RemoteUpdateRecord r = serverPendingUpdates.getElementDirect( i );
        delete [] r.email;
        delete [] r.actionString;
        delete r.request;
        }
    serverPendingUpdates.deleteAll();
    }



static double lastCurseSettingCheckTime = 0;

static double curseSettingCheckInterval = 10;

static double tokenTime = 7200.0;
static double decrementTime = 3600.0;


static void stepCurses() {

    

    double curTime = Time::getCurrentTime();
    
    if( curTime - lastCurseSettingCheckTime > curseSettingCheckInterval ) {
        tokenTime = SettingsManager::getFloatSetting( "curseTokenTime",
                                                      7200.0 );
        decrementTime = 
            SettingsManager::getFloatSetting( "curseDecrementTime", 3600.0 );
        
        lastCurseSettingCheckTime = curTime;
        }
    

    for( int i=0; i<playerNames.size(); i++ ) {
        PlayerNameRecord *r = playerNames.getElement( i );
        
        if( curTime - playerNameTimeout > r->timeCreated ) {
            delete [] r->name;
            delete [] r->email;
            playerNames.deleteElement( i );
            i--;
            }
        else {
            // records are in order by time, all remaining records too new
            break;
            }
        }

    for( int i=0; i<curseRecords.size(); i++ ) {
        CurseRecord *r = curseRecords.getElement( i );
    
        
        if( r->tokens == 0 ) {
            double aliveTime = 0;
            if( r->alive ) {
                aliveTime = curTime - r->aliveStartTimeSinceTokenSpent;
                }


            if( r->livedTimeSinceTokenSpent + aliveTime >= tokenTime ) {
                r->tokens ++;
                r->livedTimeSinceTokenSpent = 0;
                r->aliveStartTimeSinceTokenSpent = curTime;
                
                newTokenEmails.push_back( stringDuplicate( r->email ) );
                }
            }
        if( r->score > 0 ) {
            double aliveTime = 0;
            if( r->alive ) {
                aliveTime = curTime - r->aliveStartTimeSinceScoreDecrement;
                }


            if( r->livedTimeSinceScoreDecrement + aliveTime >= decrementTime ) {
                r->score --;
                
                logCurseScore( r->email, r->score );
                
                r->livedTimeSinceScoreDecrement = 0;
                r->aliveStartTimeSinceScoreDecrement = curTime;
                }
            }


        if( ! r->alive )
        if( ( r->tokens == 1 && r->score == 0 )
            ||
            curTime - r->lastBirthTime > 3600 * 96 ) {
            // nothing to track for this record
            // or record stale by 96 hours
            
            // delete it
            delete [] r->email;
            curseRecords.deleteElement( i );
            i--;
            }
        }
    
    }




// never returns NULL
static CurseRecord *findCurseRecord( char *inEmail ) {
    for( int i=0; i<curseRecords.size(); i++ ) {
        CurseRecord *r = curseRecords.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            return r;
            }
        }

    
    // always makes new if it doesn't exist
    double curTime = Time::getCurrentTime();

    CurseRecord r = { stringDuplicate( inEmail ),
                      // starts with 1 token
                      1,
                      // starts with 0 score
                      0,
                      false,
                      false,
                      0,
                      0,
                      0,
                      0,
                      curTime };
    
    
    curseRecords.push_back( r );
    
    return curseRecords.getElement( curseRecords.size() - 1 );
    }



static CurseStatus getCurseLevel( CurseRecord *inRecord ) {

    CurseStatus returnRec = { 0, 0 };
    

    if( inRecord == NULL ) {
        return returnRec;
        }
    
    int score = inRecord->score;
    
    if( score == 0 ) {
        return returnRec;
        }
    
    int thresh = SettingsManager::getIntSetting( "curseThreshold", 10 );
    
    if( score < thresh ) {
        return returnRec;
        }

    
    returnRec.curseLevel = score / thresh;
    returnRec.excessPoints = ( score - thresh ) + 1;

    return returnRec;
    }






void cursesLogBirth( char *inEmail ) {
    stepCurses();
    
    CurseRecord *r = findCurseRecord( inEmail );
    
    double curTime = Time::getCurrentTime();
    
    r->alive = true;
    
    r->bornCursed = ( getCurseLevel( r ).curseLevel > 0 );
    r->aliveStartTimeSinceTokenSpent = curTime;
    r->aliveStartTimeSinceScoreDecrement = curTime;
    
    r->lastBirthTime = curTime;
    }



void cursesLogDeath( char *inEmail, double inAge ) {
    CurseRecord *r = findCurseRecord( inEmail );
    
    if( r->alive ) {
        
        r->alive = false;

        double lifeTimeSinceToken = 
            Time::getCurrentTime() - r->aliveStartTimeSinceTokenSpent;
        double lifeTimeSinceScoreDecrement = 
            Time::getCurrentTime() - r->aliveStartTimeSinceScoreDecrement;
        
        r->livedTimeSinceTokenSpent += lifeTimeSinceToken;
        r->livedTimeSinceScoreDecrement += lifeTimeSinceScoreDecrement;
        }

    if( useCurseServer ) {
        double sec = inAge * 60;
        
        RemoteUpdateRecord r = {
            stringDuplicate( inEmail ),
            autoSprintf( "action=live_time&seconds=%f", sec ),
            -1,
            NULL };
        
        
        

        serverPendingUpdates.push_back( r );
        }
    }



// can return NULL if name not found
static CurseRecord *findCurseRecordByName( char *inName ) {
    
    char *email = NULL;
    
    for( int i=0; i<playerNames.size(); i++ ) {
        PlayerNameRecord *r = playerNames.getElement( i );
        
        if( strcmp( r->name, inName ) == 0 ) {
            email = r->email;
            break;
            }
        }
    if( email != NULL ) {
        return findCurseRecord( email );
        }
    return NULL;
    }




char hasCurseToken( char *inEmail ) {
    stepCurses();    

    CurseRecord *r = findCurseRecord( inEmail );
    
    if( r->tokens < 1 ) {  
        return false;
        }
    return true;
    }




void getNewCurseTokenHolders( SimpleVector<char*> *inEmailList ) {
    stepCurses();
    
    // some of newTokenEmails might be stale now and no longer have
    // curse points
    for( int i=0; i<newTokenEmails.size(); i++ ) {
        
        char *email = newTokenEmails.getElementDirect( i );
        
        CurseRecord *r = findCurseRecord( email );

        if( r != NULL && r->tokens > 0 ) {   
            inEmailList->push_back( email );
            }
        else {
            delete [] email;
            }
        }
    newTokenEmails.deleteAll();
    }





char cursePlayer( int inGiverID, char *inGiverEmail, 
                  char *inReceiverName ) {
    stepCurses();
    
    CurseRecord *receiverRecord = findCurseRecordByName( inReceiverName );
    
    if( receiverRecord == NULL ) {
        return false;
        }
    
    if( !useCurseServer && receiverRecord->bornCursed ) {
        // already getting born cursed, from local curses, 
        // leave them alone for now
        return false;
        }

    if( strcmp( receiverRecord->email, inGiverEmail ) == 0 ) {
        // giver is receiver, block
        return false;
        }
    
    
    CurseRecord *giverRecord = findCurseRecord( inGiverEmail );
    
    
    if( giverRecord->tokens < 1 ) {  
        // giver has no tokens left
        return false;
        }

    giverRecord->tokens -= 1;
    
    double curTime = Time::getCurrentTime();

    if( giverRecord->alive ) {
        giverRecord->aliveStartTimeSinceTokenSpent = curTime;
        }
    giverRecord->livedTimeSinceTokenSpent = 0;
    
    receiverRecord->score ++;
    receiverRecord->livedTimeSinceScoreDecrement = 0;
    
    if( receiverRecord->alive ) {
        receiverRecord->aliveStartTimeSinceScoreDecrement = curTime;
        }

    logCurse( inGiverID, inGiverEmail, receiverRecord->email );
    
    logCurseScore( receiverRecord->email, receiverRecord->score );
    

    if( useCurseServer ) {

        RemoteUpdateRecord r = {
            stringDuplicate( receiverRecord->email ),
            stringDuplicate( "action=curse" ),
            -1,
            NULL };
        
        serverPendingUpdates.push_back( r );
        }

    return true;
    }



void logPlayerNameForCurses( char *inPlayerEmail, char *inPlayerName ) {
    // structure to track names per player
    PlayerNameRecord r = { stringDuplicate( inPlayerName ),
                           stringDuplicate( inPlayerEmail ),
                           Time::getCurrentTime() };
    
    playerNames.push_back( r );
    }




CurseStatus getCurseLevel( char *inPlayerEmail ) {
    stepCurses();
    
    CurseRecord *r = findCurseRecord( inPlayerEmail );


    if( ! useCurseServer ) {
        return getCurseLevel( r );
        }
    else {
        // see if record exists yet
        for( int i=0; i<serverRemoteRecords.size(); i++ ) {
            RemoteCurseRecord *r = serverRemoteRecords.getElement( i );
            
            if( strcmp( r->email, inPlayerEmail ) == 0 ) {
                // hit
                
                int status = r->curseStatus;
                
                if( status != -1 ) {
                    delete [] r->email;
                    delete r->request;
                    serverRemoteRecords.deleteElement( i );
                    }
                CurseStatus returnRecord = { status, r->excessCursePoints };
                return returnRecord;
                }
            }

        RemoteCurseRecord r = { stringDuplicate( inPlayerEmail ), -1, 0, NULL };

        char *curseServerSharedSecret = 
            SettingsManager::getStringSetting( 
                "curseServerSharedSecret", 
                "secret_phrase" );
        

        char *hash = hmac_sha1( curseServerSharedSecret, inPlayerEmail );
                    
        delete [] curseServerSharedSecret;

        char *encodedEmail = URLUtils::urlEncode( r.email );
        
        
        char *url = autoSprintf( 
                        "%s?action=is_cursed"
                        "&email=%s"
                        "&email_hash_value=%s",
                        curseServerURL,
                        encodedEmail,
                        hash );
                    
        delete [] encodedEmail;
        delete [] hash;

        r.request = new WebRequest( "GET", url, NULL );
        printf( "Starting new web request for %s\n", url );
                    
        delete [] url;
        
        serverRemoteRecords.push_back( r );

        CurseStatus returnRecord = { -1, 0 }; 
        return returnRecord;
        }
    
    }




char isNameDuplicateForCurses( char *inPlayerName ) {
    int numRec = playerNames.size();
    
    for( int i=0; i<numRec; i++ ) {
        
        PlayerNameRecord *r = playerNames.getElement( i );
        
        if( strcmp( inPlayerName, r->name ) == 0 ) {
            return true;
            }
        }
    return false;
    }





void stepCurseServerRequests() {
    if( !useCurseServer ) {
        return;
        }
    
    
    for( int i=0; i<serverRemoteRecords.size(); i++ ) {
        RemoteCurseRecord *r = serverRemoteRecords.getElement( i );

        if( r->curseStatus == -1 ) {

            int result = r->request->step();
            
            
            if( result == -1 ) {
                AppLog::info( "Request to curse server failed." );
                r->curseStatus = 0;
                }
            else if( result == 1 ) {
                // done, have result   
                char *webResult = r->request->getResult();
                
                int curseStatus = 0;
                int excessCursePoints = 0;
                
                sscanf( webResult, "%d %d", &curseStatus, &excessCursePoints );
                
                delete [] webResult;
                
                r->curseStatus = curseStatus;
                r->excessCursePoints = excessCursePoints;
                }
            }
        }
    
    for( int i=0; i<serverPendingUpdates.size(); i++ ) {
        RemoteUpdateRecord *r = serverPendingUpdates.getElement( i );
        
        if( r->sequenceNumber == -1 && r->request == NULL ) {
            // haven't sent first sequence number request yet

            char *encodedEmail = URLUtils::urlEncode( r->email );
        
        
            char *url = autoSprintf( 
                "%s?action=get_sequence_number"
                "&email=%s",
                curseServerURL,
                encodedEmail );
                    
            delete [] encodedEmail;

            r->request = new WebRequest( "GET", url, NULL );
            printf( "Starting new web request for %s\n", url );
            
            delete [] url;
            continue;
            }
        

        int result = r->request->step();

        if( result == -1 ) {
            AppLog::info( "Request to curse server failed." );
            
            delete [] r->email;
            delete [] r->actionString;
            delete r->request;
            serverPendingUpdates.deleteElement( i );
            return;
            }
        else if( result == 1 ) {
            // done, have result
            char *webResult = r->request->getResult();

            if( r->sequenceNumber == -1 ) {
                // result is sequence number
                
                sscanf( webResult, "%d", &( r->sequenceNumber ) );            
                delete [] webResult;
                
                if( r->sequenceNumber == -1 ) {
                    AppLog::info( 
                        "Failed to get seq number from curse server." );
                    
                    delete [] r->email;
                    delete [] r->actionString;
                    delete r->request;
                    serverPendingUpdates.deleteElement( i );
                    return;
                    }
                
                delete r->request;


                char *curseServerSharedSecret = 
                    SettingsManager::getStringSetting( 
                        "curseServerSharedSecret", 
                        "secret_phrase" );
        
                char *seqString = autoSprintf( "%d", r->sequenceNumber );
                
                char *hash = hmac_sha1( curseServerSharedSecret, 
                                        seqString );
                delete [] seqString;
                
                    
                delete [] curseServerSharedSecret;

                char *encodedEmail = URLUtils::urlEncode( r->email );
        
        
                char *url = autoSprintf( 
                    "%s?%s"
                    "&email=%s"
                    "&sequence_number=%d"
                    "&hash_value=%s",
                    curseServerURL,
                    r->actionString,
                    encodedEmail,
                    r->sequenceNumber,
                    hash );
                    
                delete [] encodedEmail;
                delete [] hash;

                r->request = new WebRequest( "GET", url, NULL );
                printf( "Starting new web request for %s\n", url );
                    
                delete [] url;
                }
            else {
                // result is from main action request
                // this means we're done!

                if( strstr( webResult, "OK" ) == NULL ) {                    
                    AppLog::infoF( 
                        "Failed to get expected result from curseServer, "
                        "got:  %s", webResult );
                    }            
                delete [] webResult;

                delete [] r->email;
                delete [] r->actionString;
                delete r->request;
                serverPendingUpdates.deleteElement( i );
                return;
                }
            }
        }
    }

