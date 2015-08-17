
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/SocketPoll.h"

#include "minorGems/system/Thread.h"

#include "minorGems/game/doublePair.h"


#include "map.h"
#include "../gameSource/transitionBank.h"

#include "minorGems/util/random/JenkinsRandomSource.h"


static JenkinsRandomSource testRandSource;



typedef struct LiveObject {
        int id;
        
        // start and dest for a move
        // same if reached destination
        int xs;
        int ys;
        
        int xd;
        int yd;
        
        int lastSentMapX;
        int lastSentMapY;

        // in grid square widths per second
        double moveSpeed;
        
        double moveTotalSeconds;
        double moveStartTime;
        

        int holdingID;

        Socket *sock;
        SimpleVector<char> *sockBuffer;

        char isNew;
        char firstMessageSent;
        char error;
        char deleteSent;

        char newMove;
        
    } LiveObject;



SimpleVector<LiveObject> players;


typedef struct ChangePosition {
        int x, y;
        
        // true if update should be sent to everyone regardless
        // of distance (like position of a new player in the world,
        // or the removal of a player).
        char global;
    };



int nextID = 0;



void quitCleanup() {
    printf( "Cleaning up on quit...\n" );
    

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        delete nextPlayer->sock;
        delete nextPlayer->sockBuffer;
        }
    
    freeMap();

    freeTransBank();
    }






volatile char quit = false;

void intHandler( int inUnused ) {
    printf( "Quit received for unix\n" );
    
    // since we handled this singal, we will return to normal execution
    quit = true;
    }


#ifdef WIN_32
#include <windows.h>
BOOL WINAPI ctrlHandler( DWORD dwCtrlType ) {
    if( CTRL_C_EVENT == dwCtrlType ) {
        printf( "Quit received for windows\n" );
        
        // will auto-quit as soon as we return from this handler
        // so cleanup now
        //quitCleanup();
        
        // seems to handle CTRL-C properly if launched by double-click
        // or batch file
        // (just not if launched directly from command line)
        quit = true;
        }
    return true;
    }
#endif


int numConnections = 0;







// reads all waiting data from socket and stores it in buffer
// returns true if socket still good, false on error
char readSocketFull( Socket *inSock, SimpleVector<char> *inBuffer ) {

    char buffer[512];
    
    int numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
    
    if( numRead == -1 ) {
        return false;
        }
    
    while( numRead > 0 ) {
        inBuffer->appendArray( buffer, numRead );

        numRead = inSock->receive( (unsigned char*)buffer, 512, 0 );
        }

    return true;
    }



// NULL if there's no full message available
char *getNextClientMessage( SimpleVector<char> *inBuffer ) {
    // find first terminal character #

    int index = inBuffer->getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    
    char *message = new char[ index + 1 ];
    
    for( int i=0; i<index; i++ ) {
        message[i] = inBuffer->getElementDirect( 0 );
        inBuffer->deleteElement( 0 );
        }
    // delete message terminal character
    inBuffer->deleteElement( 0 );
    
    message[ index ] = '\0';
    
    return message;
    }





typedef enum messageType {
	MOVE,
    USE,
    GRAB,
    DROP,
    UNKNOWN
    } messageType;


typedef struct ClientMessage {
        messageType type;
        int x, y;
    } ClientMessage;




ClientMessage parseMessage( char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    int numRead = sscanf( inMessage, 
                          "%99s %d %d#", nameBuffer, &( m.x ), &( m.y ) );


    if( numRead != 3 ) {
        m.type = UNKNOWN;
        return m;
        }
    

    if( strcmp( nameBuffer, "MOVE" ) == 0) {
        m.type = MOVE;
        }
    else if( strcmp( nameBuffer, "USE" ) == 0 ) {
        m.type = USE;
        }
    else if( strcmp( nameBuffer, "GRAB" ) == 0 ) {
        m.type = GRAB;
        }
    else if( strcmp( nameBuffer, "DROP" ) == 0 ) {
        m.type = DROP;
        }
    else {
        m.type = UNKNOWN;
        }
    
    return m;
    }




// returns NULL if there are no matching moves
char *getMovesMessage( char inNewMovesOnly, 
                       SimpleVector<ChangePosition> *inChangeVector = NULL,
                       int inOneIDOnly = -1 ) {
    
    SimpleVector<char> messageBuffer;

    messageBuffer.appendElementString( "PM\n" );

    int numPlayers = players.size();
                
    
    int numLines = 0;

    for( int i=0; i<numPlayers; i++ ) {
                
        LiveObject *o = players.getElement( i );
                

        if( ( o->xd != o->xs || o->yd != o->ys )
            &&
            ( o->newMove || !inNewMovesOnly ) 
            && ( inOneIDOnly == -1 || inOneIDOnly == o->id ) ) {

 
            // p_id xs ys xd yd fraction_done eta_sec
            
            double deltaSec = Time::getCurrentTime() - o->moveStartTime;
            
            double etaSec = o->moveTotalSeconds - deltaSec;
                
            if( inNewMovesOnly ) {
                o->newMove = false;
                }
            
            // holding no object for now
            char *messageLine = 
                autoSprintf( "%d %d %d %d %d %.3f %.3f\n", o->id, 
                             o->xs, o->ys, 
                             o->xd - o->xs, o->yd - o->ys, 
                             o->moveTotalSeconds, etaSec );
                                    
            messageBuffer.appendElementString( messageLine );
            delete [] messageLine;
            
            if( inChangeVector != NULL ) {
                ChangePosition p = { o->xd, o->yd, false };
                inChangeVector->push_back( p );
                }

            numLines ++;
            
            }
        }
    
        
    if( numLines > 0 ) {
        
        messageBuffer.push_back( '#' );
                
        char *message = messageBuffer.getElementString();
        
        return message;
        }
    
    return NULL;
    
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }




// sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
int sendMapChunkMessage( LiveObject *inO ) {
    int messageLength;
    
    unsigned char *mapChunkMessage = getChunkMessage( inO->xs,
                                                      inO->ys, 
                                                      &messageLength );
                
                

                
    int numSent = 
        inO->sock->send( mapChunkMessage, 
                         messageLength, 
                         false, false );
                
    delete [] mapChunkMessage;
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = inO->xs;
        inO->lastSentMapY = inO->ys;
        }
    else if( numSent == -1 ) {
        inO->error = true;
        }
    return numSent;
    }



double intDist( int inXA, int inYA, int inXB, int inYB ) {
    int dx = inXA - inXB;
    int dy = inYA - inYB;

    return sqrt(  dx * dx + dy * dy );
    }






int main() {

    printf( "\nServer starting up\n\n" );

    printf( "Press CTRL-C to shut down server gracefully\n\n" );

    signal( SIGINT, intHandler );

#ifdef WIN_32
    SetConsoleCtrlHandler( ctrlHandler, TRUE );
#endif

    initTransBank();

    initMap();
    
    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    
    
    SocketPoll sockPoll;
    
    
    
    SocketServer server( port, 256 );
    
    sockPoll.addSocketServer( &server );
    
    printf( "Listening for connection on port %d\n", port );

    while( !quit ) {
    
        int numLive = players.size();
        

        // check if any are still moving
        // if so, we must busy-loop over them until moves are
        // complete
        char anyMoving = false;
        double minMoveTime = 999999;
        
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            if( nextPlayer->xd != nextPlayer->xs ||
                nextPlayer->yd != nextPlayer->ys ) {
                
                double moveTimeLeft =
                    nextPlayer->moveTotalSeconds -
                    ( Time::getCurrentTime() - nextPlayer->moveStartTime );
                
                if( moveTimeLeft < 0 ) {
                    moveTimeLeft = 0;
                    }
                
                if( moveTimeLeft < minMoveTime ) {
                    minMoveTime = moveTimeLeft;
                    }
                anyMoving = true;
                }
            }
        
        SocketOrServer *readySock =  NULL;

        if( !anyMoving ) {
            // use 0 cpu when total idle
            // but wake up periodically to catch quit signals, etc
            readySock = sockPoll.wait( 2000 );
            }
        else {
            // players are connected and moving, must do move updates anyway
            
            if( minMoveTime > 0 ) {
                
                // use a timeout based on shortest time to complete move
                // so we'll wake up and catch it
                readySock = sockPoll.wait( (int)( minMoveTime * 1000 ) );
                }
            }    
        
        
        
        
        if( readySock != NULL && !readySock->isSocket ) {
            // server ready
            Socket *sock = server.acceptConnection( 0 );

            if( sock != NULL ) {
                
                
                printf( "Got connection\n" );
                numConnections ++;
                
                LiveObject newObject;
                newObject.id = nextID;
                nextID++;
                newObject.xs = 0;
                newObject.ys = 0;
                newObject.xd = 0;
                newObject.yd = 0;
                newObject.lastSentMapX = 0;
                newObject.lastSentMapY = 0;
                newObject.moveSpeed = 4;
                newObject.moveTotalSeconds = 0;
                newObject.holdingID = 0;
                newObject.sock = sock;
                newObject.sockBuffer = new SimpleVector<char>();
                newObject.isNew = true;
                newObject.firstMessageSent = false;
                newObject.error = false;
                newObject.deleteSent = false;
                newObject.newMove = false;

                sockPoll.addSocket( sock );
                
                players.push_back( newObject );            
            
                printf( "New player connected as player %d\n", newObject.id );

                printf( "Listening for another connection on port %d\n", 
                        port );
                }
            }
        
        

        numLive = players.size();
        

        // listen for any messages from clients 

        // track index of each player that needs an update sent about it
        // we compose the full update message below
        SimpleVector<int> playerIndicesToSendUpdatesAbout;
        
        // accumulated text of update lines
        SimpleVector<char> newUpdates;
        SimpleVector<ChangePosition> newUpdatesPos;

        SimpleVector<char> mapChanges;
        SimpleVector<ChangePosition> mapChangesPos;
        
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            

            char result = 
                readSocketFull( nextPlayer->sock, nextPlayer->sockBuffer );
            
            if( ! result ) {
                nextPlayer->error = true;
                }

            char *message = getNextClientMessage( nextPlayer->sockBuffer );
            
            if( message != NULL ) {
                printf( "Got client message: %s\n", message );
                
                ClientMessage m = parseMessage( message );
                
                delete [] message;
                
                //Thread::staticSleep( 
                //    testRandSource.getRandomBoundedInt( 0, 450 ) );
                
                // if player is still moving, ignore all actions
                // except for move interrupts
                if( ( nextPlayer->xs == nextPlayer->xd &&
                      nextPlayer->ys == nextPlayer->yd ) 
                    ||
                    m.type == MOVE ) {
                    
                    if( m.type == MOVE ) {
                        if( nextPlayer->xs != nextPlayer->xd ||
                            nextPlayer->ys != nextPlayer->yd ) {
                    
                            // a new move interrupting a non-stationary object
                            
                            // compute closest starting position part way along
                            // path
                            double fractionDone = 
                                ( Time::getCurrentTime() - 
                                  nextPlayer->moveStartTime )
                                / nextPlayer->moveTotalSeconds;
                    
                            doublePair start = { (double)nextPlayer->xs, 
                                                 (double)nextPlayer->ys };
                            doublePair dest = { (double)nextPlayer->xd, 
                                                (double)nextPlayer->yd };
                            
                            doublePair cur =
                                add( mult( dest, fractionDone ),
                                     mult( start, 1 - fractionDone ) );
                            
                            nextPlayer->xs = lrintf( cur.x );
                            nextPlayer->ys = lrintf( cur.y );
                            }


                        nextPlayer->xd = m.x;
                        nextPlayer->yd = m.y;
                        
                        doublePair start = { (double)nextPlayer->xs, 
                                             (double)nextPlayer->ys };
                        doublePair dest = { (double)m.x, (double)m.y };
                        
                        double dist = distance( start, dest );
                        
                        
                        nextPlayer->moveTotalSeconds = dist / 
                            nextPlayer->moveSpeed;

                        nextPlayer->moveStartTime = Time::getCurrentTime();
                        
                        nextPlayer->newMove = true;
                        }
                    else if( m.type == USE ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );

                        if( isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) ) {
                            
                            // can only use on targets next to us for now,
                            // no diags
                            
                            int target = getMapObject( m.x, m.y );
                            
                            if( target != 0 ) {
                                
                                TransRecord *r = 
                                    getTrans( nextPlayer->holdingID, 
                                              target );

                                if( r != NULL ) {
                                    nextPlayer->holdingID = r->newActor;
                                    
                                    setMapObject( m.x, m.y, r->newTarget );
                                    
                                    char *changeLine =
                                        autoSprintf( "%d %d %d\n",
                                                     m.x, m.y, r->newTarget );
                                
                                    mapChanges.
                                        appendElementString( changeLine );
                                    
                                    ChangePosition p = { m.x, m.y, false };
                                    mapChangesPos.push_back( p );
                

                                    delete [] changeLine;
                                    }
                                else if( nextPlayer->holdingID == 0 ) {
                                    // no bare-hand transition applies to
                                    // this target object
                                    // treat it like GRAB
                                    setMapObject( m.x, m.y, 0 );
                                    
                                    nextPlayer->holdingID = target;
                                
                                
                                    char *changeLine =
                                        autoSprintf( "%d %d %d\n",
                                                     m.x, m.y, 0 );
                                
                                    mapChanges.appendElementString( 
                                        changeLine );
                                    
                                    ChangePosition p = { m.x, m.y, false };
                                    mapChangesPos.push_back( p );

                                    delete [] changeLine;
                                    }    
                                }
                            }
                        }                    
                    else if( m.type == DROP ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );

                        if( isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) ) {
                            
                            int target = getMapObject( m.x, m.y );
                            
                            if( nextPlayer->holdingID != 0 && 
                                target == 0 ) {
                                
                                // empty spot to drop into
                                    
                                setMapObject( m.x, m.y, 
                                              nextPlayer->holdingID );
                                
                                char *changeLine =
                                    autoSprintf( "%d %d %d\n",
                                                 m.x, m.y,
                                                 nextPlayer->holdingID );
                                
                                mapChanges.appendElementString( 
                                    changeLine );
                                
                                ChangePosition p = { m.x, m.y, false };
                                mapChangesPos.push_back( p );
                
                                delete [] changeLine;
                                
                                nextPlayer->holdingID = 0;
                                }
                            }
                        }
                    else if( m.type == GRAB ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );

                        if( isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) ) {
                            
                            int target = getMapObject( m.x, m.y );
                            
                            if( nextPlayer->holdingID == 0 && 
                                target != 0 ) {
                                
                                // something to grab

                                setMapObject( m.x, m.y, 0 );
                                
                                nextPlayer->holdingID = target;
                                
                                char *changeLine =
                                    autoSprintf( "%d %d %d\n",
                                                 m.x, m.y, 0 );
                                
                                mapChanges.appendElementString( 
                                    changeLine );
                                
                                ChangePosition p = { m.x, m.y, false };
                                mapChangesPos.push_back( p );    

                                delete [] changeLine;
                                }
                            }
                        }
                    }                
                }
            
                
            if( nextPlayer->isNew ) {
                // their first position is an update
                

                playerIndicesToSendUpdatesAbout.push_back( i );
                
                nextPlayer->isNew = false;
                
                // force this PU to be sent to everyone
                ChangePosition p = { 0, 0, true };
                newUpdatesPos.push_back( p );
                }
            else if( nextPlayer->error && ! nextPlayer->deleteSent ) {
                char *updateLine = autoSprintf( "%d %d X X %.2f\n", 
                                                nextPlayer->id,
                                                nextPlayer->holdingID,
                                                nextPlayer->moveSpeed );
                
                newUpdates.appendElementString( updateLine );
                ChangePosition p = { 0, 0, true };
                newUpdatesPos.push_back( p );

                delete [] updateLine;
                
                nextPlayer->isNew = false;
                
                nextPlayer->deleteSent = true;
                }
            else {
                // check if they are done moving
                // if so, send an update
                

                if( nextPlayer->xd != nextPlayer->xs ||
                    nextPlayer->yd != nextPlayer->ys ) {
                
                    
                    if( Time::getCurrentTime() - nextPlayer->moveStartTime
                        >
                        nextPlayer->moveTotalSeconds ) {
                        
                        // done
                        nextPlayer->xs = nextPlayer->xd;
                        nextPlayer->ys = nextPlayer->yd;
                        nextPlayer->newMove = false;
                        

                        playerIndicesToSendUpdatesAbout.push_back( i );
                        }
                    }
                
                }
            
            
            }
        

        
        

        
        for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( 
                playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

            char *updateLine = autoSprintf( "%d %d %d %d %.2f\n", 
                                            nextPlayer->id,
                                            nextPlayer->holdingID,
                                            nextPlayer->xs, 
                                            nextPlayer->ys,
                                            nextPlayer->moveSpeed );
            
            newUpdates.appendElementString( updateLine );
            ChangePosition p = { nextPlayer->xs, nextPlayer->ys, false };
            newUpdatesPos.push_back( p );

            delete [] updateLine;
            }
        

        
        SimpleVector<ChangePosition> movesPos;        

        char *moveMessage = getMovesMessage( true, &movesPos );
        
        int moveMessageLength = 0;
        
        if( moveMessage != NULL ) {
            moveMessageLength = strlen( moveMessage );
            }
        
                



        char *updateMessage = NULL;
        int updateMessageLength = 0;
        
        if( newUpdates.size() > 0 ) {
            newUpdates.push_back( '#' );
            char *temp = newUpdates.getElementString();

            updateMessage = concatonate( "PU\n", temp );
            delete [] temp;

            updateMessageLength = strlen( updateMessage );
            }
        

        char *mapChangeMessage = NULL;
        int mapChangeMessageLength = 0;
        
        if( mapChanges.size() > 0 ) {
            mapChanges.push_back( '#' );
            char *temp = mapChanges.getElementString();

            mapChangeMessage = concatonate( "MX\n", temp );
            delete [] temp;

            mapChangeMessageLength = strlen( mapChangeMessage );
            }
        

        
        // send moves and updates to clients
        
        for( int i=0; i<numLive; i++ ) {
            
            LiveObject *nextPlayer = players.getElement(i);
            
            
            if( ! nextPlayer->firstMessageSent ) {
                

                // first, send the map chunk around them
                
                int numSent = sendMapChunkMessage( nextPlayer );
                
                if( numSent == -2 ) {
                    // still not sent, try again later
                    continue;
                    }



                // now send starting message
                SimpleVector<char> messageBuffer;

                messageBuffer.appendElementString( "PU\n" );

                int numPlayers = players.size();
            
                // must be last in message
                char *playersLine;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject o = *( players.getElement( i ) );
                

                    // holding no object for now
                    char *messageLine = 
                        autoSprintf( "%d %d %d %d %.2f\n", o.id, o.holdingID,
                                     o.xs, o.ys, o.moveSpeed );
                    

                    if( o.id != nextPlayer->id ) {
                        messageBuffer.appendElementString( messageLine );
                        delete [] messageLine;
                        }
                    else {
                        // save until end
                        playersLine = messageLine;
                        }
                    }

                messageBuffer.appendElementString( playersLine );
                delete [] playersLine;
                
                messageBuffer.push_back( '#' );
            
                char *message = messageBuffer.getElementString();
                int messageLength = strlen( message );

                numSent = 
                    nextPlayer->sock->send( (unsigned char*)message, 
                                            messageLength, 
                                            false, false );
                
                delete [] message;
                

                if( numSent == -1 ) {
                    nextPlayer->error = true;
                    }
                else if( numSent != messageLength ) {
                    // still not sent, try again later
                    continue;
                    }



                char *movesMessage = getMovesMessage( false );
                
                if( movesMessage != NULL ) {
                    
                
                    messageLength = strlen( movesMessage );
                    
                    numSent = 
                        nextPlayer->sock->send( (unsigned char*)movesMessage, 
                                                messageLength, 
                                            false, false );
                    
                    delete [] movesMessage;
                    

                    if( numSent == -1 ) {
                        nextPlayer->error = true;
                        }
                    else if( numSent != messageLength ) {
                        // still not sent, try again later
                        continue;
                        }
                    }
                
                nextPlayer->firstMessageSent = true;
                }
            else {
                // this player has first message, ready for updates/moves
                
                if( abs( nextPlayer->xd - nextPlayer->lastSentMapX ) > 10
                    ||
                    abs( nextPlayer->yd - nextPlayer->lastSentMapY ) > 10 ) {
                
                    // moving out of bounds of chunk, send update
                    
                    
                    sendMapChunkMessage( nextPlayer );


                    // send updates about any non-moving players
                    // that are in this chunk
                    SimpleVector<char> chunkPlayerUpdates;

                    SimpleVector<char> chunkPlayerMoves;
                    
                    for( int j=0; j<numLive; j++ ) {
                        LiveObject *otherPlayer = 
                            players.getElement( j );
                        
                        if( otherPlayer->id != nextPlayer->id ) {
                            // not us

                            double d = intDist( nextPlayer->xd,
                                                nextPlayer->yd,
                                                otherPlayer->xd,
                                                otherPlayer->yd );
                            
                            
                            if( d <= getChunkDimension() / 2 ) {
                                
                                // send next player a player update
                                // about this player, telling nextPlayer
                                // where this player was last stationary
                                // and what they're holding
                        
                                char *updateLine = autoSprintf( 
                                    "%d %d %d %d %.2f\n", 
                                    otherPlayer->id,
                                    otherPlayer->holdingID,
                                    otherPlayer->xs, 
                                    otherPlayer->ys,
                                    otherPlayer->moveSpeed ); 

                                chunkPlayerUpdates.appendElementString( 
                                    updateLine );
                                delete [] updateLine;

                                if( otherPlayer->xs != otherPlayer->xd
                                    ||
                                    otherPlayer->ys != otherPlayer->yd ) {
                            
                                    // moving too
                                    // send message telling nextPlayer
                                    // about this move in progress

                                    char *message =
                                        getMovesMessage( false, NULL, 
                                                         otherPlayer->id );
                            
                            
                                    chunkPlayerMoves.appendElementString( 
                                        message );
                                    
                                    delete [] message;
                                    }
                                }
                            }
                        }


                    if( chunkPlayerUpdates.size() > 0 ) {
                        chunkPlayerUpdates.push_back( '#' );
                        char *temp = chunkPlayerUpdates.getElementString();

                        char *message = concatonate( "PU\n", temp );
                        delete [] temp;


                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)message, 
                                strlen( message ), 
                                false, false );

                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }
                        
                        delete [] message;
                        }

                    
                    if( chunkPlayerMoves.size() > 0 ) {
                        char *temp = chunkPlayerMoves.getElementString();

                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)temp, 
                                strlen( temp ), 
                                false, false );
                        
                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }

                        delete [] temp;
                        }
                    }
                // done handling sending new map chunk and player updates
                // for players in the new chunk
                
                
                

                double maxDist = 32;


                if( updateMessage != NULL ) {

                    double minUpdateDist = 64;
                    
                    for( int u=0; u<newUpdatesPos.size(); u++ ) {
                        ChangePosition *p = newUpdatesPos.getElement( u );
                        
                        // update messages can be global when a new
                        // player joins or an old player is deleted
                        if( p->global ) {
                            minUpdateDist = 0;
                            }
                        else {
                            double d = intDist( p->x, p->y, 
                                                nextPlayer->xd, 
                                                nextPlayer->yd );
                    
                            if( d < minUpdateDist ) {
                                minUpdateDist = d;
                                }
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)updateMessage, 
                                updateMessageLength, 
                                false, false );

                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }
                        }
                    
                    }
                if( moveMessage != NULL ) {
                    
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<movesPos.size(); u++ ) {
                        ChangePosition *p = movesPos.getElement( u );
                        
                        // move messages are never global

                        double d = intDist( p->x, p->y, 
                                            nextPlayer->xd, nextPlayer->yd );
                    
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        
                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)moveMessage, 
                                moveMessageLength, 
                                false, false );

                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }
                        }
                    }
                if( mapChangeMessage != NULL ) {
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<mapChangesPos.size(); u++ ) {
                        ChangePosition *p = mapChangesPos.getElement( u );
                        
                        // map changes are never global

                        double d = intDist( p->x, p->y, 
                                            nextPlayer->xd, nextPlayer->yd );
                        
                        if( d < minUpdateDist ) {
                            minUpdateDist = d;
                            }
                        }

                    if( minUpdateDist <= maxDist ) {
                        int numSent = 
                            nextPlayer->sock->send( 
                                (unsigned char*)mapChangeMessage, 
                                mapChangeMessageLength, 
                                false, false );
                        
                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }
                        }
                    }
                
                }
            }

        if( moveMessage != NULL ) {
            delete [] moveMessage;
            }
        if( updateMessage != NULL ) {
            delete [] updateMessage;
            }
        if( mapChangeMessage != NULL ) {
            delete [] mapChangeMessage;
            }
        

        
        // handle closing any that have an error
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement(i);

            if( nextPlayer->error && nextPlayer->deleteSent ) {
                printf( "Closing connection to player %d on error\n",
                        nextPlayer->id );
                
                sockPoll.removeSocket( nextPlayer->sock );
                
                delete nextPlayer->sock;
                delete nextPlayer->sockBuffer;
                players.deleteElement( i );
                i--;
                }
            }

        }
    

    quitCleanup();
    
    
    printf( "Done.\n" );

    return 0;
    }
