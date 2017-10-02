#include "triggers.h"
#include "map.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/network/Socket.h"


static char triggersEnabled = false;

static int triggerStep = 0;


typedef struct PlayerMapping {
        const char *email;
        int displayID;
    } PlayerMapping;

static SimpleVector<PlayerMapping> playerMap;
    


typedef struct DummyMoves {
        int step;
        const char *moveText;
    } DummyMoves;
        


typedef struct LiveDummySocket {
        Socket *sock;
        SimpleVector<DummyMoves> moves;
    } LiveDummySocket;


SimpleVector<LiveDummySocket> dummySockets;

    

void initTriggers() {
    triggersEnabled = 
        SettingsManager::getIntSetting( "allowTriggerRequests", 0 );
                    
    }



void freeTriggers() {
    triggersEnabled = false;

    for( int i=0; i<dummySockets.size(); i++ ) {
        delete dummySockets.getElementDirect(i).sock;
        }
    dummySockets.deleteAll();
    }


char areTriggersEnabled() {
    return triggersEnabled;
    }



int getTriggerPlayerDisplayID( const char *inPlayerEmail ) {
    for( int i=0; i<playerMap.size(); i++ ) {
        PlayerMapping p = playerMap.getElementDirect( i );
        
        if( strcmp( p.email, inPlayerEmail ) == 0 ) {
            return p.displayID;
            }
        }

    return -1;
    }



void trigger( int inTriggerNumber ) {
    if( inTriggerNumber == 0 ) {
        setMapObject( 2, 0, 418 );
        }
    if( inTriggerNumber == 1 ) {
        setMapObject( 2, 0, 153 );
        }
    }



void stepTriggers() {
    triggerStep ++;
    
    for( int i=0; i<dummySockets.size(); i++ ) {
        LiveDummySocket *s = dummySockets.getElement( i );
        
        // fixme... read all available data and discard it

        if( s->moves.size() > 0 &&
            s->moves.getElementDirect(0).step <= triggerStep ) {
            
            const char *text = s->moves.getElementDirect(0).moveText;
            
            int messageLength = strlen( text );

            int numSent = 
                s->sock->send( (unsigned char*)text, 
                               messageLength, 
                               false, false );
            
            if( numSent == -1 ) {
                delete s->sock;
                dummySockets.deleteElement( i );
                i--;
                }
            else if( numSent == messageLength ) {
                // done sending this message
                s->moves.deleteElement( 0 );
                }
            // else hold for later
            
            }
        }
    
    }


    
    


