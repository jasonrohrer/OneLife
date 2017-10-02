#include "triggers.h"
#include "map.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketClient.h"


#include <stdlib.h>


static char triggersEnabled = false;

static double triggerStartTime = 0;


typedef struct PlayerMapping {
        const char *email;
        int displayID;
        double startAge;
        GridPos startPos;
    } PlayerMapping;

static SimpleVector<PlayerMapping> playerMap;
    


typedef struct DummyMove {
        double triggerTimeSec;
        const char *moveText;
    } DummyMoves;
        


typedef struct LiveDummySocket {
        double socketStartTime;
        Socket *sock;
        GridPos pos;

        SimpleVector<DummyMove> moves;
    } LiveDummySocket;


SimpleVector<LiveDummySocket> dummySockets;

    

void initTriggers() {
    triggersEnabled = 
        SettingsManager::getIntSetting( "allowTriggerRequests", 0 );
    
    if( triggersEnabled ) {
        
        if( SettingsManager::getIntSetting( "requireClientPassword", 1 )
            ||
            SettingsManager::getIntSetting( "requireTicketServerCheck", 1 ) ) {
            
            printf( "\n\nERROR:  Client-issued triggers enabled "
                    "(allowTriggerRequests in settings folder), but "
                    "requireTicketServerCheck or requireClientPassword "
                    "is enabled in server settings folder.  Exiting.\n\n" );
            exit( 0 );
            }
        }
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


double getTriggerPlayerAge( const char *inPlayerEmail ) {
    for( int i=0; i<playerMap.size(); i++ ) {
        PlayerMapping p = playerMap.getElementDirect( i );
        
        if( strcmp( p.email, inPlayerEmail ) == 0 ) {
            return p.startAge;
            }
        }

    return -1;
    }



GridPos getTriggerPlayerPos( const char *inPlayerEmail ) {
    for( int i=0; i<playerMap.size(); i++ ) {
        PlayerMapping p = playerMap.getElementDirect( i );
        
        if( strcmp( p.email, inPlayerEmail ) == 0 ) {
            return p.startPos;
            }
        }

    GridPos badPos = { -1, -1 };
    return badPos;
    }



static LiveDummySocket newDummyPlayer( const char *inEmail,
                                       int inDisplayID,
                                       double inStartAge,
                                       GridPos inStartPos ) {
    PlayerMapping m = { inEmail, inDisplayID, inStartAge, inStartPos };
    
    playerMap.push_back( m );
    

    LiveDummySocket s;
    s.pos = inStartPos;
    
    s.socketStartTime = Time::getCurrentTime();
    
    int port = SettingsManager::getIntSetting( "port", 8005 );

    HostAddress a( stringDuplicate( "localhost" ), port );

    char timeout;
    s.sock = SocketClient::connectToServer( &a, 100, &timeout );

    // next send login message

    char *message = autoSprintf( "LOGIN %s dummyHash dummyHash#", inEmail );
    
    int messageLength = strlen( message );
    
    s.sock->send( (unsigned char*)message, 
                      messageLength, 
                      false, false );
    
    return s;
    }



void sendDummyMove( LiveDummySocket *inDummy, 
                    SimpleVector<GridPos> *inOffsets ) {
    
    char *message = autoSprintf( "MOVE %d %d ", 
                                 inDummy->pos.x, inDummy->pos.y );
    
    
    for( int i=0; i<inOffsets->size(); i++ ) {
        
        char *oldMessage = message;
        
        message = 
            concatonate( message,
                         autoSprintf( "%d %d ",
                                      inOffsets->getElementDirect( i ).x,
                                      inOffsets->getElementDirect( i ).y ) );
        delete [] oldMessage;
        }

    // end pos
    inDummy->pos.x += inOffsets->getElementDirect( inOffsets->size() - 1 ).x;
    inDummy->pos.y += inOffsets->getElementDirect( inOffsets->size() - 1 ).y;
    

    char *oldMessage = message;
    
    message = concatonate( message, "#" );
    
    delete [] oldMessage;

    inDummy->sock->send( (unsigned char*)message, 
                         strlen( message ), 
                         false, false );
    
    delete [] message;
    }


// offset is from current pos
void sendDummyAction( LiveDummySocket *inDummy, 
                      const char *inAction, GridPos inOffset ) {
    char *message = autoSprintf( "%s %d %d#",
                                 inAction, 
                                 inDummy->pos.x + inOffset.x,
                                 inDummy->pos.y + inOffset.y );
    
    inDummy->sock->send( (unsigned char*)message, 
                         strlen( message ), 
                         false, false );
    
    delete [] message;
    }

                                 




LiveDummySocket *alice = NULL;




void trigger( int inTriggerNumber ) {
    
    if( triggerStartTime == 0 ) {
        triggerStartTime = Time::getCurrentTime();
        }
    
    GridPos offset = { 0, 0 };

    
    if( inTriggerNumber == 0 ) {
        setMapObject( 2, 0, 418 );
        }
    else if( inTriggerNumber == 1 ) {
        setMapObject( 2, 0, 153 );
        }
    else if( inTriggerNumber == 2 ) {
        GridPos startPos = { 1, 1 };
        
        LiveDummySocket s = newDummyPlayer( "dummy1@test.com", 350, 20,
                                            startPos );
        
        dummySockets.push_back( s );

        
        alice = 
            dummySockets.getElement( dummySockets.size() - 1 );        
        }
    else if( inTriggerNumber == 3 ) {
        SimpleVector<GridPos> move;
        offset.x = 1;
        
        move.push_back( offset );
        offset.x = 2;
        
        move.push_back( offset );
        
        sendDummyMove( alice, &move );
        }
    else if( inTriggerNumber == 4 ) {
        SimpleVector<GridPos> move;
        offset.x = -1;
        
        move.push_back( offset );
        offset.x = -2;
        
        move.push_back( offset );
        
        sendDummyMove( alice, &move );
        }
    else if( inTriggerNumber == 5 ) {
        offset.y = 1;
        sendDummyAction( alice, "USE", offset );
        }
    else if( inTriggerNumber == 6 ) {        
        SimpleVector<GridPos> move;
        
        offset.x = -1;
        
        move.push_back( offset );
        
        sendDummyMove( alice, &move );
        }
    else if( inTriggerNumber == 7 ) {
        offset.y = 1;
        sendDummyAction( alice, "USE", offset );
        }
    
    }



void stepTriggers() {
    if( !triggersEnabled ) {
        return;
        }
    
    double curTime = Time::getCurrentTime();
    
    for( int i=0; i<dummySockets.size(); i++ ) {
        LiveDummySocket *s = dummySockets.getElement( i );
        
        // read all available data and discard it
        
        char buffer[512];
    
        int numRead = 1;
        
        while( numRead > 0 ) {
            numRead = s->sock->receive( (unsigned char*)buffer, 512, 0 );
            }
        
        

        if( s->moves.size() > 0
            &&
            s->socketStartTime + s->moves.getElementDirect(0).triggerTimeSec <= 
            curTime ) {
            
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


    
    


