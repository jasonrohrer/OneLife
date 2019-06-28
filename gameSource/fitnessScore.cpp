#include "fitnessScore.h"
#include "accountHmac.h"
#include "message.h"


#include "minorGems/game/game.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"
#include "minorGems/network/web/URLUtils.h"


static int webRequest = -1;

static const char *nextAction = "";

static int nextSequenceNumber = -1;

static char *readyResult = NULL;


static char *fitnessServerURL = NULL;

static int useFitnessServer = 0;


extern char *userEmail;


static char *leaderboardName = NULL;

static double score = -1;
static int rank = -1;


typedef struct OffspringRecord {
        char *name;
        char *relationName;
        int displayID;
        int diedSecAgo;
        double age;
        double oldScore;
        double newScore;
    } OffspringRecord;


static SimpleVector<OffspringRecord> recentOffspring;


static void freeOffspringRecord( OffspringRecord inR ) {
    delete [] inR.name;
    delete [] inR.relationName;
    }


static void freeAllOffspring() {
    for( int i=0; i<recentOffspring.size(); i++ ) {
        freeOffspringRecord( recentOffspring.getElementDirect( i ) );
        }
    recentOffspring.deleteAll();
    }




void initFitnessScore() {
    fitnessServerURL = SettingsManager::getStringSetting( 
        "fitnessServerURL", 
        "http://localhost/jcr13/fitnessServer/server.php" );
    
    useFitnessServer = SettingsManager::getIntSetting( "useFitnessServer", 0 );
    }
                                                          


void freeFitnessScore() {
    delete [] fitnessServerURL;
    fitnessServerURL = NULL;

    if( webRequest != -1 ) {
        clearWebRequest( webRequest );
        webRequest = -1;
        }
    
    if( readyResult != NULL ) {
        delete [] readyResult;
        readyResult = NULL;
        }

    if( leaderboardName != NULL ) {
        delete [] leaderboardName;
        leaderboardName = NULL;
        }

    freeAllOffspring();
    }



char usingFitnessServer() {
    return useFitnessServer;
    }
    


static void startGettingSequenceNumber() {
    if( webRequest != -1 ) {
        clearWebRequest( webRequest );
        webRequest = -1;
        }

    nextSequenceNumber = -1;

    char *encodedEmail = URLUtils::urlEncode( userEmail );

    char *url = autoSprintf( "%s?action=get_client_sequence_number"
                             "&email=%s",
                             fitnessServerURL,
                             encodedEmail );
    delete [] encodedEmail;
    
    webRequest = startWebRequest( "GET", url, NULL );

    delete [] url;
    }



void triggerFitnessScoreUpdate() {
    if( !useFitnessServer ) {
        return;
        }

    score = -1;
    rank = -1;
    
    nextAction = "get_client_score";
    startGettingSequenceNumber();
    }



void triggerFitnessScoreDetailsUpdate() {
    if( !useFitnessServer ) {
        return;
        }
    
    score = -1;
    rank = -1;
    
    freeAllOffspring();

    nextAction = "get_client_score_details";
    startGettingSequenceNumber();
    }



static void stepActiveRequest() {
    if( webRequest == -1 ) {
        return;
        }
    int result = stepWebRequest( webRequest );
    
    if( result == -1 ) {
        // error
        clearWebRequest( webRequest );
        webRequest = -1;
        return;
        }
    
    if( result == 1 ) {
        // done!
        char *result = getWebResult( webRequest );

        if( nextSequenceNumber == -1 ) {
            // fetching sequence number
            sscanf( result, "%d", &nextSequenceNumber );
            delete [] result;


            // use sequence number to make next request

            char *pureKey = getPureAccountKey();

            char *stringToHash = autoSprintf( "%d", nextSequenceNumber );
            
            char *hash = hmac_sha1( pureKey, stringToHash );

            delete [] pureKey;
            delete [] stringToHash;

            char *encodedEmail = URLUtils::urlEncode( userEmail );

            char *url = autoSprintf( "%s?action=%s"
                                     "&email=%s",
                                     "&sequence_number=%d",
                                     "&hash_value=%s",
                                     fitnessServerURL,
                                     nextAction,
                                     encodedEmail,
                                     nextSequenceNumber,
                                     hash );
            delete [] encodedEmail;
            delete [] hash;
    
            webRequest = startWebRequest( "GET", url, NULL );
            
            delete [] url;
            }
        else {
            // pass result out whole
            readyResult = result;
            }
        
        clearWebRequest( webRequest );
        webRequest = -1;
        }


    if( readyResult != NULL ) {    
        // in either case, scan score, name, and rank
        
        if( leaderboardName != NULL ) {
            delete [] leaderboardName;
            }
        leaderboardName = new char[100];
        
        sscanf( "%99s\n%lf\n%d", leaderboardName, &score, &rank );



        if( strcmp( nextAction, "get_client_score_details" ) == 0 ) {
            // parse details too
            SimpleVector<char *> *lines = tokenizeString( readyResult );

            // skip already-parsed header and OK at end
            for( int i=3; i< lines->size()-1; i++ ) {
                
                char *line = lines->getElementDirect( i );
                
                // Eve_Jones,You,353,4495,14.7841,1.32155,2.6678
                
                int numParts;
                char **parts = split( line, ",", &numParts );
                
                OffspringRecord r;
                r.name = stringDuplicate( parts[0] );
                r.relationName = stringDuplicate( parts[1] );
                
                sscanf( parts[2], "%d", &( r.displayID ) );
                sscanf( parts[3], "%d", &( r.diedSecAgo ) );
                sscanf( parts[4], "%lf", &( r.age ) );
                sscanf( parts[5], "%lf", &( r.oldScore ) );
                sscanf( parts[6], "%lf", &( r.newScore ) );
                
                for( int j=0; j<numParts; j++ ) {
                    delete [] parts[j];
                    }
                delete [] parts;
                
                recentOffspring.push_back( r );
                }

            lines->deallocateStringElements();
            delete lines;
            }
        }
    
    }




// These draw nothing if latest data (after last trigger) not ready yet

void drawFitnessScore( doublePair inPos ) {
    if( !useFitnessServer ) {
        return;
        }

    if( score != -1 ) {        
        
        const char *rankSuffix = "TH";
        
        switch( rank % 10 ) {
            case 1:
                rankSuffix = "ST";
                break;
            case 2:
                rankSuffix = "ND";
                break;
            case 3:
                rankSuffix = "RD";
                break;
            default:
                rankSuffix = "TH";
                break;
            }
        
        char *message = 
            autoSprintf( translate( "scoreMessage" ), score, rank, rankSuffix );
        
        drawMessage( message, inPos );

        delete [] message;
        }
    else {
        stepActiveRequest();
        }
    }


void drawFitnessScoreDetails( doublePair inPos ) {
    if( !useFitnessServer ) {
        return;
        }

    

    if( score != -1 ) {
        drawFitnessScore( inPos );

        inPos.y -= 50;
        
        
        drawMessage( "FIXME:  list goes here", inPos );
        }
    }

