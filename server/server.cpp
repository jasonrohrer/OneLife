
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/network/SocketPoll.h"
#include "minorGems/network/web/WebRequest.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/system/Thread.h"

#include "minorGems/game/doublePair.h"


#include "map.h"
#include "../gameSource/transitionBank.h"
#include "../gameSource/objectBank.h"

#include "minorGems/util/random/JenkinsRandomSource.h"


static JenkinsRandomSource randSource;


typedef struct GridPos {
        int x;
        int y;
    } GridPos;


#define HEAT_MAP_D 16

float targetHeat = 10;


#define PERSON_OBJ_ID 12



// keep a running sequence number to challenge each connecting client
// to produce new login hashes, avoiding replay attacks.
static unsigned int nextSequenceNumber = 1;


static int requireClientPassword = 1;
static int requireTicketServerCheck = 1;
static char *clientPassword = NULL;
static char *ticketServerURL = NULL;


// for incoming socket connections that are still in the login process
typedef struct FreshConnection {
        Socket *sock;
        SimpleVector<char> *sockBuffer;

        unsigned int sequenceNumber;

        WebRequest *ticketServerRequest;

        char error;

        // for tracking connections that have failed to LOGIN 
        // in a timely manner
        double connectionStartTimeSeconds;

        char *email;
    } FreshConnection;


SimpleVector<FreshConnection> newConnections;



typedef struct LiveObject {
        char *email;
        
        int id;
        
        // object ID used to visually represent this player
        int displayID;
        
        // time that this life started (for computing age)
        double lifeStartTimeSeconds;
        

        // held by other player?
        char heldByOther;
        
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
        
        double moveTotalSeconds;
        double moveStartTime;
        

        int holdingID;

        // where on map held object was picked up from
        char heldOriginValid;
        int heldOriginX;
        int heldOriginY;

        int numContained;
        int *containedIDs;

        Socket *sock;
        SimpleVector<char> *sockBuffer;

        char isNew;
        char firstMessageSent;
        char error;
        char deleteSent;

        char newMove;
        
        // heat map that player carries around with them
        // every time they stop moving, it is updated to compute
        // their local temp
        float heatMap[ HEAT_MAP_D * HEAT_MAP_D ];

        // heat of local object
        // map is tracked in heat units (each object produces an 
        // integer amount of heat)
        // it is mapped into 0..1 based on targetHeat to set this value here
        float heat;
        
        int foodStore;
        
        // wall clock time when we should decrement the food store
        double foodDecrementETASeconds;
        
        // should we send player a food status message
        char foodUpdate;


        ClothingSet clothing;
        
    } LiveObject;



SimpleVector<LiveObject> players;



static LiveObject *getLiveObject( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    
    return NULL;
    }



static int getLiveObjectIndex( int inID ) {
    for( int i=0; i<players.size(); i++ ) {
        LiveObject *o = players.getElement( i );
        
        if( o->id == inID ) {
            return i;
            }
        }

    return -1;
    }





int nextID = 0;



void quitCleanup() {
    printf( "Cleaning up on quit...\n" );
    

    for( int i=0; i<newConnections.size(); i++ ) {
        FreshConnection *nextConnection = newConnections.getElement( i );
        
        delete nextConnection->sock;
        delete nextConnection->sockBuffer;
        

        if( nextConnection->ticketServerRequest != NULL ) {
            delete nextConnection->ticketServerRequest;
            }

        if( nextConnection->email != NULL ) {
            delete [] nextConnection->email;
            }
        }
    newConnections.deleteAll();
    


    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        delete nextPlayer->sock;
        delete nextPlayer->sockBuffer;

        
        if( nextPlayer->email != NULL  ) {
            delete [] nextPlayer->email;
            }

        if( nextPlayer->containedIDs != NULL ) {
            delete [] nextPlayer->containedIDs;
            }
        
        if( nextPlayer->pathToDest != NULL ) {
            delete [] nextPlayer->pathToDest;
            }
        }
    players.deleteAll();


    freeMap();

    freeTransBank();
    freeObjectBank();

    if( clientPassword != NULL ) {
        delete [] clientPassword;
        clientPassword = NULL;
        }
    

    if( ticketServerURL != NULL ) {
        delete [] ticketServerURL;
        ticketServerURL = NULL;
        }
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
    SELF,
    REMV,
    DROP,
    KILL,
    SAY,
    UNKNOWN
    } messageType;




typedef struct ClientMessage {
        messageType type;
        int x, y;

        // some messages have extra positions attached
        int numExtraPos;

        // NULL if there are no extra
        GridPos *extraPos;

        // null if type not SAY
        char *saidText;
        
    } ClientMessage;


static int pathDeltaMax = 16;


// if extraPos present in result, destroyed by caller
ClientMessage parseMessage( char *inMessage ) {
    
    char nameBuffer[100];
    
    ClientMessage m;
    
    m.numExtraPos = 0;
    m.extraPos = NULL;
    m.saidText = NULL;
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
    else if( strcmp( nameBuffer, "SELF" ) == 0 ) {
        m.type = SELF;
        }
    else if( strcmp( nameBuffer, "REMV" ) == 0 ) {
        m.type = REMV;
        }
    else if( strcmp( nameBuffer, "DROP" ) == 0 ) {
        m.type = DROP;
        }
    else if( strcmp( nameBuffer, "KILL" ) == 0 ) {
        m.type = KILL;
        }
    else if( strcmp( nameBuffer, "SAY" ) == 0 ) {
        m.type = SAY;

        // look after second space
        char *firstSpace = strstr( inMessage, " " );
        
        if( firstSpace != NULL ) {
            
            char *secondSpace = strstr( &( firstSpace[1] ), " " );
            
            if( secondSpace != NULL ) {

                char *thirdSpace = strstr( &( secondSpace[1] ), " " );
                
                if( thirdSpace != NULL ) {
                    m.saidText = stringDuplicate( &( thirdSpace[1] ) );
                    }
                }
            }
        }
    else {
        m.type = UNKNOWN;
        }
    
    return m;
    }



// compute closest starting position part way along
// path
// (-1 if closest spot is starting spot not included in path steps)
int computePartialMovePathStep( LiveObject *inPlayer ) {
    
    double fractionDone = 
        ( Time::getCurrentTime() - 
          inPlayer->moveStartTime )
        / inPlayer->moveTotalSeconds;
    
    if( fractionDone > 1 ) {
        fractionDone = 1;
        }
    
    int c = 
        lrint( ( inPlayer->pathLength ) *
               fractionDone );
    return c - 1;
    }



GridPos computePartialMoveSpot( LiveObject *inPlayer ) {

    int c = computePartialMovePathStep( inPlayer );

    if( c >= 0 ) {
        
        GridPos cPos = inPlayer->pathToDest[c];
        
        return cPos;
        }
    else {
        GridPos cPos = { inPlayer->xs, inPlayer->ys };
        
        return cPos;
        }
    }



int foodDecrementTimeSeconds = 20;


double computeFoodDecrementTimeSeconds( LiveObject *inPlayer ) {
    double value = foodDecrementTimeSeconds * 2 * inPlayer->heat;
    
    if( value > foodDecrementTimeSeconds ) {
        // also reduce if too hot (above 0.5 heat)
        
        double extra = value - foodDecrementTimeSeconds;

        value = foodDecrementTimeSeconds - extra;
        }
    
    if( value < 5 ) {
        value = 5;
        }
    return value;
    }


double getAgeRate() {
    return 1.0 / 60.0;
    }



double computeAge( LiveObject *inPlayer ) {
    double deltaSeconds = 
        Time::getCurrentTime() - inPlayer->lifeStartTimeSeconds;
    
    return deltaSeconds * getAgeRate();
    }


// false for male, true for female
char getFemale( LiveObject *inPlayer ) {
    ObjectRecord *r = getObject( inPlayer->displayID );
    
    return ! r->male;
    }




int computeFoodCapacity( LiveObject *inPlayer ) {
    int ageInYears = lrint( computeAge( inPlayer ) );
    
    if( ageInYears > 20 ) {
        ageInYears = 20;
        }

    return ageInYears + 2;
    }


double computeMoveSpeed( LiveObject *inPlayer ) {
    double age = computeAge( inPlayer );
    

    double speed = 4;
    

    if( age < 20 ) {
        speed *= age / 20;
        }
    if( age > 40 ) {
        // half speed by 60, then keep slowing down after that
        speed -= (age - 40 ) * 2.0 / 20.0;
        
        }
    
    int foodCap = computeFoodCapacity( inPlayer );
    
    
    if( inPlayer->foodStore <= foodCap / 2 ) {
        // jumps instantly to 1/2 speed at half food, then decays after that
        speed *= inPlayer->foodStore / (double) foodCap;
        }
    
    // never move at 0 speed, divide by 0 errors for eta times
    if( speed < 0.1 ) {
        speed = 0.1;
        }
    
    if( inPlayer->holdingID > 0 ) {
        ObjectRecord *r = getObject( inPlayer->holdingID );

        speed *= r->speedMult;
        }
    

    return speed;
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




// checks both grid of objects and live, non-moving player positions
char isMapSpotEmpty( int inX, int inY, char inConsiderPlayers = true ) {
    int target = getMapObject( inX, inY );
    
    if( target != 0 ) {
        return false;
        }
    
    if( !inConsiderPlayers ) {
        return true;
        }

    int numLive = players.size();
    
    for( int i=0; i<numLive; i++ ) {
        LiveObject *nextPlayer = players.getElement( i );
        
        if( // not about to be deleted
            ! nextPlayer->error &&
            // held players aren't on map (their coordinates are stale)
            ! nextPlayer->heldByOther &&
            // stationary
            nextPlayer->xs == nextPlayer->xd &&
            nextPlayer->ys == nextPlayer->yd &&
            // in this spot
            inX == nextPlayer->xd &&
            inY == nextPlayer->yd ) {
            return false;            
            } 
        }
    
    return true;
    }




// drops an object held by a player at target x,y location
// doesn't check for adjacency (so works for thrown drops too)
// if target spot blocked, will search for empty spot to throw object into
void handleDrop( int inX, int inY, LiveObject *inDroppingPlayer,
                 SimpleVector<char> *inMapChanges, 
                 SimpleVector<ChangePosition> *inChangePosList,
                 SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    ObjectRecord *droppedObject = getObject( inDroppingPlayer->holdingID );
    
    
    int targetX = inX;
    int targetY = inY;
    

    if( getMapObject( inX, inY ) != 0 ) {
        // drop spot blocked
        // search for another
        // throw held into nearest empty spot
                                    
        char found = false;
        int foundX, foundY;
        
        // change direction of throw
        // to match direction of 
        // drop action
        int xDir = inX - inDroppingPlayer->xd;
        int yDir = inY - inDroppingPlayer->yd;
                                    
        
        // check in y dir first at each
        // expanded radius?
        char yFirst = false;
        
        if( yDir != 0 ) {
            yFirst = true;
            }
        
        for( int d=1; d<10 && !found; d++ ) {
            
            char doneY0 = false;
            
            for( int yD = -d; yD<=d && !found; 
                 yD++ ) {
                
                if( ! doneY0 ) {
                    yD = 0;
                    }
                
                if( yDir != 0 ) {
                    yD *= yDir;
                    }
                
                char doneX0 = false;
                
                for( int xD = -d; 
                     xD<=d && !found; 
                     xD++ ) {
                    
                    if( ! doneX0 ) {
                        xD = 0;
                        }
                    
                    if( xDir != 0 ) {
                        xD *= xDir;
                        }
                    
                    
                    if( yD == 0 && xD == 0 ) {
                        if( ! doneX0 ) {
                            doneX0 = true;
                            
                            // back up in loop
                            xD = -d - 1;
                            }
                        continue;
                        }
                                                
                    int x = 
                        inDroppingPlayer->xd + xD;
                    int y = 
                        inDroppingPlayer->yd + yD;
                                                
                    if( yFirst ) {
                        // swap them
                        // to reverse order
                        // of expansion
                        x = 
                            inDroppingPlayer->xd + yD;
                        y =
                            inDroppingPlayer->yd + xD;
                        }
                                                


                    if( 
                        isMapSpotEmpty( x, y ) ) {
                                                    
                        found = true;
                        foundX = x;
                        foundY = y;
                        }
                                                    
                    if( ! doneX0 ) {
                        doneX0 = true;
                                                        
                        // back up in loop
                        xD = -d - 1;
                        }
                    }
                                                
                if( ! doneY0 ) {
                    doneY0 = true;
                                                
                    // back up in loop
                    yD = -d - 1;
                    }
                }
            }

        if( found ) {
            targetX = foundX;
            targetY = foundY;
            }
        else {
            // no place to drop it, it disappears

            // UNLESS we're holding a baby,
            // then just put the baby where we are
            if( inDroppingPlayer->holdingID < 0 ) {
                int babyID = - inDroppingPlayer->holdingID;
                
                LiveObject *babyO = getLiveObject( babyID );
                
                if( babyO != NULL ) {
                    babyO->xd = inDroppingPlayer->xd;
                    babyO->xs = inDroppingPlayer->xd;
                    
                    babyO->yd = inDroppingPlayer->yd;
                    babyO->ys = inDroppingPlayer->yd;

                    babyO->heldByOther = false;

                    if( getFemale( inDroppingPlayer ) ) {    
                        // reset food decrement time
                        babyO->foodDecrementETASeconds =
                            Time::getCurrentTime() +
                            computeFoodDecrementTimeSeconds( babyO );
                        }
                    
                    inPlayerIndicesToSendUpdatesAbout->push_back( 
                        getLiveObjectIndex( babyID ) );
                    }
                
                }
            
            inDroppingPlayer->holdingID = 0;
            inDroppingPlayer->heldOriginValid = 0;
            inDroppingPlayer->heldOriginX = 0;
            inDroppingPlayer->heldOriginY = 0;

            if( inDroppingPlayer->numContained != 0 ) {
                delete [] inDroppingPlayer->containedIDs;
                inDroppingPlayer->containedIDs = NULL;
                inDroppingPlayer->numContained = 0;
                }
            return;
            }            
        }
    
    
    if( inDroppingPlayer->holdingID < 0 ) {
        // dropping a baby
        
        int babyID = - inDroppingPlayer->holdingID;
                
        LiveObject *babyO = getLiveObject( babyID );
        
        if( babyO != NULL ) {
            babyO->xd = targetX;
            babyO->xs = targetX;
                    
            babyO->yd = targetY;
            babyO->ys = targetY;
            
            babyO->heldByOther = false;

            if( getFemale( inDroppingPlayer ) ) {    
                // reset food decrement time
                babyO->foodDecrementETASeconds =
                    Time::getCurrentTime() +
                    computeFoodDecrementTimeSeconds( babyO );
                }

            inPlayerIndicesToSendUpdatesAbout->push_back( 
                getLiveObjectIndex( babyID ) );
            }
        
        inDroppingPlayer->holdingID = 0;
        inDroppingPlayer->heldOriginValid = 0;
        inDroppingPlayer->heldOriginX = 0;
        inDroppingPlayer->heldOriginY = 0;
        
        return;
        }
    

    setMapObject( targetX, targetY, inDroppingPlayer->holdingID );
                                
    if( inDroppingPlayer->numContained != 0 ) {
        for( int c=0;
             c < inDroppingPlayer->numContained;
             c++ ) {
            addContained( 
                targetX, targetY,
                inDroppingPlayer->containedIDs[c] );
            }
        delete [] inDroppingPlayer->containedIDs;
        inDroppingPlayer->containedIDs = NULL;
        inDroppingPlayer->numContained = 0;
        }
                                
                                
    char *changeLine =
        getMapChangeLineString(
            targetX, targetY, inDroppingPlayer->id );
                                
    inMapChanges->appendElementString( 
        changeLine );
                                
    ChangePosition p = { targetX, targetY, false };
    inChangePosList->push_back( p );
                
    delete [] changeLine;
                                
    inDroppingPlayer->holdingID = 0;
    inDroppingPlayer->heldOriginValid = 0;
    inDroppingPlayer->heldOriginX = 0;
    inDroppingPlayer->heldOriginY = 0;
    
                                
    // watch out for truncations of in-progress
    // moves of other players
                         
    if( droppedObject->blocksWalking ) {
    
        GridPos dropSpot = { targetX, targetY };
          
        int numLive = players.size();
                      
        for( int j=0; j<numLive; j++ ) {
            LiveObject *otherPlayer = 
                players.getElement( j );
                                    
            if( otherPlayer->xd != otherPlayer->xs ||
                otherPlayer->yd != otherPlayer->ys ) {
                
                GridPos cPos = 
                    computePartialMoveSpot( otherPlayer );
                                        
                if( distance( cPos, dropSpot ) 
                    <= 2 * pathDeltaMax ) {
                                            
                    // this is close enough
                    // to this path that it might
                    // block it
                
                    int c = computePartialMovePathStep( otherPlayer );

                    // -1 means starting, pre-path pos is closest
                    // push it up to first path step
                    if( c < 0 ) {
                        c = 0;
                        }

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
                        blockedStep > 0 ) {
                                                
                        otherPlayer->pathLength
                            = blockedStep;
                        otherPlayer->pathTruncated
                            = true;

                        // update timing
                        double dist = 
                            otherPlayer->pathLength;
                            
                                                
                        double distAlreadyDone = c;
                            
                        double moveSpeed = computeMoveSpeed( otherPlayer );

                        otherPlayer->moveTotalSeconds 
                            = 
                            dist / 
                            moveSpeed;
                            
                        double secondsAlreadyDone = 
                            distAlreadyDone / 
                            moveSpeed;
                                
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
                             
                        otherPlayer->posForced = true;
                    
                        inPlayerIndicesToSendUpdatesAbout->push_back( j );
                        }
                    } 
                                        
                }                                    
            }
        }
    
                                
    }



LiveObject *getAdultHolding( LiveObject *inBabyObject ) {
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {
            return adultO;
            }
        }
    return NULL;
    }



void handleForcedBabyDrop( 
    LiveObject *inBabyObject,
    SimpleVector<char> *inMapChanges, 
    SimpleVector<ChangePosition> *inChangePosList,
    SimpleVector<int> *inPlayerIndicesToSendUpdatesAbout ) {
    
    int numLive = players.size();
    
    for( int j=0; j<numLive; j++ ) {
        LiveObject *adultO = players.getElement( j );

        if( - adultO->holdingID == inBabyObject->id ) {

            // don't need to send update about this adult's
            // holding status.
            // the update sent about the baby will inform clients
            // that the baby is no longer held by this adult
            //inPlayerIndicesToSendUpdatesAbout->push_back( j );
            
            GridPos dropPos;
            
            if( adultO->xd == 
                adultO->xs &&
                adultO->yd ==
                adultO->ys ) {
                
                dropPos.x = adultO->xd;
                dropPos.y = adultO->yd;
                }
            else {
                dropPos = 
                    computePartialMoveSpot( adultO );
                }
            
            
            handleDrop( 
                dropPos.x, dropPos.y, 
                adultO,
                inMapChanges, 
                inChangePosList,
                inPlayerIndicesToSendUpdatesAbout );

            
            break;
            }
        }
    }




char isMapSpotBlocking( int inX, int inY ) {
    int target = getMapObject( inX, inY );

    if( target == 0 ) {
        return false;
        }

    ObjectRecord *obj = getObject( target );
    
    if( obj->blocksWalking ) {
        return true;
        }

    return false;
    }






// returns 0 for NULL
static int objectRecordToID( ObjectRecord *inRecord ) {
    if( inRecord == NULL ) {
        return 0;
        }
    else {
        return inRecord->id;
        }
    }


// inDelete true to send X X for position
static char *getUpdateLine( LiveObject *inPlayer, char inDelete ) {

    char *holdingString = getHoldingString( inPlayer );
    
    char *posString;
    if( inDelete ) {
        posString = stringDuplicate( "0 X X" );
        }
    else {
        posString = autoSprintf( "%d %d %d",          
                                 inPlayer->posForced,
                                 inPlayer->xs, 
                                 inPlayer->ys );
        }
    

    char *updateLine = autoSprintf( 
        "%d %d %s %d %d %d %.2f %s %.2f %.2f %.2f %d,%d,%d,%d\n",
        inPlayer->id,
        inPlayer->displayID,
        holdingString,
        inPlayer->heldOriginValid,
        inPlayer->heldOriginX,
        inPlayer->heldOriginY,
        inPlayer->heat,
        posString,
        computeAge( inPlayer ),
        getAgeRate(),
        computeMoveSpeed( inPlayer ),
        objectRecordToID( inPlayer->clothing.hat ),
        objectRecordToID( inPlayer->clothing.tunic ),
        objectRecordToID( inPlayer->clothing.frontShoe ),
        objectRecordToID( inPlayer->clothing.backShoe ) );
    
    delete [] holdingString;
    delete [] posString;

    return updateLine;
    }

    



void processedLogggedInPlayer( Socket *inSock,
                               SimpleVector<char> *inSockBuffer,
                               char *inEmail ) {
    numConnections ++;
                
    LiveObject newObject;

    newObject.email = inEmail;
    
    newObject.id = nextID;
    nextID++;
                
    newObject.displayID = getRandomPersonObject();
                            
    newObject.lifeStartTimeSeconds = Time::getCurrentTime();
                            
    newObject.heldByOther = false;
                            
    int numOfAge = 0;
                            
    int numPlayers = players.size();
                            
    SimpleVector<LiveObject*> parentChoices;
                            
    for( int i=0; i<numPlayers; i++ ) {
                                
        double age = computeAge( players.getElement( i ) );
                    
        char f = getFemale( players.getElement( i ) );
                    
        if( age >= 14 && age <= 45 && f ) {
            numOfAge ++;
            parentChoices.push_back( players.getElement( i ) );
            }
        }
                

    if( numOfAge == 0 ) {
        // all existing children are good spawn spot for Eve
                    
        for( int i=0; i<numPlayers; i++ ) {
            parentChoices.push_back( players.getElement( i ) );
            }

        // new Eve
        // she starts almost full grown
        newObject.lifeStartTimeSeconds -= 14 * 60;

                    
        int tryCount = 0;
                    
        while( ! getFemale( &newObject ) && tryCount < 100 ) {
            newObject.displayID = getRandomPersonObject();
            tryCount ++;
            }
        }
                
                
    // else player starts as newborn
                

    // start full up to capacity with food
    newObject.foodStore = computeFoodCapacity( &newObject );

    newObject.heat = 0.5;

    newObject.foodDecrementETASeconds =
        Time::getCurrentTime() + 
        computeFoodDecrementTimeSeconds( &newObject );
                
    newObject.foodUpdate = true;
		
    newObject.clothing = getEmptyClothingSet();

    newObject.xs = 0;
    newObject.ys = 0;
    newObject.xd = 0;
    newObject.yd = 0;
                

                
    if( parentChoices.size() > 0 ) {
        // born to an existing player
        int parentIndex = 
            randSource.getRandomBoundedInt( 0,
                                            players.size() - 1 );
                    
        LiveObject *parent = players.getElement( parentIndex );
                    
        if( parent->xs == parent->xd && 
            parent->ys == parent->yd ) {
                        
            // stationary parent
            newObject.xs = parent->xs;
            newObject.ys = parent->ys;
                        
            newObject.xd = parent->xs;
            newObject.yd = parent->ys;
            }
        else {
            // find where parent is along path
            GridPos cPos = computePartialMoveSpot( parent );
                        
            newObject.xs = cPos.x;
            newObject.ys = cPos.y;
                        
            newObject.xd = cPos.x;
            newObject.yd = cPos.y;
            }
        }                    
    // else starts at 0,0 by default (lone Eve)


    
    newObject.pathLength = 0;
    newObject.pathToDest = NULL;
    newObject.pathTruncated = 0;
    newObject.lastSentMapX = 0;
    newObject.lastSentMapY = 0;
    newObject.moveTotalSeconds = 0;
    newObject.holdingID = 0;
    newObject.heldOriginValid = 0;
    newObject.heldOriginX = 0;
    newObject.heldOriginY = 0;
    newObject.numContained = 0;
    newObject.containedIDs = NULL;
    newObject.sock = inSock;
    newObject.sockBuffer = inSockBuffer;
    newObject.isNew = true;
    newObject.firstMessageSent = false;
    newObject.error = false;
    newObject.deleteSent = false;
    newObject.newMove = false;
                
                
                
                
                
    for( int i=0; i<HEAT_MAP_D * HEAT_MAP_D; i++ ) {
        newObject.heatMap[i] = 0;
        }

    players.push_back( newObject );            
            
    printf( "New player connected as player %d\n", newObject.id );
    }






int main() {

    printf( "\nServer starting up\n\n" );

    nextSequenceNumber = 
        SettingsManager::getIntSetting( "sequenceNumber", 1 );

    requireClientPassword =
        SettingsManager::getIntSetting( "requireClientPassword", 1 );
    
    requireTicketServerCheck =
        SettingsManager::getIntSetting( "requireTicketServerCheck", 1 );
    
    clientPassword = 
        SettingsManager::getStringSetting( "clientPassword" );
    

    if( clientPassword == NULL ) {
        requireClientPassword = 0;
        }


    ticketServerURL = 
        SettingsManager::getStringSetting( "ticketServerURL" );
    

    if( ticketServerURL == NULL ) {
        requireTicketServerCheck = 0;
        }



#ifdef WIN_32
    printf( "Press CTRL-C to shut down server gracefully\n\n" );

    SetConsoleCtrlHandler( ctrlHandler, TRUE );
#else
    printf( "Press CTRL-Z to shut down server gracefully\n\n" );

    signal( SIGTSTP, intHandler );
#endif

    char rebuilding;
    initObjectBankStart( &rebuilding );
    
    while( initObjectBankStep() < 1.0 );
    initObjectBankFinish();
        
    initTransBankStart( &rebuilding );
    while( initTransBankStep() < 1.0 );
    initTransBankFinish();
    
    
    initMap();
    
    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    
    
    SocketPoll sockPoll;
    
    
    
    SocketServer server( port, 256 );
    
    sockPoll.addSocketServer( &server );
    
    printf( "Listening for connection on port %d\n", port );

    while( !quit ) {
    
        int numLive = players.size();
        

        // check for timeout for shortest player move or food decrement
        // so that we wake up from listening to socket to handle it
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
                }
            
            // look at food decrement time too
                
            double timeLeft =
                nextPlayer->foodDecrementETASeconds - Time::getCurrentTime();
                        
            if( timeLeft < 0 ) {
                timeLeft = 0;
                }
            if( timeLeft < minMoveTime ) {
                minMoveTime = timeLeft;
                }                
            }
        
        SocketOrServer *readySock =  NULL;

        double pollTimeout = 2;
        
        if( minMoveTime < pollTimeout ) {
            // shorter timeout if we have to wake up for a move
            
            // HOWEVER, always keep max timout at 2 sec
            // so we always wake up periodically to catch quit signals, etc

            pollTimeout = minMoveTime;
            }
        


        
        char anyTicketServerRequestsOut = false;

        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );

            if( nextConnection->ticketServerRequest != NULL ) {
                anyTicketServerRequestsOut = true;
                break;
                }
            }
        
        if( anyTicketServerRequestsOut ) {
            // need to step outstanding ticket server web requests
            // sleep a tiny amount of time to avoid cpu spin
            pollTimeout = 0.01;
            }
        

        // we thus use zero CPU as long as no messages or new connections
        // come in, and only wake up when some timed action needs to be
        // handled
        
        readySock = sockPoll.wait( (int)( pollTimeout * 1000 ) );
        
        
        
        
        if( readySock != NULL && !readySock->isSocket ) {
            // server ready
            Socket *sock = server.acceptConnection( 0 );

            if( sock != NULL ) {
                
                printf( "Got connection\n" );

                FreshConnection newConnection;
                
                newConnection.connectionStartTimeSeconds = 
                    Time::getCurrentTime();

                newConnection.email = NULL;

                newConnection.sock = sock;

                newConnection.sequenceNumber = nextSequenceNumber;
                
                nextSequenceNumber ++;
                
                SettingsManager::setSetting( "sequenceNumber",
                                             (int)nextSequenceNumber );
                
                // wait for email and hashes to come from client
                // (and maybe ticket server check isn't required by settings)
                newConnection.ticketServerRequest = NULL;
                newConnection.error = false;

                char *message = autoSprintf( "SN\n%lu\n#", 
                                             newConnection.sequenceNumber );

                int messageLength = strlen( message );
                
                int numSent = 
                    sock->send( (unsigned char*)message, 
                                messageLength, 
                                false, false );
                    
                delete [] message;
                    

                if( numSent != messageLength ) {
                    // failed or blocked on our first send attempt

                    // reject it right away

                    delete sock;
                    sock = NULL;
                    }
                else {
                    // first message sent okay
                    newConnection.sockBuffer = new SimpleVector<char>();
                    

                    sockPoll.addSocket( sock );

                    newConnections.push_back( newConnection );
                    }

                printf( "Listening for another connection on port %d\n", 
                        port );
    
                }
            }
        


        // listen for messages from new connections

        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );
            

            if( nextConnection->ticketServerRequest != NULL ) {
                
                int result = nextConnection->ticketServerRequest->step();
                

                if( result == -1 ) {
                    printf( "Request to ticket server failed, "
                            "client rejected.\n" );
                    nextConnection->error = true;
                    }
                else if( result == 1 ) {
                    // done, have result

                    char *webResult = 
                        nextConnection->ticketServerRequest->getResult();
                    
                    if( strstr( webResult, "INVALID" ) != NULL ) {
                        printf( "Client key hmac rejected by ticket server, "
                                "client rejected.\n" );
                        nextConnection->error = true;
                        }
                    else if( strstr( webResult, "VALID" ) != NULL ) {
                        // correct!


                        const char *message = "ACCEPTED\n#";
                        int messageLength = strlen( message );
                
                        int numSent = 
                            nextConnection->sock->send( 
                                (unsigned char*)message, 
                                messageLength, 
                                false, false );
                        

                        if( numSent != messageLength ) {
                            printf( "Failed to write to client socket, "
                                    "client rejected.\n" );
                            nextConnection->error = true;
                            }
                        else {
                            // ready to start normal message exchange
                            // with client
                            
                            printf( "Got new player logged in\n" );
                            
                            processedLogggedInPlayer( 
                                nextConnection->sock,
                                nextConnection->sockBuffer,
                                nextConnection->email );
                            
                            delete nextConnection->ticketServerRequest;
                            newConnections.deleteElement( i );
                            i--;
                            }
                        }
                    delete [] webResult;
                    }
                }
            else {

                double timeDelta = Time::getCurrentTime() -
                    nextConnection->connectionStartTimeSeconds;
                

                

                char result = 
                    readSocketFull( nextConnection->sock,
                                    nextConnection->sockBuffer );
                
                if( ! result ) {
                    printf( "Failed to read from client socket, "
                            "client rejected.\n" );
                    nextConnection->error = true;
                    }
                
                char *message = 
                    getNextClientMessage( nextConnection->sockBuffer );
                
                if( message != NULL ) {
                    
                    
                    if( strstr( message, "LOGIN" ) != NULL ) {
                        
                        SimpleVector<char *> *tokens =
                            tokenizeString( message );
                        
                        if( tokens->size() == 4 ) {
                            
                            nextConnection->email = 
                                stringDuplicate( 
                                    tokens->getElementDirect( 1 ) );
                            char *pwHash = tokens->getElementDirect( 2 );
                            char *keyHash = tokens->getElementDirect( 3 );
                            

                            char emailAlreadyLoggedIn = false;
                            

                            for( int p=0; p<players.size(); p++ ) {
                                LiveObject *o = players.getElement( p );
                                

                                if( strcmp( o->email, 
                                            nextConnection->email ) == 0 ) {
                                    emailAlreadyLoggedIn = true;
                                    break;
                                    }
                                }

                            if( emailAlreadyLoggedIn ) {
                                    printf( 
                                        "Another client already "
                                        "connected as %s, "
                                        "client rejected.\n",
                                        nextConnection->email );
                                    nextConnection->error = true;
                                    }

                            if( requireClientPassword &&
                                ! nextConnection->error  ) {

                                char *value = 
                                    autoSprintf( 
                                        "%d",
                                        nextConnection->sequenceNumber );
                                
                                char *trueHash = hmac_sha1( clientPassword, 
                                                            value );

                                delete [] value;
                                

                                if( strcmp( trueHash, pwHash ) != 0 ) {
                                    printf( "Client password hmac bad, "
                                            "client rejected.\n" );
                                    nextConnection->error = true;
                                    }

                                delete [] trueHash;
                                }
                            
                            if( requireTicketServerCheck &&
                                ! nextConnection->error ) {
                                

                                char *url = autoSprintf( 
                                    "%s?action=check_ticket_hash"
                                    "&email=%s"
                                    "&hash_value=%s"
                                    "&string_to_hash=%lu",
                                    ticketServerURL,
                                    nextConnection->email,
                                    keyHash,
                                    nextConnection->sequenceNumber );

                                nextConnection->ticketServerRequest =
                                    new WebRequest( "GET", url, NULL );
                                
                                delete [] url;
                                }
                            else if( !requireTicketServerCheck &&
                                     !nextConnection->error ) {
                                
                                // let them in without checking
                                
                                const char *message = "ACCEPTED\n#";
                                int messageLength = strlen( message );
                
                                int numSent = 
                                    nextConnection->sock->send( 
                                        (unsigned char*)message, 
                                        messageLength, 
                                        false, false );
                        

                                if( numSent != messageLength ) {
                                    printf( "Failed to send on client socket, "
                                            "client rejected.\n" );
                                    nextConnection->error = true;
                                    }
                                else {
                                    // ready to start normal message exchange
                                    // with client
                            
                                    printf( "Got new player logged in\n" );
                                    
                                    processedLogggedInPlayer( 
                                        nextConnection->sock,
                                        nextConnection->sockBuffer,
                                        nextConnection->email );
                                    
                                    delete nextConnection->ticketServerRequest;
                                    newConnections.deleteElement( i );
                                    i--;
                                    }
                                }
                            }
                        else {
                            printf( "LOGIN message has wrong format, "
                                    "client rejected.\n" );
                            nextConnection->error = true;
                            }


                        tokens->deallocateStringElements();
                        delete tokens;
                        }
                    else {
                        printf( "Client's first message not LOGIN, "
                                "client rejected.\n" );
                        nextConnection->error = true;
                        }
                    
                    delete [] message;
                    }
                else if( timeDelta > 10 ) {
                    printf( "Client failed to LOGIN after 10 seconds, "
                            "client rejected.\n" );
                    nextConnection->error = true;
                    }
                }
            }
            
            
        

        // now clean up any new connections that have errors
        
        for( int i=0; i<newConnections.size(); i++ ) {
            
            FreshConnection *nextConnection = newConnections.getElement( i );
            
            if( nextConnection->error ) {
                
                // try sending REJECTED message at end

                const char *message = "REJECTED\n#";
                nextConnection->sock->send( (unsigned char*)message,
                                            strlen( message ), false, false );
                
                delete nextConnection->sock;
                delete nextConnection->sockBuffer;
                
                if( nextConnection->ticketServerRequest != NULL ) {
                    delete nextConnection->ticketServerRequest;
                    }
                
                if( nextConnection->email != NULL ) {
                    delete [] nextConnection->email;
                    }

                newConnections.deleteElement( i );
                i--;
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


        // separate accumulated text of updates for player deletes
        // these are global, so they're not tagged with positions for
        // spatial filtering
        SimpleVector<char> newDeleteUpdates;
        

        SimpleVector<char> mapChanges;
        SimpleVector<ChangePosition> mapChangesPos;
        
        SimpleVector<char> newSpeech;
        SimpleVector<ChangePosition> newSpeechPos;

        
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
                
                if( m.type == UNKNOWN ) {
                    printf( "Client error, unknown message type.\n" );
                    nextPlayer->error = true;
                    }

                //Thread::staticSleep( 
                //    testRandSource.getRandomBoundedInt( 0, 450 ) );
                
                // if player is still moving, ignore all actions
                // except for move interrupts
                if( ( nextPlayer->xs == nextPlayer->xd &&
                      nextPlayer->ys == nextPlayer->yd ) 
                    ||
                    m.type == MOVE ||
                    m.type == SAY ) {
                    

                    if( m.type == MOVE && nextPlayer->heldByOther ) {
                        // baby wiggling out of parent's arms
                        handleForcedBabyDrop( 
                            nextPlayer,
                            &mapChanges, 
                            &mapChangesPos,
                            &playerIndicesToSendUpdatesAbout );
                        
                        // drop them and ignore rest of their move
                        // request, until they click again
                        }
                    else if( m.type == MOVE ) {
                        //Thread::staticSleep( 1000 );


                        printf( "  Processing move, "
                                "we think player at old start pos %d,%d\n",
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

                            GridPos cPos = 
                                computePartialMoveSpot( nextPlayer );
                                                 
                            printf( "   we think player in motion at %d,%d\n",
                                    cPos.x,
                                    cPos.y );
       
                            nextPlayer->xs = cPos.x;
                            nextPlayer->ys = cPos.y;
                            
                            
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
                                
                                    int c = computePartialMovePathStep( 
                                        nextPlayer );
                                    
                                    // -1 means starting, pre-path 
                                    // pos is closest
                                    // push it up to first path step
                                    if( c < 0 ) {
                                        c = 0;
                                        }


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

                                    if( isMapSpotBlocking( pos.x, pos.y ) ) {
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
                            
                                double moveSpeed = computeMoveSpeed( 
                                    nextPlayer );
                                
                                nextPlayer->moveTotalSeconds = dist / 
                                    moveSpeed;
                            
                                double secondsAlreadyDone = distAlreadyDone / 
                                    moveSpeed;
                                
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
                    else if( m.type == SAY && m.saidText != NULL ) {
                        unsigned int sayLimit = 
                            (unsigned int)( 
                                floor( computeAge( nextPlayer ) ) + 1 );
                        
                        if( strlen( m.saidText ) > sayLimit ) {
                            // truncate
                            m.saidText[ sayLimit ] = '\0';
                            }
                        
                        
                        char *line = autoSprintf( "%d %s\n", nextPlayer->id,
                                                  m.saidText );
                        
                        newSpeech.appendElementString( line );
                        
                        delete [] line;
                        
                        delete [] m.saidText;


                        ChangePosition p = { nextPlayer->xd, nextPlayer->yd, 
                                             false };
                        newSpeechPos.push_back( p );
                        }
                    else if( m.type == KILL ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        if( nextPlayer->holdingID > 0 &&
                            ! (m.x == nextPlayer->xd &&
                               m.y == nextPlayer->yd ) ) {

                            // holding something
                            ObjectRecord *heldObj = 
                                getObject( nextPlayer->holdingID );
                            
                            if( heldObj->deadlyDistance > 0 ) {
                                // it's deadly

                                GridPos targetPos = { m.x, m.y };
                                GridPos playerPos = { nextPlayer->xd,
                                                      nextPlayer->yd };
                                
                                double d = distance( targetPos,
                                                     playerPos );
                                
                                if( heldObj->deadlyDistance >= d ) {
                                    // target is close enough

                                    // is anyone there?
                                    int numLive = players.size();
                                    
                                    LiveObject *hitPlayer = NULL;
                                    
                                    for( int j=0; j<numLive; j++ ) {
                                        LiveObject *otherPlayer = 
                                            players.getElement( j );
                                    
                                        

                                        if( otherPlayer->xd == 
                                            otherPlayer->xs &&
                                            otherPlayer->yd ==
                                            otherPlayer->ys ) {
                                            // other player standing still
                                            
                                            if( otherPlayer->xd ==
                                                m.x &&
                                                otherPlayer->yd ==
                                                m.y ) {
                                                
                                                // hit
                                                hitPlayer = otherPlayer;
                                                break;
                                                }
                                            }
                                        else {
                                            // other player moving
                
                                            GridPos cPos = 
                                                computePartialMoveSpot( 
                                                    otherPlayer );
                                        
                                            if( equal( cPos, targetPos ) ) {
                                                // hit
                                                hitPlayer = otherPlayer;
                                                break;
                                                }
                                            }
                                        }

                                    
                                    char hitWillDropSomething = false;
                                    
                                    if( hitPlayer != NULL ) {
                                        
                                        if( hitPlayer->holdingID != 0 ) {
                                
                                            hitWillDropSomething = true;
                                            }


                                        // break the connection with 
                                        // them
                                        hitPlayer->error = true;
                                        }
                                    
                                    
                                    // a player either hit or not
                                    // in either case, weapon was used
                                    
                                    // check for a transition for weapon

                                    // 0 is generic "on person" target
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  0 );

                                    if( r != NULL ) {
                                        nextPlayer->holdingID = r->newActor;
                                        }

                                    if( ! hitWillDropSomething &&
                                        hitPlayer != NULL ) {
                                        // player hit, and their hands were
                                        // empty.  Make them hold the new
                                        // target result of the hit.

                                        // they'll drop this later
                                        // when their death is handled
                                        hitPlayer->holdingID = r->newTarget;
                                        }
                                    else if( hitPlayer == NULL &&
                                             isMapSpotEmpty( m.x, m.y ) ) {    
                                        // no player hit, and target ground
                                        // spot is empty
                                        setMapObject( m.x, m.y, 
                                                      r->newTarget );

                                        char *changeLine =
                                            getMapChangeLineString(
                                                m.x, m.y );
                                    
                                        mapChanges.
                                            appendElementString( changeLine );
                                    
                                        ChangePosition p = { m.x, m.y, false };
                                        mapChangesPos.push_back( p );
                                    
                                        delete [] changeLine;
                                        }
                                    // else new target, post-kill-attempt
                                    // is lost
                                    }
                                }
                            }
                        }
                    else if( m.type == USE ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );

                        if( isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) 
                            ||
                            ( m.x == nextPlayer->xd &&
                              m.y == nextPlayer->yd ) ) {
                            
                            // can only use on targets next to us for now,
                            // no diags
                            
                            int target = getMapObject( m.x, m.y );
                            
                            if( target != 0 ) {                                

                                // try using object on this target 
                                
                                TransRecord *r = NULL;
                                
                                if( nextPlayer->holdingID >= 0 ) {
                                    // negative holding is ID of baby
                                    // which can't be used
                                    // (and no bare hand action available)
                                    r = getTrans( nextPlayer->holdingID,
                                                  target );
                                    }
                                
                                if( r != NULL ) {
                                    int oldContained = 
                                        nextPlayer->numContained;

                                    nextPlayer->holdingID = r->newActor;
                                    
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;

                                    // can newly changed container hold
                                    // less than what it could contain
                                    // before?

                                    int newHeldSlots = getNumContainerSlots( 
                                        nextPlayer->holdingID );

                                    if( newHeldSlots < oldContained ) {
                                        // truncate
                                        nextPlayer->numContained
                                            = newHeldSlots;
                                        }

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
                                else if( nextPlayer->holdingID == 0 &&
                                         ! getObject( target )->permanent ) {
                                    // no bare-hand transition applies to
                                    // this non-permanent target object
                                    
                                    // treat it like pick up
                                    
                                    nextPlayer->containedIDs =
                                        getContained( 
                                            m.x, m.y,
                                            &( nextPlayer->numContained ) );
                                    
                                    clearAllContained( m.x, m.y );
                                    setMapObject( m.x, m.y, 0 );
                                    
                                    nextPlayer->holdingID = target;
                                    
                                    nextPlayer->heldOriginValid = 1;
                                    nextPlayer->heldOriginX = m.x;
                                    nextPlayer->heldOriginY = m.y;

                                    char *changeLine =
                                        getMapChangeLineString(
                                            m.x, m.y );
                                    
                                    mapChanges.appendElementString( 
                                        changeLine );
                                    
                                    ChangePosition p = { m.x, m.y, false };
                                    mapChangesPos.push_back( p );
                                    
                                    delete [] changeLine;
                                    }
                                else if( nextPlayer->holdingID != 0 ) {
                                    // no transition for what we're
                                    // holding on target

                                    // check if surrounded

                                    int nA = 
                                        getMapObject( nextPlayer->xd - 1, 
                                                      nextPlayer->yd );
                                    int nB = 
                                        getMapObject( nextPlayer->xd + 1, 
                                                      nextPlayer->yd );
                                    int nC = 
                                        getMapObject( nextPlayer->xd, 
                                                      nextPlayer->yd - 1 );
                                    int nD = 
                                        getMapObject( nextPlayer->xd, 
                                                      nextPlayer->yd + 1 );
                                    
                                    if( nA != 0 && nB != 0 && 
                                        nC != 0 && nD != 0 ) {
                                        

                                        // surrounded while holding
                                    
                                        // throw held into nearest empty spot
                                        
                                        handleDrop( 
                                            m.x, m.y, 
                                            nextPlayer,
                                            &mapChanges, 
                                            &mapChangesPos,
                                            &playerIndicesToSendUpdatesAbout );
                                        }
                                    
                                    // action doesn't happen, just the drop
                                    }
                                }
                            else if( nextPlayer->holdingID != 0 &&
                                     ! (m.x == nextPlayer->xd &&
                                        m.y == nextPlayer->yd ) ) {
                                // target location emtpy
                                // target not where we're standing
                                // we're holding something
                                
                                // consider a use-on-bare-ground transtion
                                
                                ObjectRecord *obj = 
                                    getObject( nextPlayer->holdingID );
                                
                                if( obj->foodValue == 0 ) {
                                    
                                    // get no-target transtion
                                    // (not a food transition, since food
                                    //   value is 0)
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  -1 );

                                    if( r != NULL &&
                                        r->newTarget != 0 ) {

                                        int oldContained = 
                                            nextPlayer->numContained;
                                        
                                        nextPlayer->holdingID = r->newActor;
                                    
                                        nextPlayer->heldOriginValid = 0;
                                        nextPlayer->heldOriginX = 0;
                                        nextPlayer->heldOriginY = 0;

                                        // can newly changed container hold
                                        // less than what it could contain
                                        // before?

                                        int newHeldSlots = 
                                            getNumContainerSlots( 
                                                nextPlayer->holdingID );

                                        if( newHeldSlots < oldContained ) {
                                            // truncate
                                            nextPlayer->numContained
                                                = newHeldSlots;
                                            }
                                                                        

                                        setMapObject( m.x, m.y, r->newTarget );
                                    
                                    
                                        char *changeLine =
                                            getMapChangeLineString( m.x, m.y );
                                        
                                        mapChanges.
                                            appendElementString( changeLine );
                                    
                                        ChangePosition p = { m.x, m.y, false };
                                        mapChangesPos.push_back( p );
                                    
                                        delete [] changeLine;
                                        }
                                    }
                                }
                            else if( nextPlayer->holdingID == 0 ) {
                                // target location empty and 
                                // and our hands are empty
                                
                                // check if there's a baby to pick up there
                                
                                GridPos targetPos = { m.x, m.y };
                                
                                // is anyone there?
                                int numLive = players.size();
                                
                                LiveObject *hitPlayer = NULL;
                                
                                for( int j=0; j<numLive; j++ ) {
                                    LiveObject *otherPlayer = 
                                        players.getElement( j );
                                    
                                    if( otherPlayer == nextPlayer ) {
                                        // don't consider click on self
                                        continue;
                                        }

                                    if( otherPlayer->xd == 
                                        otherPlayer->xs &&
                                        otherPlayer->yd ==
                                        otherPlayer->ys ) {
                                        // other player standing still
                                            
                                        if( otherPlayer->xd ==
                                            m.x &&
                                            otherPlayer->yd ==
                                            m.y ) {
                                                
                                            // hit
                                            hitPlayer = otherPlayer;
                                            break;
                                            }
                                        }
                                    else {
                                        // other player moving
                                        
                                        GridPos cPos = 
                                            computePartialMoveSpot( 
                                                otherPlayer );
                                        
                                        if( equal( cPos, targetPos ) ) {
                                            // hit
                                            hitPlayer = otherPlayer;
                                            break;
                                            }
                                        }
                                    }
                                
                                if( hitPlayer != NULL &&
                                    computeAge( hitPlayer ) < 5  ) {
                                    
                                    // negative holding IDs to indicate
                                    // holding another player
                                    nextPlayer->holdingID = -hitPlayer->id;
                                    
                                    hitPlayer->heldByOther = true;
                                    
                                    // force baby to drop what they are
                                    // holding

                                    if( hitPlayer->holdingID != 0 ) {
                                        handleDrop( 
                                            m.x, m.y, hitPlayer,
                                            &mapChanges, &mapChangesPos,
                                            &playerIndicesToSendUpdatesAbout );
                                        }
                                    
                                    if( hitPlayer->xd != hitPlayer->xs
                                        ||
                                        hitPlayer->yd != hitPlayer->ys ) {
                                        
                                        // force baby to stop moving
                                        hitPlayer->xd = m.x;
                                        hitPlayer->yd = m.y;
                                        hitPlayer->xs = m.x;
                                        hitPlayer->ys = m.y;
                                        
                                        // but don't send an update
                                        // about this
                                        // (everyone will get the pick-up
                                        //  update for the holding adult)
                                        }
                                    
                                    // if adult female, baby auto-fed
                                    if( getFemale( nextPlayer ) ) {
                                        
                                        hitPlayer->foodStore = 
                                            computeFoodCapacity( hitPlayer );
                
                                        hitPlayer->foodUpdate = true;
                                        }
                                    
                                    nextPlayer->heldOriginValid = 1;
                                    nextPlayer->heldOriginX = m.x;
                                    nextPlayer->heldOriginY = m.y;
                                    }
                                
                                }
                            }
                        }
                    else if( m.type == SELF ) {
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        if( m.x == nextPlayer->xd &&
                            m.y == nextPlayer->yd ) {
                            
                            // use on self
                            if( nextPlayer->holdingID > 0 ) {
                                ObjectRecord *obj = 
                                    getObject( nextPlayer->holdingID );
                                
                                if( obj->foodValue > 0 ) {
                                    nextPlayer->foodStore += obj->foodValue;
                                    
                                    int cap =
                                        computeFoodCapacity( nextPlayer );
                                    
                                    if( nextPlayer->foodStore > cap ) {
                                        nextPlayer->foodStore = cap;
                                        }
                                    
                                    // get eat transtion
                                    TransRecord *r = 
                                        getTrans( nextPlayer->holdingID, 
                                                  -1 );

                                    // default, holding nothing after eating
                                    nextPlayer->holdingID = 0;


                                    if( r != NULL ) {
                                        nextPlayer->holdingID = r->newActor;
                                        }
                                    
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                    
                                    nextPlayer->foodUpdate = true;
                                    }
                                else if( obj->clothing != 'n' ) {
                                    // wearable
                                    
                                    nextPlayer->holdingID = 0;
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;

                                    ObjectRecord *oldC = NULL;
                                    
                                    switch( obj->clothing ) {
                                        case 'h':
                                            oldC = nextPlayer->clothing.hat;
                                            nextPlayer->clothing.hat = obj;
                                            if( oldC != NULL ) {
                                                nextPlayer->holdingID =
                                                    oldC->id;
                                                }
                                            break;
                                        case 't':
                                            oldC = nextPlayer->clothing.tunic;
                                            nextPlayer->clothing.tunic = obj;
                                            if( oldC != NULL ) {
                                                nextPlayer->holdingID =
                                                    oldC->id;
                                                }
                                            break;
                                        case 's':
                                            if( nextPlayer->clothing.backShoe
                                                == NULL ) {
                                                nextPlayer->clothing.backShoe 
                                                    = obj;
                                                }
                                            else if( 
                                                nextPlayer->clothing.frontShoe
                                                == NULL ) {
                                                nextPlayer->clothing.frontShoe 
                                                    = obj;
                                                }
                                            else {
                                                // replace a shoe
                                                
                                                oldC = 
                                                    nextPlayer->
                                                    clothing.frontShoe;
                                                nextPlayer->clothing.frontShoe 
                                                    = obj;
                                                if( oldC != NULL ) {
                                                    nextPlayer->holdingID =
                                                        oldC->id;
                                                    }
                                                }
                                            break;
                                        }
                                    }
                                }
                            else {
                                // empty hand on self, remove clothing
                                if( nextPlayer->clothing.frontShoe != NULL ) {
                                    nextPlayer->holdingID =
                                        nextPlayer->clothing.frontShoe->id;
                                    nextPlayer->clothing.frontShoe = NULL;
                                    }
                                else if( nextPlayer->clothing.backShoe 
                                         != NULL ) {
                                    nextPlayer->holdingID =
                                        nextPlayer->clothing.backShoe->id;
                                    nextPlayer->clothing.backShoe = NULL;
                                    }
                                else if( nextPlayer->clothing.hat != NULL ) {
                                    nextPlayer->holdingID =
                                        nextPlayer->clothing.hat->id;
                                    nextPlayer->clothing.hat = NULL;
                                    }
                                else if( nextPlayer->clothing.tunic != NULL ) {
                                    nextPlayer->holdingID =
                                        nextPlayer->clothing.tunic->id;
                                    nextPlayer->clothing.tunic = NULL;
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
                                            nextPlayer->yd ) 
                            ||
                            ( m.x == nextPlayer->xd &&
                              m.y == nextPlayer->yd ) ) {
                            
                            if( nextPlayer->holdingID != 0 ) {
                                
                                if( nextPlayer->holdingID < 0 ) {
                                    // baby drop
                                    int target = getMapObject( m.x, m.y );
                                    
                                    if( target == 0 // nothing here
                                        ||
                                        ! getObject( target )->
                                            blocksWalking ) {
                                        handleDrop( 
                                            m.x, m.y, nextPlayer,
                                            &mapChanges, &mapChangesPos,
                                            &playerIndicesToSendUpdatesAbout );
                                        }    
                                    }
                                else if( isMapSpotEmpty( m.x, m.y ) ) {
                                
                                    // empty spot to drop non-baby into
                                    
                                    handleDrop( 
                                        m.x, m.y, nextPlayer,
                                        &mapChanges, &mapChangesPos,
                                        &playerIndicesToSendUpdatesAbout );
                                    }
                                else if( nextPlayer->holdingID > 0 ) {
                                    // target not empty
                                    // non-baby drop only
                                    
                                    int target = getMapObject( m.x, m.y );
                            
                                    if( target != 0 ) {
                                        int targetSlots = 
                                            getNumContainerSlots( target );
                                        
                                        int slotSize =
                                            getObject( target )->slotSize;
                                        
                                        int containSize =
                                            getObject( 
                                                nextPlayer->holdingID )->
                                            containSize;

                                        int numIn = 
                                            getNumContained( m.x, m.y );
                                        
                                        if( numIn < targetSlots &&
                                            isContainable( 
                                                nextPlayer->holdingID ) &&
                                            containSize <= slotSize ) {
                                            
                                            // add to container
                                        
                                            addContained( 
                                                m.x, m.y,
                                                nextPlayer->holdingID );
                                        
                                            nextPlayer->holdingID = 0;
                                            nextPlayer->heldOriginValid = 0;
                                            nextPlayer->heldOriginX = 0;
                                            nextPlayer->heldOriginY = 0;
                                            
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
                                        }
                                    else {
                                        // no object here
                                        
                                        // maybe there's a person
                                        // standing here

                                        // only allow drop if what we're
                                        // dropping is non-blocking
                                        ObjectRecord *droppedObj
                                            = getObject( 
                                                nextPlayer->holdingID );
                                        
                                        if( ! droppedObj->blocksWalking ) {
                                            
                                             handleDrop( 
                                              m.x, m.y, nextPlayer,
                                              &mapChanges, &mapChangesPos,
                                              &playerIndicesToSendUpdatesAbout 
                                              );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    else if( m.type == REMV ) {
                        // send update even if action fails (to let them
                        // know that action is over)
                        playerIndicesToSendUpdatesAbout.push_back( i );
                        
                        nextPlayer->heldOriginValid = 0;
                        nextPlayer->heldOriginX = 0;
                        nextPlayer->heldOriginY = 0;
                        


                        if( isGridAdjacent( m.x, m.y,
                                            nextPlayer->xd, 
                                            nextPlayer->yd ) 
                            ||
                            ( m.x == nextPlayer->xd &&
                              m.y == nextPlayer->yd ) ) {
                            
                            // can only use on targets next to us for now,
                            // no diags
                            
                            int target = getMapObject( m.x, m.y );
                            
                            if( target != 0 ) {
                            
                                int numIn = 
                                    getNumContained( m.x, m.y );
                                

                                if( nextPlayer->holdingID == 0 && 
                                    numIn > 0 ) {
                                    // get from container
                                    
                                    nextPlayer->holdingID =
                                        removeContained( m.x, m.y );
                                    
                                    // contained objects aren't animating
                                    // in a way that needs to be smooth
                                    // transitioned on client
                                    nextPlayer->heldOriginValid = 0;
                                    nextPlayer->heldOriginX = 0;
                                    nextPlayer->heldOriginY = 0;
                                        
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
                                }
                            }
                        
                        }
                    
                    
                    if( m.numExtraPos > 0 ) {
                        delete [] m.extraPos;
                        }
                    }                
                }
            }



        // now that messages have been processed for all
        // loop over and handle all post-message checks

        // for example, if a player later in the list sends a message
        // killing an earlier player, we need to check to see that
        // player deleted afterward here
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
                
            if( nextPlayer->isNew ) {
                // their first position is an update
                

                playerIndicesToSendUpdatesAbout.push_back( i );
                
                nextPlayer->isNew = false;
                
                // force this PU to be sent to everyone
                ChangePosition p = { 0, 0, true };
                newUpdatesPos.push_back( p );
                }
            else if( nextPlayer->error && ! nextPlayer->deleteSent ) {
                
                if( nextPlayer->heldByOther ) {
                    
                    handleForcedBabyDrop( nextPlayer,
                                          &mapChanges, 
                                          &mapChangesPos,
                                          &playerIndicesToSendUpdatesAbout );
                    }
                
                char *updateLine = getUpdateLine( nextPlayer, true );

                newDeleteUpdates.appendElementString( updateLine );
                
                delete [] updateLine;
                
                nextPlayer->isNew = false;
                
                nextPlayer->deleteSent = true;

                GridPos dropPos;
                
                if( nextPlayer->xd == 
                    nextPlayer->xs &&
                    nextPlayer->yd ==
                    nextPlayer->ys ) {
                    // deleted player standing still
                    
                    dropPos.x = nextPlayer->xd;
                    dropPos.y = nextPlayer->yd;
                    }
                else {
                    // player moving
                    
                    dropPos = 
                        computePartialMoveSpot( nextPlayer );
                    }


                // assume death markes non-blocking, so it's safe
                // to drop one even if other players standing here
                if( isMapSpotEmpty( dropPos.x, dropPos.y, false ) ) {
                    int deathID = getRandomDeathMarker();
                    
                    if( deathID > 0 ) {
                        
                        setMapObject( dropPos.x, dropPos.y, 
                                      getRandomDeathMarker() );
                        
                        
                        ObjectRecord *deathObject = getObject( deathID );
                        
                        if( deathObject->numSlots >= 4 ) {
                            // room for clothing
                            
                            if( nextPlayer->clothing.tunic != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.tunic->id );
                                }
                            if( nextPlayer->clothing.backShoe != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.backShoe->id );
                                }
                            if( nextPlayer->clothing.frontShoe != NULL ) {
                                
                                addContained( 
                                    dropPos.x, dropPos.y,
                                    nextPlayer->clothing.frontShoe->id );
                                }
                            if( nextPlayer->clothing.hat != NULL ) {
                                
                                addContained( dropPos.x, dropPos.y,
                                              nextPlayer->clothing.hat->id );
                                }
                            }

                        char *changeLine =
                            getMapChangeLineString( dropPos.x, dropPos.y );
                        
                        mapChanges.appendElementString( changeLine );
                        
                        ChangePosition p = { dropPos.x, dropPos.y, false };
                        mapChangesPos.push_back( p );
                        
                        delete [] changeLine;
                        }  
                    }
                if( nextPlayer->holdingID != 0 ) {
                                        
                    // drop what they were holding
                    // this will almost always involve a throw
                    // (death marker, at least, will be in the way)
                    handleDrop( 
                        dropPos.x, dropPos.y, 
                        nextPlayer,
                        &mapChanges, 
                        &mapChangesPos,
                        &playerIndicesToSendUpdatesAbout );
                    }
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
                
                // check if we need to decrement their food
                if( Time::getCurrentTime() > 
                    nextPlayer->foodDecrementETASeconds ) {
                    
                    char heldByFemale = false;

                    if( nextPlayer->heldByOther ) {
                        LiveObject *adultO = getAdultHolding( nextPlayer );
                        
                        if( adultO != NULL &&
                            getFemale( adultO ) ) {
                    
                            heldByFemale = true;
                            }
                        }
                    
                    if( !heldByFemale ) {
                        nextPlayer->foodStore --;
                        }
                    nextPlayer->foodDecrementETASeconds +=
                        computeFoodDecrementTimeSeconds( nextPlayer );
                    
                    if( nextPlayer->foodStore <= 0 ) {
                        // player has died
                        
                        // break the connection with them
                        nextPlayer->error = true;

                        // no negative
                        nextPlayer->foodStore = 0;
                        }
                    
                    if( !heldByFemale ) {
                        nextPlayer->foodUpdate = true;
                        }
                    }
                
                }
            
            
            }
        

        
        

        
        for( int i=0; i<playerIndicesToSendUpdatesAbout.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement( 
                playerIndicesToSendUpdatesAbout.getElementDirect( i ) );


            // recompute heat map
            
            // assume instant decay of heat off edge (infinite heat sink
            // outside of our window).  Otherwise, we achieve stasis
            // if entire map filled with same value
            
            for( int y=0; y<HEAT_MAP_D; y++ ) {
                nextPlayer->heatMap[ y * HEAT_MAP_D ] = 0;
                nextPlayer->heatMap[ y * HEAT_MAP_D + HEAT_MAP_D - 1 ] = 0;
                }
            for( int x=0; x<HEAT_MAP_D; x++ ) {
                nextPlayer->heatMap[ x ] = 0;
                nextPlayer->heatMap[ (HEAT_MAP_D - 1) * HEAT_MAP_D + x ] = 0;
                }
            
            int heatOutputGrid[ HEAT_MAP_D * HEAT_MAP_D ];
            float rGrid[ HEAT_MAP_D * HEAT_MAP_D ];

            for( int y=0; y<HEAT_MAP_D; y++ ) {
                int mapY = nextPlayer->ys + y - HEAT_MAP_D / 2;
                
                for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
                    int mapX = nextPlayer->xs + x - HEAT_MAP_D / 2;
                    

                    ObjectRecord *o = getObject( getMapObject( mapX, mapY ) );
                    
                    int j = y * HEAT_MAP_D + x;

                    if( o != NULL ) {
                        heatOutputGrid[j] = o->heatValue;
                        rGrid[j] = o->rValue;
                        }
                    else {
                        heatOutputGrid[j] = 0;
                        rGrid[j] = 0;
                        }
                    }
                }

            // clothing is additive to R value at center spot

            float clothingR = 0;
            
            if( nextPlayer->clothing.hat != NULL ) {
                clothingR += nextPlayer->clothing.hat->rValue;
                }
            if( nextPlayer->clothing.tunic != NULL ) {
                clothingR += nextPlayer->clothing.tunic->rValue;
                }
            if( nextPlayer->clothing.frontShoe != NULL ) {
                clothingR += nextPlayer->clothing.frontShoe->rValue;
                }
            if( nextPlayer->clothing.backShoe != NULL ) {
                clothingR += nextPlayer->clothing.backShoe->rValue;
                }
            
            int playerMapIndex = 
                ( HEAT_MAP_D / 2 ) * HEAT_MAP_D +
                ( HEAT_MAP_D / 2 );
            

            rGrid[ playerMapIndex ] += clothingR;
            
            
            if( rGrid[ playerMapIndex ] > 1 ) {
                
                rGrid[ playerMapIndex ] = 1;
                }
            

            // body itself produces 1 unit of heat
            // (r value of clothing can hold this in
            heatOutputGrid[ playerMapIndex ] += 1;

            
            //double startTime = Time::getCurrentTime();
            
            int numCycles = 20;
            
            int numNeighbors = 4;
            int ndx[4] = { 0, 1,  0, -1 };
            int ndy[4] = { 1, 0, -1,  0 };

            for( int c=0; c<numCycles; c++ ) {
                
                float tempHeatGrid[ HEAT_MAP_D * HEAT_MAP_D ];
                memcpy( tempHeatGrid, nextPlayer->heatMap, 
                        HEAT_MAP_D * HEAT_MAP_D * sizeof( float ) );
                
                for( int y=1; y<HEAT_MAP_D-1; y++ ) {
                    for( int x=1; x<HEAT_MAP_D-1; x++ ) {
                        int j = y * HEAT_MAP_D + x;
            
                        float heatDelta = 0;
                
                        float centerWeight = 1 - rGrid[j];

                        float centerOldHeat = tempHeatGrid[j];

                        for( int n=0; n<numNeighbors; n++ ) {
                
                            int nx = x + ndx[n];
                            int ny = y + ndy[n];
                        
                            int nj = ny * HEAT_MAP_D + nx;
                        
                            float nWeight = 1 - rGrid[ nj ];
                    
                            heatDelta += centerWeight * nWeight *
                                ( tempHeatGrid[ nj ] - centerOldHeat );
                            }
                
                        nextPlayer->heatMap[j] = 
                            tempHeatGrid[j] + heatDelta / numNeighbors;
                
                        nextPlayer->heatMap[j] += heatOutputGrid[j];
                        }
                    }
                }
            
            //printf( "Computing %d cycles took %f ms\n",
            //        numCycles, 
            //        ( Time::getCurrentTime() - startTime ) * 1000 );
            /*
            printf( "Player heat map:\n" );
            
            for( int y=0; y<HEAT_MAP_D; y++ ) {
                
                for( int x=0; x<HEAT_MAP_D; x++ ) {
                    
                    if( y == HEAT_MAP_D / 2 &&
                        x == HEAT_MAP_D / 2 ) {
                        printf( "p" );
                        }
                    if( heatOutputGrid[ y * HEAT_MAP_D + x ] > 0 ) {
                        printf( "H" );
                        }

                    printf( "%.1f ", 
                            nextPlayer->heatMap[ y * HEAT_MAP_D + x ] );
                            //tempHeatGrid[ y * HEAT_MAP_D + x ] );
                    }
                printf( "\n" );
                }
            */

            float playerHeat = 
                nextPlayer->heatMap[ playerMapIndex ];
            
            // convert into 0..1 range, where 0.5 represents targetHeat
            nextPlayer->heat = ( playerHeat / targetHeat ) / 2;
            if( nextPlayer->heat > 1 ) {
                nextPlayer->heat = 1;
                }
            

            
            char *updateLine = getUpdateLine( nextPlayer, false );

            
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



        char *deleteUpdateMessage = NULL;
        int deleteUpdateMessageLength = 0;
        
        if( newDeleteUpdates.size() > 0 ) {
            newDeleteUpdates.push_back( '#' );
            char *temp = newDeleteUpdates.getElementString();

            deleteUpdateMessage = concatonate( "PU\n", temp );
            delete [] temp;

            deleteUpdateMessageLength = strlen( deleteUpdateMessage );
            }
        


        // add changes from auto-decays on map
        stepMap( &mapChanges, &mapChangesPos );
        
        
        char *mapChangeMessage = NULL;
        int mapChangeMessageLength = 0;
        
        if( mapChanges.size() > 0 ) {
            mapChanges.push_back( '#' );
            char *temp = mapChanges.getElementString();

            mapChangeMessage = concatonate( "MX\n", temp );
            delete [] temp;

            mapChangeMessageLength = strlen( mapChangeMessage );
            }
        


        char *speechMessage = NULL;
        int speechMessageLength = 0;
        
        if( newSpeech.size() > 0 ) {
            newSpeech.push_back( '#' );
            char *temp = newSpeech.getElementString();

            speechMessage = concatonate( "PS\n", temp );
            delete [] temp;

            speechMessageLength = strlen( speechMessage );
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
                
                    LiveObject *o = players.getElement( i );
                

                    char *messageLine = getUpdateLine( o, false );
                    
                    if( o->id != nextPlayer->id ) {
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
                
                if( abs( nextPlayer->xd - nextPlayer->lastSentMapX ) > 3
                    ||
                    abs( nextPlayer->yd - nextPlayer->lastSentMapY ) > 5 ) {
                
                    // moving out of bounds of chunk, send update
                    
                    
                    sendMapChunkMessage( nextPlayer );


                    // send updates about any non-moving players
                    // that are in this chunk
                    SimpleVector<char> chunkPlayerUpdates;

                    SimpleVector<char> chunkPlayerMoves;
                    

                    // add chunk updates for held babies first
                    for( int j=0; j<numLive; j++ ) {
                        LiveObject *otherPlayer = players.getElement( j );
                        
                        if( otherPlayer->heldByOther ) {
                            LiveObject *adultO = 
                                getAdultHolding( otherPlayer );
                            
                            if( adultO != NULL ) {
                                

                                if( adultO->id != nextPlayer->id ) {
                                    // not us

                                    double d = intDist( nextPlayer->xd,
                                                        nextPlayer->yd,
                                                        adultO->xd,
                                                        adultO->yd );
                            
                            
                                    if( d <= getChunkDimension() / 2 ) {
                                        // adult holding this baby
                                        // is close enough
                                        // send update about baby
                                        char *updateLine = 
                                            getUpdateLine( otherPlayer, 
                                                           false ); 
                                    
                                        chunkPlayerUpdates.
                                            appendElementString( updateLine );
                                        delete [] updateLine;
                                        }
                                    }
                                }
                            }
                        }
                    
                    
                    // now send updates about all non-held babies,
                    // including any adults holding on-chunk babies
                    // here, AFTER we update about the babies

                    // (so their held status overrides the baby's stale
                    //  position status).
                    for( int j=0; j<numLive; j++ ) {
                        LiveObject *otherPlayer = 
                            players.getElement( j );
                        
                        if( !otherPlayer->heldByOther &&
                            otherPlayer->id != nextPlayer->id ) {
                            // not us
                            // not a held baby (covered above)

                            double d = intDist( nextPlayer->xd,
                                                nextPlayer->yd,
                                                otherPlayer->xd,
                                                otherPlayer->yd );
                            
                            
                            if( d <= getChunkDimension() / 2 ) {
                                
                                // send next player a player update
                                // about this player, telling nextPlayer
                                // where this player was last stationary
                                // and what they're holding

                                char *updateLine = 
                                    getUpdateLine( otherPlayer, false ); 
                                    
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
                if( speechMessage != NULL ) {
                    double minUpdateDist = 64;
                    
                    for( int u=0; u<newSpeechPos.size(); u++ ) {
                        ChangePosition *p = newSpeechPos.getElement( u );
                        
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
                                (unsigned char*)speechMessage, 
                                speechMessageLength, 
                                false, false );
                        
                        if( numSent == -1 ) {
                            nextPlayer->error = true;
                            }
                        }
                    }
                

                // EVERYONE gets updates about deleted players
                if( deleteUpdateMessage != NULL ) {
                    int numSent = 
                        nextPlayer->sock->send( 
                            (unsigned char*)deleteUpdateMessage, 
                            deleteUpdateMessageLength, 
                            false, false );
                    
                    if( numSent == -1 ) {
                        nextPlayer->error = true;
                        }
                    }
                


                if( nextPlayer->foodUpdate ) {
                    // send this player a food status change

                     char *foodMessage = autoSprintf( 
                         "FX\n"
                         "%d %d %.2f\n"
                         "#", 
                         nextPlayer->foodStore,
                         computeFoodCapacity( nextPlayer ),
                         computeMoveSpeed( nextPlayer ) );
                     
                     int numSent = 
                         nextPlayer->sock->send( 
                             (unsigned char*)foodMessage, 
                             strlen( foodMessage ), 
                             false, false );

                     if( numSent == -1 ) {
                         nextPlayer->error = true;
                         }
                     
                     delete [] foodMessage;
                     
                     nextPlayer->foodUpdate = false;
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
        if( speechMessage != NULL ) {
            delete [] speechMessage;
            }
        if( deleteUpdateMessage != NULL ) {
            delete [] deleteUpdateMessage;
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

                if( nextPlayer->email != NULL ) {
                    delete [] nextPlayer->email;
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

void drawSprite( void*, doublePair, double, double, char ) {
    }

void setDrawColor( float inR, float inG, float inB, float inA ) {
    }




#include "../gameSource/spriteBank.h"
SpriteRecord *getSpriteRecord( int inSpriteID ) {
    return NULL;
    }



char getSpriteHit( int inID, int inXCenterOffset, int inYCenterOffset ) {
    return false;
    }


char getUsesMultiplicativeBlending( int inID ) {
    return false;
    }


void toggleMultiplicativeBlend( char inMultiplicative ) {
    }

