#include "curses.h"
#include "curseLog.h"


#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/system/Time.h"


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
    }



static void stepCurses() {

    double tokenTime = SettingsManager::getFloatSetting( "curseTokenTime",
                                                         7200.0 );
    double decrementTime = 
        SettingsManager::getFloatSetting( "curseDecrementTime", 3600.0 );
    

    double curTime = Time::getCurrentTime();
    
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
            curTime - r->lastBirthTime > 3600 * 48 ) {
            // nothing to track for this record
            // or record stale by 48 hours
            
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



static int getCurseLevel( CurseRecord *inRecord ) {

    if( inRecord == NULL ) {
        return 0;
        }
    
    int score = inRecord->score;
    
    if( score == 0 ) {
        return 0;
        }
    
    int thresh = SettingsManager::getIntSetting( "curseThreshold", 10 );
    
    if( score < thresh ) {
        return 0;
        }
    
    return score / thresh;
    }






void cursesLogBirth( char *inEmail ) {
    stepCurses();
    
    CurseRecord *r = findCurseRecord( inEmail );
    
    double curTime = Time::getCurrentTime();
    
    r->alive = true;
    
    r->bornCursed = ( getCurseLevel( r ) > 0 );
    r->aliveStartTimeSinceTokenSpent = curTime;
    r->aliveStartTimeSinceScoreDecrement = curTime;
    
    r->lastBirthTime = curTime;
    }



void cursesLogDeath( char *inEmail ) {
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
    inEmailList->push_back_other( &newTokenEmails );
    
    newTokenEmails.deleteAll();
    }





char cursePlayer( int inGiverID, char *inGiverEmail, 
                  char *inReceiverName ) {
    stepCurses();
    
    CurseRecord *receiverRecord = findCurseRecordByName( inReceiverName );
    
    if( receiverRecord == NULL ) {
        return false;
        }
    
    if( receiverRecord->bornCursed ) {
        // already getting born cursed, leave them alone for now
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

    return true;
    }



void logPlayerNameForCurses( char *inPlayerEmail, char *inPlayerName ) {
    // structure to track names per player
    PlayerNameRecord r = { stringDuplicate( inPlayerName ),
                           stringDuplicate( inPlayerEmail ),
                           Time::getCurrentTime() };
    
    playerNames.push_back( r );
    }




int getCurseLevel( char *inPlayerEmail ) {
    stepCurses();
    
    CurseRecord *r = findCurseRecord( inPlayerEmail );
    
    return getCurseLevel( r );
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

