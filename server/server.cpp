
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
#include "../gameSource/objectBank.h"

#include "minorGems/util/random/JenkinsRandomSource.h"


static JenkinsRandomSource testRandSource;


typedef struct GridPos {
        int x;
        int y;
    } GridPos;



typedef struct LiveObject {
        int id;
        
        // start and dest for a move
        // same if reached destination
        int xs;
        int ys;
        
        int xd;
        int yd;
        
        // next player update should be flagged
        // as a forced position change
        char posForced;
        

        int pathLength;
        GridPos *pathToDest;
        
        char pathTruncated;

        int lastSentMapX;
        int lastSentMapY;

        // in grid square widths per second
        double moveSpeed;
        
        double moveTotalSeconds;
        double moveStartTime;
        

        int holdingID;

        int numContained;
        int *containedIDs;

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
    } ChangePosition;



int nextID = 0;



void quitCleanup() {
    printf( "Cleaning up on quit...\n" );
    

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        delete nextPlayer->sock;
        delete nextPlayer->sockBuffer;

        
        if( nextPlayer->containedIDs != NULL ) {
            delete [] nextPlayer->containedIDs;
            }
        
        if( nextPlayer->pathToDest != NULL ) {
            delete [] nextPlayer->pathToDest;
            }
        }
    
    freeMap();

    freeTransBank();
    freeObjectBank();
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

        // some messages have extra positions attached
        int numExtraPos;

        // NULL if there are no extra
        GridPos *extraPos;
    } ClientMessage;


static int pathDeltaMax = 16;


// if extraPos present in result, destroyed by caller
ClientMessage parseMessage( char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.numExtraPos = 0;
    m.extraPos = NULL;
    
    // don't require # terminator here

    int numRead = sscanf( inMessage, 
                          "%99s %d %d", nameBuffer, &( m.x ), &( m.y ) );


    if( numRead != 3 ) {
        m.type = UNKNOWN;
        return m;
        }
    

    if( strcmp( nameBuffer, "MOVE" ) == 0) {
        m.type = MOVE;

        SimpleVector<char *> *tokens =
            tokenizeString( inMessage );
        
        // require an odd number greater than 5
        if( tokens->size() < 5 || tokens->size() % 2 != 1 ) {
            tokens->deallocateStringElements();
            delete tokens;
            
            m.type = UNKNOWN;
            return m;
            }
        
        int numTokens = tokens->size();
        
        m.numExtraPos = (numTokens - 3) / 2;
        
        m.extraPos = new GridPos[ m.numExtraPos ];

        for( int e=0; e<m.numExtraPos; e++ ) {
            
            char *xToken = tokens->getElementDirect( 3 + e * 2 );
            char *yToken = tokens->getElementDirect( 3 + e * 2 + 1 );
            
            
            sscanf( xToken, "%d", &( m.extraPos[e].x ) );
            sscanf( yToken, "%d", &( m.extraPos[e].y ) );
            
            if( abs( m.extraPos[e].x ) > pathDeltaMax ||
                abs( m.extraPos[e].y ) > pathDeltaMax ) {
                // path goes too far afield
                
                // terminate it here
                m.numExtraPos = e;
                
                if( e == 0 ) {
                    delete [] m.extraPos;
                    m.extraPos = NULL;
                    }
                break;
                }
                

            // make them absolute
            m.extraPos[e].x += m.x;
            m.extraPos[e].y += m.y;
            }
        
        tokens->deallocateStringElements();
        delete tokens;
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
            
            
            
            SimpleVector<char> messageLineBuffer;
        
            // start is absolute
            char *startString = autoSprintf( "%d %d %d %.3f %.3f %d", 
                                             o->id, 
                                             o->xs, o->ys, 
                                             o->moveTotalSeconds, etaSec,
                                             o->pathTruncated );
            // mark that this has been sent
            o->pathTruncated = false;
            
            messageLineBuffer.appendElementString( startString );
            delete [] startString;
            
            for( int p=0; p<o->pathLength; p++ ) {
                // rest are relative to start
                char *stepString = autoSprintf( " %d %d", 
                                                o->pathToDest[p].x
                                                - o->xs,
                                                o->pathToDest[p].y
                                                - o->ys );
                
                messageLineBuffer.appendElementString( stepString );
                delete [] stepString;
                }
        
        

            char *messageLine = messageLineBuffer.getElementString();
                                    
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


static char isGridAdjacent( GridPos inA, GridPos inB ) {
    return isGridAdjacent( inA.x, inA.y, inB.x, inB.y );
    }



static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }


static double distance( GridPos inA, GridPos inB ) {
    return sqrt( ( inA.x - inB.x ) * ( inA.x - inB.x )
                 +
                 ( inA.y - inB.y ) * ( inA.y - inB.y ) );
    }




// sets lastSentMap in inO if chunk goes through
// returns result of send, auto-marks error in inO
int sendMapChunkMessage( LiveObject *inO ) {
    int messageLength;
    
    unsigned char *mapChunkMessage = getChunkMessage( inO->xd,
                                                      inO->yd, 
                                                      &messageLength );
                
                

                
    int numSent = 
        inO->sock->send( mapChunkMessage, 
                         messageLength, 
                         false, false );
                
    delete [] mapChunkMessage;
                

    if( numSent == messageLength ) {
        // sent correctly
        inO->lastSentMapX = inO->xd;
        inO->lastSentMapY = inO->yd;
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




char *getHoldingString( LiveObject *inObject ) {
    if( inObject->numContained == 0 ) {
        return autoSprintf( "%d", inObject->holdingID );
        }

    
    SimpleVector<char> buffer;
    

    char *idString = autoSprintf( "%d", inObject->holdingID );
    
    buffer.appendElementString( idString );
    
    delete [] idString;
    
    
    if( inObject->numContained > 0 ) {
        for( int i=0; i<inObject->numContained; i++ ) {
            
            char *idString = autoSprintf( ",%d", inObject->containedIDs[i] );
    
            buffer.appendElementString( idString );
    
            delete [] idString;
            }
        }
    
    return buffer.getElementString();
    }



int main() {

    printf( "\nServer starting up\n\n" );

    printf( "Press CTRL-C to shut down server gracefully\n\n" );

    signal( SIGTSTP, intHandler );

#ifdef WIN_32
    SetConsoleCtrlHandler( ctrlHandler, TRUE );
#endif

    initObjectBank();
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
                newObject.pathLength = 0;
                newObject.pathToDest = NULL;
                newObject.pathTruncated = 0;
                newObject.lastSentMapX = 0;
                newObject.lastSentMapY = 0;
                newObject.moveSpeed = 4;
                newObject.moveTotalSeconds = 0;
                newObject.holdingID = 0;
                newObject.numContained = 0;
                newObject.containedIDs = NULL;
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
                        //Thread::staticSleep( 1000 );
                        printf( "  Processing move, "
                                "we think player at %d,%d\n",
                                nextPlayer->xs,
                                nextPlayer->ys );

                        char interrupt = false;
                        char pathPrefixAdded = false;
                        
                        // first, construct a path from any existing
                        // path PLUS path that player is suggesting
                        SimpleVector<GridPos> unfilteredPath;

                        if( nextPlayer->xs != nextPlayer->xd ||
                            nextPlayer->ys != nextPlayer->yd ) {
                            
                            // a new move interrupting a non-stationary object
                            interrupt = true;

                            // compute closest starting position part way along
                            // path
                            double fractionDone = 
                                ( Time::getCurrentTime() - 
                                  nextPlayer->moveStartTime )
                                / nextPlayer->moveTotalSeconds;
                            
                            if( fractionDone > 1 ) {
                                fractionDone = 1;
                                }
                            
                            int c = 
                                lrint( ( nextPlayer->pathLength  - 1 ) *
                                       fractionDone );
                            GridPos cPos = nextPlayer->pathToDest[c];
                                                        
                            nextPlayer->xs = cPos.x;
                            nextPlayer->ys = cPos.y;
                            

                            printf( "Interrupt at step %d in a %d step path "
                                    " at %d,%d\n",
                                    c + 1, nextPlayer->pathLength,
                                    nextPlayer->xs, nextPlayer->ys );
                            
                            
                            char cOnTheirNewPath = false;
                            

                            for( int p=0; p<m.numExtraPos; p++ ) {
                                if( equal( cPos, m.extraPos[p] ) ) {
                                    cOnTheirNewPath = true;
                                    break;
                                    }
                                }
                            
                            if( cPos.x == m.x && cPos.y == m.y ) {
                                // also if equal to their start pos
                                cOnTheirNewPath = true;
                                }
                            


                            if( !cOnTheirNewPath ) {
                                // add prefix to their path from
                                // c to the start of their path
                                
                                // index where they think they are

                                // could be ahead or behind where we think
                                // they are
                                
                                int theirPathIndex = -1;
                            
                                for( int p=0; p<nextPlayer->pathLength; p++ ) {
                                    GridPos pos = nextPlayer->pathToDest[p];
                                    
                                    if( m.x == pos.x && m.y == pos.y ) {
                                        // reached point along old path
                                        // where player thinks they 
                                        // actually are
                                        theirPathIndex = p;
                                        break;
                                        }
                                    }
                            
                                printf( "They are on our path at index %d\n",
                                        theirPathIndex );
                            
                                if( theirPathIndex != -1 ) {
                                    // okay, they think they are on last path
                                    // that we had for them

                                    // step through path from where WE
                                    // think they should be to where they
                                    // think they are and add this as a prefix
                                    // to the path they submitted
                                    // (we may walk backward along the old
                                    //  path to do this)
                                
                                    int pathStep = 0;
                                    
                                    if( theirPathIndex < c ) {
                                        pathStep = -1;
                                        }
                                    else if( theirPathIndex > c ) {
                                        pathStep = 1;
                                        }
                                    
                                    if( pathStep != 0 ) {
                                        for( int p = c + pathStep; 
                                             p != theirPathIndex + pathStep; 
                                             p += pathStep ) {
                                            GridPos pos = 
                                                nextPlayer->pathToDest[p];
                                            
                                            unfilteredPath.push_back( pos );
                                            }
                                        }
                                    // otherwise, they are where we think
                                    // they are, and we don't need to prefix
                                    // their path

                                    printf( "Prefixing their path "
                                            "with %d steps\n",
                                            unfilteredPath.size() );
                                    }
                                }
                            }
                        
                        if( unfilteredPath.size() > 0 ) {
                            pathPrefixAdded = true;
                            }

                        // now add path player says they want to go down

                        for( int p=0; p < m.numExtraPos; p++ ) {
                            unfilteredPath.push_back( m.extraPos[p] );
                            }
                        

                        printf( "Unfiltered path = " );
                        for( int p=0; p<unfilteredPath.size(); p++ ) {
                            printf( "(%d, %d) ",
                                    unfilteredPath.getElementDirect(p).x, 
                                    unfilteredPath.getElementDirect(p).y );
                            }
                        printf( "\n" );

                        // remove any duplicate spots due to doubling back

                        for( int p=1; p<unfilteredPath.size(); p++ ) {
                            
                            if( equal( unfilteredPath.getElementDirect(p-1),
                                       unfilteredPath.getElementDirect(p) ) ) {
                                unfilteredPath.deleteElement( p );
                                p--;
                                printf( "FOUND duplicate path element\n" );
                                }
                            }
                            
                                
                                       
                        
                        nextPlayer->xd = m.extraPos[ m.numExtraPos - 1].x;
                        nextPlayer->yd = m.extraPos[ m.numExtraPos - 1].y;
                        
                        
                        if( nextPlayer->xd == nextPlayer->xs &&
                            nextPlayer->yd == nextPlayer->ys ) {
                            // this move request truncates to where
                            // we think player actually is

                            // send update to terminate move right now
                            playerIndicesToSendUpdatesAbout.push_back( i );
                            printf( "A move that takes player "
                                    "where they already are, "
                                    "ending move now\n" );
                            }
                        else {
                            // an actual move away from current xs,ys

                            if( interrupt ) {
                                printf( "Got valid move interrupt\n" );
                                }
                                

                            // check path for obstacles
                            // and make sure it contains the location
                            // where we think they are
                            
                            char truncated = 0;
                            
                            SimpleVector<GridPos> validPath;

                            char startFound = false;
                            
                            
                            int startIndex = 0;
                            // search from end first to find last occurrence
                            // of start pos
                            for( int p=unfilteredPath.size() - 1; p>=0; p-- ) {
                                
                                if( unfilteredPath.getElementDirect(p).x 
                                      == nextPlayer->xs
                                    &&
                                    unfilteredPath.getElementDirect(p).y 
                                      == nextPlayer->ys ) {
                                    
                                    startFound = true;
                                    startIndex = p;
                                    break;
                                    }
                                }
                            
                            printf( "Start index = %d\n", startIndex );
                            
                            if( ! startFound &&
                                ! isGridAdjacent( 
                                    unfilteredPath.
                                      getElementDirect(startIndex).x,
                                    unfilteredPath.
                                      getElementDirect(startIndex).y,
                                    nextPlayer->xs,
                                    nextPlayer->ys ) ) {
                                // path start jumps away from current player 
                                // start
                                // ignore it
                                }
                            else {
                                
                                GridPos lastValidPathStep =
                                    { m.x, m.y };
                                
                                if( pathPrefixAdded ) {
                                    lastValidPathStep.x = nextPlayer->xs;
                                    lastValidPathStep.y = nextPlayer->ys;
                                    }
                                
                                // we know where we think start
                                // of this path should be,
                                // but player may be behind this point
                                // on path (if we get their message late)
                                // So, it's not safe to pre-truncate
                                // the path

                                // However, we will adjust timing, below,
                                // to match where we think they should be

                                for( int p=0; 
                                     p<unfilteredPath.size(); p++ ) {
                                
                                    GridPos pos = 
                                        unfilteredPath.getElementDirect(p);

                                    if( getMapObject( pos.x, pos.y ) != 0 ) {
                                        // blockage in middle of path
                                        // terminate path here
                                        truncated = 1;
                                        break;
                                        }

                                    // make sure it's not more
                                    // than one step beyond
                                    // last step

                                    if( ! isGridAdjacent( 
                                            pos, lastValidPathStep ) ) {
                                        // a path with a break in it
                                        // terminate it here
                                        truncated = 1;
                                        break;
                                        }
                                    
                                    // no blockage, no gaps, add this step
                                    validPath.push_back( pos );
                                    lastValidPathStep = pos;
                                    }
                                }
                            
                            if( validPath.size() == 0 ) {
                                // path not permitted
                                printf( "Path submitted by player "
                                        "not valid, "
                                        "ending move now\n" );

                                nextPlayer->xd = nextPlayer->xs;
                                nextPlayer->yd = nextPlayer->ys;
                                
                                nextPlayer->posForced = true;

                                // send update about them to end the move
                                // right now
                                playerIndicesToSendUpdatesAbout.push_back( i );
                                }
                            else {
                                // a good path
                                
                                if( nextPlayer->pathToDest != NULL ) {
                                    delete [] nextPlayer->pathToDest;
                                    nextPlayer->pathToDest = NULL;
                                    }

                                nextPlayer->pathTruncated = truncated;
                                
                                nextPlayer->pathLength = validPath.size();
                                
                                nextPlayer->pathToDest = 
                                    validPath.getElementArray();
                                    
                                // path may be truncated from what was 
                                // requested, so set new d
                                nextPlayer->xd = 
                                    nextPlayer->pathToDest[ 
                                        nextPlayer->pathLength - 1 ].x;
                                nextPlayer->yd = 
                                    nextPlayer->pathToDest[ 
                                        nextPlayer->pathLength - 1 ].y;

                                // distance is number of orthogonal steps
                            
                                double dist = validPath.size();
                            

                                double distAlreadyDone = startIndex;
                            
                                nextPlayer->moveTotalSeconds = dist / 
                                    nextPlayer->moveSpeed;
                            
                                double secondsAlreadyDone = distAlreadyDone / 
                                    nextPlayer->moveSpeed;
                                
                                printf( "Skipping %f seconds along new %f-"
                                        "second path\n",
                                        secondsAlreadyDone, 
                                        nextPlayer->moveTotalSeconds );
                                
                                nextPlayer->moveStartTime = 
                                    Time::getCurrentTime() - 
                                    secondsAlreadyDone;
                            
                                nextPlayer->newMove = true;
                                }
                            }
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
                            
                                int targetSlots = 
                                    getNumContainerSlots( target );
                                
                                int numIn = 
                                    getNumContained( m.x, m.y );
                                

                                // try using object on this target 
                                
                                TransRecord *r = 
                                    getTrans( nextPlayer->holdingID, 
                                              target );

                                if( r != NULL ) {
                                    nextPlayer->holdingID = r->newActor;
                                    
                                    // has target shrunken as a container?
                                    int newSlots = 
                                        getNumContainerSlots( r->newTarget );
 
                                    shrinkContainer( m.x, m.y, newSlots );
                                                                        

                                    setMapObject( m.x, m.y, r->newTarget );
                                    
                                    
                                    char *changeLine =
                                        getMapChangeLineString(
                                            m.x, m.y );
                                    
                                    mapChanges.
                                        appendElementString( changeLine );
                                    
                                    ChangePosition p = { m.x, m.y, false };
                                    mapChangesPos.push_back( p );
                                    
                                    
                                    delete [] changeLine;
                                    }
                                else if( nextPlayer->holdingID != 0  &&
                                         numIn < targetSlots &&
                                         isContainable( 
                                             nextPlayer->holdingID ) ) {
                                    // add to container
                                        
                                    addContained( m.x, m.y,
                                                  nextPlayer->holdingID );
                                        
                                    nextPlayer->holdingID = 0;
                                    
                                    char *changeLine =
                                        getMapChangeLineString(
                                            m.x, m.y );
                                    
                                    mapChanges.
                                        appendElementString( 
                                            changeLine );
                                    
                                    delete [] changeLine;
                                    
                                    ChangePosition p = { m.x, m.y, 
                                                         false };
                                    mapChangesPos.push_back( p );
                                    }
                                else if( nextPlayer->holdingID == 0 && 
                                         numIn > 0 ) {
                                    // get from container
    
                                    nextPlayer->holdingID =
                                        removeContained( m.x, m.y );
                                        
                                    char *changeLine =
                                        getMapChangeLineString(
                                            m.x, m.y );
                                    
                                    mapChanges.
                                        appendElementString( 
                                            changeLine );
                                    
                                    delete [] changeLine;
                                    
                                    ChangePosition p = { m.x, m.y, 
                                                         false };
                                    mapChangesPos.push_back( p );
                                    }
                                else if( nextPlayer->holdingID == 0 ) {
                                    // no bare-hand transition applies to
                                    // this target object
                                    
                                    // and nothing in it to take out
                                    
                                    // treat it like GRAB
                                    
                                    nextPlayer->containedIDs =
                                        getContained( 
                                            m.x, m.y,
                                            &( nextPlayer->numContained ) );
                                    
                                    clearAllContained( m.x, m.y );
                                    setMapObject( m.x, m.y, 0 );
                                    
                                    nextPlayer->holdingID = target;
                                    
                                    char *changeLine =
                                        getMapChangeLineString(
                                            m.x, m.y );
                                    
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
                        //Thread::staticSleep( 2000 );
                        
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
                                
                                if( nextPlayer->numContained != 0 ) {
                                    for( int c=0;
                                         c < nextPlayer->numContained;
                                         c++ ) {
                                        addContained( 
                                            m.x, m.y,
                                            nextPlayer->containedIDs[c] );
                                        }
                                    delete [] nextPlayer->containedIDs;
                                    nextPlayer->containedIDs = NULL;
                                    nextPlayer->numContained = 0;
                                    }
                                
                                
                                char *changeLine =
                                    getMapChangeLineString(
                                        m.x, m.y );
                                
                                mapChanges.appendElementString( 
                                    changeLine );
                                
                                ChangePosition p = { m.x, m.y, false };
                                mapChangesPos.push_back( p );
                
                                delete [] changeLine;
                                
                                nextPlayer->holdingID = 0;

                                
                                // watch out for truncations of in-progress
                                // moves of other players
                                
                                GridPos dropSpot = { m.x, m.y };
                                
                                for( int j=0; j<numLive; j++ ) {
                                    LiveObject *otherPlayer = 
                                        players.getElement( j );
                                    
                                    if( otherPlayer->xd != otherPlayer->xs ||
                                        otherPlayer->yd != otherPlayer->ys ) {
                
                                        double fractionDone = 
                                            ( Time::getCurrentTime() - 
                                              otherPlayer->moveStartTime )
                                            / otherPlayer->moveTotalSeconds;
                                        
                                        if( fractionDone > 1 ) {
                                            fractionDone = 1;
                                            }
                                        
                                        int c = lrint( 
                                            ( otherPlayer->pathLength  - 1 ) *
                                            fractionDone );
                                        GridPos cPos = 
                                            otherPlayer->pathToDest[c];
                                        
                                        printf( 
                                            "Checking how drop at %d,%d "
                                            "affects player at step %d "
                                            "along len %d path\n",
                                            m.x, m.y,
                                            c + 1, otherPlayer->pathLength );
                                        
                                        if( distance( cPos, dropSpot ) 
                                            <= 2 * pathDeltaMax ) {
                                            
                                            // this is close enough
                                            // to this path that it might
                                            // block it

                                            char blocked = false;
                                            int blockedStep = -1;
                                            
                                            for( int p=c; 
                                                 p<otherPlayer->pathLength;
                                                 p++ ) {
                                                
                                                if( equal( 
                                                        otherPlayer->
                                                        pathToDest[p],
                                                        dropSpot ) ) {
                                                    
                                                    blocked = true;
                                                    blockedStep = p;
                                                    break;
                                                    }
                                                }
                                            
                                            if( blocked ) {
                                                printf( 
                                                    "  Blocked by drop\n" );
                                                }
                                            

                                            if( blocked &&
                                                blockedStep > 1 ) {
                                                
                                                otherPlayer->pathLength
                                                    = blockedStep;
                                                otherPlayer->pathTruncated
                                                    = true;

                                                // update timing
                                                double dist = 
                                                    otherPlayer->pathLength;
                            
                                                
                                                double distAlreadyDone = c;
                            
                                                otherPlayer->moveTotalSeconds 
                                                    = 
                                                    dist / 
                                                    otherPlayer->moveSpeed;
                            
                                                double secondsAlreadyDone = 
                                                    distAlreadyDone / 
                                                    otherPlayer->moveSpeed;
                                
                                                otherPlayer->moveStartTime = 
                                                    Time::getCurrentTime() - 
                                                    secondsAlreadyDone;
                            
                                                otherPlayer->newMove = true;
                                                
                                                otherPlayer->xd 
                                                    = otherPlayer->pathToDest[
                                                        blockedStep - 1].x;
                                                otherPlayer->yd 
                                                    = otherPlayer->pathToDest[
                                                        blockedStep - 1].y;
                                                
                                                }
                                            else if( blocked ) {
                                                // cutting off path
                                                // right at the beginning
                                                // nothing left

                                                // end move now
                                                otherPlayer->xd = 
                                                    otherPlayer->xs;
                                                
                                                otherPlayer->yd = 
                                                    otherPlayer->ys;
                                                
                                                playerIndicesToSendUpdatesAbout
                                                    .push_back( i );
                                                }
                                            } 
                                        
                                        }
                                    
                                    }
                                
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
                                
                                nextPlayer->containedIDs =
                                    getContained( 
                                        m.x, m.y,
                                        &( nextPlayer->numContained ) );
                                        
                                clearAllContained( m.x, m.y );

                                setMapObject( m.x, m.y, 0 );
                                
                                nextPlayer->holdingID = target;
                                
                                char *changeLine =
                                    getMapChangeLineString(
                                        m.x, m.y );
                                
                                mapChanges.appendElementString( 
                                    changeLine );
                                
                                ChangePosition p = { m.x, m.y, false };
                                mapChangesPos.push_back( p );    

                                delete [] changeLine;
                                }
                            }
                        }
                    
                    if( m.numExtraPos > 0 ) {
                        delete [] m.extraPos;
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
                char *holdingString = getHoldingString( nextPlayer );
                
                char *updateLine = autoSprintf( "%d %s 0 X X %.2f\n", 
                                                nextPlayer->id,
                                                holdingString,
                                                nextPlayer->moveSpeed );
                
                delete [] holdingString;

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
                        

                        printf( "Player %d's move is done at %d,%d\n", i,
                                nextPlayer->xs,
                                nextPlayer->ys );

                        if( nextPlayer->pathTruncated ) {
                            // truncated, but never told them about it
                            // force update now
                            nextPlayer->posForced = true;
                            }
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        }
                    }
                
                }
            
            
            }
        

        
        

        
        for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( 
                playerIndicesToSendUpdatesAbout.getElementDirect( i ) );

            char *holdingString = getHoldingString( nextPlayer );
            
            char *updateLine = autoSprintf( "%d %s %d %d %d %.2f\n", 
                                            nextPlayer->id,
                                            holdingString,
                                            nextPlayer->posForced,
                                            nextPlayer->xs, 
                                            nextPlayer->ys,
                                            nextPlayer->moveSpeed );
            
            delete [] holdingString;
            
            nextPlayer->posForced = false;

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
                        autoSprintf( "%d %d 0 %d %d %.2f\n", o.id, o.holdingID,
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
                
                if( abs( nextPlayer->xd - nextPlayer->lastSentMapX ) > 7
                    ||
                    abs( nextPlayer->yd - nextPlayer->lastSentMapY ) > 7 ) {
                
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
                        
                                char *holdingString = 
                                    getHoldingString( otherPlayer );
                                
                                char *updateLine = autoSprintf( 
                                    "%d %s 0 %d %d %.2f\n", 
                                    otherPlayer->id,
                                    holdingString,
                                    otherPlayer->xs, 
                                    otherPlayer->ys,
                                    otherPlayer->moveSpeed ); 

                                delete [] holdingString;
                                
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
                
                if( nextPlayer->containedIDs != NULL ) {
                    delete [] nextPlayer->containedIDs;
                    }
                
                if( nextPlayer->pathToDest != NULL ) {
                    delete [] nextPlayer->pathToDest;
                    }
                players.deleteElement( i );
                i--;
                }
            }

        }
    

    quitCleanup();
    
    
    printf( "Done.\n" );

    return 0;
    }



// implement null versions of these to allow a headless build
// we never call drawObject, but we need to use other objectBank functions


void *getSprite( int ) {
    return NULL;
    }

void drawSprite( void*, doublePair, double ) {
    }


