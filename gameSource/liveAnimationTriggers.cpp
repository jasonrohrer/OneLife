#include "liveAnimationTriggers.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include <stdio.h>


static char enabled = false;

// backtick is trigger key
static char triggerKey = '`';


static const char *triggerFileName = "liveTriggers.txt";


typedef struct Trigger {
        AnimType anim;
        int extraAnimIndex;
        int stepDuration;
        int numStepsPlayed;

        char isServerTrigger;
        int serverTriggerNumber;

        // NULL if not a SAY trigger
        char *sayMessage;
    } Trigger;


static SimpleVector<Trigger> triggers;


static Trigger nullTrigger = { endAnimType, -1, 0, 0 };

static Trigger currentTrigger = nullTrigger;

    

void initLiveTriggers() {
    enabled = SettingsManager::getIntSetting( "enableLiveTriggers", 0 );

    if( !enabled ) return;
    

    FILE *f = fopen( triggerFileName, "r" );
    
    if( f == NULL ) {
        printf( "Failed to read live anim trigger file %s\n", triggerFileName );
        enabled = false;
        return;
        }
    
    char readTrigger = true;
    
    // switch back and forth between these
    // so that triggered animations automatically blend between each other
    AnimType anim1 = extra;
    AnimType anim2 = extraB;

    int nextServerTrigger = 0;

    while( readTrigger ) {
        Trigger t;
        
        t.isServerTrigger = false;
        t.sayMessage = NULL;

        
        readTrigger = false;

        char buffer[100];
        
        int numRead = fscanf( f, "%99s ", buffer );
        
        if( numRead == 1 ) {
            
            if( strcmp( buffer, "anim" ) == 0 ) {

                numRead = fscanf( f, "%d %d", 
                                  &( t.extraAnimIndex ), &( t.stepDuration ) );
        
                if( numRead == 2 ) {
                    t.numStepsPlayed = 0;
                    t.anim = anim1;
                    
                    // swap
                    AnimType temp = anim1;
                    anim1 = anim2;
                    anim2 = temp;
                    
                    triggers.push_back( t );
                    readTrigger = true;
                    }
                }
            else if( strcmp( buffer, "say" ) == 0 ) {
                char buffer[100];
                
                // read quoted string
                numRead = fscanf( f, "\"%99[^\"]\"", buffer );
                if( numRead == 1 ) {
                    // read a string
                    t.sayMessage = stringDuplicate( buffer );
                    triggers.push_back( t );
                    readTrigger = true;
                    }
                }
            else if( strcmp( buffer, "server" ) == 0 ) {
                char buffer[100];
                
                numRead = fscanf( f, "%99s", buffer );
        
        
                if( numRead == 1 ) {
                    t.isServerTrigger = true;
                    
                    if( strcmp( buffer, "n" ) == 0 ) {
                        t.serverTriggerNumber = nextServerTrigger;
                        
                        nextServerTrigger++;
                        triggers.push_back( t );
                        readTrigger = true;
                       
                        }
                    else {
                                    
                        numRead = sscanf( buffer, "%d", 
                                          &( t.serverTriggerNumber ) );
                        
                        if( numRead == 1 ) {
                            triggers.push_back( t );
                            readTrigger = true;
                            }
                        }
                    }
                }            
            }
        }
    
    printf( "Read %d triggers from anim trigger file %s\n",
            triggers.size(), triggerFileName );

    fclose( f );    
    }

    


void freeLiveTriggers() {
    if( !enabled ) return;    

    for( int i=0; i<triggers.size(); i++ ) {
        Trigger t = triggers.getElementDirect(i);
        if( t.sayMessage != NULL ) {
            delete [] t.sayMessage;
            }
        }

    triggers.deleteAll();
    }




char anyLiveTriggersLeft() {
    if( !enabled ) {
        return false;
        }
    
    return ( triggers.size() > 0 );
    }

    

static int lastPlayedServerTrigger = 0;


// send in a key command received from user that make trigger an animation
void registerTriggerKeyCommand( unsigned char inASCII,
                                LivingLifePage *inPage ) {
    if( !enabled ) return;
    
    if( inASCII == triggerKey ) {
        
        if( triggers.size() > 0 ) {

            Trigger nextTrigger = triggers.getElementDirect( 0 );
            
            if( nextTrigger.isServerTrigger ) {
                
                char *message = autoSprintf( "TRIGGER %d#",
                                             nextTrigger.serverTriggerNumber );
                
                lastPlayedServerTrigger = nextTrigger.serverTriggerNumber;
                
                inPage->sendToServerSocket( message );
                
                delete [] message;
                }
            else if( nextTrigger.sayMessage != NULL ) {
                char *message = 
                    autoSprintf( "SAY 0 0 %s#", nextTrigger.sayMessage );
                
                inPage->sendToServerSocket( message );
                
                delete [] message;
                
                delete [] nextTrigger.sayMessage;
                }
            else {
                currentTrigger = nextTrigger;
                }
            
            triggers.deleteElement( 0 );
            }
        else {
            // send another server trigger by default
            
            lastPlayedServerTrigger++;
            
            char *message = autoSprintf( "TRIGGER %d#",
                                         lastPlayedServerTrigger );
                
            inPage->sendToServerSocket( message );
            
            delete [] message;
            }
        }
    }




AnimType stepLiveTriggers( char *outNew ) {
    if( !enabled ) return endAnimType;
    
    if( currentTrigger.extraAnimIndex > -1 ) {

        if( currentTrigger.numStepsPlayed == 0 ) {
            *outNew = true;
            }
        else {
            *outNew = false;
            }
        
        currentTrigger.numStepsPlayed ++;
        
        if( currentTrigger.numStepsPlayed > currentTrigger.stepDuration ) {
            currentTrigger = nullTrigger;
            }


        if( currentTrigger.anim == extra ) {
            setExtraIndex( currentTrigger.extraAnimIndex );
            }
        else if( currentTrigger.anim == extraB ) {
            setExtraIndexB( currentTrigger.extraAnimIndex );
            }        
        }

    return currentTrigger.anim;    
    }

