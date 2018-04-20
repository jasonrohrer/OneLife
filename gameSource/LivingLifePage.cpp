#include "LivingLifePage.h"

#include "objectBank.h"
#include "spriteBank.h"
#include "transitionBank.h"
#include "categoryBank.h"
#include "soundBank.h"
#include "whiteSprites.h"
#include "message.h"
#include "groundSprites.h"

#include "accountHmac.h"

#include "liveObjectSet.h"
#include "ageControl.h"
#include "musicPlayer.h"

#include "liveAnimationTriggers.h"

#include "../commonSource/fractalNoise.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/MinPriorityQueue.h"


#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/io/file/File.h"

#include "minorGems/formats/encodingUtils.h"

#include "minorGems/system/Thread.h"

#include "minorGems/util/log/AppLog.h"

#include "minorGems/crypto/hashes/sha1.h"

#include <stdlib.h>//#include <math.h>


#define MAP_D 64
#define MAP_NUM_CELLS 4096

extern int versionNumber;
extern int dataVersionNumber;

extern double frameRateFactor;

extern Font *mainFont;
extern Font *mainFontReview;
extern Font *handwritingFont;
extern Font *pencilFont;
extern Font *pencilErasedFont;


// to make all erased pencil fonts lighter
static float pencilErasedFontExtraFade = 0.75;


extern doublePair lastScreenViewCenter;

extern double viewWidth;
extern double viewHeight;

extern int screenW, screenH;

extern char *serverIP;
extern int serverPort;

extern char *userEmail;


extern float musicLoudness;


static JenkinsRandomSource randSource( 340403 );
static JenkinsRandomSource remapRandSource( 340403 );


static int lastScreenMouseX, lastScreenMouseY;
static char mouseDown = false;
static int mouseDownFrames = 0;

static int minMouseDownFrames = 30;


static int screenCenterPlayerOffsetX, screenCenterPlayerOffsetY;


static float lastMouseX = 0;
static float lastMouseY = 0;


// set to true to render for teaser video
static char teaserVideo = false;





// most recent home at end

typedef struct {
        GridPos pos;
        char ancient;
    } HomePos;

    

static SimpleVector<HomePos> homePosStack;


// returns pointer to record, NOT destroyed by caller, or NULL if 
// home unknown
static  GridPos *getHomeLocation() {
    int num = homePosStack.size();
    if( num > 0 ) {
        return &( homePosStack.getElement( num - 1 )->pos );
        }
    else {
        return NULL;
        }
    }



static void removeHomeLocation( int inX, int inY ) {
    for( int i=0; i<homePosStack.size(); i++ ) {
        GridPos p = homePosStack.getElementDirect( i ).pos;
        
        if( p.x == inX && p.y == inY ) {
            homePosStack.deleteElement( i );
            break;
            }
        }
    }



static void addHomeLocation( int inX, int inY ) {
    removeHomeLocation( inX, inY );
    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = false;
    
    homePosStack.push_back( p );
    }


static void addAncientHomeLocation( int inX, int inY ) {
    removeHomeLocation( inX, inY );

    // remove all ancient pos
    // there can be only one ancient
    for( int i=0; i<homePosStack.size(); i++ ) {
        if( homePosStack.getElementDirect( i ).ancient ) {
            homePosStack.deleteElement( i );
            i--;
            }
        }

    GridPos newPos = { inX, inY };
    HomePos p;
    p.pos = newPos;
    p.ancient = true;
    
    homePosStack.push_front( p );
    }






// returns if -1 no home needs to be shown (home unknown)
// otherwise, returns 0..7 index of arrow
static int getHomeDir( doublePair inCurrentPlayerPos, 
                       char *outTooClose = NULL ) {
    GridPos *p = getHomeLocation();
    
    if( p == NULL ) {
        return -1;
        }
    
    
    if( outTooClose != NULL ) {
        *outTooClose = false;
        }
    

    doublePair homePos = { (double)p->x, (double)p->y };
    
    doublePair vector = sub( homePos, inCurrentPlayerPos );

    double dist = length( vector );

    if( dist < 5 ) {
        // too close

        if( outTooClose != NULL ) {
            *outTooClose = true;
            }
        
        if( dist == 0 ) {
            // can't compute angle
            return -1;
            }
        }
    
    
    double a = angle( vector );

    // north is 0
    a -= M_PI / 2; 

    
    if( a <  - M_PI / 8 ) {
        a += 2 * M_PI;
        }
    
    int index = lrint( 8 * a / ( 2 * M_PI ) );
    
    return index;
    }



char *getRelationName( LiveObject *inOurObject, LiveObject *inTheirObject ) {
    SimpleVector<int> ourLin = inOurObject->lineage;
    SimpleVector<int> theirLin = inTheirObject->lineage;
    
    int ourID = inOurObject->id;
    int theirID = inTheirObject->id;

    char theyMale = getObject( inTheirObject->displayID )->male;
    

    if( ourLin.size() == 0 && theirLin.size() == 0 ) {
        // both eve, no relation
        return NULL;
        }

    const char *main = "";
    char grand = false;
    int numGreats = 0;
    int cousinNum = 0;
    int cousinRemovedNum = 0;
    
    char found = false;

    for( int i=0; i<theirLin.size(); i++ ) {
        if( theirLin.getElementDirect( i ) == ourID ) {
            found = true;
            
            if( theyMale ) {
                main = translate( "son" );
                }
            else {
                main = translate( "daughter" );
                }
            if( i > 0  ) {
                grand = true;
                }
            numGreats = i - 1;
            }
        }

    if( ! found ) {
        for( int i=0; i<ourLin.size(); i++ ) {
            if( ourLin.getElementDirect( i ) == theirID ) {
                found = true;
                main = translate( "mother" );
                if( i > 0  ) {
                    grand = true;
                    }
                numGreats = i - 1;
                }
            }
        }
    
    if( ! found ) {
        // not a direct descendent or ancestor

        // look for shared relation
        int ourMatchIndex = -1;
        int theirMatchIndex = -1;
        
        for( int i=0; i<ourLin.size(); i++ ) {
            for( int j=0; j<theirLin.size(); j++ ) {
                
                if( ourLin.getElementDirect( i ) == 
                    theirLin.getElementDirect( j ) ) {
                    ourMatchIndex = i;
                    theirMatchIndex = j;
                    break;
                    }
                }
            if( ourMatchIndex != -1 ) {
                break;
                }
            }
        
        if( ourMatchIndex == -1 ) {
            return NULL;
            }
        
        found = true;

        if( theirMatchIndex == 0 && ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "brother" );
                }
            else {
                main = translate( "sister" );
                }
            }
        else if( theirMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "uncle" );
                }
            else {
                main = translate( "aunt" );
                }
            numGreats = ourMatchIndex - 1;
            }
        else if( ourMatchIndex == 0 ) {
            if( theyMale ) {
                main = translate( "nephew" );
                }
            else {
                main = translate( "niece" );
                }
            numGreats = theirMatchIndex - 1;
            }
        else {
            // cousin of some kind
            
            main = translate( "cousin" );
            
            // shallowest determines cousin number
            // diff determines removed number
            if( ourMatchIndex <= theirMatchIndex ) {
                cousinNum = ourMatchIndex;
                cousinRemovedNum = theirMatchIndex - ourMatchIndex;
                }
            else {
                cousinNum = theirMatchIndex;
                cousinRemovedNum = ourMatchIndex - theirMatchIndex;
                }
            }
        }


    SimpleVector<char> buffer;
    
    buffer.appendElementString( translate( "your" ) );
    buffer.appendElementString( " " );

    for( int i=0; i<numGreats; i++ ) {
        buffer.appendElementString( translate( "great" ) );
        }
    if( grand ) {
        buffer.appendElementString( translate( "grand" ) );
        }
    
    if( cousinNum > 0 ) {
        int remainingCousinNum = cousinNum;

        if( cousinNum >= 30 ) {
            buffer.appendElementString( translate( "distant" ) );
            remainingCousinNum = 0;
            }
        
        if( cousinNum > 20 && cousinNum < 30 ) {
            buffer.appendElementString( translate( "twenty" ) );
            remainingCousinNum = cousinNum - 20;
            }

        if( remainingCousinNum > 0  ) {
            char *numth = autoSprintf( "%dth", remainingCousinNum );
            buffer.appendElementString( translate( numth ) );
            delete [] numth;
            }
        buffer.appendElementString( " " );
        }
    
    buffer.appendElementString( main );
    
    if( cousinRemovedNum > 0 ) {
        buffer.appendElementString( " " );
        
        if( cousinRemovedNum > 9 ) {
            buffer.appendElementString( translate( "manyTimes" ) );
            }
        else {
            char *numTimes = autoSprintf( "%dTimes", cousinRemovedNum );
            buffer.appendElementString( translate( numTimes ) );
            delete [] numTimes;
            }
        buffer.appendElementString( " " );
        buffer.appendElementString( translate( "removed" ) );
        }

    return buffer.getElementString();
    }








// base speed for animations that aren't speeded up or slowed down
// when player moving at a different speed, anim speed is modified
#define BASE_SPEED 3.75


int numServerBytesRead = 0;
int numServerBytesSent = 0;

int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;


static char hideGuiPanel = false;




static char *lastMessageSentToServer = NULL;


// destroyed internally if not NULL
static void replaceLastMessageSent( char *inNewMessage ) {
    if( lastMessageSentToServer != NULL ) {
        delete [] lastMessageSentToServer;
        }
    lastMessageSentToServer = inNewMessage;
    }




SimpleVector<unsigned char> serverSocketBuffer;


// reads all waiting data from socket and stores it in buffer
// returns false on socket error
static char readServerSocketFull( int inServerSocket ) {

    unsigned char buffer[512];
    
    int numRead = readFromSocket( inServerSocket, buffer, 512 );
    
    
    while( numRead > 0 ) {
        serverSocketBuffer.appendArray( buffer, numRead );
        numServerBytesRead += numRead;

        numRead = readFromSocket( inServerSocket, buffer, 512 );
        }    

    if( numRead == -1 ) {
        printf( "Failed to read from server socket at time %f\n",
                game_getCurrentTime() );
        return false;
        }
    
    return true;
    }



static double timeLastMessageSent = 0;



void LivingLifePage::sendToServerSocket( char *inMessage ) {
    timeLastMessageSent = game_getCurrentTime();
    
    printf( "Sending message to server: %s\n", inMessage );
    
    replaceLastMessageSent( stringDuplicate( inMessage ) );    

    int len = strlen( inMessage );
    
    int numSent = sendToSocket( mServerSocket, (unsigned char*)inMessage, len );
    
    if( numSent == len ) {
        numServerBytesSent += len;
        overheadServerBytesSent += 52;
        }
    else {
        printf( "Failed to send message to server socket "
                "at time %f "
                "(tried to send %d, but numSent=%d)\n", 
                game_getCurrentTime(), len, numSent );
        closeSocket( mServerSocket );
        mServerSocket = -1;

        if( mFirstServerMessagesReceived  ) {
            
            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath();
            }
        else {
            setSignal( "loginFailed" );
            }
        }
    
    }




static char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
    }


static double distance2( GridPos inA, GridPos inB ) {
    int dX = inA.x - inB.x;
    int dY = inA.y - inB.y;
    
    return dX * dX + dY * dY;
    }



static void printPath( GridPos *inPath, int inLength ) {
    for( int i=0; i<inLength; i++ ) {
        printf( "(%d,%d) ", inPath[i].x, inPath[i].y );
        }
    printf( "\n" );
    }


static void removeDoubleBacksFromPath( GridPos **inPath, int *inLength ) {
    
    SimpleVector<GridPos> filteredPath;
    
    int dbA = -1;
    int dbB = -1;
    
    int longestDB = 0;

    GridPos *path = *inPath;
    int length = *inLength;

    for( int e=0; e<length; e++ ) {
                                    
        for( int f=e; f<length; f++ ) {
            
            if( equal( path[e],
                       path[f] ) ) {
                                            
                int dist = f - e;
                                            
                if( dist > longestDB ) {
                                                
                    dbA = e;
                    dbB = f;
                    longestDB = dist;
                    }
                }
            }
        }
                                
    if( longestDB > 0 ) {
                                    
        printf( "Removing loop with %d elements\n",
                longestDB );

        for( int e=0; e<=dbA; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        // skip loop between

        for( int e=dbB + 1; e<length; e++ ) {
            filteredPath.push_back( 
                path[e] );
            }
                                    
        *inLength = filteredPath.size();
                                    
        delete [] path;
                                    
        *inPath = 
            filteredPath.getElementArray();
        }
    }





static double computeCurrentAge( LiveObject *inObj ) {
    if( inObj->finalAgeSet ) {
        return inObj->age;
        }
    else {
        if( inObj->tempAgeOverrideSet ) {
            double curTime = game_getCurrentTime();
            
            if( curTime - inObj->tempAgeOverrideSetTime < 5 ) {
                // baby cries for 5 seconds each time they speak
            
                // update age using clock
                return inObj->tempAgeOverride + 
                    inObj->ageRate * 
                    ( curTime - inObj->tempAgeOverrideSetTime );
                }
            else {
                // temp override over
                inObj->tempAgeOverrideSet = false;
                }
            }
        
        // update age using clock
        return inObj->age + 
            inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
        }
    
    }




static void stripDescriptionComment( char *inString ) {
    // pound sign is used for trailing developer comments
    // that aren't show to end user, cut them off if they exist
    char *firstPound = strstr( inString, "#" );
            
    if( firstPound != NULL ) {
        firstPound[0] = '\0';
        }
    }



typedef enum messageType {
    SHUTDOWN,
    SERVER_FULL,
	SEQUENCE_NUMBER,
    ACCEPTED,
    REJECTED,
    MAP_CHUNK,
    MAP_CHANGE,
    PLAYER_UPDATE,
    PLAYER_MOVES_START,
    PLAYER_OUT_OF_RANGE,
    PLAYER_SAYS,
    FOOD_CHANGE,
    LINEAGE,
    NAMES,
    APOCALYPSE,
    DYING,
    MONUMENT_CALL,
    COMPRESSED_MESSAGE,
    UNKNOWN
    } messageType;



messageType getMessageType( char *inMessage ) {
    char *copy = stringDuplicate( inMessage );
    
    char *firstBreak = strstr( copy, "\n" );
    
    if( firstBreak == NULL ) {
        delete [] copy;
        return UNKNOWN;
        }
    
    firstBreak[0] = '\0';
    
    messageType returnValue = UNKNOWN;

    if( strcmp( copy, "SHUTDOWN" ) == 0 ) {
        returnValue = SHUTDOWN;
        }
    else if( strcmp( copy, "SERVER_FULL" ) == 0 ) {
        returnValue = SERVER_FULL;
        }
    else if( strcmp( copy, "SN" ) == 0 ) {
        returnValue = SEQUENCE_NUMBER;
        }
    else if( strcmp( copy, "ACCEPTED" ) == 0 ) {
        returnValue = ACCEPTED;
        }
    else if( strcmp( copy, "REJECTED" ) == 0 ) {
        returnValue = REJECTED;
        }
    else if( strcmp( copy, "CM" ) == 0 ) {
        returnValue = COMPRESSED_MESSAGE;
        }
    else if( strcmp( copy, "MC" ) == 0 ) {
        returnValue = MAP_CHUNK;
        }
    else if( strcmp( copy, "MX" ) == 0 ) {
        returnValue = MAP_CHANGE;
        }
    else if( strcmp( copy, "PU" ) == 0 ) {
        returnValue = PLAYER_UPDATE;
        }
    else if( strcmp( copy, "PM" ) == 0 ) {
        returnValue = PLAYER_MOVES_START;
        }
    else if( strcmp( copy, "PO" ) == 0 ) {
        returnValue = PLAYER_OUT_OF_RANGE;
        }
    else if( strcmp( copy, "PS" ) == 0 ) {
        returnValue = PLAYER_SAYS;
        }
    else if( strcmp( copy, "FX" ) == 0 ) {
        returnValue = FOOD_CHANGE;
        }
    else if( strcmp( copy, "LN" ) == 0 ) {
        returnValue = LINEAGE;
        }
    else if( strcmp( copy, "NM" ) == 0 ) {
        returnValue = NAMES;
        }
    else if( strcmp( copy, "AP" ) == 0 ) {
        returnValue = APOCALYPSE;
        }
    else if( strcmp( copy, "DY" ) == 0 ) {
        returnValue = DYING;
        }
    else if( strcmp( copy, "MN" ) == 0 ) {
        returnValue = MONUMENT_CALL;
        }
    
    delete [] copy;
    return returnValue;
    }



doublePair getVectorFromCamera( int inMapX, int inMapY ) {
    doublePair vector = 
        { inMapX - lastScreenViewCenter.x / CELL_D, 
          inMapY - lastScreenViewCenter.y / CELL_D };
    
    return vector;
    }

                                 


char *pendingMapChunkMessage = NULL;
int pendingCompressedChunkSize;

char pendingCMData = false;
int pendingCMCompressedSize = 0;
int pendingCMDecompressedSize = 0;


SimpleVector<char*> readyPendingReceivedMessages;


// NULL if there's no full message available
char *getNextServerMessage() {
    
    if( readyPendingReceivedMessages.size() > 0 ) {
        char *message = readyPendingReceivedMessages.getElementDirect( 0 );
        readyPendingReceivedMessages.deleteElement( 0 );
        printf( "Playing a held pending message\n" );
        return message;
        }

    if( pendingMapChunkMessage != NULL ) {
        // wait for full binary data chunk to arrive completely
        // after message before we report that the message is ready

        if( serverSocketBuffer.size() >= pendingCompressedChunkSize ) {
            char *returnMessage = pendingMapChunkMessage;
            pendingMapChunkMessage = NULL;

            return returnMessage;
            }
        else {
            // wait for more data to arrive before saying this MC message
            // is ready
            return NULL;
            }
        }
    
    if( pendingCMData ) {
        if( serverSocketBuffer.size() >= pendingCMCompressedSize ) {
            pendingCMData = false;
            
            unsigned char *compressedData = 
                new unsigned char[ pendingCMCompressedSize ];
            
            for( int i=0; i<pendingCMCompressedSize; i++ ) {
                compressedData[i] = serverSocketBuffer.getElementDirect( i );
                }
            serverSocketBuffer.deleteStartElements( pendingCMCompressedSize );
            
            unsigned char *decompressedMessage =
                zipDecompress( compressedData, 
                               pendingCMCompressedSize,
                               pendingCMDecompressedSize );

            delete [] compressedData;

            if( decompressedMessage == NULL ) {
                printf( "Decompressing CM message failed\n" );
                return NULL;
                }
            else {
                char *textMessage = new char[ pendingCMDecompressedSize + 1 ];
                memcpy( textMessage, decompressedMessage,
                       pendingCMDecompressedSize );
                textMessage[ pendingCMDecompressedSize ] = '\0';
                
                delete [] decompressedMessage;
                
                return textMessage;
                }
            }
        else {
            // wait for more data to arrive
            return NULL;
            }
        }
    


    // find first terminal character #

    int index = serverSocketBuffer.getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    
    char *message = new char[ index + 1 ];
    
    for( int i=0; i<index; i++ ) {
        message[i] = (char)( serverSocketBuffer.getElementDirect( i ) );
        }
    // delete message and terminal character
    serverSocketBuffer.deleteStartElements( index + 1 );
    
    message[ index ] = '\0';

    if( getMessageType( message ) == MAP_CHUNK ) {
        pendingMapChunkMessage = message;
        
        int sizeX, sizeY, x, y, binarySize;
        sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                &sizeX, &sizeY,
                &x, &y, &binarySize, &pendingCompressedChunkSize );


        return getNextServerMessage();
        }
    else if( getMessageType( message ) == COMPRESSED_MESSAGE ) {
        pendingCMData = true;
        
        printf( "Got compressed message header:\n%s\n\n", message );

        sscanf( message, "CM\n%d %d\n", 
                &pendingCMDecompressedSize, &pendingCMCompressedSize );

        delete [] message;
        return NULL;
        }
    else {
        return message;
        }
    }




doublePair gridToDouble( GridPos inGridPos ) {
    doublePair d = { (double) inGridPos.x, (double) inGridPos.y };
    
    return d;
    }





static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }



static GridPos sub( GridPos inA, GridPos inB ) {
    GridPos result = { inA.x - inB.x, inA.y - inB.y };
    
    return result;
    }






// measure a possibly truncated path, compensating for diagonals
static double measurePathLength( LiveObject *inObject,
                                 int inPathLength ) {
    // diags are square root of 2 in length
    double diagLength = 1.4142356237;
    

    double totalLength = 0;
    
    if( inPathLength < 2 ) {
        return totalLength;
        }
    

    GridPos lastPos = inObject->pathToDest[0];
    
    for( int i=1; i<inPathLength; i++ ) {
        
        GridPos thisPos = inObject->pathToDest[i];
        
        if( thisPos.x != lastPos.x &&
            thisPos.y != lastPos.y ) {
            totalLength += diagLength;
            }
        else {
            // not diag
            totalLength += 1;
            }
        lastPos = thisPos;
        }
    
    return totalLength;
    }









SimpleVector<LiveObject> gameObjects;


static LiveObject *getGameObject( int inID ) {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            return o;
            }
        }
    return NULL;
    }



extern char autoAdjustFramerate;
extern int baseFramesPerSecond;


void updateMoveSpeed( LiveObject *inObject ) {
    double etaSec = inObject->moveEtaTime - game_getCurrentTime();
    
    double moveLeft = measurePathLength( inObject, inObject->pathLength ) -
        measurePathLength( inObject, inObject->currentPathStep + 1 );
    

    // count number of turns, which we execute faster than we should
    // because of path smoothing,
    // and use them to reduce overall speed to compensate
    int numTurns = 0;

    if( inObject->currentPathStep < inObject->pathLength - 1 ) {
        GridPos lastDir = sub( 
            inObject->pathToDest[inObject->currentPathStep + 1],
            inObject->pathToDest[inObject->currentPathStep] );
        
        for( int p=inObject->currentPathStep+1; 
             p<inObject->pathLength -1; p++ ) {
            
            GridPos dir = sub( 
                inObject->pathToDest[p+1],
                inObject->pathToDest[p] );

            if( !equal( dir, lastDir ) ) {
                numTurns++;
                lastDir = dir;
                }
            }

        }
    
    double turnTimeBoost = 0.08 * numTurns;

    etaSec += turnTimeBoost;
    
    if( etaSec < 0.1 ) {
        // less than 1/10 of a second
        // this includes 0 values and negative values
        // we DO NOT want infinite or negative move speeds

        printf( "updateMoveSpeed sees etaSec of %f, too low, "
                "upping to 0.1 sec\n", etaSec );
        
        etaSec = 0.1;
        }
    

    double speedPerSec = moveLeft / etaSec;


    // pretend that frame rate is constant
    double fps = baseFramesPerSecond / frameRateFactor;
    
    inObject->currentSpeed = speedPerSec / fps;
    printf( "fixed speed = %f\n", inObject->currentSpeed );
    
    inObject->currentGridSpeed = speedPerSec;
    
    // slow move speed for testing
    //inObject->currentSpeed *= 0.25;

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }


// should match limit on server
static int pathFindingD = 32;


void LivingLifePage::computePathToDest( LiveObject *inObject ) {
    
    GridPos start;
    start.x = lrint( inObject->currentPos.x );
    start.y = lrint( inObject->currentPos.y );

    // make sure start is on our last path, if we have one
    if( inObject->pathToDest != NULL ) {
        char startOnPath = false;

        for( int i=0; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x == start.x &&
                inObject->pathToDest[i].y == start.y ) {
                startOnPath = true;
                break;
                }
            }
        
        if( ! startOnPath ) {
            // find closest spot on old path
            int closestI = -1;
            int closestD2 = 9999999;
            
            for( int i=0; i<inObject->pathLength; i++ ) {
                int dx = inObject->pathToDest[i].x - start.x;
                int dy = inObject->pathToDest[i].y - start.y;
                
                int d2 = dx * dx + dy * dy;
                
                if( d2 < closestD2 ) {
                    closestD2 = d2;
                    closestI = i;
                    }
                }

            start = inObject->pathToDest[ closestI ];
            }
        }
    
                
            

    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        inObject->pathToDest = NULL;
        }


    GridPos end = { inObject->xd, inObject->yd };
        
    // window around player's start position
    int numPathMapCells = pathFindingD * pathFindingD;
    char *blockedMap = new char[ numPathMapCells ];

    // assume all blocked
    memset( blockedMap, true, numPathMapCells );

    int pathOffsetX = pathFindingD/2 - start.x;
    int pathOffsetY = pathFindingD/2 - start.y;


    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
            
                // note that unknowns (-1) count as blocked too
                if( mMap[ mapI ] == 0
                    ||
                    ( mMap[ mapI ] != -1 && 
                      ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                    
                    blockedMap[ y * pathFindingD + x ] = false;
                    }
                }
            }
        }

    // now add extra blocked spots for wide objects
    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
            if( mapY >= 0 && mapY < mMapD &&
                mapX >= 0 && mapX < mMapD ) { 

                int mapI = mapY * mMapD + mapX;
                
                if( mMap[ mapI ] > 0 ) {
                    ObjectRecord *o = getObject( mMap[ mapI ] );
                    
                    if( o->wide ) {
                        
                        for( int dx = - o->leftBlockingRadius;
                             dx <= o->rightBlockingRadius; dx++ ) {
                            
                            int newX = x + dx;
                            
                            if( newX >=0 && newX < pathFindingD ) {
                                blockedMap[ y * pathFindingD + newX ] = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    
    
    start.x += pathOffsetX;
    start.y += pathOffsetY;
    
    end.x += pathOffsetX;
    end.y += pathOffsetY;
    
    double startTime = game_getCurrentTime();

    GridPos closestFound;
    
    char pathFound = false;
    
    if( inObject->useWaypoint ) {
        GridPos waypoint = { inObject->waypointX, inObject->waypointY };
        waypoint.x += pathOffsetX;
        waypoint.y += pathOffsetY;
        
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, waypoint, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        if( pathFound && inObject->pathToDest != NULL &&
            inObject->pathLength > inObject->maxWaypointPathLength ) {
            
            // path through waypoint too long, use waypoint as dest
            // instead
            delete [] inObject->pathToDest;
            pathFound = pathFind( pathFindingD, pathFindingD,
                                  blockedMap, 
                                  start, waypoint, 
                                  &( inObject->pathLength ),
                                  &( inObject->pathToDest ),
                                  &closestFound );
            inObject->xd = inObject->waypointX;
            inObject->yd = inObject->waypointY;
            }
        }
    else {
        pathFound = pathFind( pathFindingD, pathFindingD,
                              blockedMap, 
                              start, end, 
                              &( inObject->pathLength ),
                              &( inObject->pathToDest ),
                              &closestFound );
        }
        

    if( pathFound && inObject->pathToDest != NULL ) {
        printf( "Path found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );

        // move into world coordinates
        for( int i=0; i<inObject->pathLength; i++ ) {
            inObject->pathToDest[i].x -= pathOffsetX;
            inObject->pathToDest[i].y -= pathOffsetY;
            }

        inObject->shouldDrawPathMarks = false;
        
        // up, down, left, right
        int dirsInPath[4] = { 0, 0, 0, 0 };
        
        for( int i=1; i<inObject->pathLength; i++ ) {
            if( inObject->pathToDest[i].x > inObject->pathToDest[i-1].x ) {
                dirsInPath[3]++;
                }
            if( inObject->pathToDest[i].x < inObject->pathToDest[i-1].x ) {
                dirsInPath[2]++;
                }
            if( inObject->pathToDest[i].y > inObject->pathToDest[i-1].y ) {
                dirsInPath[1]++;
                }
            if( inObject->pathToDest[i].y < inObject->pathToDest[i-1].y ) {
                dirsInPath[0]++;
                }
            }
        
        if( ( dirsInPath[0] > 1 && dirsInPath[1] > 1 )
            ||
            ( dirsInPath[2] > 1 && dirsInPath[3] > 1 ) ) {

            // path contains switchbacks, making in confusing without
            // path marks
            inObject->shouldDrawPathMarks = true;
            }
        
        GridPos aGridPos = inObject->pathToDest[0];
        GridPos bGridPos = inObject->pathToDest[1];
        
        doublePair aPos = { (double)aGridPos.x, (double)aGridPos.y };
        doublePair bPos = { (double)bGridPos.x, (double)bGridPos.y };
        
        inObject->currentMoveDirection =
            normalize( sub( bPos, aPos ) );
        }
    else {
        printf( "Path not found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );
        
        if( !pathFound ) {
            
            inObject->closestDestIfPathFailedX = 
                closestFound.x - pathOffsetX;
            
            inObject->closestDestIfPathFailedY = 
                closestFound.y - pathOffsetY;
            }
        else {
            // degen case where start == end?
            inObject->closestDestIfPathFailedX = inObject->xd;
            inObject->closestDestIfPathFailedY = inObject->yd;
            }
        
        
        }
    
    inObject->currentPathStep = 0;
    inObject->numFramesOnCurrentStep = 0;
    inObject->onFinalPathStep = false;
    
    delete [] blockedMap;
    }



static void addNewAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastAnim = inObject->curAnim;
    inObject->curAnim = inNewAnim;
    inObject->lastAnimFade = 1;
            
    inObject->lastAnimationFrameCount = inObject->animationFrameCount;

    if( inObject->lastAnim == moving ) {
        inObject->frozenRotFrameCount = inObject->lastAnimationFrameCount;
        inObject->frozenRotFrameCountUsed = false;
        }
    else if( inObject->curAnim == moving &&
             inObject->lastAnim != moving &&
             inObject->frozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->animationFrameCount = inObject->frozenRotFrameCount;
        }
    else if( ( inObject->curAnim == ground || inObject->curAnim == ground2 )
             &&
             inObject->lastAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }



static void addNewHeldAnimDirect( LiveObject *inObject, AnimType inNewAnim ) {
    inObject->lastHeldAnim = inObject->curHeldAnim;
    inObject->curHeldAnim = inNewAnim;
    inObject->lastHeldAnimFade = 1;
    
    inObject->lastHeldAnimationFrameCount = 
        inObject->heldAnimationFrameCount;

    if( inObject->lastHeldAnim == moving ) {
        inObject->heldFrozenRotFrameCount = 
            inObject->lastHeldAnimationFrameCount;
        inObject->heldFrozenRotFrameCountUsed = false;
        }
    else if( inObject->curHeldAnim == moving &&
             inObject->lastHeldAnim != moving &&
             inObject->heldFrozenRotFrameCountUsed ) {
        // switching back to moving
        // resume from where frozen
        inObject->heldAnimationFrameCount = inObject->heldFrozenRotFrameCount;
        }
    else if( ( inObject->curHeldAnim == ground || 
               inObject->curHeldAnim == ground2 ) 
             &&
             inObject->lastHeldAnim == held ) {
        // keep old frozen frame count as we transition away
        // from held
        }
    }


static void addNewAnimPlayerOnly( LiveObject *inObject, AnimType inNewAnim ) {
    
    if( inObject->curAnim != inNewAnim || 
        inObject->futureAnimStack->size() > 0 ) {
        

        // if we're still in the middle of fading, finish the fade,
        // by pushing this animation on the stack...
        // ...but NOT if we're fading TO ground.
        // Cut that off, and start our next animation right away.
        if( inObject->lastAnimFade != 0 && 
            inObject->curAnim != ground &&
            inObject->curAnim != ground2 ) {
                        
            // don't double stack
            if( inObject->futureAnimStack->size() == 0 ||
                inObject->futureAnimStack->getElementDirect(
                    inObject->futureAnimStack->size() - 1 ) 
                != inNewAnim ) {
                
                inObject->futureAnimStack->push_back( inNewAnim );
                }
            }
        else {
            addNewAnimDirect( inObject, inNewAnim );
            }
        }
    }


static void addNewAnim( LiveObject *inObject, AnimType inNewAnim ) {
    addNewAnimPlayerOnly( inObject, inNewAnim );

    AnimType newHeldAnim = inNewAnim;
    
    if( inObject->holdingID < 0 ) {
        // holding a baby
        // never show baby's moving animation
        // baby always stuck in held animation when being held

        newHeldAnim = held;
        }
    else if( inObject->holdingID > 0 && 
             ( newHeldAnim == ground || newHeldAnim == ground2 || 
               newHeldAnim == doing || newHeldAnim == eating ) ) {
        // ground is used when person comes to a hault,
        // but for the held item, we should still show the held animation
        // same if person is starting a doing or eating animation
        newHeldAnim = held;
        }

    if( inObject->curHeldAnim != newHeldAnim ) {

        if( inObject->lastHeldAnimFade != 0 ) {
            
            // don't double stack
            if( inObject->futureHeldAnimStack->size() == 0 ||
                inObject->futureHeldAnimStack->getElementDirect(
                    inObject->futureHeldAnimStack->size() - 1 ) 
                != newHeldAnim ) {
                
                inObject->futureHeldAnimStack->push_back( newHeldAnim );
                }
            }
        else {
            addNewHeldAnimDirect( inObject, newHeldAnim );
            }
        }    
        
    }






// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;
static char nextActionEating = false;
static char nextActionDropping = false;


// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static int playerActionTargetX, playerActionTargetY;
static char playerActionTargetNotAdjacent = false;


int ourID;

char lastCharUsed = 'A';

char mapPullMode = false;
int mapPullStartX = -10;
int mapPullEndX = 10;
int mapPullStartY = -10;
int mapPullEndY = 10;

int mapPullCurrentX;
int mapPullCurrentY;
char mapPullCurrentSaved = false;
char mapPullCurrentSent = false;
char mapPullModeFinalImage = false;

Image *mapPullTotalImage = NULL;
int numScreensWritten = 0;


static int apocalypseInProgress = false;
static double apocalypseDisplayProgress = 0;
static double apocalypseDisplaySeconds = 6;

static double remapPeakSeconds = 60;
static double remapDelaySeconds = 30;


static Image *expandToPowersOfTwoWhite( Image *inImage ) {
    
    int w = 1;
    int h = 1;
                    
    while( w < inImage->getWidth() ) {
        w *= 2;
        }
    while( h < inImage->getHeight() ) {
        h *= 2;
        }
    
    return inImage->expandImage( w, h, true );
    }



static void splitAndExpandSprites( const char *inTgaFileName, int inNumSprites,
                                   SpriteHandle *inDestArray ) {
    
    Image *full = readTGAFile( inTgaFileName );
    if( full != NULL ) {
        
        int spriteW = full->getWidth() / inNumSprites;
        
        int spriteH = full->getHeight();
        
        for( int i=0; i<inNumSprites; i++ ) {
            
            Image *part = full->getSubImage( i * spriteW, 0, 
                                          spriteW, spriteH );
            Image *partExpanded = expandToPowersOfTwoWhite( part );
            
            
            delete part;

            inDestArray[i] = fillSprite( partExpanded, false );
            
            delete partExpanded;
            }

        delete full;
        }

    }



void LivingLifePage::clearMap() {
    for( int i=0; i<mMapD *mMapD; i++ ) {
        // -1 represents unknown
        // 0 represents known empty
        mMap[i] = -1;
        mMapBiomes[i] = -1;
        mMapFloors[i] = -1;
        
        mMapAnimationFrameCount[i] = randSource.getRandomBoundedInt( 0, 10000 );
        mMapAnimationLastFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );
        
        mMapAnimationFrozenRotFrameCount[i] = 0;

        mMapFloorAnimationFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );

        mMapCurAnimType[i] = ground;
        mMapLastAnimType[i] = ground;
        mMapLastAnimFade[i] = 0;

        mMapDropOffsets[i].x = 0;
        mMapDropOffsets[i].y = 0;
        mMapDropRot[i] = 0;
        mMapDropSounds[i] = blankSoundUsage;

        mMapMoveOffsets[i].x = 0;
        mMapMoveOffsets[i].y = 0;
        mMapMoveSpeeds[i] = 0;
        
        mMapTileFlips[i] = false;
        
        mMapPlayerPlacedFlags[i] = false;
        }
    }



LivingLifePage::LivingLifePage() 
        : mServerSocket( -1 ), 
          mFirstServerMessagesReceived( 0 ),
          mMapGlobalOffsetSet( false ),
          mMapD( MAP_D ),
          mMapOffsetX( 0 ),
          mMapOffsetY( 0 ),
          mEKeyEnabled( false ),
          mEKeyDown( false ),
          mGuiPanelSprite( loadSprite( "guiPanel.tga", false ) ),
          mGuiBloodSprite( loadSprite( "guiBlood.tga", false ) ),
          mNotePaperSprite( loadSprite( "notePaper.tga", false ) ),
          mFloorSplitSprite( loadSprite( "floorSplit.tga", false ) ),
          mCellBorderSprite( loadWhiteSprite( "cellBorder.tga" ) ),
          mCellFillSprite( loadWhiteSprite( "cellFill.tga" ) ),
          mHomeSlipSprite( loadSprite( "homeSlip.tga", false ) ),
          mLastMouseOverID( 0 ),
          mCurMouseOverID( 0 ),
          mChalkBlotSprite( loadWhiteSprite( "chalkBlot.tga" ) ),
          mPathMarkSprite( loadWhiteSprite( "pathMark.tga" ) ),
          mSayField( handwritingFont, 0, 1000, 10, true, NULL,
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! " ),
          mDeathReason( NULL ),
          mShowHighlights( true ) {
    
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0.0f;
    mCurMouseOverCellFadeRate = 0.04;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;

    // we're not showing a cursor on note paper, so arrow key behavior
    // is confusing.
    mSayField.setIgnoreArrowKeys( true );

    initLiveTriggers();

    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }
    

    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;
          
    hideGuiPanel = SettingsManager::getIntSetting( "hideGameUI", 0 );

    mHungerSound = loadSoundSprite( "otherSounds", "hunger.aiff" );
    mPulseHungerSound = false;
    

    if( mHungerSound != NULL ) {
        toggleVariance( mHungerSound, true );
        }
    

    mHungerSlipSprites[0] = loadSprite( "fullSlip.tga", false );
    mHungerSlipSprites[1] = loadSprite( "hungrySlip.tga", false );
    mHungerSlipSprites[2] = loadSprite( "starvingSlip.tga", false );
    

    // not visible, drawn under world at 0, 0, and doesn't move with camera
    // still, we can use it to receive/process/filter typing events
    addComponent( &mSayField );
    
    mSayField.unfocus();
    
    
    mNotePaperHideOffset.x = -242;
    mNotePaperHideOffset.y = -420;


    mHomeSlipHideOffset.x = 0;
    mHomeSlipHideOffset.y = -360;


    for( int i=0; i<3; i++ ) {    
        mHungerSlipShowOffsets[i].x = -540;
        mHungerSlipShowOffsets[i].y = -250;
    
        mHungerSlipHideOffsets[i].x = -540;
        mHungerSlipHideOffsets[i].y = -370;
        
        mHungerSlipWiggleTime[i] = 0;
        mHungerSlipWiggleAmp[i] = 0;
        mHungerSlipWiggleSpeed[i] = 0.05;
        }
    mHungerSlipShowOffsets[2].y += 20;
    mHungerSlipHideOffsets[2].y -= 20;

    mHungerSlipShowOffsets[2].y -= 50;
    mHungerSlipShowOffsets[1].y -= 30;
    mHungerSlipShowOffsets[0].y += 18;


    mHungerSlipWiggleAmp[1] = 0.5;
    mHungerSlipWiggleAmp[2] = 0.5;

    mHungerSlipWiggleSpeed[2] = 0.075;

    mStarvingSlipLastPos[0] = 0;
    mStarvingSlipLastPos[1] = 0;
    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        }
    
    mHungerSlipVisible = -1;

    
    
    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        char *name = autoSprintf( "hintSheet%d.tga", i + 1 );    
        mHintSheetSprites[i] = loadSprite( name, false );
        delete [] name;
        
        mHintHideOffset[i].x = 900;
        mHintHideOffset[i].y = -370;
        
        mHintTargetOffset[i] = mHintHideOffset[i];
        mHintPosOffset[i] = mHintHideOffset[i];
        
        mHintExtraOffset[i].x = 0;
        mHintExtraOffset[i].y = 0;

        mHintMessage[i] = NULL;
        mHintMessageIndex[i] = 0;
        
        mNumTotalHints[i] = 0;
        }
    
    mLiveHintSheetIndex = -1;

    mCurrentHintObjectID = 0;
    mCurrentHintIndex = 0;
    
    mNextHintObjectID = 0;
    mNextHintIndex = 0;
    
    mLastHintSortedSourceID = 0;
    
    int maxObjectID = getMaxObjectID();
    
    mHintBookmarks = new int[ maxObjectID + 1 ];

    for( int i=0; i<=maxObjectID; i++ ) {
        mHintBookmarks[i] = 0;
        }
    


    mMap = new int[ mMapD * mMapD ];
    mMapBiomes = new int[ mMapD * mMapD ];
    mMapFloors = new int[ mMapD * mMapD ];
    
    mMapCellDrawnFlags = new char[ mMapD * mMapD ];

    mMapContainedStacks = new SimpleVector<int>[ mMapD * mMapD ];
    mMapSubContainedStacks = 
        new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];
    
    mMapAnimationFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationLastFrameCount =  new double[ mMapD * mMapD ];
    mMapAnimationFrozenRotFrameCount =  new double[ mMapD * mMapD ];
    
    mMapFloorAnimationFrameCount =  new int[ mMapD * mMapD ];

    mMapCurAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimFade =  new double[ mMapD * mMapD ];
    
    mMapDropOffsets = new doublePair[ mMapD * mMapD ];
    mMapDropRot = new double[ mMapD * mMapD ];
    mMapDropSounds = new SoundUsage[ mMapD * mMapD ];

    mMapMoveOffsets = new doublePair[ mMapD * mMapD ];
    mMapMoveSpeeds = new double[ mMapD * mMapD ];

    mMapTileFlips = new char[ mMapD * mMapD ];
    
    mMapPlayerPlacedFlags = new char[ mMapD * mMapD ];
    

    clearMap();


    splitAndExpandSprites( "hungerBoxes.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxSprites );
    splitAndExpandSprites( "hungerBoxFills.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillSprites );

    splitAndExpandSprites( "hungerBoxesErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxErasedSprites );
    splitAndExpandSprites( "hungerBoxFillsErased.tga", NUM_HUNGER_BOX_SPRITES, 
                           mHungerBoxFillErasedSprites );

    splitAndExpandSprites( "tempArrows.tga", NUM_TEMP_ARROWS, 
                           mTempArrowSprites );
    splitAndExpandSprites( "tempArrowsErased.tga", NUM_TEMP_ARROWS, 
                           mTempArrowErasedSprites );

    splitAndExpandSprites( "hungerDashes.tga", NUM_HUNGER_DASHES, 
                           mHungerDashSprites );
    splitAndExpandSprites( "hungerDashesErased.tga", NUM_HUNGER_DASHES, 
                           mHungerDashErasedSprites );

    splitAndExpandSprites( "hungerBars.tga", NUM_HUNGER_DASHES, 
                           mHungerBarSprites );
    splitAndExpandSprites( "hungerBarsErased.tga", NUM_HUNGER_DASHES, 
                           mHungerBarErasedSprites );

    
    splitAndExpandSprites( "homeArrows.tga", NUM_HOME_ARROWS, 
                           mHomeArrowSprites );
    splitAndExpandSprites( "homeArrowsErased.tga", NUM_HOME_ARROWS, 
                           mHomeArrowErasedSprites );


    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    mCurrentDes = NULL;
    mCurrentLastAteString = NULL;

    mShowHighlights = 
        SettingsManager::getIntSetting( "showMouseOverHighlights", 1 );

    mEKeyEnabled = 
        SettingsManager::getIntSetting( "eKeyForRightClick", 0 );


    if( teaserVideo ) {
        mTeaserArrowLongSprite = loadWhiteSprite( "teaserArrowLong.tga" );
        mTeaserArrowMedSprite = loadWhiteSprite( "teaserArrowMed.tga" );
        mTeaserArrowShortSprite = loadWhiteSprite( "teaserArrowShort.tga" );
        mTeaserArrowVeryShortSprite = 
            loadWhiteSprite( "teaserArrowVeryShort.tga" );
        
        mLineSegmentSprite = loadWhiteSprite( "lineSegment.tga" );
        }
          

    }




void LivingLifePage::clearLiveObjects() {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *nextObject =
            gameObjects.getElement( i );
        
        nextObject->pendingReceivedMessages.deallocateStringElements();

        if( nextObject->containedIDs != NULL ) {
            delete [] nextObject->containedIDs;
            }

        if( nextObject->subContainedIDs != NULL ) {
            delete [] nextObject->subContainedIDs;
            }
        
        if( nextObject->pathToDest != NULL ) {
            delete [] nextObject->pathToDest;
            }

        if( nextObject->currentSpeech != NULL ) {
            delete [] nextObject->currentSpeech;
            }

        if( nextObject->relationName != NULL ) {
            delete [] nextObject->relationName;
            }

        if( nextObject->name != NULL ) {
            delete [] nextObject->name;
            }

        delete nextObject->futureAnimStack;
        delete nextObject->futureHeldAnimStack;
        }
    
    gameObjects.deleteAll();
    }




LivingLifePage::~LivingLifePage() {
    printf( "Total received = %d bytes (+%d in headers), "
            "total sent = %d bytes (+%d in headers)\n",
            numServerBytesRead, overheadServerBytesRead,
            numServerBytesSent, overheadServerBytesSent );
    
    freeLiveTriggers();

    readyPendingReceivedMessages.deallocateStringElements();
    
    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    

    clearLiveObjects();

    mOldDesStrings.deallocateStringElements();
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }

    mOldLastAteStrings.deallocateStringElements();
    if( mCurrentLastAteString != NULL ) {
        delete [] mCurrentLastAteString;
        }

    mSentChatPhrases.deallocateStringElements();
    
    if( mServerSocket != -1 ) {
        closeSocket( mServerSocket );
        }

    
    delete [] mMapAnimationFrameCount;
    delete [] mMapAnimationLastFrameCount;
    delete [] mMapAnimationFrozenRotFrameCount;

    delete [] mMapFloorAnimationFrameCount;

    delete [] mMapCurAnimType;
    delete [] mMapLastAnimType;
    delete [] mMapLastAnimFade;
    
    delete [] mMapDropOffsets;
    delete [] mMapDropRot;
    delete [] mMapDropSounds;

    delete [] mMapMoveOffsets;
    delete [] mMapMoveSpeeds;

    delete [] mMapTileFlips;
    
    delete [] mMapContainedStacks;
    delete [] mMapSubContainedStacks;
    
    delete [] mMap;
    delete [] mMapBiomes;
    delete [] mMapFloors;

    delete [] mMapCellDrawnFlags;

    delete [] mMapPlayerPlacedFlags;

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }

    if( lastMessageSentToServer != NULL ) {
        delete [] lastMessageSentToServer;
        lastMessageSentToServer = NULL;
        }
    
    if( mHungerSound != NULL ) {    
        freeSoundSprite( mHungerSound );
        }
    
    for( int i=0; i<3; i++ ) {
        freeSprite( mHungerSlipSprites[i] );
        }

    freeSprite( mGuiPanelSprite );
    freeSprite( mGuiBloodSprite );
    
    freeSprite( mFloorSplitSprite );
    
    freeSprite( mCellBorderSprite );
    freeSprite( mCellFillSprite );
    
    freeSprite( mNotePaperSprite );
    freeSprite( mChalkBlotSprite );
    freeSprite( mPathMarkSprite );
    freeSprite( mHomeSlipSprite );
    
    if( teaserVideo ) {
        freeSprite( mTeaserArrowLongSprite );
        freeSprite( mTeaserArrowMedSprite );
        freeSprite( mTeaserArrowShortSprite );
        freeSprite( mTeaserArrowVeryShortSprite );
        freeSprite( mLineSegmentSprite );
        }
    
    for( int i=0; i<4; i++ ) {
        freeSprite( mGroundOverlaySprite[i] );
        }
    

    for( int i=0; i<NUM_HUNGER_BOX_SPRITES; i++ ) {
        freeSprite( mHungerBoxSprites[i] );
        freeSprite( mHungerBoxFillSprites[i] );
        
        freeSprite( mHungerBoxErasedSprites[i] );
        freeSprite( mHungerBoxFillErasedSprites[i] );
        }

    for( int i=0; i<NUM_TEMP_ARROWS; i++ ) {
        freeSprite( mTempArrowSprites[i] );
        freeSprite( mTempArrowErasedSprites[i] );
        }

    for( int i=0; i<NUM_HUNGER_DASHES; i++ ) {
        freeSprite( mHungerDashSprites[i] );
        freeSprite( mHungerDashErasedSprites[i] );
        freeSprite( mHungerBarSprites[i] );
        freeSprite( mHungerBarErasedSprites[i] );
        }
    
    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
        freeSprite( mHomeArrowSprites[i] );
        freeSprite( mHomeArrowErasedSprites[i] );
        }



    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        freeSprite( mHintSheetSprites[i] );
        
        if( mHintMessage[i] != NULL ) {
            delete [] mHintMessage[i];
            }
        }
    
    delete [] mHintBookmarks;


    if( mDeathReason != NULL ) {
        delete [] mDeathReason;
        }
    }



char LivingLifePage::isMapBeingPulled() {
    return mapPullMode;
    }



void LivingLifePage::adjustAllFrameCounts( double inOldFrameRateFactor,
                                           double inNewFrameRateFactor ) {
    int numMapCells = mMapD * mMapD;
    
    for( int i=0; i<numMapCells; i++ ) {
        
        double timeVal = inOldFrameRateFactor * mMapAnimationFrameCount[ i ];
        mMapAnimationFrameCount[i] = lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = inOldFrameRateFactor * mMapAnimationFrozenRotFrameCount[ i ];
        mMapAnimationFrozenRotFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );

        timeVal = inOldFrameRateFactor * mMapAnimationLastFrameCount[ i ];
        mMapAnimationLastFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        
        timeVal = 
            inOldFrameRateFactor * mMapFloorAnimationFrameCount[ i ];
        mMapFloorAnimationFrameCount[i] = 
            lrint( timeVal / inNewFrameRateFactor );
        }

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );

        double timeVal = inOldFrameRateFactor * o->animationFrameCount;
        o->animationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->heldAnimationFrameCount;
        o->heldAnimationFrameCount = timeVal / inNewFrameRateFactor;
        
        timeVal = inOldFrameRateFactor * o->lastAnimationFrameCount;
        o->lastAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->lastHeldAnimationFrameCount;
        o->lastHeldAnimationFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->frozenRotFrameCount;
        o->frozenRotFrameCount = timeVal / inNewFrameRateFactor;

        timeVal = inOldFrameRateFactor * o->heldFrozenRotFrameCount;
        o->heldFrozenRotFrameCount = timeVal / inNewFrameRateFactor;
        }
    }



LiveObject *LivingLifePage::getOurLiveObject() {
    
    return getLiveObject( ourID );
    }




LiveObject *LivingLifePage::getLiveObject( int inID ) {
    
    LiveObject *obj = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == inID ) {
            obj = o;
            break;
            }
        }
    return obj;
    }



SimpleVector<char*> *splitLines( const char *inString,
                                 double inMaxWidth ) {
    
    // break into lines
    SimpleVector<char *> *tokens = 
        tokenizeString( inString );
    
    
    // collect all lines before drawing them
    SimpleVector<char *> *lines = new SimpleVector<char*>();
    
    
    if( tokens->size() > 0 ) {
        // start with firt token
        char *firstToken = tokens->getElementDirect( 0 );
        
        lines->push_back( firstToken );
        
        tokens->deleteElement( 0 );
        }
    
    
    while( tokens->size() > 0 ) {
        char *nextToken = tokens->getElementDirect( 0 );
        
        char *currentLine = lines->getElementDirect( lines->size() - 1 );
         
        char *expandedLine = autoSprintf( "%s %s", currentLine, nextToken );
         
        if( handwritingFont->measureString( expandedLine ) <= inMaxWidth ) {
            // replace current line
            delete [] currentLine;
            lines->deleteElement(  lines->size() - 1 );
             
            lines->push_back( expandedLine );
            }
        else {
            // expanded is too long
            // put token at start of next line
            delete [] expandedLine;
             
            lines->push_back( stringDuplicate( nextToken ) );
            }
         

        delete [] nextToken;
        tokens->deleteElement( 0 );
        }
    
    delete tokens;
    
    return lines;
    }



// forces uppercase
void LivingLifePage::drawChalkBackgroundString( doublePair inPos, 
                                                const char *inString,
                                                double inFade,
                                                double inMaxWidth,
                                                LiveObject *inSpeaker,
                                                int inForceMinChalkBlots ) {
    
    char *stringUpper = stringToUpperCase( inString );

    SimpleVector<char*> *lines = splitLines( inString, inMaxWidth );
    
    delete [] stringUpper;

    
    if( lines->size() == 0 ) {
        delete lines;
        return;
        }

    double lineSpacing = handwritingFont->getFontHeight() / 2 + 5;
    
    double firstLineY =  inPos.y + ( lines->size() - 1 ) * lineSpacing;
    
    if( firstLineY > lastScreenViewCenter.y + 330 ) {
        firstLineY = lastScreenViewCenter.y + 330;
        }


    if( inSpeaker->dying ) {
        setDrawColor( .65, 0, 0, inFade );
        }
    else {
        setDrawColor( 1, 1, 1, inFade );
        }
    
    // with a fixed seed
    JenkinsRandomSource blotRandSource( 0 );
        
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        

        double length = handwritingFont->measureString( line );
            
        int numBlots = lrint( 0.25 + length / 20 ) + 1;
        
        if( inForceMinChalkBlots != -1 && numBlots < inForceMinChalkBlots ) {
            numBlots = inForceMinChalkBlots;
            }
    
        doublePair blotSpacing = { 20, 0 };
    
        doublePair firstBlot = 
            { inPos.x, firstLineY - i * lineSpacing};

        
        for( int b=0; b<numBlots; b++ ) {
            doublePair blotPos = add( firstBlot, mult( blotSpacing, b ) );
        
            double rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            
            // double hit vertically
            blotPos.y += 5;
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            
            blotPos.y -= 10;
            rot = blotRandSource.getRandomDouble();
            drawSprite( mChalkBlotSprite, blotPos, 1.0, rot );
            }
        }
    
    if( inSpeaker->dying ) {
        setDrawColor( 1, 1, 1, inFade );
        }
    else {
        setDrawColor( 0, 0, 0, inFade );
        }
    
    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        
        doublePair lineStart = 
            { inPos.x, firstLineY - i * lineSpacing};
        
        handwritingFont->drawString( line, lineStart, alignLeft );
        delete [] line;
        }

    delete lines;
    }




void LivingLifePage::handleAnimSound( int inObjectID, double inAge, 
                                      AnimType inType,
                                      int inOldFrameCount, int inNewFrameCount,
                                      double inPosX, double inPosY ) {    
    
            
    double oldTimeVal = frameRateFactor * inOldFrameCount / 60.0;
            
    double newTimeVal = frameRateFactor * inNewFrameCount / 60.0;
                

    AnimationRecord *anim = getAnimation( inObjectID, inType );
    if( anim != NULL ) {
                    
        for( int s=0; s<anim->numSounds; s++ ) {
            
            if( anim->soundAnim[s].sound.numSubSounds == 0 ) {
                continue;
                }
            
            
            if( ( anim->soundAnim[s].ageStart != -1 &&
                  inAge < anim->soundAnim[s].ageStart )
                ||
                ( anim->soundAnim[s].ageEnd != -1 &&
                  inAge >= anim->soundAnim[s].ageEnd ) ) {
                
                continue;
                }
            
            
            double hz = anim->soundAnim[s].repeatPerSec;
                        
            double phase = anim->soundAnim[s].repeatPhase;
            
            if( hz != 0 ) {
                double period = 1 / hz;
                
                double startOffsetSec = phase * period;
                
                int oldPeriods = 
                    lrint( 
                        floor( ( oldTimeVal - startOffsetSec ) / 
                               period ) );
                
                int newPeriods = 
                    lrint( 
                        floor( ( newTimeVal - startOffsetSec ) / 
                               period ) );
                
                if( newPeriods > oldPeriods ) {
                    SoundUsage u = anim->soundAnim[s].sound;
                    
                    
                    if( anim->soundAnim[s].footstep ) {
                        
                        // check if we're on a floor

                        int x = lrint( inPosX );
                        int y = lrint( inPosY );

                        int i = getMapIndex( x, y );
                        
                        if( i != -1 && mMapFloors[i] > 0 ) {
                            
                            ObjectRecord *f = getObject( mMapFloors[i] );
                            
                            if( f->usingSound.numSubSounds > 0 ) {
                                u = f->usingSound;
                                }
                            }
                        }
                    
                    
                    
                    playSound( u,
                               getVectorFromCamera( inPosX, inPosY ) );
                    
                    }
                }
            }
        }
    }




void LivingLifePage::drawMapCell( int inMapI, 
                                  int inScreenX, int inScreenY,
                                  char inHighlightOnly ) {
            
    int oID = mMap[ inMapI ];

    int objectHeight = 0;
    
    if( oID > 0 ) {
        
        objectHeight = getObjectHeight( oID );
        
        double oldFrameCount = mMapAnimationFrameCount[ inMapI ];

        if( !mapPullMode ) {
            
            if( mMapCurAnimType[ inMapI ] == moving ) {
                double animSpeed = 1.0;
                ObjectRecord *movingObj = getObject( oID );
            
                if( movingObj->speedMult < 1.0 ) {
                    // only slow anims down don't speed them up
                    animSpeed *= movingObj->speedMult;
                    }

                mMapAnimationFrameCount[ inMapI ] += animSpeed;
                mMapAnimationLastFrameCount[ inMapI ] += animSpeed;
                mMapAnimationFrozenRotFrameCount[ inMapI ] += animSpeed;
                }
            else {
                mMapAnimationFrameCount[ inMapI ] ++;
                mMapAnimationLastFrameCount[ inMapI ] ++;
                }

            
            if( mMapLastAnimFade[ inMapI ] > 0 ) {
                mMapLastAnimFade[ inMapI ] -= 0.05 * frameRateFactor;
                if( mMapLastAnimFade[ inMapI ] < 0 ) {
                    mMapLastAnimFade[ inMapI ] = 0;
                    
                    AnimType newType;

                    if( mMapMoveSpeeds[ inMapI ] == 0 ) {
                        newType = ground;
                        }
                    else if( mMapMoveSpeeds[ inMapI ] > 0 ) {
                        // transition to moving now
                        newType = moving;
                        }

                    if( mMapCurAnimType[ inMapI ] != newType ) {
                        
                        mMapLastAnimType[ inMapI ] = mMapCurAnimType[ inMapI ];
                        mMapCurAnimType[ inMapI ] = newType;
                        mMapLastAnimFade[ inMapI ] = 1;
                        
                        mMapAnimationLastFrameCount[ inMapI ] =
                            mMapAnimationFrameCount[ inMapI ];
                        
                        mMapAnimationFrameCount[ inMapI ] = 0;
                        
                        if( newType == moving ) {
                            mMapAnimationFrameCount[ inMapI ] = 
                                mMapAnimationFrozenRotFrameCount[ inMapI ];
                            }
                        }
                    
                    }
                }
            }

        doublePair pos = { (double)inScreenX, (double)inScreenY };
        double rot = 0;
        
        if( mMapDropOffsets[ inMapI ].x != 0 ||
            mMapDropOffsets[ inMapI ].y != 0 ) {
                    
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapDropOffsets[ inMapI ] );
                    
            double step = frameRateFactor * 0.0625;
            double rotStep = frameRateFactor * 0.03125;
                    
            if( length( delta ) < step ) {
                        
                mMapDropOffsets[ inMapI ].x = 0;
                mMapDropOffsets[ inMapI ].y = 0;
                }
            else {
                mMapDropOffsets[ inMapI ] =
                    add( mMapDropOffsets[ inMapI ],
                         mult( normalize( delta ), step ) );
                }
            
            if( mMapDropOffsets[ inMapI ].x == 0 &&
                mMapDropOffsets[ inMapI ].y == 0 ) {
                // done dropping into place
                if( mMapDropSounds[ inMapI ].numSubSounds > 0 ) {
                    
                    playSound( mMapDropSounds[ inMapI ],
                               getVectorFromCamera( 
                                   (double)inScreenX / CELL_D,
                                   (double)inScreenY / CELL_D ) );
                    mMapDropSounds[ inMapI ] = blankSoundUsage;
                    }
                
                }
            
                
            
            double rotDelta = 0 - mMapDropRot[ inMapI ];
            
            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            if( fabs( rotDelta ) < rotStep ) {
                mMapDropRot[ inMapI ] = 0;
                }
            else {
                double rotSign = 1;
                if( rotDelta < 0 ) {
                    rotSign = -1;
                    }
                
                mMapDropRot[ inMapI ] = 
                    mMapDropRot[ inMapI ] + rotSign * rotStep;
                }
            

                                        
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            pos = add( pos, mult( mMapDropOffsets[ inMapI ], CELL_D ) );
            
            rot = mMapDropRot[ inMapI ];
            }
        
        if( mMapMoveSpeeds[inMapI] > 0 &&
            ( mMapMoveOffsets[ inMapI ].x != 0 ||
              mMapMoveOffsets[ inMapI ].y != 0  ) ) {

            pos = add( pos, mult( mMapMoveOffsets[ inMapI ], CELL_D ) );
            }
        


        setDrawColor( 1, 1, 1, 1 );
                
        AnimType curType = ground;
        AnimType fadeTargetType = ground;
        double animFade = 1;

        if( mMapMoveSpeeds[ inMapI ] > 0 ) {
            curType = moving;
            fadeTargetType = moving;
            animFade = 1;
            }
        


        double timeVal = frameRateFactor * 
            mMapAnimationFrameCount[ inMapI ] / 60.0;
                
        double frozenRotTimeVal = frameRateFactor *
            mMapAnimationFrozenRotFrameCount[ inMapI ] / 60.0;

        double targetTimeVal = timeVal;

        if( mMapLastAnimFade[ inMapI ] != 0 ) {
            animFade = mMapLastAnimFade[ inMapI ];
            curType = mMapLastAnimType[ inMapI ];
            fadeTargetType = mMapCurAnimType[ inMapI ];
            
            timeVal = frameRateFactor * 
                mMapAnimationLastFrameCount[ inMapI ] / 60.0;
            }
                
                
        // ignore
        char used = false;
            
        char flip = mMapTileFlips[ inMapI ];
        
        ObjectRecord *obj = getObject( oID );
        if( obj->permanent && 
            ( obj->blocksWalking || obj->drawBehindPlayer ) ) {
            // permanent, blocking objects (e.g., walls) 
            // or permanent behind-player objects (e.g., roads) 
            // are never drawn flipped
            flip = false;
            }
        
        char highlight = false;
        float highlightFade = 1.0f;
        
        if( mCurMouseOverID > 0 &&
            ! mCurMouseOverSelf &&
            mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x == inMapI ) {
            
            if( mCurMouseOverBehind ) {
                highlight = inHighlightOnly;
                }
            else {
                highlight = true;
                }
        
            highlightFade = mCurMouseOverFade;
            }
        else {
            for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
                GridPos prev = mPrevMouseOverSpots.getElementDirect( i );
                
                if( prev.y * mMapD + prev.x == inMapI ) {
                    if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
                        highlight = inHighlightOnly;
                        }
                    else {
                        highlight = true;
                        }
                    highlightFade = 
                        mPrevMouseOverSpotFades.getElementDirect(i);
                    }    
                }
            }
        
        if( ! mShowHighlights ) {
            highlight = false;
            }
        

        if( !mapPullMode && !inHighlightOnly ) {
            handleAnimSound( oID, 0, mMapCurAnimType[ inMapI ], oldFrameCount, 
                             mMapAnimationFrameCount[ inMapI ],
                             pos.x / CELL_D,
                             pos.y / CELL_D );
            }
        

        
        int numPasses = 1;
        int startPass = 0;
        
        if( highlight ) {
            
            // first pass normal draw
            // then three stencil passes (second and third with a subtraction)
            numPasses = 6;
            
            if( highlightFade != 1.0f ) {
                //fadeHandle = addGlobalFade( highlightFade );
                }

            if( inHighlightOnly ) {
                startPass = 1;
                }
            }
        
        for( int i=startPass; i<numPasses; i++ ) {
            
            doublePair passPos = pos;
            
            if( highlight ) {
                
                switch( i ) {
                    case 0:
                        // normal color draw
                        break;
                    case 1:
                        // opaque portion
                        startAddingToStencil( false, true, .99f );
                        break;
                    case 2:
                        // first fringe
                        startAddingToStencil( false, true, .07f );
                        break;
                    case 3:
                        // subtract opaque from fringe to get just first fringe
                        startAddingToStencil( false, false, .99f );
                        break;
                    case 4:
                        // second fringe
                        // ignore totally transparent stuff
                        // like invisible animation layers
                        startAddingToStencil( false, true, 0.01f );
                        break;
                    case 5:
                        // subtract first fringe from fringe to get 
                        // just secon fringe
                        startAddingToStencil( false, false, .07f );
                        break;
                    default:
                        break;
                    }

                }

        if( mMapContainedStacks[ inMapI ].size() > 0 ) {
            int *stackArray = 
                mMapContainedStacks[ inMapI ].getElementArray();
            SimpleVector<int> *subStackArray =
                mMapSubContainedStacks[ inMapI ].getElementArray();
            
            drawObjectAnim( oID, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &used,
                            endAnimType,
                            endAnimType,
                            passPos, rot, false, flip,
                            -1,
                            false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            mMapContainedStacks[ inMapI ].size(),
                            stackArray, subStackArray );
            delete [] stackArray;
            delete [] subStackArray;
            }
        else {
            drawObjectAnim( oID, 2, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType, 
                            targetTimeVal,
                            frozenRotTimeVal,
                            &used,
                            endAnimType,
                            endAnimType,
                            passPos, rot,
                            false,
                            flip, -1,
                            false, false, false,
                            getEmptyClothingSet(), NULL );
            }
        

        if( highlight ) {
            
            
            float mainFade = .35f;
        
            toggleAdditiveBlend( true );
            
            doublePair squarePos = passPos;
            
            if( objectHeight > 1.5 * CELL_D ) {
                squarePos.y += 192;
                }
            
            int squareRad = 286;
            
            switch( i ) {
                case 0:
                    // normal color draw
                    break;
                case 1:
                    // opaque portion
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                case 2:
                    // first fringe
                    // wait until next pass to isolate fringe
                    break;
                case 3:
                    // now first fringe is isolated in stencil
                    startDrawingThroughStencil( false );

                    setDrawColor( 1, 1, 1, highlightFade * mainFade * .5 );

                    drawSquare( squarePos, squareRad );

                    stopStencil();                    
                    break;
                case 4:
                    // second fringe
                    // wait until next pass to isolate fringe
                    break;
                case 5:
                    // now second fringe is isolated in stencil
                    startDrawingThroughStencil( false );
                    
                    setDrawColor( 1, 1, 1, highlightFade * mainFade *.25 );
                    
                    drawSquare( squarePos, squareRad );
                    
                    stopStencil();
                    break;
                default:
                    break;
                }
            toggleAdditiveBlend( false );
            }

        
            }
        
        
        
        }
    else if( oID == -1 ) {
        // unknown
        doublePair pos = { (double)inScreenX, (double)inScreenY };
                
        setDrawColor( 0, 0, 0, 0.5 );
        drawSquare( pos, 14 );
        }

    }


SimpleVector<doublePair> trail;
SimpleVector<FloatColor> trailColors;
double pathStepDistFactor = 0.2;

FloatColor trailColor = { 0, 0.5, 0, 0.25 };



int LivingLifePage::getMapIndex( int inWorldX, int inWorldY ) {
    int mapTargetX = inWorldX - mMapOffsetX + mMapD / 2;
    int mapTargetY = inWorldY - mMapOffsetY + mMapD / 2;

    if( mapTargetY >= 0 && mapTargetY < mMapD &&
        mapTargetX >= 0 && mapTargetX < mMapD ) {
                    
        return mapTargetY * mMapD + mapTargetX;
        }
    return -1;
    }




ObjectAnimPack LivingLifePage::drawLiveObject( 
    LiveObject *inObj,
    SimpleVector<LiveObject *> *inSpeakers,
    SimpleVector<doublePair> *inSpeakersPos ) {    


    ObjectAnimPack returnPack;
    returnPack.inObjectID = -1;


    if( inObj->hide || inObj->outOfRange ) {
        return returnPack;
        }


    inObj->onScreen = true;



    if( ! inObj->allSpritesLoaded ) {
        // skip drawing until fully loaded
        return returnPack;
        }

    // current pos
                    
    doublePair pos = mult( inObj->currentPos, CELL_D );
    

    if( inObj->heldByDropOffset.x != 0 ||
        inObj->heldByDropOffset.y != 0 ) {
                    
        doublePair nullOffset = { 0, 0 };
                    
        
        doublePair delta = sub( nullOffset, 
                                inObj->heldByDropOffset );
                    
        double step = frameRateFactor * 0.0625;

        if( length( delta ) < step ) {
            
            inObj->heldByDropOffset.x = 0;
            inObj->heldByDropOffset.y = 0;
            
            ObjectRecord *displayObj = 
                getObject( inObj->displayID );
            
            if( displayObj->usingSound.numSubSounds > 0 ) {
                // play baby's using sound as they are put down
                // we no longer have access to parent's using sound
                playSound( displayObj->usingSound,
                           getVectorFromCamera(
                               inObj->currentPos.x, inObj->currentPos.y ) );
                }
            }
        else {
            inObj->heldByDropOffset =
                add( inObj->heldByDropOffset,
                     mult( normalize( delta ), step ) );
            }
                            
        // step offset BEFORE applying it
        // (so we don't repeat starting position)
        pos = add( pos, mult( inObj->heldByDropOffset, CELL_D ) );
        }
    

    doublePair actionOffset = { 0, 0 };
    
    if( false )
    if( inObj->curAnim == moving ) {
        trail.push_back( pos );
        trailColors.push_back( trailColor );

        while( trail.size() > 1000 ) {
            trail.deleteElement( 0 );
            trailColors.deleteElement( 0 );
            }
        }
    

    int targetX = playerActionTargetX;
    int targetY = playerActionTargetY;
    
    if( inObj->id != ourID ) {
        targetX = inObj->actionTargetX;
        targetY = inObj->actionTargetY;
        }
                
    if( inObj->curAnim != eating &&
        inObj->lastAnim != eating &&
        inObj->pendingActionAnimationProgress != 0 ) {
                    
        // wiggle toward target

        
        int trueTargetX = targetX + inObj->actionTargetTweakX;
        int trueTargetY = targetY + inObj->actionTargetTweakY;
        
                    
        float xDir = 0;
        float yDir = 0;
                
        doublePair dir = { trueTargetX - inObj->currentPos.x,
                           trueTargetY - inObj->currentPos.y };
        
        if( dir.x != 0 || dir.y != 0 ) {    
            dir = normalize( dir );
            }
        
        xDir = dir.x;
        yDir = dir.y;
    
        if( inObj->currentPos.x < trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = false;
                }
            }
        if( inObj->currentPos.x > trueTargetX ) {
            if( inObj->currentSpeed == 0 ) {
                inObj->holdingFlip = true;
                }
            }

        double wiggleMax = CELL_D *.5 *.90;
        
        double halfWiggleMax = wiggleMax * 0.5;

        if( xDir == 0 && yDir == 0 ) {
            // target where we're standing
            // wiggle tiny bit down
            yDir = -1;
            
            halfWiggleMax *= 0.25;
            }
        else if( xDir == 0 && yDir == -1 ) {
            // moving down causes feet to cross object in our same tile
            // move less
            halfWiggleMax *= 0.5;
            }
        

        
        double offset =
            halfWiggleMax - 
            halfWiggleMax * 
            cos( 2 * M_PI * inObj->pendingActionAnimationProgress );
                    
                    
        actionOffset.x += xDir * offset;
        actionOffset.y += yDir * offset;
        }
                
                
    // bare hands action OR holding something
    // character wiggle
    if( inObj->pendingActionAnimationProgress != 0 ) {
                    
        pos = add( pos, actionOffset );
        }                

                
    AnimType curType = inObj->curAnim;
    AnimType fadeTargetType = inObj->curAnim;
                
    double animFade = 1.0;
                
    double timeVal = frameRateFactor * 
        inObj->animationFrameCount / 60.0;
    
    double targetTimeVal = timeVal;

    double frozenRotTimeVal = frameRateFactor * 
        inObj->frozenRotFrameCount / 60.0;

    if( inObj->lastAnimFade > 0 ) {
        curType = inObj->lastAnim;
        fadeTargetType = inObj->curAnim;
        animFade = inObj->lastAnimFade;
        
        timeVal = frameRateFactor * 
            inObj->lastAnimationFrameCount / 60.0;
        }
                

    setDrawColor( 1, 1, 1, 1 );
    //setDrawColor( red, 0, blue, 1 );
    //mainFont->drawString( string, 
    //                      pos, alignCenter );
                
    double age = computeCurrentAge( inObj );


    ObjectRecord *heldObject = NULL;

    int hideClosestArm = 0;
    char hideAllLimbs = false;


    if( inObj->holdingID != 0 ) { 
        if( inObj->holdingID > 0 ) {
            heldObject = getObject( inObj->holdingID );
            }
        else if( inObj->holdingID < 0 ) {
            // held baby
            LiveObject *babyO = getGameObject( - inObj->holdingID );
            
            if( babyO != NULL ) {    
                heldObject = getObject( babyO->displayID );
                }
            }
        }
    
    
    getArmHoldingParameters( heldObject, &hideClosestArm, &hideAllLimbs );
    

    // override animation types for people who are riding in something
    // animationBank will freeze the time on their arms whenever they are
    // in a moving animation.
    AnimType frozenArmType = endAnimType;
    AnimType frozenArmFadeTargetType = endAnimType;
    
    if( hideClosestArm == -2 
        ||
        ( inObj->holdingID > 0 && getObject( inObj->holdingID )->rideable )
        ||
        ( inObj->lastHoldingID > 0 && 
          getObject( inObj->lastHoldingID )->rideable ) ) {
    
        if( curType == ground2 || curType == moving ) {
            frozenArmType = moving;
            }
        if( fadeTargetType == ground2 || fadeTargetType == moving ) {
            frozenArmFadeTargetType = moving;
            }
        }
    
    char alreadyDrawnPerson = false;
    
    HoldingPos holdingPos;
    holdingPos.valid = false;
    

    if( inObj->holdingID > 0 &&
        heldObject->rideable ) {
        // don't draw now,
        // wait until we know rideable's offset
        }
    else {
        alreadyDrawnPerson = true;
        doublePair personPos = pos;
        
        // decay away from riding offset, if any
        if( inObj->ridingOffset.x != 0 ||
            inObj->ridingOffset.y != 0 ) {
            
            doublePair nullOffset = { 0, 0 };
            
            
            doublePair delta = sub( nullOffset, inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {
            
                inObj->ridingOffset.x = 0;
                inObj->ridingOffset.y = 0;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }
                            
            // step offset BEFORE applying it
            // (so we don't repeat starting position)
            personPos = add( personPos, inObj->ridingOffset );
            }
        


        holdingPos =
            drawObjectAnim( inObj->displayID, 2, curType, 
                            timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &( inObj->frozenRotFrameCountUsed ),
                            frozenArmType,
                            frozenArmFadeTargetType,
                            personPos,
                            0,
                            false,
                            inObj->holdingFlip,
                            age,
                            // don't actually hide body parts until
                            // held object is done sliding into place
                            hideClosestArm,
                            hideAllLimbs,
                            inObj->heldPosOverride && 
                            ! inObj->heldPosOverrideAlmostOver,
                            inObj->clothing,
                            inObj->clothingContained );
        }
    
        
    if( inObj->holdingID != 0 ) { 
        doublePair holdPos;
        
        double holdRot = 0;
        
        computeHeldDrawPos( holdingPos, pos,
                            heldObject,
                            inObj->holdingFlip,
                            &holdPos, &holdRot );

                
        doublePair heldObjectDrawPos = holdPos;
        
        if( heldObject != NULL && heldObject->rideable ) {
            heldObjectDrawPos = pos;
            }
        

        heldObjectDrawPos = mult( heldObjectDrawPos, 1.0 / CELL_D );
        
        if( inObj->heldPosOverride && 
            ! inObj->heldPosOverrideAlmostOver &&
            ! equal( heldObjectDrawPos, inObj->heldObjectPos ) ) {
                        
            doublePair delta = sub( heldObjectDrawPos, inObj->heldObjectPos );
            double rotDelta = holdRot - inObj->heldObjectRot;

            if( rotDelta > 0.5 ) {
                rotDelta = rotDelta - 1;
                }
            else if( rotDelta < -0.5 ) {
                rotDelta = 1 + rotDelta;
                }

            // as slide gets longer, we speed up
            double longSlideModifier = 1;
            
            double slideTime = inObj->heldPosSlideStepCount * frameRateFactor;
            
            if( slideTime > 30 ) {
                // more than a half second
                longSlideModifier = pow( slideTime / 30, 2 );
                }

            double step = frameRateFactor * 0.0625 * longSlideModifier;
            double rotStep = frameRateFactor * 0.03125;
            
            if( length( delta ) < step ) {
                inObj->heldObjectPos = heldObjectDrawPos;
                inObj->heldPosOverrideAlmostOver = true;
                }
            else {
                inObj->heldObjectPos =
                    add( inObj->heldObjectPos,
                         mult( normalize( delta ),
                               step ) );
                
                heldObjectDrawPos = inObj->heldObjectPos;
                }

            if( fabs( rotDelta ) < rotStep ) {
                inObj->heldObjectRot = holdRot;
                }
            else {

                double rotDir = 1;
                if( rotDelta < 0 ) {
                    rotDir = -1;
                    }

                inObj->heldObjectRot =
                    inObj->heldObjectRot + rotDir * rotStep;
                
                holdRot = inObj->heldObjectRot;
                }

            inObj->heldPosSlideStepCount ++;
            }
        else {
            inObj->heldPosOverride = false;
            inObj->heldPosOverrideAlmostOver = false;
            // track it every frame so we have a good
            // base point for smooth move when the object
            // is dropped
            inObj->heldObjectPos = heldObjectDrawPos;
            inObj->heldObjectRot = holdRot;
            }
          
        doublePair worldHoldPos = heldObjectDrawPos;
          
        heldObjectDrawPos = mult( heldObjectDrawPos, CELL_D );
        
        if( heldObject == NULL || 
            ! heldObject->rideable ) {
            
            holdPos = heldObjectDrawPos;
            }

        setDrawColor( 1, 1, 1, 1 );
                    
                    
        AnimType curHeldType = inObj->curHeldAnim;
        AnimType fadeTargetHeldType = inObj->curHeldAnim;
                
        double heldAnimFade = 1.0;
                    
        double heldTimeVal = frameRateFactor * 
            inObj->heldAnimationFrameCount / 60.0;
        
        double targetHeldTimeVal = heldTimeVal;
        
        double frozenRotHeldTimeVal = frameRateFactor * 
            inObj->heldFrozenRotFrameCount / 60.0;
        

        if( !alreadyDrawnPerson ) {
            doublePair personPos = pos;
            
            doublePair targetRidingOffset = sub( personPos, holdPos );

            
            ObjectRecord *personObj = getObject( inObj->displayID );

            targetRidingOffset = 
                sub( targetRidingOffset, 
                     getAgeBodyOffset( 
                         age,
                         personObj->spritePos[ 
                             getBodyIndex( personObj, age ) ] ) );
            
            // step toward target to smooth
            doublePair delta = sub( targetRidingOffset, 
                                    inObj->ridingOffset );
            
            double step = frameRateFactor * 8;

            if( length( delta ) < step ) {            
                inObj->ridingOffset = targetRidingOffset;
                }
            else {
                inObj->ridingOffset =
                    add( inObj->ridingOffset,
                         mult( normalize( delta ), step ) );
                }

            personPos = add( personPos, inObj->ridingOffset );

            // rideable object
            holdingPos =
                drawObjectAnim( inObj->displayID, 2, curType, 
                                timeVal,
                                animFade,
                                fadeTargetType,
                                targetTimeVal,
                                frozenRotTimeVal,
                                &( inObj->frozenRotFrameCountUsed ),
                                frozenArmType,
                                frozenArmFadeTargetType,
                                personPos,
                                0,
                                false,
                                inObj->holdingFlip,
                                age,
                                // don't actually hide body parts until
                                // held object is done sliding into place
                                hideClosestArm,
                                hideAllLimbs,
                                inObj->heldPosOverride && 
                                ! inObj->heldPosOverrideAlmostOver,
                                inObj->clothing,
                                inObj->clothingContained );
            }
        


        // animate baby with held anim just like any other held object
        if( inObj->lastHeldAnimFade > 0 ) {
            curHeldType = inObj->lastHeldAnim;
            fadeTargetHeldType = inObj->curHeldAnim;
            heldAnimFade = inObj->lastHeldAnimFade;
            
            heldTimeVal = frameRateFactor * 
                inObj->lastHeldAnimationFrameCount / 60.0;
            }
                        
                    
        if( inObj->holdingID < 0 ) {
            // draw baby here
            int babyID = - inObj->holdingID;
            
            LiveObject *babyO = getGameObject( babyID );
            
            if( babyO != NULL ) {
                
                // save flip so that it sticks when baby set down
                babyO->holdingFlip = inObj->holdingFlip;
                
                // save world hold pos for smooth set-down of baby
                babyO->lastHeldByRawPosSet = true;
                babyO->lastHeldByRawPos = worldHoldPos;

                returnPack =
                    drawObjectAnimPacked( 
                                babyO->displayID, curHeldType, 
                                heldTimeVal,
                                heldAnimFade,
                                fadeTargetHeldType,
                                targetHeldTimeVal,
                                frozenRotHeldTimeVal,
                                &( inObj->heldFrozenRotFrameCountUsed ),
                                endAnimType,
                                endAnimType,
                                holdPos,
                                // never apply held rot to baby
                                0,
                                false,
                                inObj->holdingFlip,
                                computeCurrentAge( babyO ),
                                // don't hide baby's hands when it is held
                                false,
                                false,
                                false,
                                babyO->clothing,
                                babyO->clothingContained,
                                0, NULL, NULL );

                if( babyO->currentSpeech != NULL ) {
                    
                    inSpeakers->push_back( babyO );
                    inSpeakersPos->push_back( holdPos );
                    }
                }
            }
        else if( inObj->numContained == 0 ) {
                        
            returnPack = 
                drawObjectAnimPacked(
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            inObj->holdingFlip, -1, false, false, false,
                            getEmptyClothingSet(), NULL,
                            0, NULL, NULL );
            }
        else {
            returnPack =
                drawObjectAnimPacked( 
                            inObj->holdingID, curHeldType, 
                            heldTimeVal,
                            heldAnimFade,
                            fadeTargetHeldType,
                            targetHeldTimeVal,
                            frozenRotHeldTimeVal,
                            &( inObj->heldFrozenRotFrameCountUsed ),
                            endAnimType,
                            endAnimType,
                            heldObjectDrawPos,
                            holdRot,
                            false,
                            inObj->holdingFlip,
                            -1, false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            inObj->numContained,
                            inObj->containedIDs,
                            inObj->subContainedIDs );
            }
        }
                
    if( inObj->currentSpeech != NULL ) {
                    
        inSpeakers->push_back( inObj );
        inSpeakersPos->push_back( pos );
        }

    return returnPack;
    }



void LivingLifePage::drawHungerMaxFillLine( doublePair inAteWordsPos,
                                            int inMaxFill,
                                            SpriteHandle *inBarSprites,
                                            SpriteHandle *inDashSprites,
                                            char inSkipBar,
                                            char inSkipDashes ) {
    
    
    
    doublePair barPos = { lastScreenViewCenter.x - 590, 
                          lastScreenViewCenter.y - 334 };
    barPos.x -= 12;
    barPos.y -= 10;
    
    
    barPos.x += 30 * inMaxFill;

    if( ! inSkipBar ) {    
        drawSprite( inBarSprites[ inMaxFill %
                                  NUM_HUNGER_DASHES ], 
                    barPos );
        }
    

    if( inSkipDashes ) {
        return;
        }

    doublePair dashPos = inAteWordsPos;
            
    dashPos.y -= 6;
    dashPos.x -= 5;

    int numDashes = 0;
            
    JenkinsRandomSource dashRandSource( 0 );

    while( dashPos.x > barPos.x + 9 ) {
        
        doublePair drawPos = dashPos;
        
        //drawPos.x += dashRandSource.getRandomBoundedInt( -2, 2 );
        //drawPos.y += dashRandSource.getRandomBoundedInt( -1, 1 );
        
        drawSprite( inDashSprites[ numDashes %
                                   NUM_HUNGER_DASHES ], 
                    drawPos );
        dashPos.x -= 15;
        //numDashes += dashRandSource.getRandomBoundedInt( 1, 10 );
        numDashes += 1;
        
        // correct shortness of last one
        if( numDashes % NUM_HUNGER_DASHES == 0 ) {
            dashPos.x += 3;
            }
        }
            
    // draw one more to connect to bar
    dashPos.x = barPos.x + 6;
    drawSprite( inDashSprites[ numDashes %
                               NUM_HUNGER_DASHES ], 
                dashPos );
    }




static void drawLine( SpriteHandle inSegmentSprite,
                      doublePair inStart, doublePair inEnd,
                      FloatColor inColor ) {
    
    doublePair dir = normalize( sub( inEnd, inStart ) );
    
    doublePair perpDir = { -dir.y, dir.x };
    
    perpDir = mult( perpDir, 2 );
    

    doublePair spriteVerts[4] = 
        { { inStart.x - perpDir.x,
            inStart.y - perpDir.y },
          { inEnd.x - perpDir.x,
            inEnd.y - perpDir.y },
          { inEnd.x + perpDir.x,
            inEnd.y + perpDir.y },
          { inStart.x + perpDir.x,
            inStart.y + perpDir.y } };
    
    FloatColor spriteColors[4] = 
        { inColor, inColor, inColor, inColor };
    
                                
    drawSprite( inSegmentSprite,
                spriteVerts, spriteColors );
    }



static double getBoundedRandom( int inX, int inY,
                                double inUpper, double inLower ) {
    double val = getXYRandom( inX, inY );
    
    return val * ( inUpper - inLower ) + inLower;
    }




static char isInBounds( int inX, int inY, int inMapD ) {
    if( inX < 0 || inY < 0 || inX > inMapD - 1 || inY > inMapD - 1 ) {
        return false;
        }
    return true;
    }



typedef struct DrawOrderRecord {
        char person;
        // if person
        LiveObject *personO;
        
        // if cell
        int mapI;
        int screenX, screenY;

        char extraMovingObj;
        // if extra moving obj
        int extraMovingIndex;
        
    } DrawOrderRecord;
        


char drawAdd = true;
char drawMult = true;

double multAmount = 0.15;
double addAmount = 0.25;


char blackBorder = false;
                                
char whiteBorder = true;


void LivingLifePage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    
    setViewCenterPosition( lastScreenViewCenter.x,
                           lastScreenViewCenter.y );

    char stillWaitingBirth = false;
    

    if( mFirstServerMessagesReceived != 3 ) {
        // haven't gotten first messages from server yet
        stillWaitingBirth = true;
        }
    else if( mFirstServerMessagesReceived == 3 ) {
        if( !mDoneLoadingFirstObjectSet ) {
            stillWaitingBirth = true;
            }
        }


    if( stillWaitingBirth ) {
        
        // draw this to cover up utility text field, but not
        // waiting icon at top
        setDrawColor( 0, 0, 0, 1 );
        drawSquare( lastScreenViewCenter, 100 );
        
        setDrawColor( 1, 1, 1, 1 );
        doublePair pos = { 0, 0 };
        drawMessage( "waitingBirth", pos );

        if( mStartedLoadingFirstObjectSet ) {
            
            pos.y = -100;
            drawMessage( "loadingMap", pos );

            // border
            setDrawColor( 1, 1, 1, 1 );
    
            drawRect( -100, -220, 
                      100, -200 );

            // inner black
            setDrawColor( 0, 0, 0, 1 );
            
            drawRect( -98, -218, 
                      98, -202 );
    
    
            // progress
            setDrawColor( .8, .8, .8, 1 );
            drawRect( -98, -218, 
                      -98 + mFirstObjectSetLoadingProgress * ( 98 * 2 ), 
                      -202 );
            }
        
        return;
        }


    //setDrawColor( 1, 1, 1, 1 );
    //drawSquare( lastScreenViewCenter, viewWidth );
    

    //if( currentGamePage != NULL ) {
    //    currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
    //    }
    
    setDrawColor( 1, 1, 1, 1 );

    int gridCenterX = 
        lrintf( lastScreenViewCenter.x / CELL_D ) - mMapOffsetX + mMapD/2;
    int gridCenterY = 
        lrintf( lastScreenViewCenter.y / CELL_D ) - mMapOffsetY + mMapD/2;
    
    // more on left and right of screen to avoid wide object tops popping in
    int xStart = gridCenterX - 7;
    int xEnd = gridCenterX + 7;

    // more on bottom of screen so that tall objects don't pop in
    int yStart = gridCenterY - 6;
    int yEnd = gridCenterY + 4;

    if( xStart < 0 ) {
        xStart = 0;
        }
    if( xStart >= mMapD ) {
        xStart = mMapD - 1;
        }
    
    if( yStart < 0 ) {
        yStart = 0;
        }
    if( yStart >= mMapD ) {
        yStart = mMapD - 1;
        }

    if( xEnd < 0 ) {
        xEnd = 0;
        }
    if( xEnd >= mMapD ) {
        xEnd = mMapD - 1;
        }
    
    if( yEnd < 0 ) {
        yEnd = 0;
        }
    if( yEnd >= mMapD ) {
        yEnd = mMapD - 1;
        }



    // don't bound floor start and end here
    // 
    // we want to show unknown biome off edge
    // instead, check before using to index mMapBiomes mid-loop
    
    // note that we can't check mMapCellDrawnFlags outside of map boundaries
    // which will result in some over-drawing out there (whole sheets with
    // tiles drawn on top).  However, given that we're not drawing anything
    // else out there, this should be okay from a performance standpoint.

    int yStartFloor = gridCenterY - 4;
    int yEndFloor = gridCenterY + 3;

    int xStartFloor = gridCenterX - 5;
    int xEndFloor = gridCenterX + 6;

    


    int numCells = mMapD * mMapD;

    memset( mMapCellDrawnFlags, false, numCells );

    // draw underlying ground biomes
    for( int y=yEndFloor; y>=yStartFloor; y-- ) {

        int screenY = CELL_D * ( y + mMapOffsetY - mMapD / 2 );

        int tileY = -lrint( screenY / CELL_D );

        
        for( int x=xStartFloor; x<=xEndFloor; x++ ) {
            int mapI = y * mMapD + x;
            
            char inBounds = isInBounds( x, y, mMapD );

            if( inBounds && mMapCellDrawnFlags[mapI] ) {
                continue;
                }

            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );
            
            int tileX = lrint( screenX / CELL_D );

            
            int b = -1;
            
            if( inBounds ) {
                b = mMapBiomes[mapI];
                }
            
            GroundSpriteSet *s = NULL;
            
            setDrawColor( 1, 1, 1, 1 );

            if( b >= 0 && b < groundSpritesArraySize ) {
                s = groundSprites[ b ];
                }
            else if( b == -1 ) {
                // unknown biome image at end
                s = groundSprites[ groundSpritesArraySize - 1 ];
                }
            
            if( s == NULL ) {
                
                // use end image with random color
                s = groundSprites[ groundSpritesArraySize - 1 ];
                
                // random draw color
                setDrawColor( getXYRandom( b, b ),
                              getXYRandom( b, b + 100 ),
                              getXYRandom( b, b + 300 ), 1 );
                /*
                // find another
                for( int i=0; i<groundSpritesArraySize && s == NULL; i++ ) {
                    s = groundSprites[ i ];
                    }
                */
                }
            
            
            if( s != NULL ) {
                
                
                            
                doublePair pos = { (double)screenX, (double)screenY };
                
                
                // wrap around
                int setY = tileY % s->numTilesHigh;
                int setX = tileX % s->numTilesWide;
                
                if( setY < 0 ) {
                    setY += s->numTilesHigh;
                    }
                if( setX < 0 ) {
                    setX += s->numTilesHigh;
                    }
                

                if( setY == 0 && setX == 0 ) {
                    
                    // check if we're on corner of all-same-biome region that
                    // we can fill with one big sheet

                    char allSameBiome = true;
                    
                    // check borders of would-be sheet too
                    for( int nY = y+1; nY >= y - s->numTilesHigh; nY-- ) {
                        
                        if( nY >=0 && nY < mMapD ) {
                            
                            for( int nX = x-1; 
                                 nX <= x + s->numTilesWide; nX++ ) {
                                
                                if( nX >=0 && nX < mMapD ) {
                                    int nI = nY * mMapD + nX;
                                    
                                    int nB = -1;
                                    
                                    if( isInBounds( nX, nY, mMapD ) ) {
                                        nB = mMapBiomes[nI];
                                        }

                                    if( nB != b ) {
                                        allSameBiome = false;
                                        break;
                                        }
                                    }
                                }

                            }
                        if( !allSameBiome ) {
                            break;
                            }
                        }
                    
                    if( allSameBiome ) {
                        
                        doublePair lastCornerPos = 
                            { pos.x + ( s->numTilesWide - 1 ) * CELL_D, 
                              pos.y - ( s->numTilesHigh - 1 ) * CELL_D };

                        doublePair sheetPos = mult( add( pos, lastCornerPos ),
                                                    0.5 );
                        
                        drawSprite( s->wholeSheet, sheetPos );
                        
                        // mark all cells under sheet as drawn
                        for( int sY = y; sY > y - s->numTilesHigh; sY-- ) {
                        
                            if( sY >=0 && sY < mMapD ) {
                            
                                for( int sX = x; 
                                     sX < x + s->numTilesWide; sX++ ) {
                                
                                    if( sX >=0 && sX < mMapD ) {
                                        int sI = sY * mMapD + sX;
                                        
                                        mMapCellDrawnFlags[sI] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                
                if( ! inBounds || ! mMapCellDrawnFlags[mapI] ) {
                    // not drawn as whole sheet
                    
                    int aboveB = -1;
                    int leftB = -1;
                    int diagB = -1;
                    
                    if( isInBounds( x -1, y, mMapD ) ) {    
                        leftB = mMapBiomes[ mapI - 1 ];
                        }
                    if( isInBounds( x, y + 1, mMapD ) ) {    
                        aboveB = mMapBiomes[ mapI + mMapD ];
                        }
                    
                    if( isInBounds( x + 1, y + 1, mMapD ) ) {    
                        diagB = mMapBiomes[ mapI + mMapD + 1 ];
                        }
                    
                    if( leftB == b &&
                        aboveB == b &&
                        diagB == b ) {
                        
                        // surrounded by same biome above and to left
                        // AND diagonally to the above-right
                        // draw square tile here to save pixel fill time
                        drawSprite( s->squareTiles[setY][setX], pos );
                        }
                    else {
                        drawSprite( s->tiles[setY][setX], pos );
                        }
                    if( inBounds ) {
                        mMapCellDrawnFlags[mapI] = true;
                        }
                    }
                }
            }
        }
    


    double hugR = CELL_D * 0.6;

    // draw floors on top of biome
    for( int y=yEnd; y>=yStart; y-- ) {
        
        int worldY = y + mMapOffsetY - mMapD / 2;

        int screenY = CELL_D * worldY;        

        for( int x=xStart; x<=xEnd; x++ ) {
            
            int worldX = x + mMapOffsetX - mMapD / 2;


            int mapI = y * mMapD + x;

            int oID = mMapFloors[mapI];

            
            int screenX = CELL_D * worldX;
            
            doublePair pos = { (double)screenX, (double)screenY };


            char drawHuggingFloor = false;
            
            // for main floor, and left and right hugging floor
            // 0 to skip a pass
            int passIDs[3] = { 0, 0, 0 };
            
            if( oID > 0 ) {
                passIDs[0] = oID;
                }
            


            if( oID <= 0) {

                
                int cellOID = mMap[mapI];
                
                if( cellOID > 0 && getObject( cellOID )->floorHugging ) {
                    
                    if( x > 0 && mMapFloors[ mapI - 1 ] > 0 ) {
                        // floor to our left
                        passIDs[1] = mMapFloors[ mapI - 1 ];
                        drawHuggingFloor = true;
                        }
                    
                    if( x < mMapD - 1 && mMapFloors[ mapI + 1 ] > 0 ) {
                        // floor to our right
                        passIDs[2] = mMapFloors[ mapI + 1 ];
                        drawHuggingFloor = true;
                        
                        }
                    }
                

                if( ! drawHuggingFloor ) {
                    continue;
                    }
                }
            
            
                    
            int oldFrameCount = 
                mMapFloorAnimationFrameCount[ mapI ];
            
            if( ! mapPullMode ) {
                mMapFloorAnimationFrameCount[ mapI ] ++;
                }



            for( int p=0; p<3; p++ ) {
                if( passIDs[p] == 0 ) {
                    continue;
                    }
                
                oID = passIDs[p];
            
                if( p > 0 ) {    
                    startAddingToStencil( false, true );
                    }
                
                if( p == 1 ) {    
                    drawRect( pos.x - hugR, pos.y + hugR, 
                              pos.x, pos.y - hugR );
                    }
                else if( p == 2 ) {
                        
                    drawRect( pos.x, pos.y + hugR, 
                              pos.x + hugR, pos.y - hugR );
                    }

                if( p > 0 ) {
                    startDrawingThroughStencil();
                    }
                

            
                if( !mapPullMode ) {                    
                    handleAnimSound( oID, 0, ground, oldFrameCount, 
                                     mMapFloorAnimationFrameCount[ mapI ],
                                     (double)screenX / CELL_D,
                                     (double)screenY / CELL_D );
                    }
            
                double timeVal = frameRateFactor * 
                    mMapFloorAnimationFrameCount[ mapI ] / 60.0;
                

                if( p > 0 ) {
                    // floor hugging pass
                    // only draw bottom layer of floor
                    setAnimLayerCutoff( 1 );
                    }
                
                
                char used;
                drawObjectAnim( oID, 2, 
                                ground, timeVal,
                                0,
                                ground, 
                                timeVal,
                                timeVal,
                                &used,
                                ground,
                                ground,
                                pos, 0,
                                false,
                                false, -1,
                                false, false, false,
                                getEmptyClothingSet(), NULL );

                if( p > 0 ) {
                    stopStencil();
                    }
                }
            
            if( passIDs[1] != passIDs[2] ) {
                setDrawColor( 1, 1, 1, 1 );
                pos.y += 10;
                drawSprite( mFloorSplitSprite, pos );
                }
            }
        }
    


    // draw overlay evenly over all floors and biomes
    doublePair groundCenterPos;

    int groundWTile = getSpriteWidth( mGroundOverlaySprite[0] );
    int groundHTile = getSpriteHeight( mGroundOverlaySprite[0] );
    
    int groundW = groundWTile * 2;
    int groundH = groundHTile * 2;

    groundCenterPos.x = lrint( lastScreenViewCenter.x / groundW ) * groundW;
    groundCenterPos.y = lrint( lastScreenViewCenter.y / groundH ) * groundH;

    toggleMultiplicativeBlend( true );
    
    // use this to lighten ground overlay
    toggleAdditiveTextureColoring( true );
    setDrawColor( multAmount, multAmount, multAmount, 1 );
    
    for( int y=-1; y<=1; y++ ) {

        doublePair pos = groundCenterPos;

        pos.y = groundCenterPos.y + y * groundH;

        for( int x=-1; x<=1; x++ ) {

            pos.x = groundCenterPos.x + x * groundW;
            
            if( drawMult ) {
                for( int t=0; t<4; t++ ) {
                    
                    doublePair posTile = pos;
                    if( t % 2 == 0 ) {
                        posTile.x -= groundWTile / 2;
                        }
                    else {
                        posTile.x += groundWTile / 2;
                        }
                    
                    if( t < 2 ) {
                        posTile.y += groundHTile / 2;
                        }
                    else {
                        posTile.y -= groundHTile / 2;
                        }
                    
                    drawSprite( mGroundOverlaySprite[t], posTile );
                    }
                }            
            }
        }
    toggleAdditiveTextureColoring( false );
    toggleMultiplicativeBlend( false );


    toggleAdditiveBlend( true );
    
    // use this to lighten ground overlay
    //toggleAdditiveTextureColoring( true );
    setDrawColor( 1, 1, 1, addAmount );
    
    for( int y=-1; y<=1; y++ ) {

        doublePair pos = groundCenterPos;
        
        pos.x += 512;
        pos.y += 512;

        pos.y = groundCenterPos.y + y * groundH;

        for( int x=-1; x<=1; x++ ) {

            pos.x = groundCenterPos.x + x * groundW;

            if( drawAdd )  {
                for( int t=0; t<4; t++ ) {
                    
                    doublePair posTile = pos;
                    if( t % 2 == 0 ) {
                        posTile.x -= groundWTile / 2;
                        }
                    else {
                        posTile.x += groundWTile / 2;
                        }
                    
                    if( t < 2 ) {
                        posTile.y += groundHTile / 2;
                        }
                    else {
                        posTile.y -= groundHTile / 2;
                        }
                    
                    drawSprite( mGroundOverlaySprite[t], posTile );
                    }
                }
            }
        }
    //toggleAdditiveTextureColoring( false );
    toggleAdditiveBlend( false );
    
    
    float maxFullCellFade = 0.5;
    float maxEmptyCellFade = 0.75;


    if( mShowHighlights 
        &&
        mCurMouseOverCellFade > 0 
        &&
        mCurMouseOverCell.x >= 0 && mCurMouseOverCell.x < mMapD
        &&
        mCurMouseOverCell.y >= 0 && mCurMouseOverCell.y < mMapD ) {
        
        int worldY = mCurMouseOverCell.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( mCurMouseOverCell.x + mMapOffsetX - mMapD / 2 );        
        
        int mapI = mCurMouseOverCell.y * mMapD + mCurMouseOverCell.x;
        
        int id = mMap[mapI];
        
        float fill = 0;
        float border = 1;        
        char drawFill = false;
        float maxFade = maxEmptyCellFade;
        
        if( id > 0 ) {
            fill = 1;
            border = 0;
            drawFill = true;
            maxFade = maxFullCellFade;
            }


        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;

        if( drawFill ) {    
            setDrawColor( fill, fill, fill, maxFade * mCurMouseOverCellFade );
            drawSprite( mCellFillSprite, cellPos );
            }
        
        
        setDrawColor( border, border, border,
                      0.75  * maxFade * mCurMouseOverCellFade );
        drawSprite( mCellBorderSprite, cellPos );        
        }

    
    if( mShowHighlights )
    for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
        float fade = mPrevMouseOverCellFades.getElementDirect( i );
        
        if( fade <= 0 ) {
            continue;
            }

        GridPos prev = mPrevMouseOverCells.getElementDirect( i );
        
        if( prev.x < 0 || prev.x >= mMapD
            ||
            prev.y < 0 || prev.y >= mMapD ) {
            
            continue;
            }
        
            
        int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );        

        
        int mapI = prev.y * mMapD + prev.x;
        
        int id = mMap[mapI];
        
        float fill = 0;
        float border = 1;
        char drawFill = false;
        float maxFade = maxEmptyCellFade;
                
        if( id > 0 ) {
            fill = 1;
            border = 0;
            drawFill = true;
            maxFade = maxFullCellFade;
            }

            
        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;
        
        if( drawFill ) {    
            setDrawColor( fill, fill, fill, maxFade * fade );
            drawSprite( mCellFillSprite, cellPos );
            }
        
        setDrawColor( border, border, border, 0.75 * maxFade * fade );
        drawSprite( mCellBorderSprite, cellPos );
        }




    if( mShowHighlights )
    for( int i=0; i<mPrevMouseClickCells.size(); i++ ) {
        float fade = mPrevMouseClickCellFades.getElementDirect( i );
        
        if( fade <= 0 ) {
            continue;
            }

        GridPos prev = mPrevMouseClickCells.getElementDirect( i );
        
        if( prev.x < 0 || prev.x >= mMapD
            ||
            prev.y < 0 || prev.y >= mMapD ) {
            
            continue;
            }
        
            
        int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int screenX = 
            CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );
        
        float border = 1;
        float maxFade = maxEmptyCellFade;
        
            
        doublePair cellPos = { (double)screenX, (double)screenY };
            
        cellPos.x += 2;
        
        setDrawColor( border, border, border, 0.75 * maxFade * fade );
        drawSprite( mCellBorderSprite, cellPos );
        }
    
    
    //int worldXStart = xStart + mMapOffsetX - mMapD / 2;
    //int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;

    //int worldYStart = xStart + mMapOffsetY - mMapD / 2;
    //int worldYEnd = xEnd + mMapOffsetY - mMapD / 2;


    // capture pointers to objects that are speaking and visible on 
    // screen

    // draw speech on top of everything at end

    SimpleVector<LiveObject *> speakers;
    SimpleVector<doublePair> speakersPos;


    // draw long path for our character
    LiveObject *ourLiveObject = getOurLiveObject();
    
    if( ourLiveObject != NULL ) {
        
        if( ourLiveObject->currentPos.x != ourLiveObject->xd 
            || ourLiveObject->currentPos.y != ourLiveObject->yd ) {
            
            if( ourLiveObject->pathToDest != NULL &&
                ourLiveObject->shouldDrawPathMarks &&
                mShowHighlights ) {
                // highlight path

                JenkinsRandomSource pathRand( 340930281 );
                
                GridPos pathSpot = ourLiveObject->pathToDest[ 0 ];
                

                GridPos endGrid = 
                    ourLiveObject->pathToDest[ ourLiveObject->pathLength - 1 ];
                
                doublePair endPos;
                endPos.x = endGrid.x * CELL_D;
                endPos.y = endGrid.y * CELL_D;
                

                doublePair playerPos = mult( ourLiveObject->currentPos,
                                             CELL_D );
                
                double distFromEnd = distance( playerPos, endPos );
                
                float endFade = 1.0f;
                

                if( distFromEnd < 2 * CELL_D ) {
                    endFade = distFromEnd / ( 2 * CELL_D );
                    }


                doublePair curPos;
                
                curPos.x = pathSpot.x * CELL_D;
                curPos.y = pathSpot.y * CELL_D;


                GridPos pathSpotB = ourLiveObject->pathToDest[ 1 ];
                    

                doublePair nextPosB;
                
                nextPosB.x = pathSpotB.x * CELL_D;
                nextPosB.y = pathSpotB.y * CELL_D;
                
                
                doublePair curDir = normalize( sub( nextPosB, curPos ) );
                

                double turnFactor = .25;
                
                int numStepsSinceDrawn = 0;
                int drawOnStep = 6;
                

                for( int p=1; p< ourLiveObject->pathLength; p++ ) {
                
                    
                    GridPos pathSpotB = ourLiveObject->pathToDest[ p ];
                    

                    doublePair nextPos;
                
                    nextPos.x = pathSpotB.x * CELL_D;
                    nextPos.y = pathSpotB.y * CELL_D;
                    
                    int closeDist = 60;
                    
                    if( p == ourLiveObject->pathLength - 1 ) {
                        closeDist = 20;
                        }
                    
                    
                    while( distance( curPos, nextPos ) > closeDist  ) {
    
                        doublePair dir = normalize( sub( nextPos, curPos ) );

                        
                        if( dot( dir, curDir ) >= 0 ) {
                            curDir = 
                                normalize( 
                                    add( curDir, 
                                         mult( dir, turnFactor ) ) );
                        
                            }
                        else {
                            // path doubles back on itself
                            // smooth turning never resolves here
                            // just make a sharp point in path instead
                            curDir = dir;
                            }
                        
                        curPos = add( curPos,
                                      mult( curDir, 6 ) );
                        
                        setDrawColor( 0, 0, 0, 
                                      ourLiveObject->pathMarkFade * endFade );
                        

                        doublePair drawPos = curPos;
                        
                        if( numStepsSinceDrawn == 0 ) {
                            
                            drawSprite( mPathMarkSprite, drawPos, 1.0, 
                                    -angle( curDir ) / ( 2 * M_PI ) + .25  );
                            }
                        
                        numStepsSinceDrawn ++;
                        if( numStepsSinceDrawn == drawOnStep ) {
                            numStepsSinceDrawn = 0;
                            }
                        }
                    }
                
                if( ourLiveObject->pathMarkFade < 1 ) {
                    ourLiveObject->pathMarkFade += 0.1 * frameRateFactor;
                    
                    if( ourLiveObject->pathMarkFade > 1 ) {
                        ourLiveObject->pathMarkFade = 1;
                        }
                    }
                }
            }
        else {
            ourLiveObject->pathMarkFade = 0;
            }
        }
    

    // FIXME:  skip these that are off screen
    // of course, we may not end up drawing paths on screen anyway
    // (probably only our own destination), so don't fix this for now

    // draw paths and destinations under everything
    
    // debug overlay
    if( false )
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );


        if( o->currentPos.x != o->xd || o->currentPos.y != o->yd ) {
            // destination
            
            char *string = autoSprintf( "[%c]", o->displayChar );
        
            doublePair pos;
            pos.x = o->xd * CELL_D;
            pos.y = o->yd * CELL_D;
        
            setDrawColor( 1, 0, 0, 1 );
            mainFont->drawString( string, 
                                  pos, alignCenter );
            delete [] string;


            if( o->pathToDest != NULL ) {
                // highlight path
                
                for( int p=0; p< o->pathLength; p++ ) {
                    GridPos pathSpot = o->pathToDest[ p ];
                    
                    pos.x = pathSpot.x * CELL_D;
                    pos.y = pathSpot.y * CELL_D;

                    setDrawColor( 1, 1, 0, 1 );
                    mainFont->drawString( "P", 
                                          pos, alignCenter );
                    }

                // draw waypoint
                
                setDrawColor( 0, 1, 1, 1 );
                pos.x = o->waypointX * CELL_D;
                pos.y = o->waypointY * CELL_D;
                mainFont->drawString( "W", 
                                      pos, alignCenter );
                    
                }
            else {
                pos.x = o->closestDestIfPathFailedX * CELL_D;
                pos.y = o->closestDestIfPathFailedY * CELL_D;
                
                setDrawColor( 1, 0, 1, 1 );
                mainFont->drawString( "P", 
                                      pos, alignCenter );
                }
            
            
            }
        }

    
    

    // reset flags so that only drawn objects get flagged
    for( int i=0; i<gameObjects.size(); i++ ) {
        gameObjects.getElement( i )->onScreen = false;
        }
    
    int numMoving = 0;
    int movingObjectsIndices[ MAP_NUM_CELLS ];
    
    // prevent double-drawing when cell goes from moving to non-moving
    // mid-draw
    char cellDrawn[ MAP_NUM_CELLS ];
    
    memset( cellDrawn, false, MAP_NUM_CELLS );
    
    for( int y=0; y<mMapD; y++ ) {
        for( int x=0; x<mMapD; x++ ) {
            int mapI = y * mMapD + x;
            
            if( mMap[ mapI ] > 0 &&
                mMapMoveSpeeds[ mapI ] > 0 ) {
                
                movingObjectsIndices[ numMoving ] = mapI;

                numMoving++;
                }
            }
        }
    

    for( int y=yEnd; y>=yStart; y-- ) {
        
        int worldY = y + mMapOffsetY - mMapD / 2;

        int screenY = CELL_D * worldY;
        

        // draw marked objects behind everything else, including players
        
        for( int x=xStart; x<=xEnd; x++ ) {
            
            int worldX = x + mMapOffsetX - mMapD / 2;


            int mapI = y * mMapD + x;

            if( cellDrawn[mapI] ) {
                continue;
                }

            int screenX = CELL_D * worldX;
            
            if( mMap[ mapI ] > 0 && 
                getObject( mMap[ mapI ] )->drawBehindPlayer &&
                mMapMoveSpeeds[ mapI ] == 0 ) {
                
                drawMapCell( mapI, screenX, screenY );
                cellDrawn[mapI] = true;
                }

            

            /*
              // debugging grid

            doublePair cellPos = { (double)screenX, (double)screenY };

            if( mMap[ mapI ] < 0 ) {
                setDrawColor( 0, 0, 0, 0.5 );
                }
            else {
                if( lrint( abs(worldY) ) % 2 == 0 ) {
                    if( lrint( abs(worldX) ) % 2 == 1 ) {
                        setDrawColor( 1, 1, 1, 0.25 );
                        }
                    else {
                        setDrawColor( 0, 0, 0, 0.25 );
                        }
                    }
                else {
                    if( lrint( abs(worldX) ) % 2 == 1 ) {
                        setDrawColor( 0, 0, 0, 0.25 );
                        }
                    else {
                        setDrawColor( 1, 1, 1, 0.25 );
                        }
                    }
                
                }
            
            doublePair cellPos = { (double)screenX, (double)screenY };
            drawSquare( cellPos, CELL_D / 2 );
            
            FloatColor c = getDrawColor();

            c.r = 1 - c.r;
            c.g = 1 - c.g;
            c.b = 1 - c.b;
            c.a = .75;
            
            setDrawColor( c );
            
            char *xString =  autoSprintf( "x:%d", worldX );
            char *yString =  autoSprintf( "y:%d", worldY );
            
            doublePair xDrawPos = cellPos;
            doublePair yDrawPos = cellPos;
            
            xDrawPos.y += CELL_D / 6;
            yDrawPos.y -= CELL_D / 6;
            
            xDrawPos.x -= CELL_D / 3;
            yDrawPos.x -= CELL_D / 3;
            

            mainFont->drawString( xString, xDrawPos, alignLeft );
            mainFont->drawString( yString, yDrawPos, alignLeft );
            
            delete [] yString;
            delete [] xString;
            */
            }

        
        SimpleVector<ObjectAnimPack> heldToDrawOnTop;

        // sorted queue of players and moving objects in this row
        // build it, then draw them in sorted order
        MinPriorityQueue<DrawOrderRecord> drawQueue;

        // draw players behind the objects in this row

        // run this loop twice, once for
        // adults, and then afterward for recently-dropped babies
        // that are still sliding into place (so that they remain
        // visibly on top of the adult who dropped them
        for( int d=0; d<2; d++ )
        for( int x=xStart; x<=xEnd; x++ ) {
            int worldX = x + mMapOffsetX - mMapD / 2;


            for( int i=0; i<gameObjects.size(); i++ ) {
        
                LiveObject *o = gameObjects.getElement( i );
                
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, don't draw now
                    continue;
                    }

                if( d == 0 &&
                    ( o->heldByDropOffset.x != 0 ||
                      o->heldByDropOffset.y != 0 ) ) {
                    // recently dropped baby, skip
                    continue;
                    }
                else if( d == 1 &&
                         o->heldByDropOffset.x == 0 &&
                         o->heldByDropOffset.y == 0 ) {
                    // not a recently-dropped baby, skip
                    continue;
                    }
                
                
                int oX = o->xd;
                int oY = o->yd;
                
                if( o->currentSpeed != 0 ) {
                    oX = lrint( o->currentPos.x );
                    oY = lrint( o->currentPos.y - 0.20 );
                    }
                
                
                if( oY == worldY && oX == worldX ) {
                    
                    // there's a player here, insert into sort queue
                    
                    DrawOrderRecord drawRec;
                    drawRec.person = true;
                    drawRec.personO = o;
                    

                    double depth = 0 - o->currentPos.y;
                    
                    if( lrint( depth ) - depth == 0 ) {
                        // break ties (co-occupied cells) by drawing 
                        // younger players in front
                        // (so that babies born appear in front of 
                        //  their mothers)

                        // vary by a tiny amount, so we don't change
                        // the way they are sorted relative to other objects
                        depth += ( 60.0 - o->age ) / 6000.0;
                        }
                    
                    drawQueue.insert( drawRec, depth );
                    }
                }
            }
        
        // now sort moving objects that fall in this row
        for( int i=0; i<numMoving; i++ ) {
            
            int mapI = movingObjectsIndices[i];
            
            if( cellDrawn[mapI] ) {
                continue;
                }
            

            int oX = mapI % mMapD;
            int oY = mapI / mMapD;
            
            int movingX = lrint( oX + mMapMoveOffsets[mapI].x );

            double movingTrueCellY = oY + mMapMoveOffsets[mapI].y;
            
            double movingTrueY =  movingTrueCellY - 0.1;
            
            int movingCellY = lrint( movingTrueCellY - 0.40 );

            if( movingCellY == y && movingX >= xStart && movingX <= xEnd ) {
                
                int movingScreenX = CELL_D * ( oX + mMapOffsetX - mMapD / 2 );
                int movingScreenY = CELL_D * ( oY + mMapOffsetY - mMapD / 2 );
                
                double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;
                
                
                //drawMapCell( mapI, movingScreenX, movingScreenY );
                
                // add to depth sorting queue
                DrawOrderRecord drawRec;
                drawRec.person = false;
                drawRec.extraMovingObj = false;
                drawRec.mapI = mapI;
                drawRec.screenX = movingScreenX;
                drawRec.screenY = movingScreenY;
                drawQueue.insert( drawRec, 0 - worldMovingY );
                
                cellDrawn[mapI] = true;
                }
            }



        

        // now sort extra moving objects that fall in this row
        
        int worldXStart = xStart + mMapOffsetX - mMapD / 2;
        int worldXEnd = xEnd + mMapOffsetX - mMapD / 2;
        
        for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {
            
            GridPos movingWorldPos = 
                mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );

            ExtraMapObject *extraO = 
                mMapExtraMovingObjects.getElement( i );

            
            int movingX = lrint( movingWorldPos.x + extraO->moveOffset.x );

            double movingTrueCellY = movingWorldPos.y + extraO->moveOffset.y;
            
            double movingTrueY =  movingTrueCellY - 0.2;
            
            int movingCellY = lrint( movingTrueCellY - 0.50 );

            if( movingCellY == worldY && 
                movingX >= worldXStart && 
                movingX <= worldXEnd ) {
                
                int mapX = movingWorldPos.x - mMapOffsetX + mMapD / 2;
                int mapY = movingWorldPos.y - mMapOffsetY + mMapD / 2;
                    
                int mapI = mapY * mMapD + mapX;


                int movingScreenX = CELL_D * movingWorldPos.x;
                int movingScreenY = CELL_D * movingWorldPos.y;
                
                double worldMovingY =  movingTrueY + mMapOffsetY - mMapD / 2;
                
                
                //drawMapCell( mapI, movingScreenX, movingScreenY );
                
                // add to depth sorting queue
                DrawOrderRecord drawRec;
                drawRec.person = false;
                drawRec.extraMovingObj = true;
                drawRec.extraMovingIndex = i;
                drawRec.mapI = mapI;
                drawRec.screenX = movingScreenX;
                drawRec.screenY = movingScreenY;
                
                drawQueue.insert( drawRec, 0 - worldMovingY );
                }
            }




        
        // now move through queue in order, drawing
        int numQueued = drawQueue.size();
        for( int q=0; q<numQueued; q++ ) {
            DrawOrderRecord drawRec = drawQueue.removeMin();
            
            if( drawRec.person ) {
                LiveObject *o = drawRec.personO;
                
                ObjectAnimPack heldPack =
                    drawLiveObject( o, &speakers, &speakersPos );
                
                if( heldPack.inObjectID != -1 ) {
                    // holding something, not drawn yet
                    
                    if( ! o->heldPosOverride ) {
                        // not sliding into place
                        // draw it now
                        drawObjectAnim( heldPack );
                        }
                    else {
                        heldToDrawOnTop.push_back( heldPack );
                        }
                    }
                }
            else if( drawRec.extraMovingObj ) {
                ExtraMapObject *mO = mMapExtraMovingObjects.getElement(
                    drawRec.extraMovingIndex );
                
                // hold non-moving dest object
                ExtraMapObject curO = copyFromMap( drawRec.mapI );
                
                // temporarily insert extra object for drawing
                putInMap( drawRec.mapI, mO );

                drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY );

                // copy back out of map to preserve effects of draw call
                // (frame count update, etc.)
                *mO = copyFromMap( drawRec.mapI );

                // put original one back
                putInMap( drawRec.mapI, &curO );
                }
            else {
                drawMapCell( drawRec.mapI, drawRec.screenX, drawRec.screenY );
                }
            }
        

        // now draw non-behind-marked map objects in this row
        // OVER the player objects in this row (so that pick up and set down
        // looks correct, and so players are behind all same-row objects)
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            
            if( cellDrawn[ mapI ] ) {
                continue;
                }
            
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 && 
                ! getObject( mMap[ mapI ] )->drawBehindPlayer &&
                mMapMoveSpeeds[ mapI ] == 0 ) {
                
                drawMapCell( mapI, screenX, screenY );
                cellDrawn[ mapI ] = true;
                }
            }

        // now draw held flying objects on top of objects in this row
        for( int i=0; i<heldToDrawOnTop.size(); i++ ) {
            drawObjectAnim( heldToDrawOnTop.getElementDirect( i ) );
            }

        } // end loop over rows on screen
    

    // finally, draw any highlighted our-placements
    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf && mCurMouseOverBehind ) {
        int worldY = mCurMouseOverSpot.y + mMapOffsetY - mMapD / 2;
        
        int screenY = CELL_D * worldY;
        
        int mapI = mCurMouseOverSpot.y * mMapD + mCurMouseOverSpot.x;
        int screenX = 
            CELL_D * ( mCurMouseOverSpot.x + mMapOffsetX - mMapD / 2 );
        
        // highlights only
        drawMapCell( mapI, screenX, screenY, true );
        }
    
    for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
        if( mPrevMouseOverSpotsBehind.getElementDirect( i ) ) {
                
            GridPos prev = mPrevMouseOverSpots.getElementDirect( i );
            
            int worldY = prev.y + mMapOffsetY - mMapD / 2;
        
            int screenY = CELL_D * worldY;
        
            int mapI = prev.y * mMapD + prev.x;
            int screenX = 
                CELL_D * ( prev.x + mMapOffsetX - mMapD / 2 );
        
            // highlights only
            drawMapCell( mapI, screenX, screenY, true );
            }
        }
    
    
    
    for( int i=0; i<speakers.size(); i++ ) {
        LiveObject *o = speakers.getElementDirect( i );
        
        doublePair pos = speakersPos.getElementDirect( i );
        
        
        doublePair speechPos = pos;

        speechPos.y += 84;

        ObjectRecord *displayObj = getObject( o->displayID );
 

        double age = computeCurrentAge( o );
        
        doublePair headPos = 
            displayObj->spritePos[ getHeadIndex( displayObj, age ) ];
        
        doublePair bodyPos = 
            displayObj->spritePos[ getBodyIndex( displayObj, age ) ];

        doublePair frontFootPos = 
            displayObj->spritePos[ getFrontFootIndex( displayObj, age ) ];
        
        headPos = add( headPos, 
                       getAgeHeadOffset( age, headPos, 
                                         bodyPos, frontFootPos ) );
        headPos = add( headPos,
                       getAgeBodyOffset( age, bodyPos ) );
        
        speechPos.y += headPos.y;
        
        int width = 250;
        int widthLimit = 250;
        
        double fullWidth = 
            handwritingFont->measureString( o->currentSpeech );
        
        if( fullWidth < width ) {
            width = (int)fullWidth;
            }
        
        
        speechPos.x -= width / 2;

        
        drawChalkBackgroundString( speechPos, o->currentSpeech, 
                                   o->speechFade, widthLimit,
                                   o );
        }
    

    /*
      // for debugging home location

    ClickRecord *home = getHomeLocation();
    
    if( home != NULL ) {
        setDrawColor( 1, 0, 0, 0.5 );
        
        int screenY = CELL_D * home->y;
        
        int screenX = CELL_D * home->x;
        
        doublePair pos = { (double)screenX, (double)screenY };
        
        drawSquare( pos, CELL_D * .45 );

        int startX = CELL_D * ourLiveObject->currentPos.x;
        int startY = CELL_D * ourLiveObject->currentPos.y;
        
        doublePair start = { (double)startX, (double)startY };

        doublePair delta = sub( pos, start );
        
        int numSteps = length( delta ) / CELL_D;

        delta = mult( delta, 1.0 / numSteps );
        
        for( int i=0; i<numSteps; i++ ) {
            
            pos = add( start, mult( delta, i ) );
            
            drawSquare( pos, CELL_D * .25 );
            }
        }
    */
    
    /*
    // for debugging click counts
    for( int i=0; i<NUM_CLICK_RECORDS; i++ ) {
        if( clicks[i].count > 0 ) {
            setDrawColor( 1, 0, 0, 1 );
            
            doublePair pos;
            pos.x = clicks[i].x * CELL_D;
            pos.y = clicks[i].y * CELL_D;
            
            char *string = autoSprintf( "%d", clicks[i].count );
            
            mainFont->drawString( string, pos, alignCenter );
            
            delete [] string;
            }
        }
    */

        
    //doublePair lastChunkCenter = { (double)( CELL_D * mMapOffsetX ), 
    //                               (double)( CELL_D * mMapOffsetY ) };
    
    setDrawColor( 0, 1, 0, 1 );
    
    // debug overlay
    //mainFont->drawString( "X", 
    //                      lastChunkCenter, alignCenter );
    
    




    
    if( false )
    for( int i=0; i<trail.size(); i++ ) {
        doublePair *p = trail.getElement( i );

        setDrawColor( trailColors.getElementDirect( i ) );

        mainFont->drawString( ".", 
                              *p, alignCenter );
        }
        

    setDrawColor( 0, 0, 0, 0.125 );
    
    int screenGridOffsetX = lrint( lastScreenViewCenter.x / CELL_D );
    int screenGridOffsetY = lrint( lastScreenViewCenter.y / CELL_D );
    
    // debug overlay
    if( false )
    for( int y=-5; y<=5; y++ ) {
        for( int x=-8; x<=8; x++ ) {
            
            doublePair pos;
            pos.x = ( x + screenGridOffsetX ) * CELL_D;
            pos.y = ( y + screenGridOffsetY ) * CELL_D;
            
            drawSquare( pos, CELL_D * .45 );
            }
        }    
    
    if( mapPullMode ) {
        
        float progress;
        
        if( ! mapPullCurrentSaved && 
            isLiveObjectSetFullyLoaded( &progress ) ) {
            
            int screenWidth, screenHeight;
            getScreenDimensions( &screenWidth, &screenHeight );
            
            Image *screen = 
                getScreenRegionRaw( 0, 0, screenWidth, screenHeight );

            int startX = lastScreenViewCenter.x - screenW / 2;
            int startY = lastScreenViewCenter.y + screenH / 2;
            startY = -startY;
            double scale = (double)screenWidth / (double)screenW;
            startX = lrint( startX * scale );
            startY = lrint( startY * scale );

            int totalW = mapPullTotalImage->getWidth();
            int totalH = mapPullTotalImage->getHeight();
            
            int totalImStartX = startX + totalW / 2;
            int totalImStartY = startY + totalH / 2;

            double gridCenterOffsetX = ( mapPullEndX + mapPullStartX ) / 2.0;
            double gridCenterOffsetY = ( mapPullEndY + mapPullStartY ) / 2.0;
            
            totalImStartX -= lrint( gridCenterOffsetX * CELL_D  * scale );
            totalImStartY += lrint( gridCenterOffsetY * CELL_D * scale );
            
            //totalImStartY =  totalH - totalImStartY;

            if( totalImStartX >= 0 && totalImStartX < totalW &&
                totalImStartY >= 0 && totalImStartY < totalH ) {
                
                mapPullTotalImage->setSubImage( totalImStartX,
                                                totalImStartY,
                                                screenWidth, screenHeight,
                                                screen );
                numScreensWritten++;
                }
            

            delete screen;
            
            mapPullCurrentSaved = true;
            
            if( mapPullModeFinalImage ) {
                mapPullMode = false;

                writeTGAFile( "mapOut.tga", mapPullTotalImage );
                delete mapPullTotalImage;
                

                // pull over

                // auto-quit
                printf( "Map pull done, auto-quitting game\n" );
                quitGame();
                }
            }
        
        // skip gui
        return;
        }


    // special mode for teaser video
    if( teaserVideo ) {
        //setDrawColor( 1, 1, 1, 1 );
        //drawRect( lastScreenViewCenter, 640, 360 );
        
        // two passes
        // first for arrows, second for labels
        // make sure arrows don't cross labels
        for( int pass=0; pass<2; pass++ )
        for( int y=yEnd; y>=yStart; y-- ) {
        
            int worldY = y + mMapOffsetY - mMapD / 2;
            //printf( "World y = %d\n", worldY );
            
            if( worldY == 1 || worldY == 0 || worldY == -10 ) {

                int xLimit = 0;
                
                if( worldY == -10 ) {
                    xLimit = -20;
                    }

                int screenY = CELL_D * worldY;
        
                
                for( int x=xStart; x<=xEnd; x++ ) {
            
                    int worldX = x + mMapOffsetX - mMapD / 2;

                    if( worldX >= xLimit ) {
                        
                        int mapI = y * mMapD + x;
                    
                        int screenX = CELL_D * worldX;
                        
                        int arrowTipX = screenX;
                        int arrowTipY = screenY;
                        
                        const char *baseDes = NULL;
                        
                        char numbered = false;
                        
                        if( mMap[ mapI ] > 0 ) {
                            ObjectRecord *o = getObject( mMap[mapI] );
                            
                            baseDes = o->description;
                            
                            // don't number natural objects
                            // or objects that occur before the
                            // start of our line
                            // or use dummies (berry bush is picked from
                            // at end of teaser video).
                            numbered = ( o->mapChance == 0 && worldX > 10 
                                         &&
                                         ! o->isUseDummy );
                            }
                        else if( worldX == -20 && worldY == -10 &&
                                 ( gameObjects.size() == 1 ||
                                   gameObjects.size() == 2 ) ) {
                            baseDes = "PLAYER ONE";
                            LiveObject *o =  gameObjects.getElement( 0 );
                            arrowTipX = o->currentPos.x * CELL_D;
                            arrowTipY += 64;
                            }
                        else if( worldX == -10 && worldY == -10 &&
                                 gameObjects.size() == 3 ) {
                            baseDes = "PLAYER TWO";
                            // point to parent location
                            // they pick up baby and keep walking
                            LiveObject *o =  gameObjects.getElement( 0 );
                            arrowTipX = o->currentPos.x * CELL_D;
                            arrowTipY += 64;
                            }
                        
                        
                        if( baseDes != NULL ) {
                            
                            doublePair labelPos;

                            
                            // speeds are center weights picked from
                            // 7/8, 8/8, or 9/8
                            // 8/8 means the label sticks to the x-center
                            // of the screen.
                            // 7/8 means it lags behind the center a bit
                            // 9/8 means it moves faster than the center a bit
                            double centerWeight = 8.0/8.0;
                            

                            // pick a new speed order every 6 tiles
                            double randOrder =
                                getXYRandom( worldX/6, worldY + 300 );
                            
                            int orderOffsets[3] = { 0, 2, 4 };
                            
                            // even distribution among 0,1,2
                            int orderIndex = 
                                lrint( ( randOrder * 2.98 ) - 0.49 );
                            
                            int orderOffset = orderOffsets[ orderIndex ];
                            
                            int worldXOffset = worldX - 11 + orderOffset;

                            if( ( worldX - 11 ) % 2 == 0 ) {
                                
                                if( worldXOffset % 6 == 0 ) {    
                                    centerWeight = 8.0 / 8.0;
                                    }
                                else if( worldXOffset % 6 == 2 ) {
                                    centerWeight = 7.0/8.0;
                                    }
                                else if( worldXOffset % 6 == 4 ) {
                                    centerWeight = 9.0/8.0;
                                    }
                                }
                            else {
                                
                                if( worldXOffset % 6 == 1 ) {
                                    centerWeight = 7.0 / 8.0;
                                    }
                                else if( worldXOffset % 6 == 3 ) {
                                    centerWeight = 8.0/8.0;
                                    }
                                else if( worldXOffset % 6 == 5 ) {
                                    centerWeight = 6.0/8.0;
                                    }
                                }
                            

                            labelPos.x = 
                                (1 - centerWeight ) * screenX + 
                                centerWeight * lastScreenViewCenter.x;
                            
                            char labelAbove = false;

                            double xWiggle = 
                                getXYRandom( worldX, 924 + worldY );
                            
                            labelPos.x += lrint( (xWiggle - 0.5) *  512 );


                            labelPos.y = lastScreenViewCenter.y;

                            double yWiggle = 
                                getXYRandom( worldX, 29 + worldY );
                            
                            labelPos.y += lrint( (yWiggle - 0.5) * 32 );
                            
                            SpriteHandle arrowSprite = mTeaserArrowMedSprite;

                            if( worldY != 0 ||
                                ( worldX > 0 && worldX < 7 ) ) {
                                
                                if( ( worldX - 11 ) % 2 == 0 ) {
                                    labelAbove = true;
                                    labelPos.y += 214;
                                    
                                    if( ( worldX - 11 ) % 6 == 2 ) {
                                        labelPos.y -= 106;
                                        arrowSprite = 
                                            mTeaserArrowVeryShortSprite;
                                        }
                                    else if( ( worldX - 11 ) % 6 == 4 ) {
                                        labelPos.y += 86;
                                        arrowSprite = mTeaserArrowLongSprite;
                                        
                                        // prevent top row from
                                        // going off top of screen
                                        if( labelPos.y - 
                                            lastScreenViewCenter.y
                                            > 224 + 86 ) {
                                            labelPos.y = lastScreenViewCenter.y
                                                + 224 + 86;
                                            }
                                        }
                                    }
                                else {
                                    labelPos.y -= 202;
                                    arrowTipY -= 48;
                                    
                                    if( ( worldX - 11 ) % 6 == 3 ) {
                                        labelPos.y -= 106;
                                        arrowSprite = mTeaserArrowLongSprite;
                                        }
                                    else if( ( worldX - 11 ) % 6 == 5 ) {
                                        labelPos.y += 86;
                                        arrowSprite = mTeaserArrowShortSprite;
                                        }
                                    }        
                                }
                            else {
                                if( worldX % 2 == 0 ) {
                                    labelPos.y = lastScreenViewCenter.y + 224;
                                    labelAbove = true;
                                    }
                                else {
                                    labelPos.y = lastScreenViewCenter.y - 224;
                                    labelAbove = false;
                                    }
                                }
                            
                            // special case loincloth for baby
                            if( worldX == -1 && worldY == -10 ) {
                                labelPos.y = lastScreenViewCenter.y - 224;
                                labelAbove = false;
                                arrowTipY -= 48;
                                }
                            // phonograph sleve
                            if( worldX == 3 && worldY == 0 ) {
                                labelPos.y = lastScreenViewCenter.y - 224;
                                labelAbove = false;
                                arrowTipY -= 48;
                                arrowSprite = mTeaserArrowVeryShortSprite;
                                }
                            // phonograph
                            if( worldX == 4 && worldY == 0 ) {
                                labelPos.y = lastScreenViewCenter.y + 224;
                                labelAbove = true;
                                labelPos.x += 256;
                                arrowTipY += 48;
                                }
                            
                            double fade = 0;
                            
                            if( fabs( lastScreenViewCenter.x - screenX )
                                < 400 ) {
                                
                                if( fabs( lastScreenViewCenter.x - screenX )
                                    < 300 ) {
                                    fade = 1;
                                    }
                                else {
                                    fade = 
                                        ( 400 - fabs( lastScreenViewCenter.x - 
                                                      screenX ) ) / 100.0;
                                    }
                                }
                            
                            if( fade > 0  ) {
                                if( fabs( lastScreenViewCenter.y - screenY )
                                    > 200 ) {
                                    fade = 0;
                                    }
                                else if( 
                                    fabs( lastScreenViewCenter.y - screenY )
                                    > 100 ) {
                                    fade = ( 200 - 
                                             fabs( lastScreenViewCenter.y - 
                                                   screenY ) ) / 100.0;
                                    }
                                }
                            

                            


                            if( pass == 0 ) {
                                double arrowStart = 27;
                                
                                if( labelAbove ) {
                                    arrowStart = -21;
                                    }
                                
                                double w = 11;
                                
                                double deltaX = arrowTipX - labelPos.x;
                                double deltaY = arrowTipY - 
                                    ( labelPos.y + arrowStart );
                                
                                double a = atan( deltaX / deltaY );
                                
                                a = 0.85 * a;
                                
                                w = w / cos( a );

                                doublePair spriteVerts[4] = 
                                    { { labelPos.x - w, 
                                        labelPos.y + arrowStart },
                                      { labelPos.x + w, 
                                        labelPos.y + arrowStart },
                                      { (double)arrowTipX + w, 
                                        (double)arrowTipY },
                                      { (double)arrowTipX - w, 
                                        (double)arrowTipY } };

                                FloatColor spriteColors[4] = 
                                    { { 1, 1, 1, (float)fade },
                                      { 1, 1, 1, (float)fade },
                                      { 1, 1, 1, (float)fade * .25f },
                                      { 1, 1, 1, (float)fade * .25f } };
                                
                                drawSprite( arrowSprite,
                                            spriteVerts, spriteColors );
                                }
                            else {
                                char *des = stringDuplicate( baseDes );
                                
                                char *poundPos = strstr( des, "#" );
                                
                                if( poundPos != NULL ) {
                                    // terminate at pound
                                    poundPos[0] = '\0';
                                    }
                                
                                SimpleVector<char*> *words = 
                                    tokenizeString( des );
                                
                                delete [] des;

                                SimpleVector<char*> finalWords;
                                for( int i=0; i< words->size(); i++ ) {
                                    char *word = words->getElementDirect( i );
                                    
                                    if( word[0] >= 65 &&
                                        word[0] <= 90 ) {
                                        
                                        char *u = stringToUpperCase( word );
                                        
                                        delete [] word;
                                        finalWords.push_back( u );
                                        }
                                    else {
                                        finalWords.push_back( word );
                                        }
                                    }
                                delete words;
                                
                                char **wordArray = finalWords.getElementArray();
                                
                                char *newDes = join( wordArray,
                                                     finalWords.size(),
                                                     " " );
                                finalWords.deallocateStringElements();
                                delete [] wordArray;
                                
                                
                                char *finalDes;
                                
                                if( worldY == 1 && numbered ) {
                                    finalDes = autoSprintf( 
                                        "%d. %s", 
                                        worldX - 10,
                                        newDes );
                                    }
                                else {
                                    finalDes = stringDuplicate( newDes );
                                    }
                                
                                delete [] newDes;
                                
                                double w = 
                                    mainFontReview->measureString( finalDes );
                                
                                doublePair rectPos = labelPos;
                                rectPos.y += 3;
                                
                                
                                

                                if( blackBorder ) {
                                    
                                    // block hole in border
                                    startAddingToStencil( false, true );
                                    setDrawColor( 1, 1, 1, 1 );
                                    drawRect( 
                                        rectPos, 
                                        w/2 + 
                                        mainFontReview->getFontHeight() / 2, 
                                        24 );
                                    
                                    startDrawingThroughStencil( true );
                                    
                                    setDrawColor( 0, 0, 0, fade );
                                    drawRect( 
                                        rectPos, 
                                        w/2 + 
                                        mainFontReview->getFontHeight() / 2 +
                                        2, 
                                        24 + 2 );
                                    
                                    stopStencil();
                                    }
                                
                                double rectW = 
                                    w/2 + mainFontReview->getFontHeight() / 2;
                                
                                double rectH = 24;
                                
                                setDrawColor( 1, 1, 1, fade );
                                drawRect( rectPos, rectW, rectH );

                                
                                if( whiteBorder ) {

                                    FloatColor lineColor = 
                                        { 1, 1, 1, (float)fade };
                                
                                    doublePair corners[4];
                                    
                                    double rOut = 20;
                                    double rIn = 4;
                                    
                                    

                                    corners[0].x = rectPos.x - rectW - 
                                        getBoundedRandom( worldX, 34,
                                                          rOut, rIn );
                                    corners[0].y = rectPos.y - rectH - 
                                        getBoundedRandom( worldX, 87,
                                                          rOut, rIn );
                                    
                                    
                                    corners[1].x = rectPos.x + rectW + 
                                        getBoundedRandom( worldX, 94,
                                                          rOut, rIn );
                                    corners[1].y = rectPos.y - rectH - 
                                        getBoundedRandom( worldX, 103,
                                                          rOut, rIn );
                                    
                                    
                                    corners[2].x = rectPos.x + rectW + 
                                        getBoundedRandom( worldX, 99,
                                                          rOut, rIn );
                                    corners[2].y = rectPos.y + rectH + 
                                        getBoundedRandom( worldX, 113,
                                                          rOut, rIn );
                                    
                                    
                                    corners[3].x = rectPos.x - rectW - 
                                        getBoundedRandom( worldX, 123,
                                                          rOut, rIn );
                                    corners[3].y = rectPos.y + rectH + 
                                        getBoundedRandom( worldX, 135,
                                                          rOut, rIn );
                                    
                                    
                                    for( int i=0; i<4; i++ ) {
                                        
                                        drawLine( mLineSegmentSprite,
                                                  corners[i], 
                                                  corners[ ( i + 1 ) % 4 ], 
                                                  lineColor );
                                        }
                                    }
                                

                                setDrawColor( 0, 0, 0, fade );
                                mainFontReview->drawString( 
                                    finalDes, 
                                    labelPos,
                                    alignCenter );
                                
                                delete [] finalDes;
                                }
                            }
                        }
                    }
                }
            }
        }
    

    if( apocalypseInProgress ) {
        toggleAdditiveBlend( true );
        
        setDrawColor( 1, 1, 1, apocalypseDisplayProgress );
        
        drawRect( lastScreenViewCenter, 640, 360 );
        
        toggleAdditiveBlend( false );
        }
    
    
    if( hideGuiPanel ) {
        // skip gui
        return;
        }


    doublePair slipPos = add( mHomeSlipPosOffset, lastScreenViewCenter );
    
    if( ! equal( mHomeSlipPosOffset, mHomeSlipHideOffset ) ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mHomeSlipSprite, slipPos );

        
        doublePair arrowPos = slipPos;
        
        arrowPos.y += 35;

        if( ourLiveObject != NULL ) {
            
            int arrowIndex = getHomeDir( ourLiveObject->currentPos );
            
            if( arrowIndex == -1 || ! mHomeArrowStates[arrowIndex].solid ) {
                // solid change

                // fade any solid
                
                int foundSolid = -1;
                for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                    if( mHomeArrowStates[i].solid ) {
                        mHomeArrowStates[i].solid = false;
                        foundSolid = i;
                        }
                    }
                if( foundSolid != -1 ) {
                    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                        if( i != foundSolid ) {
                            mHomeArrowStates[i].fade -= 0.0625;
                            printf( "Fade %d down to %f\n",
                                    i,
                                    mHomeArrowStates[i].fade );
                            
                            if( mHomeArrowStates[i].fade < 0 ) {
                                mHomeArrowStates[i].fade = 0;
                                }
                            }
                        }                
                    }
                }
            
            if( arrowIndex != -1 ) {
                mHomeArrowStates[arrowIndex].solid = true;
                mHomeArrowStates[arrowIndex].fade = 1.0;
                }
            
            toggleMultiplicativeBlend( true );
            
            toggleAdditiveTextureColoring( true );
        
            for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                HomeArrow a = mHomeArrowStates[i];
                
                if( ! a.solid ) {
                    
                    float v = 1.0 - a.fade;
                    setDrawColor( v, v, v, 1 );
                    drawSprite( mHomeArrowErasedSprites[i], arrowPos );
                    }
                }
            
            toggleAdditiveTextureColoring( false );


            
            if( arrowIndex != -1 ) {
                
                
                setDrawColor( 1, 1, 1, 1 );
                
                drawSprite( mHomeArrowSprites[arrowIndex], arrowPos );
                }
                            
            toggleMultiplicativeBlend( false );
            }
        }



    doublePair notePos = add( mNotePaperPosOffset, lastScreenViewCenter );

    if( ! equal( mNotePaperPosOffset, mNotePaperHideOffset ) ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mNotePaperSprite, notePos );
        }
        
    int lineSpacing = 20;

    

    doublePair paperPos = add( mNotePaperPosOffset, lastScreenViewCenter );

    if( mSayField.isFocused() ) {
        char *partialSay = mSayField.getText();

        char *strUpper = stringToUpperCase( partialSay );
        
        delete [] partialSay;

        SimpleVector<char*> *lines = splitLines( strUpper, 345 );
        
        mNotePaperPosTargetOffset.y = mNotePaperHideOffset.y + 58;
        
        if( lines->size() > 1 ) {    
            mNotePaperPosTargetOffset.y += 20 * ( lines->size() - 1 );
            }
        
        doublePair drawPos = paperPos;

        drawPos.x -= 160;
        drawPos.y += 79;


        doublePair drawPosTemp = drawPos;
        

        for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
            char *oldString = mLastKnownNoteLines.getElementDirect( i );
            int oldLen = strlen( oldString );
            
            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    oldString,
                                    drawPosTemp,
                                    alignLeft );
            
            int newLen = 0;
            
            if( i < lines->size() ) {
                // compare lines

                newLen = strlen( lines->getElementDirect( i ) );
                
                }
            

            // any extra chars?
                    
            for( int j=newLen; j<oldLen; j++ ) {
                mErasedNoteChars.push_back( oldString[j] );
                       
                mErasedNoteCharOffsets.push_back(
                    sub( charPos.getElementDirect( j ),
                         paperPos ) );
                
                mErasedNoteCharFades.push_back( 1.0f );
                }
            
            drawPosTemp.y -= lineSpacing;
            }
        mLastKnownNoteLines.deallocateStringElements();
        
        for( int i=0; i<lines->size(); i++ ) {
            mLastKnownNoteLines.push_back( 
                stringDuplicate( lines->getElementDirect(i) ) );
            }
        

    
        delete [] strUpper;

        
        
        setDrawColor( 0, 0, 0, 1 );
        
        mCurrentNoteChars.deleteAll();
        mCurrentNoteCharOffsets.deleteAll();
        
        for( int i=0; i<lines->size(); i++ ) {
            char *line = lines->getElementDirect( i );
            
            pencilFont->drawString( line, drawPos,
                                    alignLeft );

            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    line,
                                    drawPos,
                                    alignLeft );

            int lineSize = strlen( line );
            
            for( int j=0; j<lineSize; j++ ) {
                mCurrentNoteChars.push_back( line[j] );
                mCurrentNoteCharOffsets.push_back( 
                    sub( charPos.getElementDirect( j ), paperPos ) );
                }

            drawPos.y -= lineSpacing;
            }
        lines->deallocateStringElements();
        delete lines;
        }
    else {
        mNotePaperPosTargetOffset = mNotePaperHideOffset;

        doublePair drawPos = paperPos;

        drawPos.x -= 160;
        drawPos.y += 79;

        for( int i=0; i<mLastKnownNoteLines.size(); i++ ) {
            // whole line gone
            
            char *oldString = mLastKnownNoteLines.getElementDirect( i );
            int oldLen = strlen( oldString );
            
            SimpleVector<doublePair> charPos;        
                    
            pencilFont->getCharPos( &charPos, 
                                    oldString,
                                    drawPos,
                                    alignLeft );
                    
            for( int j=0; j<oldLen; j++ ) {
                mErasedNoteChars.push_back( oldString[j] );
                        
                mErasedNoteCharOffsets.push_back(
                    sub( charPos.getElementDirect( j ),
                         paperPos ) );
                
                mErasedNoteCharFades.push_back( 1.0f );
                }
            
            drawPos.y -= lineSpacing;

            }
        mLastKnownNoteLines.deallocateStringElements();
        }
    

    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        if( ! equal( mHintPosOffset[i], mHintHideOffset[i] ) 
            &&
            mHintMessage[i] != NULL ) {
            
            doublePair hintPos  = 
                add( mHintPosOffset[i], lastScreenViewCenter );
            
            hintPos = add( hintPos, mHintExtraOffset[i] );


            char *pageNum = NULL;
            double pageNumExtra = 0;
            
            if( mNumTotalHints[i] > 1 ) {
                
                pageNum = 
                    autoSprintf( "(%d %s %d)",
                                 mHintMessageIndex[i] + 1, 
                                 translate( "ofHint" ),
                                 mNumTotalHints[i] );
            
                double extraA = handwritingFont->measureString( pageNum );
            
                double extraB = 
                    handwritingFont->measureString( translate( "tabHint" ) );
                
                if( extraB > extraA ) {
                    extraA = extraB;
                    }
                
                pageNumExtra = extraA;
                
                hintPos.x -= extraA;
                hintPos.x -= 10;
                }
            

            setDrawColor( 1, 1, 1, 1 );
            drawSprite( mHintSheetSprites[i], hintPos );
            

            setDrawColor( 0, 0, 0, 1.0f );
            double lineSpacing = handwritingFont->getFontHeight() / 2 + 5;
            
            int numLines;
            
            char **lines = split( mHintMessage[i], "#", &numLines );
            
            doublePair lineStart = hintPos;
            lineStart.x -= 280;
            lineStart.y += 30;
            for( int l=0; l<numLines; l++ ) {
                
                if( l == 1 ) {
                    doublePair drawPos = lineStart;
                    drawPos.x -= 5;
                    
                    handwritingFont->drawString( "+",
                                                 drawPos, alignRight );
                    }
                
                if( l == 2 ) {
                    doublePair drawPos = lineStart;
                    drawPos.x -= 5;
                    
                    handwritingFont->drawString( "=",
                                                 drawPos, alignRight );
                    }
                
                handwritingFont->drawString( lines[l], 
                                             lineStart, alignLeft );
                    
                delete [] lines[l];
                
                lineStart.y -= lineSpacing;
                }
            delete [] lines;
            

            if( pageNum != NULL ) {
                
                // now draw tab message
                
                lineStart = hintPos;
                lineStart.x -= 280;
                
                lineStart.x -= mHintExtraOffset[i].x;
                lineStart.x += 20;
                
                lineStart.y += 30;
                
                handwritingFont->drawString( pageNum, lineStart, alignLeft );
                
                delete [] pageNum;
                
                lineStart.y -= 2 * lineSpacing;
                
                lineStart.x += pageNumExtra;
                handwritingFont->drawString( translate( "tabHint" ), 
                                             lineStart, alignRight );
                }
            }
        }



    
    setDrawColor( 0, 0, 0, 1 );
    for( int i=0; i<mErasedNoteChars.size(); i++ ) {
        setDrawFade( mErasedNoteCharFades.getElementDirect( i ) *
                     pencilErasedFontExtraFade );
        
        pencilErasedFont->
            drawCharacterSprite( 
                mErasedNoteChars.getElementDirect( i ), 
                add( paperPos, 
                     mErasedNoteCharOffsets.getElementDirect( i ) ) );
        }



    setDrawColor( 1, 1, 1, 1 );
    
    for( int i=0; i<3; i++ ) { 
        if( !equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            doublePair slipPos = lastScreenViewCenter;
            slipPos = add( slipPos, mHungerSlipPosOffset[i] );
            
            if( mHungerSlipWiggleAmp[i] > 0 ) {
                
                double distFromHidden =
                    mHungerSlipPosOffset[i].y - mHungerSlipHideOffsets[i].y;

                // amplitude grows when we are further from
                // hidden, and shrinks again as we go back down

                double slipHarmonic = 
                    ( 0.5 * ( 1 - cos( mHungerSlipWiggleTime[i] ) ) ) *
                    mHungerSlipWiggleAmp[i] * distFromHidden;
                
                slipPos.y += slipHarmonic;
                

                if( i == 2 ) {
                    
                    if( mStarvingSlipLastPos[0] != 0 &&
                        mStarvingSlipLastPos[1] != 0 ) {
                        double lastDir = mStarvingSlipLastPos[1] - 
                            mStarvingSlipLastPos[0];
                        
                        
                        if( lastDir > 0 ) {
                            
                            double newDir = 
                                slipHarmonic - mStarvingSlipLastPos[1];
                            
                            if( newDir < 0 ) {
                                // peak
                                if( mPulseHungerSound && 
                                    mHungerSound != NULL ) {
                                    // make sure it can be heard, even
                                    // if paused
                                    setSoundLoudness( 1.0 );
                                    playSoundSprite( mHungerSound );
                                    }
                                }
                            }
                        
                        }
                    mStarvingSlipLastPos[0] = mStarvingSlipLastPos[1];
                    
                    mStarvingSlipLastPos[1] = slipHarmonic;
                    }
                }
            

            drawSprite( mHungerSlipSprites[i], slipPos );
            }
        }
    
    // info panel at bottom
    setDrawColor( 1, 1, 1, 1 );
    doublePair panelPos = lastScreenViewCenter;
    panelPos.y -= 242 + 32 + 16 + 6;
    drawSprite( mGuiPanelSprite, panelPos );

    if( ourLiveObject != NULL &&
        ourLiveObject->dying ) {
        toggleMultiplicativeBlend( true );
        doublePair bloodPos = panelPos;
        bloodPos.y -= 32;
        bloodPos.x -= 32;
        drawSprite( mGuiBloodSprite, bloodPos );
        toggleMultiplicativeBlend( false );
        }
    


    if( ourLiveObject != NULL ) {
        setDrawColor( 1, 1, 1, 1 );
        toggleMultiplicativeBlend( true );

        for( int i=0; i<ourLiveObject->foodCapacity; i++ ) {
            doublePair pos = { lastScreenViewCenter.x - 590, 
                               lastScreenViewCenter.y - 334 };
        
            pos.x += i * 30;
            drawSprite( 
                    mHungerBoxSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                
            if( i < ourLiveObject->foodStore ) {                
                drawSprite( 
                    mHungerBoxFillSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            else if( i < ourLiveObject->maxFoodStore ) {
                drawSprite( 
                    mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            }
        for( int i=ourLiveObject->foodCapacity; 
             i < ourLiveObject->maxFoodCapacity; i++ ) {
            doublePair pos = { lastScreenViewCenter.x - 590, 
                               lastScreenViewCenter.y - 334 };
            
            pos.x += i * 30;
            drawSprite( 
                mHungerBoxErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                pos );
            
            if( i < ourLiveObject->maxFoodStore ) {
                drawSprite( 
                    mHungerBoxFillErasedSprites[ i % NUM_HUNGER_BOX_SPRITES ], 
                    pos );
                }
            }
        
        
                
        
        doublePair pos = { lastScreenViewCenter.x + 546, 
                           lastScreenViewCenter.y - 319 };

        if( mCurrentArrowHeat != -1 ) {
            
            if( mCurrentArrowHeat != ourLiveObject->heat ) {
                
                for( int i=0; i<mOldArrows.size(); i++ ) {
                    OldArrow *a = mOldArrows.getElement( i );
                    
                    a->fade -= 0.01;
                    if( a->fade < 0 ) {
                        mOldArrows.deleteElement( i );
                        i--;
                        }
                    }
                

                OldArrow a;
                a.i = mCurrentArrowI;
                a.heat = mCurrentArrowHeat;
                a.fade = 1.0;

                mOldArrows.push_back( a );

                mCurrentArrowI++;
                mCurrentArrowI = mCurrentArrowI % NUM_TEMP_ARROWS;
                }
            }

        toggleAdditiveTextureColoring( true );
        
        for( int i=0; i<mOldArrows.size(); i++ ) {
            doublePair pos2 = pos;
            OldArrow *a = mOldArrows.getElement( i );
            
            float v = 1.0 - a->fade;
            setDrawColor( v, v, v, 1 );
            pos2.x += ( a->heat - 0.5 ) * 120;

            // no sub pixel positions
            pos2.x = round( pos2.x );

            drawSprite( mTempArrowErasedSprites[a->i], pos2 );
            }
        toggleAdditiveTextureColoring( false );
        
        setDrawColor( 1, 1, 1, 1 );

        mCurrentArrowHeat = ourLiveObject->heat;
        
        pos.x += ( mCurrentArrowHeat - 0.5 ) * 120;
        
        // no sub pixel positions
        pos.x = round( pos.x );
        
        drawSprite( mTempArrowSprites[mCurrentArrowI], pos );
        
        toggleMultiplicativeBlend( false );
        

        for( int i=0; i<mOldDesStrings.size(); i++ ) {
            doublePair pos = { lastScreenViewCenter.x, 
                               lastScreenViewCenter.y - 313 };
            float fade =
                mOldDesFades.getElementDirect( i );
            
            setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
            pencilErasedFont->drawString( 
                mOldDesStrings.getElementDirect( i ), pos, alignCenter );
            }


        doublePair atePos = { lastScreenViewCenter.x, 
                              lastScreenViewCenter.y - 347 };
        
        int shortestFill = 100;
        
        
        for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
            float fade =
                mOldLastAteFades.getElementDirect( i );
            
            setDrawColor( 0, 0, 0, fade * pencilErasedFontExtraFade );
            
            pencilErasedFont->drawString( 
                mOldLastAteStrings.getElementDirect( i ), atePos, alignLeft );

            toggleMultiplicativeBlend( true );
            toggleAdditiveTextureColoring( true );
            
            float v = 1.0f - mOldLastAteBarFades.getElementDirect( i );
            setDrawColor( v, v, v, 1 );
            
            int fillMax = mOldLastAteFillMax.getElementDirect( i );

            if( fillMax < shortestFill ) {
                shortestFill = fillMax;
                }

            drawHungerMaxFillLine( atePos, 
                                   fillMax,
                                   mHungerBarErasedSprites,
                                   mHungerDashErasedSprites, 
                                   false,
                                   // only draw dashes once, for longest
                                   // one
                                   true );


            toggleAdditiveTextureColoring( false );
            toggleMultiplicativeBlend( false );
            }

        if( shortestFill < 100 ) {
            toggleMultiplicativeBlend( true );
            setDrawColor( 1, 1, 1, 1 );
            
            drawHungerMaxFillLine( atePos, 
                                   shortestFill,
                                   mHungerBarErasedSprites,
                                   mHungerDashErasedSprites, 
                                   true,
                                   // draw longest erased dash line once
                                   false );
            toggleMultiplicativeBlend( false );
            }
        

        if( mCurrentLastAteString != NULL ) {
            setDrawColor( 0, 0, 0, 1 );
        
            pencilFont->drawString( 
                mCurrentLastAteString, atePos, alignLeft );
            
            
            toggleMultiplicativeBlend( true );
            setDrawColor( 1, 1, 1, 1 );
            
            drawHungerMaxFillLine( atePos, 
                                   mCurrentLastAteFillMax,
                                   mHungerBarSprites,
                                   mHungerDashSprites,
                                   false, false );
            
            toggleMultiplicativeBlend( false );
            }

        
        if( mCurMouseOverID != 0 || mLastMouseOverID != 0 ) {
            int idToDescribe = mCurMouseOverID;
            
            if( mCurMouseOverID == 0 ) {
                idToDescribe = mLastMouseOverID;
                }

            
            
            doublePair pos = { lastScreenViewCenter.x, 
                               lastScreenViewCenter.y - 313 };

            char *des = NULL;
            char *desToDelete = NULL;
            
            if( idToDescribe == -99 ) {
                if( ourLiveObject->holdingID > 0 &&
                    getObject( ourLiveObject->holdingID )->foodValue > 0 ) {
                    
                    des = autoSprintf( "%s %s",
                                       translate( "eat" ),
                                       getObject( ourLiveObject->holdingID )->
                                       description );
                    desToDelete = des;
                    }
                else {
                    des = (char*)translate( "you" );
                    if( ourLiveObject->name != NULL ) {
                        des = autoSprintf( "%s - %s", des, 
                                           ourLiveObject->name );
                        desToDelete = des;
                        }
                    }
                }
            else if( idToDescribe < 0 ) {
                LiveObject *otherObj = getLiveObject( -idToDescribe );
                
                if( otherObj != NULL ) {
                    des = otherObj->relationName;
                    }
                if( des == NULL ) {
                    des = (char*)translate( "unrelated" );
                    }
                if( otherObj != NULL && otherObj->name != NULL ) {
                    des = autoSprintf( "%s - %s",
                                       otherObj->name, des );
                    desToDelete = des;
                    }
                }
            else {
                des = getObject( idToDescribe )->description;
                }
            
            char *stringUpper = stringToUpperCase( des );

            if( desToDelete != NULL ) {
                delete [] desToDelete;
                }

            stripDescriptionComment( stringUpper );
            

            if( mCurrentDes == NULL ) {
                mCurrentDes = stringDuplicate( stringUpper );
                }
            else {
                if( strcmp( mCurrentDes, stringUpper ) != 0 ) {
                    // adding a new one to stack, fade out old
                    
                    for( int i=0; i<mOldDesStrings.size(); i++ ) {
                        float fade =
                            mOldDesFades.getElementDirect( i );
                        
                        if( fade > 0.5 ) {
                            fade -= 0.20;
                            }
                        else {
                            fade -= 0.1;
                            }
                        
                        *( mOldDesFades.getElement( i ) ) = fade;
                        if( fade <= 0 ) {
                            mOldDesStrings.deallocateStringElement( i );
                            mOldDesFades.deleteElement( i );
                            i--;
                            }
                        else if( strcmp( mCurrentDes, 
                                         mOldDesStrings.getElementDirect(i) )
                                 == 0 ) {
                            // already in stack, move to top
                            mOldDesStrings.deallocateStringElement( i );
                            mOldDesFades.deleteElement( i );
                            i--;
                            }
                        }
                    
                    mOldDesStrings.push_back( mCurrentDes );
                    mOldDesFades.push_back( 1.0f );
                    mCurrentDes = stringDuplicate( stringUpper );
                    }
                }
            
            
            setDrawColor( 0, 0, 0, 1 );
            pencilFont->drawString( stringUpper, pos, alignCenter );
            
            delete [] stringUpper;
            }
        else {
            if( mCurrentDes != NULL ) {
                // done with this des

                // move to top of stack
                for( int i=0; i<mOldDesStrings.size(); i++ ) {
                    if( strcmp( mCurrentDes, 
                                mOldDesStrings.getElementDirect(i) )
                        == 0 ) {
                        // already in stack, move to top
                        mOldDesStrings.deallocateStringElement( i );
                        mOldDesFades.deleteElement( i );
                        i--;
                        }
                    }
                
                mOldDesStrings.push_back( mCurrentDes );
                mOldDesFades.push_back( 1.0f );
                mCurrentDes = NULL;
                }
            }
            

        
        }
    
    }




char nearEndOfMovement( LiveObject *inPlayer ) {
    if( inPlayer->currentSpeed == 0  ) {
        return true;
        }
    else if( inPlayer->currentPathStep >= inPlayer->pathLength - 2 ) {
        return true;
        }
    return false;
    }



void playPendingReceivedMessages( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );
    
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        readyPendingReceivedMessages.push_back( 
            inPlayer->pendingReceivedMessages.getElementDirect( i ) );
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void playPendingReceivedMessagesRegardingOthers( LiveObject *inPlayer ) {
    printf( "Playing %d pending received messages for %d "
            "    (skipping those that don't affect other players or map)\n", 
            inPlayer->pendingReceivedMessages.size(), inPlayer->id );

    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID != inPlayer->id ) {
                readyPendingReceivedMessages.push_back( message );
                }
            else {
                delete [] message;
                }
            }
        else {
            // not a PU, keep it no matter what (map change, etc.
            readyPendingReceivedMessages.push_back( message );
            }
        }
    inPlayer->pendingReceivedMessages.deleteAll();
    }



void dropPendingReceivedMessagesRegardingID( LiveObject *inPlayer,
                                             int inIDToDrop ) {
    for( int i=0; i<inPlayer->pendingReceivedMessages.size(); i++ ) {
        char *message = inPlayer->pendingReceivedMessages.getElementDirect( i );
        char match = false;
        
        if( strstr( message, "PU" ) == message ) {
            // only keep PU's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PU\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        else if( strstr( message, "PM" ) == message ) {
            // only keep PM's not about this player
            
            int messageID = -1;
            
            sscanf( message, "PM\n%d", &messageID );
            
            if( messageID == inIDToDrop ) {
                match = true;
                }
            }
        
        if( match ) {
            delete [] message;
            inPlayer->pendingReceivedMessages.deleteElement( i );
            i--;
            }
        }
    }



void LivingLifePage::applyReceiveOffset( int *inX, int *inY ) {
    if( mMapGlobalOffsetSet ) {
        *inX -= mMapGlobalOffset.x;
        *inY -= mMapGlobalOffset.y;
        }
    }



int LivingLifePage::sendX( int inX ) {
    if( mMapGlobalOffsetSet ) {
        return inX + mMapGlobalOffset.x;
        }
    return inX;
    }



int LivingLifePage::sendY( int inY ) {
    if( mMapGlobalOffsetSet ) {
        return inY + mMapGlobalOffset.y;
        }
    return inY;
    }



char *LivingLifePage::getDeathReason() {
    if( mDeathReason != NULL ) {
        return stringDuplicate( mDeathReason );
        }
    else {
        return NULL;
        }
    }



void LivingLifePage::handleOurDeath() {
    
    if( mDeathReason == NULL ) {
        mDeathReason = stringDuplicate( "" );
        }
    
    int years = (int)floor( computeCurrentAge( getOurLiveObject() ) );

    char *ageString;
    
    if( years == 1 ) {
        ageString = stringDuplicate( translate( "deathAgeOne" ) );
        }
    else {
        ageString = autoSprintf( translate( "deathAge" ), years );
        }
    
    char *partialReason = stringDuplicate( mDeathReason );
    delete [] mDeathReason;
    
    mDeathReason = autoSprintf( "%s####%s", ageString, partialReason );
    
    delete [] ageString;
    delete [] partialReason;
    

    setSignal( "died" );
    instantStopMusic();
    // so sound tails are not still playing when we we get reborn
    fadeSoundSprites( 0.1 );
    setSoundLoudness( 0 );
    }



static char isCategory( int inID ) {
    if( inID <= 0 ) {
        return false;
        }
    
    CategoryRecord *c = getCategory( inID );
    
    if( c == NULL ) {
        return false;
        }
    if( c->objectIDSet.size() > 0 ) {
        return true;
        }
    return false;
    }


static char isFood( int inID ) {
    if( inID <= 0 ) {
        return false;
        }
    
    ObjectRecord *o = getObject( inID );
    
    if( o->foodValue > 0 ) {
        return true;
        }
    else {
        return false;
        }
    }




static char getTransHintable( TransRecord *inTrans ) {

    if( inTrans->actor >= 0 && 
        ( inTrans->target > 0 || inTrans->target == -1 ) &&
        ( inTrans->newActor > 0 || inTrans->newTarget > 0 ) ) {


        if( isCategory( inTrans->actor ) ) {
            return false;
            }
        if( isCategory( inTrans->target ) ) {
            return false;
            }
        
        if( inTrans->target == -1 && inTrans->newTarget == 0 &&
            ! isFood( inTrans->actor ) ) {
            // generic one-time-use transition
            return false;
            }

        return true;
        }
    else {
        return false;
        }
    }



int LivingLifePage::getNumHints( int inObjectID ) {
    

    if( mLastHintSortedSourceID == inObjectID ) {
        return mLastHintSortedList.size();
        }
    

    // else need to regenerated sorted list

    mLastHintSortedSourceID = inObjectID;
    mLastHintSortedList.deleteAll();

    // heap sort
    MinPriorityQueue<TransRecord*> queue;
    
    SimpleVector<TransRecord*> *trans = getAllUses( inObjectID );
    
    int numTrans = trans->size();

    // skip any that repeat exactly the same string
    // (example:  goose pond in different states)
    SimpleVector<char *> otherActorStrings;
    SimpleVector<char *> otherTargetStrings;
    

    for( int i=0; i<numTrans; i++ ) {
        TransRecord *tr = trans->getElementDirect( i );
        
        if( getTransHintable( tr ) ) {
            
            int depth = 0;
            
            if( tr->actor > 0 && tr->actor != inObjectID ) {
                depth = getObjectDepth( tr->actor );
                }
            else if( tr->target > 0 && tr->target != inObjectID ) {
                depth = getObjectDepth( tr->target );
                }
            
            
            char stringAlreadyPresent = false;
            
            if( tr->actor > 0 && tr->actor != inObjectID ) {
                ObjectRecord *otherObj = getObject( tr->actor );
                
                char *trimmedDesc = stringDuplicate( otherObj->description );
                stripDescriptionComment( trimmedDesc );

                for( int s=0; s<otherActorStrings.size(); s++ ) {
                    
                    if( strcmp( trimmedDesc, 
                                otherActorStrings.getElementDirect( s ) )
                        == 0 ) {
                        
                        stringAlreadyPresent = true;
                        break;
                        }    
                    }

                if( !stringAlreadyPresent ) {
                    otherActorStrings.push_back( trimmedDesc );
                    }
                else {
                    delete [] trimmedDesc;
                    }
                }
            
            if( tr->target > 0 && tr->target != inObjectID ) {
                ObjectRecord *otherObj = getObject( tr->target );
                
                char *trimmedDesc = stringDuplicate( otherObj->description );
                stripDescriptionComment( trimmedDesc );

                for( int s=0; s<otherTargetStrings.size(); s++ ) {
                    
                    if( strcmp( trimmedDesc, 
                                otherTargetStrings.getElementDirect( s ) )
                        == 0 ) {
                        
                        stringAlreadyPresent = true;
                        break;
                        }    
                    }

                if( !stringAlreadyPresent ) {
                    otherTargetStrings.push_back( trimmedDesc );
                    }
                else {
                    delete [] trimmedDesc;
                    }
                }

            
            if( !stringAlreadyPresent ) {
                queue.insert( tr, depth );
                }
            }
        }
    
    otherActorStrings.deallocateStringElements();
    otherTargetStrings.deallocateStringElements();

    int numInQueue = queue.size();
    
    for( int i=0; i<numInQueue; i++ ) {
        mLastHintSortedList.push_back( queue.removeMin() );
        }
    
    
    return mLastHintSortedList.size();
    }



static int findMainObjectID( int inObjectID ) {
    if( inObjectID <= 0 ) {
        return inObjectID;
        }
    
    ObjectRecord *o = getObject( inObjectID );
    
    if( o == NULL ) {
        return inObjectID;
        }
    
    if( o->isUseDummy ) {
        return o->useDummyParent;
        }
    else {
        return inObjectID;
        }
    }



char *LivingLifePage::getHintMessage( int inObjectID, int inIndex ) {

    if( inObjectID != mLastHintSortedSourceID ) {
        getNumHints( inObjectID );
        }

    TransRecord *found = NULL;

    if( inIndex < mLastHintSortedList.size() ) {    
        found = mLastHintSortedList.getElementDirect( inIndex );
        }
    

    
    if( found != NULL ) {
        int actor = findMainObjectID( found->actor );
        int target = findMainObjectID( found->target );
        int newActor = findMainObjectID( found->newActor );
        int newTarget = findMainObjectID( found->newTarget );

        int result = 0;
        

        if( target != newTarget &&
            newTarget > 0 &&
            actor != newActor &&
            newActor > 0 ) {
            // both actor and target change
            // we need to decide which is the most important result
            // to hint
            
            if( actor == 0 && newActor > 0 ) {
                // something new ends in your hand, that's more important
                result = newActor;
                }
            else {
                // if the trans takes one of the elements to a deeper
                // state, that's more important, starting with actor
                if( getObjectDepth( newActor ) > getObjectDepth( actor ) ) {
                    result = newActor;
                    }
                else if( getObjectDepth( newTarget ) > 
                         getObjectDepth( target ) ) {
                    result = newTarget;
                    }
                // else neither actor or target becomes deeper
                // which result is deeper?
                else if( getObjectDepth( newActor ) > 
                         getObjectDepth( newTarget ) ) {
                    result = newActor;
                    }
                else {
                    result = newTarget;
                    }
                }
            }
        else if( target != newTarget && 
            newTarget > 0 ) {
            
            result = newTarget;
            }
        else if( actor != newActor && 
            newActor > 0 ) {
            
            result = newActor;
            }
        
        char *actorString;
        
        if( actor > 0 ) {
            actorString = stringToUpperCase( getObject( actor )->description );
            stripDescriptionComment( actorString );
            }
        else if( actor == 0 ) {
            actorString = stringDuplicate( translate( "bareHandHint" ) );
            }
        else {
            actorString = stringDuplicate( "" );
            }

        
        char eventually = false;
        
        
        char *targetString;
        
        if( target > 0 ) {
            targetString = 
                stringToUpperCase( getObject( target )->description );
            stripDescriptionComment( targetString );
            }
        else if( target == -1 && actor > 0 ) {
            ObjectRecord *actorObj = getObject( actor );
            
            if( actorObj->foodValue ) {
                targetString = stringDuplicate( translate( "eatHint" ) );
                
                if( result == 0 && 
                    actor == newActor &&
                    actorObj->numUses > 1 ) {
                    // see if there's a last-use transition and give hint
                    // about what it produces, in the end

                    int lastDummy = 
                        actorObj->useDummyIDs[ 0 ];
                    
                    TransRecord *lastUseTrans = getTrans( lastDummy, -1 );
                    
                    if( lastUseTrans != NULL ) {
                        if( lastUseTrans->newActor > 0 ) {
                            result = findMainObjectID( lastUseTrans->newActor );
                            eventually = true;
                            }
                        else if( lastUseTrans->newTarget > 0 ) {
                            result = 
                                findMainObjectID( lastUseTrans->newTarget );
                            eventually = true;
                            }
                        }    
                    }
                }
            else {
                targetString = stringDuplicate( translate( "bareGroundHint" ) );
                }
            }
        else {
            targetString = stringDuplicate( "" );
            }
        

        char *resultString;
        
        if( result > 0 ) {
            resultString = 
                stringToUpperCase( getObject( result )->description );
            }
        else {
            resultString = stringDuplicate( translate( "nothingHint" ) );
            }
        
        stripDescriptionComment( resultString );
        
        if( eventually ) {
            char *old = resultString;
            resultString = autoSprintf( "%s %s", old, 
                                        translate( "eventuallyHint" ) );
            delete [] old;
            }

        
        char *fullString =
            autoSprintf( "%s#%s#%s", actorString,
                         targetString, 
                         resultString );
        
        delete [] actorString;
        delete [] targetString;
        delete [] resultString;
        
        return fullString;
        }
    else {
        return stringDuplicate( translate( "noHint" ) );
        }
    }



// inNewID > 0
static char shouldCreationSoundPlay( int inOldID, int inNewID ) {
    // make sure this is really a fresh creation
    // of newID, and not a cycling back around
    // for a reusable object
    
    // also not useDummies that have the same
    // parent
    char sameParent = false;
    
    ObjectRecord *obj = getObject( inNewID );

    if( inOldID > 0 && inNewID > 0 ) {
        ObjectRecord *oldObj = getObject( inOldID );
        
        if( obj->isUseDummy &&
            oldObj->isUseDummy &&
            obj->useDummyParent ==
            oldObj->useDummyParent ) {
            sameParent = true;
            }
        else if( obj->numUses > 1 
                 &&
                 oldObj->isUseDummy 
                 &&
                 oldObj->useDummyParent
                 == inNewID ) {
            sameParent = true;
            }
        else if( oldObj->numUses > 1 
                 &&
                 obj->isUseDummy 
                 &&
                 obj->useDummyParent
                 == inOldID ) {
            sameParent = true;
            }
        }
    
    if( ! sameParent 
        &&
        ( ! obj->creationSoundInitialOnly
          ||
          inOldID <= 0
          ||
          ( ! isSpriteSubset( inOldID, inNewID ) 
            &&
            ! isAncestor( inOldID, inNewID, 1 ) ) ) ) {
        return true;
        }

    return false;
    }


static char checkIfHeldContChanged( LiveObject *inOld, LiveObject *inNew ) {    
                    
    if( inOld->numContained != inNew->numContained ) {
        return true;
        }
    else {
        for( int c=0; c<inOld->numContained; c++ ) {
            if( inOld->containedIDs[c] != 
                inNew->containedIDs[c] ) {
                return true;
                }
            if( inOld->subContainedIDs[c].size() != 
                inNew->subContainedIDs[c].size() ) {
                return true;
                }
            for( int s=0; s<inOld->subContainedIDs[c].size();
                 s++ ) {
                if( inOld->subContainedIDs[c].
                    getElementDirect( s ) !=
                    inNew->subContainedIDs[c].
                    getElementDirect( s ) ) {
                    return true;
                    }
                }
            }
        }
    return false;
    }




void LivingLifePage::sendBugReport( int inBugNumber ) {
    char *bugString = stringDuplicate( "" );

    if( lastMessageSentToServer != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Just sent: [%s]",
                                 temp, lastMessageSentToServer );
        delete [] temp;
        }
    if( nextActionMessageToSend != NULL ) {
        char *temp = bugString;
        bugString = autoSprintf( "%s   Waiting to send: [%s]",
                                 temp, nextActionMessageToSend );
        delete [] temp;
        }
    
    // clear # terminators from message
    char *spot = strstr( bugString, "#" );
    
    while( spot != NULL ) {
        spot[0] = ' ';
        spot = strstr( bugString, "#" );
        }
    
    
    char *bugMessage = autoSprintf( "BUG %d %s#", inBugNumber, bugString );
    
    delete [] bugString;

    sendToServerSocket( bugMessage );
    delete [] bugMessage;
    }
    


        
void LivingLifePage::step() {
    if( apocalypseInProgress ) {
        double stepSize = frameRateFactor / ( apocalypseDisplaySeconds * 60.0 );
        apocalypseDisplayProgress += stepSize;
        }
    
    if( mRemapPeak > 0 ) {
        if( mRemapDelay < 1 ) {
            double stepSize = 
                frameRateFactor / ( remapDelaySeconds * 60.0 );
            mRemapDelay += stepSize;
            }
        else {

            double stepSize = 
                mRemapDirection * frameRateFactor / ( remapPeakSeconds * 60.0 );
        
            mCurrentRemapFraction += stepSize;
            
            if( stepSize > 0 && mCurrentRemapFraction >= mRemapPeak ) {
                mCurrentRemapFraction = mRemapPeak;
                mRemapDirection *= -1;
                }
            if( stepSize < 0 && mCurrentRemapFraction <= 0 ) {
                mCurrentRemapFraction = 0;
                mRemapPeak = 0;
                }
            setRemapFraction( mCurrentRemapFraction );
            }
        }
    

    if( mouseDown ) {
        mouseDownFrames++;
        }
    
    if( mServerSocket == -1 ) {
        mServerSocket = openSocketConnection( serverIP, serverPort );
        return;
        }
    

    double pageLifeTime = game_getCurrentTime() - mPageStartTime;
    
    if( pageLifeTime < 1 ) {
        return;
        }
    else {
        setWaiting( false );
        }


    // first, read all available data from server
    char readSuccess = readServerSocketFull( mServerSocket );
    

    if( ! readSuccess ) {
        closeSocket( mServerSocket );
        mServerSocket = -1;

        if( mFirstServerMessagesReceived  ) {
            
            if( mDeathReason != NULL ) {
                delete [] mDeathReason;
                }
            mDeathReason = stringDuplicate( translate( "reasonDisconnected" ) );
            
            handleOurDeath();
            }
        else {
            setSignal( "loginFailed" );
            }
        return;
        }
    
    if( mLastMouseOverID != 0 ) {
        mLastMouseOverFade -= 0.01 * frameRateFactor;
        
        if( mLastMouseOverFade < 0 ) {
            mLastMouseOverID = 0;
            }
        }
    


    // move moving objects
    int numCells = mMapD * mMapD;
    
    for( int i=0; i<numCells; i++ ) {
                
        if( mMapMoveSpeeds[i] > 0 &&
            ( mMapMoveOffsets[ i ].x != 0 ||
              mMapMoveOffsets[ i ].y != 0  ) ) {

            
            doublePair nullOffset = { 0, 0 };
                    

            doublePair delta = sub( nullOffset, 
                                    mMapMoveOffsets[ i ] );
                    
            double step = frameRateFactor * mMapMoveSpeeds[ i ] / 60.0;
                    
            if( length( delta ) < step ) {
                        
                mMapMoveOffsets[ i ].x = 0;
                mMapMoveOffsets[ i ].y = 0;
                mMapMoveSpeeds[ i ] = 0;
                
                if( mMapCurAnimType[ i ] != ground ) {
                        
                    mMapLastAnimType[ i ] = mMapCurAnimType[ i ];
                    mMapCurAnimType[ i ] = ground;
                    mMapLastAnimFade[ i ] = 1;
                    
                    mMapAnimationLastFrameCount[ i ] =
                        mMapAnimationFrameCount[ i ];
                    
                    mMapAnimationFrozenRotFrameCount[ i ] = 
                        mMapAnimationLastFrameCount[ i ];
                    
                    mMapAnimationFrameCount[ i ] = 0;
                    }
                }
            else {
                mMapMoveOffsets[ i ] =
                    add( mMapMoveOffsets[ i ],
                         mult( normalize( delta ), step ) );
                }
            }
        }
    
    
    // step extra moving objects
    for( int i=0; i<mMapExtraMovingObjects.size(); i++ ) {
        
        ExtraMapObject *o = mMapExtraMovingObjects.getElement( i );
        
        doublePair nullOffset = { 0, 0 };
                    

        doublePair delta = sub( nullOffset, o->moveOffset );
        
        double step = frameRateFactor * o->moveSpeed / 60.0;
                    
        if( length( delta ) < step ) {
            // reached dest
            
            o->moveOffset.x = 0;
            o->moveOffset.y = 0;
            o->moveSpeed = 0;
                
            if( o->curAnimType != ground ) {
                        
                o->lastAnimType = o->curAnimType;
                o->curAnimType = ground;
                o->lastAnimFade = 1;
                    
                o->animationLastFrameCount = o->animationFrameCount;
                    
                o->animationFrameCount = 0;
                }

            GridPos worldPos = 
                mMapExtraMovingObjectsDestWorldPos.getElementDirect( i );
            
            int mapI = getMapIndex( worldPos.x, worldPos.y );
            
            if( mapI != -1 ) {
                // put it in dest
                putInMap( mapI, o );
                mMap[ mapI ] = 
                    mMapExtraMovingObjectsDestObjectIDs.getElementDirect( i );
                }
            
            mMapExtraMovingObjects.deleteElement( i );
            mMapExtraMovingObjectsDestWorldPos.deleteElement( i );
            mMapExtraMovingObjectsDestObjectIDs.deleteElement( i );
            i--;
            }
        else {
            o->moveOffset =
                add( o->moveOffset,
                     mult( normalize( delta ), step ) );
            }
        }
    



    if( mCurMouseOverID > 0 && ! mCurMouseOverSelf ) {
        mCurMouseOverFade += 0.2 * frameRateFactor;
        if( mCurMouseOverFade >= 1 ) {
            mCurMouseOverFade = 1.0;
            }
        }
    
    for( int i=0; i<mPrevMouseOverSpotFades.size(); i++ ) {
        float f = mPrevMouseOverSpotFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverSpotFades.deleteElement( i );
            mPrevMouseOverSpots.deleteElement( i );
            mPrevMouseOverSpotsBehind.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverSpotFades.getElement( i ) ) = f;
            }
        }
    
    
    
    if( mCurMouseOverCell.x != -1 ) {
        
        LiveObject *ourObject = getOurLiveObject();

        // we're not mousing over any object or person
        if( mCurMouseOverID == 0 &&
            ! mCurMouseOverPerson &&
            // AND we're not moving AND we're holding something
            ourObject != NULL &&
            ! ourObject->inMotion &&
            ourObject->holdingID > 0 ) {
            // fade cell in
            mCurMouseOverCellFade += 
                mCurMouseOverCellFadeRate * frameRateFactor;
            
            if( mCurMouseOverCellFade >= 1 ) {
                mCurMouseOverCellFade = 1.0;
                }
            }
        else {
            // fade cell out
            mCurMouseOverCellFade -= 0.1 * frameRateFactor;
            if( mCurMouseOverCellFade < 0 ) {
                mCurMouseOverCellFade = 0;
                }
            }
        }
    
    for( int i=0; i<mPrevMouseOverCellFades.size(); i++ ) {
        float f = mPrevMouseOverCellFades.getElementDirect( i );
        
        f -= 0.1 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseOverCellFades.deleteElement( i );
            mPrevMouseOverCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseOverCellFades.getElement( i ) ) = f;
            }
        }


    for( int i=0; i<mPrevMouseClickCellFades.size(); i++ ) {
        float f = mPrevMouseClickCellFades.getElementDirect( i );
        
        f -= 0.02 * frameRateFactor;
        
        
        if( f <= 0 ) {
            mPrevMouseClickCellFades.deleteElement( i );
            mPrevMouseClickCells.deleteElement( i );
            i--;
            }
        else {
            *( mPrevMouseClickCellFades.getElement( i ) ) = f;
            }
        }
    
    
    if( ! equal( mNotePaperPosOffset, mNotePaperPosTargetOffset ) ) {
        doublePair delta = 
            sub( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        double d = distance( mNotePaperPosTargetOffset, mNotePaperPosOffset );
        
        
        if( d <= 1 ) {
            mNotePaperPosOffset = mNotePaperPosTargetOffset;
            
            if( equal( mNotePaperPosTargetOffset, mNotePaperHideOffset ) ) {
                mLastKnownNoteLines.deallocateStringElements();
                mErasedNoteChars.deleteAll();
                mErasedNoteCharOffsets.deleteAll();
                mErasedNoteCharFades.deleteAll();
                }
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mNotePaperPosOffset = 
                add( mNotePaperPosOffset,
                     mult( dir, speed ) );
            }
        
        }
    
    
    
    LiveObject *ourObject = getOurLiveObject();

    if( ourObject != NULL ) {    
        char tooClose = false;
        
        int homeArrow = getHomeDir( ourObject->currentPos, &tooClose );
        
        if( homeArrow != -1 && ! tooClose ) {
            mHomeSlipPosTargetOffset.y = mHomeSlipHideOffset.y + 68;
            }
        else {
            mHomeSlipPosTargetOffset.y = mHomeSlipHideOffset.y;
            }
        }

    

    if( ! equal( mHomeSlipPosOffset, mHomeSlipPosTargetOffset ) ) {
        doublePair delta = 
            sub( mHomeSlipPosTargetOffset, mHomeSlipPosOffset );
        
        double d = distance( mHomeSlipPosTargetOffset, mHomeSlipPosOffset );
        
        
        if( d <= 1 ) {
            mHomeSlipPosOffset = mHomeSlipPosTargetOffset;

            if( equal( mHomeSlipPosTargetOffset, mHomeSlipHideOffset ) ) {
                // fully hidden
                // clear all arrow states
                for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
                    mHomeArrowStates[i].solid = false;
                    mHomeArrowStates[i].fade = 0;
                    }
                }
            }
        else {
            int speed = frameRateFactor * 4;

            if( d < 8 ) {
                speed = lrint( frameRateFactor * d / 2 );
                }

            if( speed > d ) {
                speed = floor( d );
                }

            if( speed < 1 ) {
                speed = 1;
                }
            
            doublePair dir = normalize( delta );
            
            mHomeSlipPosOffset = 
                add( mHomeSlipPosOffset,
                     mult( dir, speed ) );
            }        
        }
    
    
    

    if( ourObject != NULL ) {
        char newTrigger = false;
        
        AnimType anim = stepLiveTriggers( &newTrigger );
        
        if( anim != endAnimType && newTrigger ) {
            addNewAnim( ourObject, anim );
            }
        else if( anim == endAnimType && ourObject->curAnim > endAnimType ) {
            // trigger is over, back to ground
            addNewAnim( ourObject, ground );
            }
        }
    
    
    if( ourObject != NULL && mNextHintObjectID != 0 &&
        getNumHints( mNextHintObjectID ) > 0 ) {
        
        if( mCurrentHintObjectID != mNextHintObjectID ||
            mCurrentHintIndex != mNextHintIndex ) {
            
            int newLiveSheetIndex = 0;

            if( mLiveHintSheetIndex != -1 ) {
                mHintTargetOffset[mLiveHintSheetIndex] = mHintHideOffset[0];
                
                newLiveSheetIndex = 
                    (mLiveHintSheetIndex + 1 ) % NUM_HINT_SHEETS;
                
                }
            
            mLiveHintSheetIndex = newLiveSheetIndex;
            
            int i = mLiveHintSheetIndex;

            mHintTargetOffset[i] = mHintHideOffset[0];
            mHintTargetOffset[i].y += 100;
            
            mHintMessageIndex[ i ] = mNextHintIndex;
            
            mCurrentHintObjectID = mNextHintObjectID;
            mCurrentHintIndex = mNextHintIndex;
            
            mHintBookmarks[ mCurrentHintObjectID ] = mCurrentHintIndex;

            mNumTotalHints[ i ] = 
                getNumHints( mCurrentHintObjectID );

            if( mHintMessage[ i ] != NULL ) {
                delete [] mHintMessage[ i ];
                }
            
            mHintMessage[ i ] = getHintMessage( mCurrentHintObjectID, 
                                                mHintMessageIndex[i] );
            

            double longestLine = 0;
            
            int numLines;
            char **lines = split( mHintMessage[i], 
                                  "#", &numLines );
                
            for( int l=0; l<numLines; l++ ) {
                double len = handwritingFont->measureString( lines[l] );
                
                if( len > longestLine ) {
                    longestLine = len;
                    }
                delete [] lines[l];
                }
            delete [] lines;

            mHintExtraOffset[ i ].x = - longestLine;
            }
        }
    


    for( int i=0; i<NUM_HINT_SHEETS; i++ ) {
        
        if( ! equal( mHintPosOffset[i], mHintTargetOffset[i] ) ) {
            doublePair delta = 
                sub( mHintTargetOffset[i], mHintPosOffset[i] );
            
            double d = distance( mHintTargetOffset[i], mHintPosOffset[i] );
            
            
            if( d <= 1 ) {
                mHintPosOffset[i] = mHintTargetOffset[i];
                
                if( equal( mHintTargetOffset[i], mHintHideOffset[i] ) ) {
                    }
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }
                
                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHintPosOffset[i] = 
                    add( mHintPosOffset[i],
                         mult( dir, speed ) );
                }
            
            }
        }
    



    
    char anySlipsMovingDown = false;
    for( int i=0; i<3; i++ ) {
        if( i != mHungerSlipVisible &&
            ! equal( mHungerSlipPosOffset[i], mHungerSlipHideOffsets[i] ) ) {
            // visible when it shouldn't be
            mHungerSlipPosTargetOffset[i] = mHungerSlipHideOffsets[i];
            anySlipsMovingDown = true;
            }
        }
    
    if( !anySlipsMovingDown ) {
        if( mHungerSlipVisible != -1 ) {
            // send one up
            mHungerSlipPosTargetOffset[ mHungerSlipVisible ] =
                mHungerSlipShowOffsets[ mHungerSlipVisible ];
            }
        }


    // move all toward their targets
    for( int i=0; i<3; i++ ) {
        if( ! equal( mHungerSlipPosOffset[i], 
                     mHungerSlipPosTargetOffset[i] ) ) {
            
            doublePair delta = 
                sub( mHungerSlipPosTargetOffset[i], mHungerSlipPosOffset[i] );
        
            double d = distance( mHungerSlipPosTargetOffset[i], 
                                 mHungerSlipPosOffset[i] );
        
        
            if( d <= 1 ) {
                mHungerSlipPosOffset[i] = mHungerSlipPosTargetOffset[i];
                if( equal( mHungerSlipPosTargetOffset[i],
                           mHungerSlipHideOffsets[i] ) ) {
                        // reset wiggle time
                    mHungerSlipWiggleTime[i] = 0;
                    }
                }
            else {
                int speed = frameRateFactor * 4;
                
                if( d < 8 ) {
                    speed = lrint( frameRateFactor * d / 2 );
                    }
                
                if( speed > d ) {
                    speed = floor( d );
                    }

                if( speed < 1 ) {
                    speed = 1;
                    }
                
                doublePair dir = normalize( delta );
                
                mHungerSlipPosOffset[i] = 
                    add( mHungerSlipPosOffset[i],
                         mult( dir, speed ) );
                }
            }
        
        if( ! equal( mHungerSlipPosOffset[i],
                     mHungerSlipHideOffsets[i] ) ) {
            
            // advance wiggle time
            mHungerSlipWiggleTime[i] += 
                frameRateFactor * mHungerSlipWiggleSpeed[i];
            }
        }


    if( playerActionPending && 
        ourObject != NULL && 
        game_getCurrentTime() - 
        ourObject->pendingActionAnimationStartTime > 10 ) {
        
        // been bouncing for four seconds with no answer from server
        
        printf( "Been waiting for response to our action request "
                "from server for > 10 seconds, giving up\n" );

        sendBugReport( 1 );

        // end it
        ourObject->pendingActionAnimationProgress = 0;
        ourObject->pendingAction = false;
                                
        playerActionPending = false;
        playerActionTargetNotAdjacent = false;

        if( nextActionMessageToSend != NULL ) {
            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
        
        int goodX = ourObject->xServer;
        int goodY = ourObject->yServer;

        printf( "   Jumping back to last-known server position of %d,%d\n",
                goodX, goodY );

        // jump to wherever server said we were before
        ourObject->inMotion = false;
                        
        ourObject->moveTotalTime = 0;
        ourObject->currentSpeed = 0;
        ourObject->currentGridSpeed = 0;
        

        ourObject->currentPos.x = goodX;
        ourObject->currentPos.y = goodY;
        
        ourObject->xd = goodX;
        ourObject->yd = goodY;
        }
    
    
    if( game_getCurrentTime() - timeLastMessageSent > 15 ) {
        // more than 15 seconds without client making a move
        // send KA to keep connection open
        sendToServerSocket( (char*)"KA 0 0#" );
        }
    


    char *message = getNextServerMessage();


    while( message != NULL ) {
        overheadServerBytesRead += 52;
        
        printf( "Got length %d message\n%s\n", 
                (int)strlen( message ), message );

        messageType type = getMessageType( message );
        
        if( mapPullMode && type != MAP_CHUNK ) {
            // ignore it---map is a frozen snapshot in time
            // or as close as we can get to it
            type = UNKNOWN;
            }
        
        if( type == SHUTDOWN ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            setSignal( "serverShutdown" );
            
            delete [] message;
            return;
            }
        else if( type == SERVER_FULL ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            setSignal( "serverFull" );
            
            delete [] message;
            return;
            }
        else if( type == SEQUENCE_NUMBER ) {
            // need to respond with LOGIN message
            
            int number = 0;

            // we don't use these for anything in client
            int currentPlayers = 0;
            int maxPlayers = 0;
            mRequiredVersion = versionNumber;
            
            sscanf( message, 
                    "SN\n"
                    "%d/%d\n"
                    "%d\n"
                    "%d\n", &currentPlayers, &maxPlayers, &number, 
                    &mRequiredVersion );
            

            if( mRequiredVersion > versionNumber ||
                ( mRequiredVersion < versionNumber &&
                  mRequiredVersion != dataVersionNumber ) ) {
                
                // if server is using a newer version than us, we must upgrade
                // our client
                
                // if server is using an older version, check that
                // their version matches our data version at least

                setSignal( "versionMismatch" );
                delete [] message;
                return;
                }

            char *pureKey = getPureAccountKey();
            
            char *password = 
                SettingsManager::getStringSetting( "serverPassword" );
            
            if( password == NULL ) {
                password = stringDuplicate( "x" );
                }
            
            char *numberString = autoSprintf( "%d", number );


            char *pwHash = hmac_sha1( password, numberString );

            char *keyHash = hmac_sha1( pureKey, numberString );
            
            delete [] pureKey;
            delete [] password;
            delete [] numberString;
            
            
            // we record the number of characters sent for playback
            // if playback is using a different email.ini setting, this
            // will fail.
            // So pad the email with up to 80 space characters
            // Thus, the login message is always this same length
            
            char *outMessage;
            if( strlen( userEmail ) <= 80 ) {    
                outMessage = autoSprintf( "LOGIN %-80s %s %s#",
                                          userEmail, pwHash, keyHash );
                }
            else {
                // their email is too long for this trick
                // don't cut it off.
                // but note that the playback will fail if email.ini
                // doesn't match on the playback machine
                outMessage = autoSprintf( "LOGIN %s %s %s#",
                                          userEmail, pwHash, keyHash );
                }
            
            delete [] pwHash;
            delete [] keyHash;

            sendToServerSocket( outMessage );
            
            delete [] outMessage;
            
            delete [] message;
            return;
            }
        else if( type == ACCEPTED ) {
            // logged in successfully, wait for next message
            
            SettingsManager::setSetting( "loginSuccess", 1 );

            delete [] message;
            return;
            }
        else if( type == REJECTED ) {
            closeSocket( mServerSocket );
            mServerSocket = -1;
            setSignal( "loginFailed" );
            
            delete [] message;
            return;
            }
        else if( type == APOCALYPSE ) {
            apocalypseDisplayProgress = 0;
            apocalypseInProgress = true;
            }
        else if( type == MONUMENT_CALL ) {
            int posX, posY, monumentID;
            
            int numRead = sscanf( message, "MN\n%d %d %d",
                                  &posX, &posY, &monumentID );
            if( numRead == 3 ) {
                applyReceiveOffset( &posX, &posY );

                doublePair pos;
                pos.x = posX;
                pos.y = posY;
                
                LiveObject *ourLiveObject = getOurLiveObject();

                if( ourLiveObject != NULL ) {
                    double d = distance( pos, ourLiveObject->currentPos );
                    
                    if( d > 32 ) {
                        addAncientHomeLocation( posX, posY );
                        
                        // play sound in distance
                        ObjectRecord *monObj = getObject( monumentID );
                        
                        if( monObj != NULL && 
                            monObj->creationSound.numSubSounds > 0 ) {    
                             
                            doublePair realVector = 
                                getVectorFromCamera( lrint( posX ),
                                                     lrint( posY ) );
                            // position off camera in that direction
                            // but fake distance
                            realVector = mult( normalize( realVector ), 4 );
                            
                            playSound( monObj->creationSound, realVector );
                            }
                        }
                    }
                }            
            }
        else if( type == MAP_CHUNK ) {
            
            int sizeX = 0;
            int sizeY = 0;
            int x = 0;
            int y = 0;
            
            int binarySize = 0;
            int compressedSize = 0;
            
            sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                    &sizeX, &sizeY, &x, &y, &binarySize, &compressedSize );
            
            printf( "Got map chunk with bin size %d, compressed size %d\n", 
                    binarySize, compressedSize );
            
            if( ! mMapGlobalOffsetSet ) {
                
                // we need 7 fraction bits to represent 128 pixels per tile
                // 32-bit float has 23 significant bits, not counting sign
                // that leaves 16 bits for tile coordinate, or 65,536
                // Give two extra bits of wiggle room
                int maxOK = 16384;
                
                if( x < maxOK &&
                    x > -maxOK &&
                    y < maxOK &&
                    y > -maxOK ) {
                    printf( "First chunk isn't too far from center, using "
                            "0,0 as our global offset\n" );
                    
                    mMapGlobalOffset.x = 0;
                    mMapGlobalOffset.y = 0;
                    mMapGlobalOffsetSet = true;
                    }
                else {
                    printf( 
                        "Using this first chunk center as our global offset:  "
                        "%d, %d\n", x, y );
                    mMapGlobalOffset.x = x;
                    mMapGlobalOffset.y = y;
                    mMapGlobalOffsetSet = true;
                    }
                }
            
            applyReceiveOffset( &x, &y );
            
            // recenter our in-ram sub-map around this new chunk
            int newMapOffsetX = x + sizeX/2;
            int newMapOffsetY = y + sizeY/2;
            
            // move old cached map cells over to line up with new center

            int xMove = mMapOffsetX - newMapOffsetX;
            int yMove = mMapOffsetY - newMapOffsetY;
            
            int *newMap = new int[ mMapD * mMapD ];
            int *newMapBiomes = new int[ mMapD * mMapD ];
            int *newMapFloors = new int[ mMapD * mMapD ];
            

            double *newMapAnimationFrameCount = new double[ mMapD * mMapD ];
            double *newMapAnimationLastFrameCount = new double[ mMapD * mMapD ];

            double *newMapAnimationFrozenRotFameCount = 
                new double[ mMapD * mMapD ];

            int *newMapFloorAnimationFrameCount = new int[ mMapD * mMapD ];
        
            AnimType *newMapCurAnimType = new AnimType[ mMapD * mMapD ];
            AnimType *newMapLastAnimType = new AnimType[ mMapD * mMapD ];
            double *newMapLastAnimFade = new double[ mMapD * mMapD ];
            
            doublePair *newMapDropOffsets = new doublePair[ mMapD * mMapD ];
            double *newMapDropRot = new double[ mMapD * mMapD ];
            SoundUsage *newMapDropSounds = new SoundUsage[ mMapD * mMapD ];

            doublePair *newMapMoveOffsets = new doublePair[ mMapD * mMapD ];
            double *newMapMoveSpeeds = new double[ mMapD * mMapD ];


            char *newMapTileFlips= new char[ mMapD * mMapD ];
            
            SimpleVector<int> *newMapContainedStacks = 
                new SimpleVector<int>[ mMapD * mMapD ];
            
            SimpleVector< SimpleVector<int> > *newMapSubContainedStacks = 
                new SimpleVector< SimpleVector<int> >[ mMapD * mMapD ];

            char *newMapPlayerPlacedFlags = new char[ mMapD * mMapD ];

            
            for( int i=0; i<mMapD *mMapD; i++ ) {
                // starts uknown, not empty
                newMap[i] = -1;
                newMapBiomes[i] = -1;
                newMapFloors[i] = -1;

                int newX = i % mMapD;
                int newY = i / mMapD;

                int worldX = newX + mMapOffsetX - mMapD / 2;
                int worldY = newY + mMapOffsetY - mMapD / 2;
                
                // each cell is different, but always the same
                newMapAnimationFrameCount[i] =
                    lrint( getXYRandom( worldX, worldY ) * 10000 );
                newMapAnimationLastFrameCount[i] =
                    newMapAnimationFrameCount[i];
                
                newMapAnimationFrozenRotFameCount[i] = 0;
                
                newMapFloorAnimationFrameCount[i] =
                    lrint( getXYRandom( worldX, worldY ) * 13853 );
                
                

                newMapCurAnimType[i] = ground;
                newMapLastAnimType[i] = ground;
                newMapLastAnimFade[i] = 0;
                newMapDropOffsets[i].x = 0;
                newMapDropOffsets[i].y = 0;
                newMapDropRot[i] = 0;
                
                newMapDropSounds[i] = blankSoundUsage;
                
                newMapMoveOffsets[i].x = 0;
                newMapMoveOffsets[i].y = 0;
                newMapMoveSpeeds[i] = 0;

                newMapTileFlips[i] = false;
                newMapPlayerPlacedFlags[i] = false;
                

                
                int oldX = newX - xMove;
                int oldY = newY - yMove;
                
                if( oldX >= 0 && oldX < mMapD
                    &&
                    oldY >= 0 && oldY < mMapD ) {
                    
                    int oI = oldY * mMapD + oldX;

                    newMap[i] = mMap[oI];
                    newMapBiomes[i] = mMapBiomes[oI];
                    newMapFloors[i] = mMapFloors[oI];

                    newMapAnimationFrameCount[i] = mMapAnimationFrameCount[oI];
                    newMapAnimationLastFrameCount[i] = 
                        mMapAnimationLastFrameCount[oI];

                    newMapAnimationFrozenRotFameCount[i] = 
                        mMapAnimationFrozenRotFrameCount[oI];

                    newMapFloorAnimationFrameCount[i] = 
                        mMapFloorAnimationFrameCount[oI];
                    
                    newMapCurAnimType[i] = mMapCurAnimType[oI];
                    newMapLastAnimType[i] = mMapLastAnimType[oI];
                    newMapLastAnimFade[i] = mMapLastAnimFade[oI];
                    newMapDropOffsets[i] = mMapDropOffsets[oI];
                    newMapDropRot[i] = mMapDropRot[oI];
                    newMapDropSounds[i] = mMapDropSounds[oI];

                    newMapMoveOffsets[i] = mMapMoveOffsets[oI];
                    newMapMoveSpeeds[i] = mMapMoveSpeeds[oI];

                    newMapTileFlips[i] = mMapTileFlips[oI];
                    

                    newMapContainedStacks[i] = mMapContainedStacks[oI];
                    newMapSubContainedStacks[i] = mMapSubContainedStacks[oI];

                    newMapPlayerPlacedFlags[i] = 
                        mMapPlayerPlacedFlags[oI];
                    }
                }
            
            memcpy( mMap, newMap, mMapD * mMapD * sizeof( int ) );
            memcpy( mMapBiomes, newMapBiomes, mMapD * mMapD * sizeof( int ) );
            memcpy( mMapFloors, newMapFloors, mMapD * mMapD * sizeof( int ) );

            memcpy( mMapAnimationFrameCount, newMapAnimationFrameCount, 
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapAnimationLastFrameCount, 
                    newMapAnimationLastFrameCount, 
                    mMapD * mMapD * sizeof( double ) );

            memcpy( mMapAnimationFrozenRotFrameCount, 
                    newMapAnimationFrozenRotFameCount, 
                    mMapD * mMapD * sizeof( double ) );

            
            memcpy( mMapFloorAnimationFrameCount, 
                    newMapFloorAnimationFrameCount, 
                    mMapD * mMapD * sizeof( int ) );

            
            memcpy( mMapCurAnimType, newMapCurAnimType, 
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimType, newMapLastAnimType,
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimFade, newMapLastAnimFade,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropOffsets, newMapDropOffsets,
                    mMapD * mMapD * sizeof( doublePair ) );
            memcpy( mMapDropRot, newMapDropRot,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropSounds, newMapDropSounds,
                    mMapD * mMapD * sizeof( SoundUsage ) );

            memcpy( mMapMoveOffsets, newMapMoveOffsets,
                    mMapD * mMapD * sizeof( doublePair ) );
            memcpy( mMapMoveSpeeds, newMapMoveSpeeds,
                    mMapD * mMapD * sizeof( double ) );

            
            memcpy( mMapTileFlips, newMapTileFlips,
                    mMapD * mMapD * sizeof( char ) );
            
            // can't memcpy vectors
            // need to assign them so copy constructors are invoked
            for( int i=0; i<mMapD *mMapD; i++ ) {
                mMapContainedStacks[i] = newMapContainedStacks[i];
                mMapSubContainedStacks[i] = newMapSubContainedStacks[i];
                }
            

            memcpy( mMapPlayerPlacedFlags, newMapPlayerPlacedFlags,
                    mMapD * mMapD * sizeof( char ) );
            
            delete [] newMap;
            delete [] newMapBiomes;
            delete [] newMapFloors;
            delete [] newMapAnimationFrameCount;
            delete [] newMapAnimationLastFrameCount;
            delete [] newMapAnimationFrozenRotFameCount;
            delete [] newMapFloorAnimationFrameCount;
            
            delete [] newMapCurAnimType;
            delete [] newMapLastAnimType;
            delete [] newMapLastAnimFade;
            delete [] newMapDropOffsets;
            delete [] newMapDropRot;
            delete [] newMapDropSounds;

            delete [] newMapMoveOffsets;
            delete [] newMapMoveSpeeds;
            
            delete [] newMapTileFlips;
            delete [] newMapContainedStacks;
            delete [] newMapSubContainedStacks;
            
            delete [] newMapPlayerPlacedFlags;
            
            

            mMapOffsetX = newMapOffsetX;
            mMapOffsetY = newMapOffsetY;
            
            
            unsigned char *compressedChunk = 
                new unsigned char[ compressedSize ];
    
            
            for( int i=0; i<compressedSize; i++ ) {
                compressedChunk[i] = serverSocketBuffer.getElementDirect( i );
                }
            serverSocketBuffer.deleteStartElements( compressedSize );

            
            unsigned char *decompressedChunk =
                zipDecompress( compressedChunk, 
                               compressedSize,
                               binarySize );
            
            delete [] compressedChunk;
            
            if( decompressedChunk == NULL ) {
                printf( "Decompressing chunk failed\n" );
                }
            else {
                
                unsigned char *binaryChunk = 
                    new unsigned char[ binarySize + 1 ];
            
                memcpy( binaryChunk, decompressedChunk, binarySize );
            
                delete [] decompressedChunk;
 
            
                // for now, binary chunk is actually just ASCII
                binaryChunk[ binarySize ] = '\0';
            
                
                SimpleVector<char *> *tokens = 
                    tokenizeString( (char*)binaryChunk );
            
                delete [] binaryChunk;


                int numCells = sizeX * sizeY;
                
                if( tokens->size() == numCells ) {
                    
                    for( int i=0; i<tokens->size(); i++ ) {
                        int cX = i % sizeX;
                        int cY = i / sizeX;
                        
                        int mapX = cX + x - mMapOffsetX + mMapD / 2;
                        int mapY = cY + y - mMapOffsetY + mMapD / 2;
                        
                        if( mapX >= 0 && mapX < mMapD
                            &&
                            mapY >= 0 && mapY < mMapD ) {
                            
                            
                            int mapI = mapY * mMapD + mapX;
                            int oldMapID = mMap[mapI];
                            
                            sscanf( tokens->getElementDirect(i),
                                    "%d:%d:%d", 
                                    &( mMapBiomes[mapI] ),
                                    &( mMapFloors[mapI] ),
                                    &( mMap[mapI] ) );
                            
                            if( mMap[mapI] != oldMapID ) {
                                // our placement status cleared
                                mMapPlayerPlacedFlags[mapI] = false;
                                }

                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            
                            if( strstr( tokens->getElementDirect(i), "," ) 
                                != NULL ) {
                                
                                int numInts;
                                char **ints = 
                                    split( tokens->getElementDirect(i), 
                                           ",", &numInts );
                                
                                delete [] ints[0];
                                
                                int numContained = numInts - 1;
                                
                                for( int c=0; c<numContained; c++ ) {
                                    SimpleVector<int> newSubStack;
                                    
                                    mMapSubContainedStacks[mapI].push_back(
                                        newSubStack );
                                    
                                    int contained = atoi( ints[ c + 1 ] );
                                    mMapContainedStacks[mapI].push_back( 
                                        contained );
                                    
                                    if( strstr( ints[c + 1], ":" ) != NULL ) {
                                        // sub-container items
                                
                                        int numSubInts;
                                        char **subInts = 
                                            split( ints[c + 1], 
                                                   ":", &numSubInts );
                                
                                        delete [] subInts[0];
                                        int numSubCont = numSubInts - 1;

                                        SimpleVector<int> *subStack =
                                            mMapSubContainedStacks[mapI].
                                            getElement(c);

                                        for( int s=0; s<numSubCont; s++ ) {
                                            subStack->push_back(
                                                atoi( subInts[ s + 1 ] ) );
                                            delete [] subInts[ s + 1 ];
                                            }

                                        delete [] subInts;
                                        }

                                    delete [] ints[ c + 1 ];
                                    }
                                delete [] ints;
                                }
                            }
                        }
                    }   
                
                tokens->deallocateStringElements();
                delete tokens;
                
                if( !( mFirstServerMessagesReceived & 1 ) ) {
                    // first map chunk just recieved
                    
                    char found = false;
                    int closestX = 0;
                    int closestY = 0;
                    
                    // only if marker starts on birth screen
                    
                    // use distance squared here, no need for sqrt
                    double closestDist = 5 * 5;

                    int mapCenterY = y + sizeY / 2;
                    int mapCenterX = x + sizeX / 2;
                    printf( "Map center = %d,%d\n", mapCenterX, mapCenterY );
                    
                    for( int mapY=0; mapY < mMapD; mapY++ ) {
                        for( int mapX=0; mapX < mMapD; mapX++ ) {
                        
                            int i = mMapD * mapY + mapX;
                            
                            int id = mMap[ i ];
                            
                            if( id > 0 ) {
                            
                                // check for home marker
                                if( getObject( id )->homeMarker ) {
                                    int worldY = mapY + mMapOffsetY - mMapD / 2;
                                    
                                    int worldX = mapX + mMapOffsetX - mMapD / 2;

                                    double dist = 
                                        pow( worldY - mapCenterY, 2 )
                                        +
                                        pow( worldX - mapCenterX, 2 );
                                    
                                    if( dist < closestDist ) {
                                        closestDist = dist;
                                        closestX = worldX;
                                        closestY = worldY;
                                        found = true;
                                        }
                                    }
                                }
                            }
                        }
                    
                    if( found ) {
                        printf( "Found starting home marker at %d,%d\n",
                                closestX, closestY );
                        addHomeLocation( closestX, closestY );
                        }
                    }
                

                mFirstServerMessagesReceived |= 1;
                }

            if( mapPullMode ) {
                
                if( x == mapPullCurrentX - sizeX/2 && 
                    y == mapPullCurrentY - sizeY/2 ) {
                    
                    lastScreenViewCenter.x = mapPullCurrentX * CELL_D;
                    lastScreenViewCenter.y = mapPullCurrentY * CELL_D;
                    setViewCenterPosition( lastScreenViewCenter.x,
                                           lastScreenViewCenter.y );
                    
                    mapPullCurrentX += 10;
                    
                    if( mapPullCurrentX > mapPullEndX ) {
                        mapPullCurrentX = mapPullStartX;
                        mapPullCurrentY += 5;
                        
                        if( mapPullCurrentY > mapPullEndY ) {
                            mapPullModeFinalImage = true;
                            }
                        }
                    mapPullCurrentSaved = false;
                    mapPullCurrentSent = false;
                    }                
                }
            }
        else if( type == MAP_CHANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {
                
                int x, y, floorID, responsiblePlayerID;
                int oldX, oldY;
                float speed = 0;
                                
                char *idBuffer = new char[500];

                int numRead = sscanf( lines[i], "%d %d %d %499s %d %d %d %f",
                                      &x, &y, &floorID, 
                                      idBuffer, &responsiblePlayerID,
                                      &oldX, &oldY, &speed );
                if( numRead == 5 || numRead == 8) {

                    applyReceiveOffset( &x, &y );

                    int mapX = x - mMapOffsetX + mMapD / 2;
                    int mapY = y - mMapOffsetY + mMapD / 2;
                    
                    if( mapX >= 0 && mapX < mMapD
                        &&
                        mapY >= 0 && mapY < mMapD ) {
                        
                        int mapI = mapY * mMapD + mapX;
                        
                        int oldFloor = mMapFloors[ mapI ];

                        mMapFloors[ mapI ] = floorID;
                        

                        int old = mMap[mapI];

                        int newID = -1;
                        
                        int oldContainedCount = 
                            mMapContainedStacks[mapI].size();
                        
                        if( responsiblePlayerID != -1 ) {
                            int rID = responsiblePlayerID;
                            if( rID < -1 ) {
                                rID = -rID;
                                }
                            LiveObject *rObj = getLiveObject( rID );
                            
                            if( rObj != NULL && 
                                rObj->pendingReceivedMessages.size() > 0 ) {
                                

                                printf( "Holding MX message caused by "
                                    "%d until later, "
                                    "%d other messages pending for them\n",
                                    rID,
                                    rObj->pendingReceivedMessages.size() );
                                
                                rObj->pendingReceivedMessages.push_back(
                                    autoSprintf( "MX\n%s\n#",
                                                 lines[i] ) );
                                
                                delete [] lines[i];
                                continue;
                                }
                            }
                        

                        if( strstr( idBuffer, "," ) != NULL ) {
                            int numInts;
                            char **ints = 
                                split( idBuffer, ",", &numInts );
                        
                            
                            newID = atoi( ints[0] );

                            mMap[mapI] = newID;
                            
                            delete [] ints[0];

                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            
                            int numContained = numInts - 1;
                            
                            for( int c=0; c<numContained; c++ ) {
                                
                                SimpleVector<int> newSubStack;
                                    
                                mMapSubContainedStacks[mapI].push_back(
                                    newSubStack );
                                
                                mMapContainedStacks[mapI].push_back(
                                    atoi( ints[ c + 1 ] ) );
                                
                                if( strstr( ints[c + 1], ":" ) != NULL ) {
                                    // sub-container items
                                
                                    int numSubInts;
                                    char **subInts = 
                                        split( ints[c + 1], ":", &numSubInts );
                                
                                    delete [] subInts[0];
                                    int numSubCont = numSubInts - 1;
                                    
                                    SimpleVector<int> *subStack =
                                        mMapSubContainedStacks[mapI].
                                        getElement(c);

                                    for( int s=0; s<numSubCont; s++ ) {
                                        subStack->push_back(
                                            atoi( subInts[ s + 1 ] ) );
                                        delete [] subInts[ s + 1 ];
                                        }

                                    delete [] subInts;
                                    }

                                delete [] ints[ c + 1 ];
                                }
                            delete [] ints;
                            }
                        else {
                            // a single int
                            newID = atoi( idBuffer );
                            mMap[mapI] = newID;
                            mMapContainedStacks[mapI].deleteAll();
                            mMapSubContainedStacks[mapI].deleteAll();
                            }
                        
                        if( speed > 0 ) {
                            // this cell moved from somewhere
                            
                            applyReceiveOffset( &oldX, &oldY );
                            
                            int oldMapI = getMapIndex( oldX, oldY );
                            
                            int sourceObjID = 0;
                            if( oldMapI != -1 ) {
                                sourceObjID = mMap[ oldMapI ];
                            
                                // check what move-trans for sourceID
                                // produces.  If it produces something
                                // show that moving instead
                                
                                TransRecord *moveTrans = 
                                    getTrans( -1, sourceObjID );
                                
                                if( moveTrans != NULL &&
                                    moveTrans->move > 0 ) {
                                    sourceObjID = moveTrans->newTarget;
                                    }
                                }
                            
                            ExtraMapObject oldObj;
                            
                            if( old > 0 && sourceObjID != 0 &&
                                getTrans( sourceObjID, old ) != NULL ) {
                                
                                // save old object while we
                                // set up new object
                                oldObj = copyFromMap( mapI );
                                oldObj.objectID = old;
                                if( old == -1 ) {
                                    oldObj.objectID = 0;
                                    }
                                }

                            mMapMoveSpeeds[mapI] = speed;
                            
                            

                            doublePair oldOffset = { 0, 0 };
                            
                            char moveEastOrWest = false;
                            
                            if( oldMapI != -1 ) {
                                // location where we came from
                                oldOffset = mMapMoveOffsets[ oldMapI ];

                                mMapAnimationFrameCount[ mapI ] =
                                    mMapAnimationFrameCount[ oldMapI ];
                                
                                mMapAnimationLastFrameCount[ mapI ] =
                                    mMapAnimationLastFrameCount[ oldMapI ];
                                
        
                                mMapAnimationFrozenRotFrameCount[ mapI ] =
                                    mMapAnimationFrozenRotFrameCount[ 
                                        oldMapI ];
                                
                                mMapCurAnimType[ mapI ] = 
                                    mMapCurAnimType[ oldMapI ];
                                mMapLastAnimType[ mapI ] = 
                                    mMapLastAnimType[ oldMapI ];
                                
                                mMapLastAnimFade[ mapI ] =
                                    mMapLastAnimFade[ oldMapI ];
                                
                                if( mMapLastAnimFade[ mapI ] == 0 &&
                                    mMapCurAnimType[ mapI ] != moving ) {
                                    // not in the middle of a fade
                                    
                                    // fade to moving
                                    mMapLastAnimType[ mapI ] =
                                        mMapCurAnimType[ mapI ];
                                    mMapCurAnimType[ mapI ] = moving;
                                    mMapLastAnimFade[ mapI ] = 1;

                                    mMapAnimationFrameCount[ mapI ] =
                                        mMapAnimationFrozenRotFrameCount[ 
                                            oldMapI ];
                                    }
                                
                                int oldLocID = mMap[ oldMapI ];
                                
                                if( oldLocID > 0 ) {
                                    TransRecord *decayTrans = 
                                        getTrans( -1, oldLocID );
                        
                                    if( decayTrans != NULL ) {
                                        
                                        if( decayTrans->move == 6 ||
                                            decayTrans->move == 7 ) {
                                            moveEastOrWest = true;
                                            }
                                        }
                                    }
                                }

                            double oldTrueX = oldX + oldOffset.x;
                            double oldTrueY = oldY + oldOffset.y;
                            
                            mMapMoveOffsets[mapI].x = oldTrueX - x;
                            mMapMoveOffsets[mapI].y = oldTrueY - y;

                            if( moveEastOrWest || x > oldTrueX ) {
                                mMapTileFlips[mapI] = false;
                                }
                            else if( x < oldTrueX ) {
                                mMapTileFlips[mapI] = true;
                                }

                                                        
                            if( old > 0 && sourceObjID != 0 &&
                                getTrans( sourceObjID, old ) != NULL ) {
                                
                                // now that we've set up new object in dest
                                // copy it
                                ExtraMapObject newObj = copyFromMap( mapI );
                                // leave source in place during move
                                newObj.objectID = sourceObjID;
                                
                                // save it as an extra obj
                                mMapExtraMovingObjects.push_back( newObj );
                                GridPos worldDestPos = { x, y };
                                mMapExtraMovingObjectsDestWorldPos.push_back(
                                    worldDestPos );
                                
                                mMapExtraMovingObjectsDestObjectIDs.push_back(
                                    newID );

                                // put old object back in place for now
                                putInMap( mapI, &oldObj );
                                }


                            if( old == 0 && sourceObjID != 0 ) {
                                // object moving into empty spot
                                // track where it came from as old
                                // so that we don't play initial
                                // creation sound by accident
                                old = sourceObjID;
                                }
                            }
                        else {
                            mMapMoveSpeeds[mapI] = 0;
                            mMapMoveOffsets[mapI].x = 0;
                            mMapMoveOffsets[mapI].y = 0;
                            }
                        
                        
                        TransRecord *nextDecayTrans = getTrans( -1, newID );
                        
                        if( nextDecayTrans != NULL ) {
                            
                            if( nextDecayTrans->move == 6 ||
                                nextDecayTrans->move == 7 ) {
                                // this object will move to left/right in future
                                // force no flip now
                                mMapTileFlips[mapI] = false;
                                }
                            }
                        

                        if( oldFloor != floorID && floorID > 0 ) {
                            // floor changed
                            
                            ObjectRecord *obj = getObject( floorID );
                            if( obj->creationSound.numSubSounds > 0 ) {    
                                    
                                playSound( obj->creationSound,
                                           getVectorFromCamera( x, y ) );
                                }
                            }
                        

                        LiveObject *responsiblePlayerObject = NULL;
                        
                        if( responsiblePlayerID > 0 ) {
                            responsiblePlayerObject = 
                                getGameObject( responsiblePlayerID );
                            }

                        if( old > 0 &&
                            old == newID &&
                            mMapContainedStacks[mapI].size() > 
                            oldContainedCount &&
                            responsiblePlayerObject != NULL &&
                            responsiblePlayerObject->holdingID == 0 ) {
                            
                            // target is changed container and
                            // responsible player's hands now empty
                            
                            
                            // first, try and play the "using"
                            // sound for the container
                            char soundPlayed = false;
                            
                            SoundUsage contSound = 
                                getObject( mMap[mapI] )->usingSound;
                            
                            if( contSound.numSubSounds > 0 ) {
                                playSound( contSound, 
                                           getVectorFromCamera( x, y ) );
                                soundPlayed = true;
                                }

                            if( ! soundPlayed ) {
                                // no container using sound defined
                                
                                
                                // play player's using sound
                            
                                SoundUsage s = getObject( 
                                    responsiblePlayerObject->displayID )->
                                    usingSound;

                                if( s.numSubSounds > 0 ) {
                                    playSound( s, getVectorFromCamera( x, y ) );
                                    }
                                }
                            }
                        


                        if( responsiblePlayerID == -1 ) {
                            // no one dropped this
                            
                            // our placement status cleared
                            mMapPlayerPlacedFlags[mapI] = false;
                            }
                        

                        // Check if a home marker has been set or removed
                        if( responsiblePlayerID != -1 &&
                            ( old != 0 ||
                              newID != 0 ) ) {
                            // player-triggered change
                            
                            int rID = responsiblePlayerID;
                            if( rID < -1 ) {
                                rID = -rID;
                                }
                            
                            if( rID == ourID ) {
                                // local player triggered
                                
                                char addedOrRemoved = false;
                                
                                if( newID != 0 &&
                                    getObject( newID )->homeMarker ) {
                                    
                                    addHomeLocation( x, y );
                                    addedOrRemoved = true;
                                    }
                                else if( old != 0 &&
                                         getObject( old )->homeMarker ) {
                                    removeHomeLocation( x, y );
                                    addedOrRemoved = true;
                                    }
                                
                                if( addedOrRemoved ) {
                                    // look in region for our home locations
                                    // that may have been removed by others
                                    // (other people pulling up our stake)
                                    // and clear them.  Our player can
                                    // no longer clear them, because they
                                    // don't exist on the map anymore
                                    
                                    for( int ry=y-7; ry<=y+7; ry++ ) {
                                        for( int rx=x-7; rx<=x+7; rx++ ) {
                                    
                                            int mapRX = 
                                                rx - mMapOffsetX + mMapD / 2;
                                            int mapRY = 
                                                ry - mMapOffsetY + mMapD / 2;
                    
                                            if( mapRX >= 0 && mapRX < mMapD
                                                &&
                                                mapRY >= 0 && mapRY < mMapD ) {
                        
                                                int mapRI = 
                                                    mapRY * mMapD + mapRX;
                        
                                                int cellID = mMap[ mapRI ];
                                                
                                                if( cellID == 0 ||
                                                    ( cellID > 0 &&
                                                      ! getObject( cellID )->
                                                        homeMarker ) ) {
                                                    
                                                    removeHomeLocation(
                                                        rx, ry );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        


                        if( old != newID && 
                            newID != 0 && 
                            responsiblePlayerID <= -1 ) {
                            // ID change, and not just a player setting
                            // an object down
                            
                            ObjectRecord *newObj = getObject( newID );


                            if( old > 0 ) {
                                
                                if( responsiblePlayerID == -1 ) {
                                    // object auto-decayed from some other
                                    // object
                                    
                                    // play decay sound
                                    ObjectRecord *obj = getObject( old );
                                    if( obj->decaySound.numSubSounds > 0 ) {    
                                    
                                        playSound( 
                                            obj->decaySound,
                                            getVectorFromCamera( x, y ) );
                                        }
                                    }
                                else if( responsiblePlayerID < -1 ) {
                                    // player caused this object to change
                                    
                                    if( newObj->creationSound.numSubSounds == 0
                                        ||
                                        ! shouldCreationSoundPlay( old, 
                                                                   newID ) ) {
                                        // no creation sound will play for new
                                        // (if it will, it will be played
                                        //  below)
                                        ObjectRecord *obj = getObject( old );
                                        
                                        // don't play using sound
                                        // if they are both use dummies
                                        if( !bothSameUseParent( old, newID )
                                            &&
                                            obj->usingSound.numSubSounds 
                                            > 0 ) {    
                                            
                                            playSound( 
                                                obj->usingSound,
                                                getVectorFromCamera( x, y ) );
                                            }
                                        }
                                    }
                                }
                            

                            
                            if( newObj->creationSound.numSubSounds > 0 ) {
                                
                                if( old == 0 && responsiblePlayerID < -1 ) {
                                    // new placement, but not set-down
                                    // we don't have information
                                    // from on-ground objects to 
                                    // check for ancestor relationships

                                    // check what the player is left
                                    // holding instead
                                    
                                    LiveObject *responsiblePlayerObject = 
                                        getGameObject( -responsiblePlayerID );
                                    if( responsiblePlayerObject != NULL ) {
                                        
                                        old = 
                                            responsiblePlayerObject->holdingID;
                                        if( old < 0 ) {
                                            old = 0;
                                            }
                                        
                                        if( old > 0 ) {
                                            TransRecord *p = 
                                                getTransProducing( old,
                                                                   newID );
                                            
                                            if( p != NULL && 
                                                p->actor > 0 ) {
                                                old = p->actor;
                                                }
                                            }
                                        }
                                    }
                                

                                if( shouldCreationSoundPlay( old, newID ) ) {
                                    playSound( newObj->creationSound,
                                               getVectorFromCamera( x, y ) );
                                    }
                                }
                            }
                            


                        if( mMap[mapI] != 0 ) {
                            ObjectRecord *newObj = getObject( mMap[mapI] );
                            
                            if( newObj->permanent && newObj->blocksWalking ) {
                                // clear the locally-stored flip for this
                                // tile
                                mMapTileFlips[mapI] = false;
                                }    
                            }
                        
                        if( old != mMap[mapI] && mMap[mapI] != 0 ) {
                            // new placement
                            
                            printf( "New placement, responsible=%d\n",
                                    responsiblePlayerID );
                            
                            if( mMapMoveSpeeds[mapI] == 0 ) {
                                
                                if( old == 0 ) {
                                    // set down into an empty spot
                                    // reset frame count
                                    mMapAnimationFrameCount[mapI] = 0;
                                    mMapAnimationLastFrameCount[mapI] = 0;
                                    }
                                else {
                                    // else, leave existing frame count alone,
                                    // since object has simply gone through a
                                    // transition
                                    
                                    // UNLESS it is a force-zero-start 
                                    // animation
                                    AnimationRecord *newAnim =
                                        getAnimation( mMap[mapI], ground );
                                    
                                    if( newAnim != NULL  && 
                                        newAnim->forceZeroStart ) {
                                        
                                        mMapAnimationFrameCount[mapI] = 0;
                                        mMapAnimationLastFrameCount[mapI] = 0;
                                        }
                                    }
                                    
                                
                                mMapCurAnimType[mapI] = ground;
                                mMapLastAnimType[mapI] = ground;
                                mMapLastAnimFade[mapI] = 0;
                                mMapAnimationFrozenRotFrameCount[mapI] = 0;
                                }
                            else {
                                mMapLastAnimType[mapI] = mMapCurAnimType[mapI];
                                mMapCurAnimType[mapI] = moving;
                                if( mMapLastAnimType[mapI] != moving ) {
                                    mMapLastAnimFade[mapI] = 1;
                                    }
                                }
                            
                            
                            LiveObject *responsiblePlayerObject = NULL;
                            
                            
                            if( responsiblePlayerID > 0 ) {
                                responsiblePlayerObject = 
                                    getGameObject( responsiblePlayerID );

                                }
                            if( responsiblePlayerID != -1 &&
                                responsiblePlayerID != 0 &&
                                getObjectHeight( mMap[mapI] ) < CELL_D ) {
                                
                                // try flagging objects as through-mousable
                                // for any change caused by a player,
                                // as long as object is small enough
                                mMapPlayerPlacedFlags[mapI] = true;
                                }
                            else if( getObjectHeight( mMap[mapI] ) >= CELL_D ) {
                                // object that has become tall enough
                                // that through-highlights feel strange
                                mMapPlayerPlacedFlags[mapI] = false;
                                }
                            // don't forget placement flags for objects
                            // that someone placed but have naturally changed,
                            // but remain short
                            
                            
                            
                            if( responsiblePlayerObject == NULL ||
                                !responsiblePlayerObject->onScreen ) {
                                
                                // set it down instantly, no drop animation
                                // (player's held offset isn't valid)
                                AnimationRecord *animR = 
                                    getAnimation( mMap[mapI], ground );

                                if( animR != NULL && 
                                    animR->randomStartPhase ) {
                                    mMapAnimationFrameCount[mapI] = 
                                        randSource.getRandomBoundedInt( 
                                            0, 10000 );
                                    mMapAnimationLastFrameCount[i] = 
                                        mMapAnimationFrameCount[mapI];
                                    }
                                mMapDropOffsets[mapI].x = 0;
                                mMapDropOffsets[mapI].y = 0;
                                mMapDropRot[mapI] = 0;
                                mMapDropSounds[mapI] = blankSoundUsage;
                                }
                            else {
                                // copy last frame count from last holder
                                // of this object (server tracks
                                // who was holding it and tell us about it)
                                
                                mMapLastAnimType[mapI] = held;
                                mMapLastAnimFade[mapI] = 1;
                                        
                                mMapAnimationFrozenRotFrameCount[mapI] =
                                    responsiblePlayerObject->
                                    heldFrozenRotFrameCount;
                                        
                                        
                                if( responsiblePlayerObject->
                                    lastHeldAnimFade == 0 ) {
                                            
                                    mMapAnimationLastFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        heldAnimationFrameCount;
                                            
                                    mMapLastAnimType[mapI] = 
                                        responsiblePlayerObject->curHeldAnim;
                                    }
                                else {
                                    // dropped object was already
                                    // in the middle of a fade
                                    mMapCurAnimType[mapI] =
                                        responsiblePlayerObject->curHeldAnim;
                                            
                                    mMapAnimationFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        heldAnimationFrameCount;
                                    
                                    mMapLastAnimType[mapI] =
                                        responsiblePlayerObject->lastHeldAnim;
                                    
                                    mMapAnimationLastFrameCount[mapI] =
                                        responsiblePlayerObject->
                                        lastHeldAnimationFrameCount;
                                    
                                    
                                    mMapLastAnimFade[mapI] =
                                        responsiblePlayerObject->
                                        lastHeldAnimFade;
                                    }
                                        
                                            
                                
                                mMapDropOffsets[mapI].x = 
                                    responsiblePlayerObject->
                                    heldObjectPos.x - x;
                                        
                                mMapDropOffsets[mapI].y = 
                                    responsiblePlayerObject->
                                    heldObjectPos.y - y;
                                        
                                mMapDropRot[mapI] = 
                                    responsiblePlayerObject->heldObjectRot;

                                mMapDropSounds[mapI] =
                                    getObject( 
                                        responsiblePlayerObject->displayID )->
                                    usingSound;

                                mMapTileFlips[mapI] =
                                    responsiblePlayerObject->holdingFlip;
                                        
                                if( responsiblePlayerObject->holdingID > 0 &&
                                    old == 0 ) {
                                    // use on bare ground transition
                                    
                                    // don't use drop offset
                                    mMapDropOffsets[mapI].x = 0;
                                    mMapDropOffsets[mapI].y = 0;
                                    
                                    mMapDropRot[mapI] = 0;
                                    mMapDropSounds[mapI] = 
                                        blankSoundUsage;
                                    
                                    if( getObject( mMap[ mapI ] )->
                                        permanent ) {
                                        // resulting in something 
                                        // permanent
                                        // on ground.  Never flip it
                                        mMapTileFlips[mapI] = false;
                                        }
                                    }
                                
                                
                                
                                if( x > 
                                    responsiblePlayerObject->xServer ) {
                                    responsiblePlayerObject->holdingFlip = 
                                        false;
                                    }
                                else if( x < 
                                         responsiblePlayerObject->xServer ) {
                                    responsiblePlayerObject->holdingFlip = 
                                        true;
                                    }
                                }
                            }
                        }
                    }
                
                delete [] idBuffer;
                
                delete [] lines[i];
                }
            
            delete [] lines;
            }
        else if( type == PLAYER_UPDATE ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            // for babies that are held, but don't exist yet in 
            // client because PU creating them hasn't been received yet
            // assume these always arrive in the same PU message
            SimpleVector<int> unusedHolderID;
            SimpleVector<int> unusedHeldID;
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                o.onScreen = false;
                
                o.allSpritesLoaded = false;
                
                o.holdingID = 0;
                
                o.useWaypoint = false;

                o.pathToDest = NULL;
                o.containedIDs = NULL;
                o.subContainedIDs = NULL;
                
                o.onFinalPathStep = false;
                
                o.age = 0;
                o.finalAgeSet = false;
                
                o.outOfRange = false;
                o.dying = false;
                
                o.name = NULL;
                o.relationName = NULL;


                o.tempAgeOverrideSet = false;
                o.tempAgeOverride = 0;

                // don't track these for other players
                o.foodStore = 0;
                o.foodCapacity = 0;                

                o.maxFoodStore = 0;
                o.maxFoodCapacity = 0;

                o.currentSpeech = NULL;
                o.speechFade = 1.0;
                
                o.heldByAdultID = -1;
                o.heldByDropOffset.x = 0;
                o.heldByDropOffset.y = 0;
                
                o.ridingOffset.x = 0;
                o.ridingOffset.y = 0;

                o.animationFrameCount = 0;
                o.heldAnimationFrameCount = 0;

                o.lastAnimationFrameCount = 0;
                o.lastHeldAnimationFrameCount = 0;
                
                
                o.frozenRotFrameCount = 0;
                o.heldFrozenRotFrameCount = 0;
                
                o.frozenRotFrameCountUsed = false;
                o.heldFrozenRotFrameCountUsed = false;
                o.clothing = getEmptyClothingSet();
                
                o.somePendingMessageIsMoreMovement = false;

                
                o.actionTargetTweakX = 0;
                o.actionTargetTweakY = 0;
                

                int forced = 0;
                int done_moving = 0;
                
                char *holdingIDBuffer = new char[500];

                int heldOriginValid, heldOriginX, heldOriginY,
                    heldTransitionSourceID;
                
                char *clothingBuffer = new char[500];
                
                int justAte = 0;
                int justAteID = 0;
                
                int facingOverride = 0;
                int actionAttempt = 0;
                int actionTargetX = 0;
                int actionTargetY = 0;
                
                double invAgeRate = 60.0;
                
                int responsiblePlayerID = -1;
                

                int numRead = sscanf( lines[i], 
                                      "%d %d "
                                      "%d "
                                      "%d "
                                      "%d %d "
                                      "%499s %d %d %d %d %f %d %d %d %d "
                                      "%lf %lf %lf %499s %d %d %d",
                                      &( o.id ),
                                      &( o.displayID ),
                                      &facingOverride,
                                      &actionAttempt,
                                      &actionTargetX,
                                      &actionTargetY,
                                      holdingIDBuffer,
                                      &heldOriginValid,
                                      &heldOriginX,
                                      &heldOriginY,
                                      &heldTransitionSourceID,
                                      &( o.heat ),
                                      &done_moving,
                                      &forced,
                                      &( o.xd ),
                                      &( o.yd ),
                                      &( o.age ),
                                      &invAgeRate,
                                      &( o.lastSpeed ),
                                      clothingBuffer,
                                      &justAte,
                                      &justAteID,
                                      &responsiblePlayerID );
                
                
                if( numRead == 23 ) {

                    applyReceiveOffset( &actionTargetX, &actionTargetY );
                    applyReceiveOffset( &heldOriginX, &heldOriginY );
                    applyReceiveOffset( &( o.xd ), &( o.yd ) );
                    
                    printf( "PLAYER_UPDATE with heldOrVal=%d, "
                            "heldOrx=%d, heldOry=%d, "
                            "pX=%d, pY=%d, heldTransSrcID=%d, "
                            "holdingString=%s\n",
                            heldOriginValid, heldOriginX, heldOriginY,
                            o.xd, o.yd, heldTransitionSourceID, 
                            holdingIDBuffer );
                    if( forced ) {
                        printf( "  POSITION FORCED\n" );
                        }

                    o.lastAgeSetTime = game_getCurrentTime();

                    o.ageRate = 1.0 / invAgeRate;

                    int numClothes;
                    char **clothes = split( clothingBuffer, ";", &numClothes );
                        
                    if( numClothes == NUM_CLOTHING_PIECES ) {
                        
                        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                            
                            int numParts;
                            char **parts = split( clothes[c], ",", &numParts );
                            
                            if( numParts > 0 ) {
                                int id = 0;
                                sscanf( parts[0], "%d", &id );
                                
                                if( id != 0 ) {
                                    setClothingByIndex( &( o.clothing ), 
                                                        c, 
                                                        getObject( id ) );
                                    
                                    if( numParts > 1 ) {
                                        for( int p=1; p<numParts; p++ ) {
                                            
                                            int cID = 0;
                                            sscanf( parts[p], "%d", &cID );
                                            
                                            if( cID != 0 ) {
                                                
                                                o.clothingContained[c].
                                                    push_back( cID );
                                                }
                                            }
                                        }
                                    }
                                }
                            for( int p=0; p<numParts; p++ ) {
                                delete [] parts[p];
                                }
                            delete [] parts;
                            }

                        }
                    
                    for( int c=0; c<numClothes; c++ ) {
                        delete [] clothes[c];
                        }
                    delete [] clothes;

                    
                    if( strstr( holdingIDBuffer, "," ) != NULL ) {
                        int numInts;
                        char **ints = split( holdingIDBuffer, ",", &numInts );
                        
                        o.holdingID = atoi( ints[0] );
                        
                        delete [] ints[0];

                        o.numContained = numInts - 1;
                        o.containedIDs = new int[ o.numContained ];
                        o.subContainedIDs = 
                            new SimpleVector<int>[ o.numContained ];
                        
                        for( int c=0; c<o.numContained; c++ ) {
                            o.containedIDs[c] = atoi( ints[ c + 1 ] );
                            
                            if( strstr( ints[c + 1], ":" ) != NULL ) {
                                // sub-container items
                                
                                int numSubInts;
                                char **subInts = 
                                    split( ints[c + 1], ":", &numSubInts );
                                
                                delete [] subInts[0];
                                int numSubCont = numSubInts - 1;
                                for( int s=0; s<numSubCont; s++ ) {
                                    o.subContainedIDs[c].push_back(
                                        atoi( subInts[ s + 1 ] ) );
                                    delete [] subInts[ s + 1 ];
                                    }
                                
                                delete [] subInts;
                                }

                            delete [] ints[ c + 1 ];
                            }
                        delete [] ints;
                        }
                    else {
                        // a single int
                        o.holdingID = atoi( holdingIDBuffer );
                        o.numContained = 0;
                        }
                    
                    
                    o.xServer = o.xd;
                    o.yServer = o.yd;

                    LiveObject *existing = NULL;

                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            existing = gameObjects.getElement(j);
                            break;
                            }
                        }
                    
                    if( existing != NULL &&
                        existing->id != ourID &&
                        existing->currentSpeed != 0 &&
                        ! forced ) {
                        // non-forced update about other player 
                        // while we're still playing their last movement
                        

                        // defer it until they're done moving
                        printf( "Holding PU message for "
                                "%d until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                existing->pendingReceivedMessages.size() );
                                
                        existing->pendingReceivedMessages.push_back(
                            autoSprintf( "PU\n%s\n#",
                                         lines[i] ) );
                        }
                    else if( existing != NULL &&
                             existing->heldByAdultID != -1 &&
                             getLiveObject( existing->heldByAdultID ) != NULL &&
                             getLiveObject( existing->heldByAdultID )->
                                 pendingReceivedMessages.size() > 0 ) {
                        // we're held by an adult who has pending
                        // messages to play at the end of their movement
                        // (server sees that their movement has already ended)
                        // Thus, an update about us should be played later also
                        // whenever the adult's messages are played back
                        
                        LiveObject *holdingPlayer = 
                            getLiveObject( existing->heldByAdultID );

                        printf( "Holding PU message for baby %d held by %d "
                                "until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                existing->heldByAdultID,
                                holdingPlayer->pendingReceivedMessages.size() );

                        holdingPlayer->
                            pendingReceivedMessages.push_back(
                                autoSprintf( "PU\n%s\n#",
                                             lines[i] ) );
                        }
                    else if( existing != NULL &&
                             responsiblePlayerID != -1 &&
                             getLiveObject( responsiblePlayerID ) != NULL &&
                             getLiveObject( responsiblePlayerID )->
                                 pendingReceivedMessages.size() > 0 ) {
                        // someone else is responsible for this change
                        // to us (we're likely a baby) and that person
                        // is still in the middle of a local walk
                        // with pending messages that will play
                        // after the walk.  Defer this message too

                        LiveObject *rO = 
                            getLiveObject( responsiblePlayerID );
                        
                        printf( "Holding PU message for %d caused by %d "
                                "until later, "
                                "%d other messages pending for them\n",
                                existing->id,
                                responsiblePlayerID,
                                rO->pendingReceivedMessages.size() );

                        rO->pendingReceivedMessages.push_back(
                            autoSprintf( "PU\n%s\n#",
                                         lines[i] ) );
                        }         
                    else if( existing != NULL ) {
                        int oldHeld = existing->holdingID;
                        int heldContChanged = false;
                        
                        if( oldHeld > 0 &&
                            oldHeld == existing->holdingID ) {
                            
                            heldContChanged =
                                checkIfHeldContChanged( &o, existing );
                            }
                        
                        
                        // receiving a PU means they aren't out of
                        // range anymore
                        if( existing->outOfRange ) {
                            // was out of range before
                            // this update is forced
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                            
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            }
                        existing->outOfRange = false;

                        
                        existing->lastHoldingID = oldHeld;
                        existing->holdingID = o.holdingID;
                        
                        if( o.id == ourID &&
                            existing->holdingID > 0 &&
                            existing->holdingID != oldHeld ) {
                            // holding something new
                            // hint about it
                            
                            mNextHintObjectID = existing->holdingID;
                            mNextHintIndex = 
                                mHintBookmarks[ mNextHintObjectID ];
                            }
                        


                        ObjectRecord *newClothing = 
                            getClothingAdded( &( existing->clothing ), 
                                              &( o.clothing ) );
                        

                        if( newClothing == NULL ) {
                            // has something been removed instead?
                            newClothing = 
                                getClothingAdded( &( o.clothing ),
                                                  &( existing->clothing ) );
                            }
                        
                        char clothingSoundPlayed = false;

                        char clothingChanged = false;
                        
                        if( newClothing != NULL ) {
                            clothingChanged = true;
                            
                            SoundUsage clothingSound = 
                                newClothing->usingSound;
                            
                            if( clothingSound.numSubSounds > 0 ) {
                                playSound( clothingSound, 
                                           getVectorFromCamera( 
                                               o.xd,
                                               o.yd ) );
                                
                                clothingSoundPlayed = true;
                                }
                            else {
                                // play generic pickup sound
                                ObjectRecord *existingObj = 
                                    getObject( existing->displayID );
                                
                                if( existingObj->usingSound.numSubSounds > 0 ) {
                                    
                                    playSound( existingObj->usingSound,
                                               getVectorFromCamera(
                                                   o.xd,
                                                   o.yd ) );
                                    clothingSoundPlayed = true;
                                    }
                                }
                            }

                        existing->clothing = o.clothing;


                        // what we're holding hasn't changed
                        // maybe action failed
                        if( o.id == ourID && existing->holdingID == oldHeld ) {
                            
                            LiveObject *ourObj = getOurLiveObject();
                            
                            if( ourObj->pendingActionAnimationProgress != 0 &&
                                ! ourObj->inMotion ) {
                                
                                addNewAnimPlayerOnly( existing, ground2 );
                                }
                            }
                        
                        if( o.id != ourID ) {
                            
                            if( actionAttempt && ! justAte &&
                                nearEndOfMovement( existing ) ) {
                                existing->actionTargetX = actionTargetX;
                                existing->actionTargetY = actionTargetY;
                                existing->pendingActionAnimationProgress = 
                                    0.025 * frameRateFactor;
                                }

                            if( heldOriginValid || 
                                facingOverride != 0 ) {
                                        
                                if( ( heldOriginValid && 
                                      heldOriginX > existing->xd )
                                    ||
                                    facingOverride == 1 ) {
                                    
                                    existing->holdingFlip = false;
                                    }
                                else if( ( heldOriginValid && 
                                           heldOriginX < existing->xd ) 
                                         ||
                                         facingOverride == -1 ) {
                                    
                                    existing->holdingFlip = true;
                                    }
                                }
                            }

                        
                        char creationSoundPlayed = false;
                        char otherSoundPlayed = false;
                        

                        if( justAte && 
                            o.id != ourID && 
                            existing->holdingID == oldHeld ) {
                            // seems like this PU is about player being
                            // fed by someone else

                            // don't interrupt walking
                            // but still play sound
                            if( nearEndOfMovement( existing ) ) {
                                addNewAnimPlayerOnly( 
                                    existing, eating );
                                }

                            ObjectRecord *ateObj =
                                getObject( justAteID );
                                        
                            if( ateObj->eatingSound.numSubSounds > 0 ) {
                                playSound( 
                                    ateObj->eatingSound,
                                    getVectorFromCamera( 
                                        existing->currentPos.x, 
                                        existing->currentPos.y ) );
                                otherSoundPlayed = true;
                                }
                            }

                        
                        if( justAte && o.id == ourID ) {
                            // we just heard from server that we
                            // finished eating
                            // play sound now
                            ObjectRecord *ateObj =
                                getObject( justAteID );
                            if( ateObj->eatingSound.numSubSounds > 0 ) {
                                playSound( 
                                    ateObj->eatingSound,
                                    getVectorFromCamera( 
                                        existing->currentPos.x, 
                                        existing->currentPos.y ) );
                                otherSoundPlayed = true;
                                }
                            if( strstr( ateObj->description, "remapStart" )
                                != NULL ) {
                                
                                if( mRemapPeak == 0 ) {
                                    // reseed
                                    setRemapSeed( 
                                        remapRandSource.getRandomBoundedInt( 
                                            0,
                                            10000000 ) );
                                    mRemapDelay = 0;
                                    }
                                        
                                // closer to max peak
                                // zeno's paradox style
                                mRemapPeak += 0.5 * ( 1 - mRemapPeak );
                                
                                // back toward peak
                                mRemapDirection = 1.0;
                                }
                            }
                        
                        
                        if( existing->holdingID == 0 ) {
                                                        
                            // don't reset these when dropping something
                            // leave them in place so that dropped object
                            // can use them for smooth fade
                            // existing->animationFrameCount = 0;
                            // existing->curAnim = ground;
                            
                            //existing->lastAnim = ground;
                            //existing->lastAnimFade = 0;
                            if( oldHeld != 0 ) {
                                if( o.id == ourID ) {
                                    addNewAnimPlayerOnly( existing, ground );
                                    }
                                else {
                                    if( justAte ) {
                                        // don't interrupt walking
                                        // but still play sound
                                        if( nearEndOfMovement( existing ) ) {
                                            addNewAnimPlayerOnly( 
                                                existing, eating );
                                            }

                                        if( oldHeld > 0 ) {
                                            ObjectRecord *oldHeldObj =
                                                getObject( oldHeld );
                                        
                                            if( oldHeldObj->
                                                eatingSound.numSubSounds > 0 ) {
                                            
                                                playSound( 
                                                    oldHeldObj->eatingSound,
                                                    getVectorFromCamera( 
                                                     existing->currentPos.x, 
                                                     existing->currentPos.y ) );
                                                otherSoundPlayed = true;
                                                }
                                            }
                                        }
                                    else {
                                        // don't interrupt walking
                                        if( nearEndOfMovement( existing ) ) {
                                            addNewAnimPlayerOnly( 
                                                existing, doing );
                                            }
                                        }
                                    
                                    // don't interrupt walking
                                    if( nearEndOfMovement( existing ) ) {
                                        addNewAnimPlayerOnly( existing, 
                                                              ground );
                                        }
                                    }
                                }
                            }
                        else if( oldHeld != existing->holdingID ||
                                 heldContChanged ) {
                            // holding something new
                            
                            // what we're holding has gone through
                            // transition.  Keep old animation going
                            // for what's held
                            
                            if( o.id == ourID ) {
                                addNewAnimPlayerOnly( existing, ground2 );
                                }
                            else {
                                if( justAte ) {
                                    // don't interrupt walking
                                    // but still play sound
                                    if( nearEndOfMovement( existing ) ) {    
                                        addNewAnimPlayerOnly( 
                                            existing, eating );
                                        }
                                    if( oldHeld > 0 ) {
                                        
                                        ObjectRecord *oldHeldObj =
                                            getObject( oldHeld );
                                        
                                        if( oldHeldObj->eatingSound.numSubSounds
                                            > 0 ) {
                                        
                                            playSound( 
                                                oldHeldObj->eatingSound,
                                                getVectorFromCamera( 
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                            otherSoundPlayed = true;
                                            }
                                        }
                                    }
                                else {
                                    // don't interrupt walking
                                    if( nearEndOfMovement( existing ) ) {
                                        addNewAnimPlayerOnly( 
                                            existing, doing );
                                        }
                                    }
                                // don't interrupt walking
                                if( nearEndOfMovement( existing ) ) {
                                    addNewAnimPlayerOnly( existing, ground2 );
                                    }
                                }
                            
                            if( heldOriginValid ) {
                                
                                // use player's using sound for pickup
                                ObjectRecord *existingObj = 
                                    getObject( existing->displayID );
                                
                                if( existingObj->usingSound.numSubSounds > 0 ) {
                                    
                                    playSound( existingObj->usingSound,
                                               getVectorFromCamera(
                                                   heldOriginX,
                                                   heldOriginY ) );
                                    otherSoundPlayed = true;
                                    }


                                // transition from last ground animation
                                // of object, keeping that frame count
                                // for smooth transition
                                
                                if( ! existing->onScreen ) {
                                    // off-screen, instant pickup,
                                    // no transition animation
                                    existing->heldPosOverride = false;
                                    existing->heldObjectPos = 
                                        existing->currentPos;
                                    existing->heldObjectRot = 0;
                                    
                                    existing->lastHeldAnimFade = 0;
                                    existing->curHeldAnim = held;
                                    }
                                else {
                                    // on-screen, slide into position
                                    // smooth animation transition
                                    existing->heldPosSlideStepCount = 0;
                                    existing->heldPosOverride = true;
                                    existing->heldPosOverrideAlmostOver = false;
                                    existing->heldObjectPos.x = heldOriginX;
                                    existing->heldObjectPos.y = heldOriginY;


                                    // check if held origin needs
                                    // tweaking because what we picked up
                                    // had a moving offset
                                    int mapHeldOriginX = 
                                        heldOriginX - mMapOffsetX + mMapD / 2;
                                    int mapHeldOriginY = 
                                        heldOriginY- mMapOffsetY + mMapD / 2;
                    
                                    if( mapHeldOriginX >= 0 && 
                                        mapHeldOriginX < mMapD
                                        &&
                                        mapHeldOriginY >= 0 && 
                                        mapHeldOriginY < mMapD ) {
                                        
                                        int mapHeldOriginI = 
                                            mapHeldOriginY * mMapD + 
                                            mapHeldOriginX;
                                        
                                        if( mMapMoveSpeeds[ mapHeldOriginI ]
                                            > 0 &&
                                            ( mMapMoveOffsets[ 
                                                  mapHeldOriginI ].x
                                              != 0 
                                              ||
                                              mMapMoveOffsets[ 
                                                  mapHeldOriginI ].y
                                              != 0 ) ) {

                                            // only do this if tweak not already
                                            // set.  Don't want to interrupt
                                            // an existing tweak
                                            if( existing->actionTargetTweakX ==
                                                0 &&
                                                existing->actionTargetTweakY ==
                                                0 ) {
                                                
                                                existing->actionTargetTweakX =
                                                    lrint( 
                                                        mMapMoveOffsets[
                                                        mapHeldOriginI ].x );
                                            
                                                existing->actionTargetTweakY =
                                                    lrint( mMapMoveOffsets[
                                                           mapHeldOriginI ].y );
                                                }
                                            
                                            existing->heldObjectPos =
                                                add( existing->heldObjectPos,
                                                     mMapMoveOffsets[ 
                                                         mapHeldOriginI ] );
                                            }    
                                        }
                                    

                                    existing->heldObjectRot = 0;
                                
                                    
                                    int mapX = 
                                        heldOriginX - mMapOffsetX + mMapD / 2;
                                    int mapY = 
                                        heldOriginY - mMapOffsetY + mMapD / 2;
                                    
                                    if( mapX >= 0 && mapX < mMapD
                                        &&
                                        mapY >= 0 && mapY < mMapD ) {
                                        
                                        int mapI = mapY * mMapD + mapX;
                                        
                                        existing->heldFrozenRotFrameCount =
                                            mMapAnimationFrozenRotFrameCount
                                            [ mapI ];
                                        existing->heldFrozenRotFrameCountUsed =
                                            false;
                                        
                                        if( mMapLastAnimFade[ mapI ] == 0 ) {
                                            existing->lastHeldAnim = 
                                                mMapCurAnimType[ mapI ];
                                            existing->lastHeldAnimFade = 1;
                                            existing->curHeldAnim = held;
                                            
                                            existing->
                                                lastHeldAnimationFrameCount =
                                                mMapAnimationFrameCount[ mapI ];
                                        
                                        existing->heldAnimationFrameCount =
                                            mMapAnimationFrameCount[ mapI ];
                                            }
                                        else {
                                            // map spot is in the middle of
                                            // an animation fade
                                            existing->lastHeldAnim = 
                                                mMapLastAnimType[ mapI ];
                                            existing->
                                                lastHeldAnimationFrameCount =
                                                mMapAnimationLastFrameCount[ 
                                                    mapI ];
                                        
                                            existing->curHeldAnim = 
                                                mMapCurAnimType[ mapI ];
                                            
                                            existing->heldAnimationFrameCount =
                                                mMapAnimationFrameCount[ mapI ];
                                            

                                            existing->lastHeldAnimFade =
                                                mMapLastAnimFade[ mapI ];
                                            
                                            existing->futureHeldAnimStack->
                                                push_back( held );
                                            }
                                        
                                        }
                                    }
                                }
                            else {
                                existing->heldObjectPos = existing->currentPos;
                                existing->heldObjectRot = 0;

                                if( existing->holdingID > 0 ) {
                                    // what player is holding changed
                                    // but they didn't pick anything up
                                    // play change sound
                                    
                                    ObjectRecord *heldObj =
                                        getObject( existing->holdingID );
                                    

                                    if( oldHeld > 0 && 
                                        heldTransitionSourceID == -1 ) {
                                        // held object auto-decayed from 
                                        // some other object
                                
                                        // play decay sound
                                        ObjectRecord *obj = 
                                            getObject( oldHeld );
                                        if( obj->decaySound.numSubSounds 
                                            > 0 ) {    
                                            
                                            playSound( 
                                                obj->decaySound,
                                                getVectorFromCamera(
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                            otherSoundPlayed = true;
                                            }
                                        }
                                    else if( oldHeld > 0 &&
                                             heldTransitionSourceID > 0 ) {
                                        
                                        TransRecord *t = 
                                            getTrans( oldHeld,
                                                      heldTransitionSourceID );
                                        
                                        if( t != NULL &&
                                            t->target != t->newTarget &&
                                            t->newTarget > 0 ) {
                                            
                                            // something produced on
                                            // ground by this transition
                                            
                                            // does it make a sound on
                                            // creation?
                                            
                                            if( getObject( t->newTarget )->
                                                creationSound.numSubSounds 
                                                > 0 ) {
                                                
                                                if( shouldCreationSoundPlay(
                                                        t->target,
                                                        t->newTarget ) ) {
                                                    
                                                    // only make one sound
                                                    otherSoundPlayed = true;
                                                    }
                                                }
                                            }
                                        }
                                    
                                    
                                    if( ! clothingChanged &&
                                        heldTransitionSourceID >= 0 &&
                                        heldObj->creationSound.numSubSounds 
                                        > 0 ) {
                                        
                                        int testAncestor = oldHeld;
                                        
                                        if( oldHeld <= 0 &&
                                            heldTransitionSourceID > 0 ) {
                                            
                                            testAncestor = 
                                                heldTransitionSourceID;
                                            }
                                        

                                        if( testAncestor > 0 ) {
                                            // new held object is result
                                            // of a transtion
                                            // (otherwise, it has been
                                            //  removed from a container

                                            // only play creation sound
                                            // if this object is truly new
                                            // (if object is flag for initial
                                            //  creation sounds only)
                                            // Check ancestor chains for
                                            // objects that loop back to their
                                            // initial state.
                                            

                                            // also not useDummies that have 
                                            // the same parent
                                            char sameParent = false;
                                
                                            if( testAncestor > 0 && 
                                                existing->holdingID > 0 ) {
                                                
                                                int holdID =
                                                    existing->holdingID;
                                                
                                                ObjectRecord *ancObj =
                                                    getObject( testAncestor );
                                                

                                                if( heldObj->isUseDummy &&
                                                    ancObj->isUseDummy &&
                                                    heldObj->useDummyParent ==
                                                    ancObj->useDummyParent ) {
                                                    sameParent = true;
                                                    }
                                                else if( heldObj->numUses > 1 
                                                         &&
                                                         ancObj->isUseDummy 
                                                         &&
                                                         ancObj->useDummyParent
                                                         == holdID ) {
                                                    sameParent = true;
                                                    }
                                                else if( ancObj->numUses > 1 
                                                         &&
                                                         heldObj->isUseDummy 
                                                         &&
                                                         heldObj->useDummyParent
                                                         == testAncestor ) {
                                                    sameParent = true;
                                                    }
                                                }
                                                

                                            if( ! sameParent 
                                                &&
                                                (! heldObj->
                                                 creationSoundInitialOnly
                                                 ||
                                                 ( ! isSpriteSubset( 
                                                     testAncestor, 
                                                     existing->holdingID )
                                                   &&
                                                   ! isAncestor(
                                                       testAncestor,
                                                       existing->holdingID,
                                                       1 ) )
                                                 ) ) {

                                                playSound( 
                                                heldObj->creationSound,
                                                getVectorFromCamera( 
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                                creationSoundPlayed = true;
                                                }
                                            }
                                        }
                                    
                                    char autoDecay = false;
                                    
                                    if( oldHeld > 0 && 
                                        existing->holdingID > 0 ) {
                                        TransRecord *dR =
                                            getTrans( -1, oldHeld );
                                        
                                        if( dR != NULL &&
                                            dR->newActor == 0 &&
                                            dR->newTarget == 
                                            existing->holdingID ) {
                                            
                                            autoDecay = true;
                                            }
                                        }
                                    
                                    
                                    if( oldHeld == 0 ||
                                        heldContChanged || 
                                        ( heldTransitionSourceID == -1 &&
                                          !autoDecay && 
                                          ! creationSoundPlayed &&
                                          ! clothingSoundPlayed ) ) {
                                        // we're holding something new

                                        existing->lastHeldAnim = held;
                                        existing->
                                            lastHeldAnimationFrameCount = 0;
                                        existing->curHeldAnim = held;
                                        existing->heldAnimationFrameCount = 0;
                                        existing->lastHeldAnimFade = 0;
                                            
                                        
                                        if( ! otherSoundPlayed && 
                                            ! creationSoundPlayed &&
                                            ! clothingSoundPlayed ) {
                                            // play generic pickup sound

                                            ObjectRecord *existingObj = 
                                                getObject( 
                                                    existing->displayID );
                                
                                            // skip this if they are dying
                                            // because they may have picked
                                            // up a wound
                                            if( !existing->dying &&
                                                existingObj->
                                                usingSound.numSubSounds > 0 ) {
                                    
                                                playSound( 
                                                    existingObj->usingSound,
                                                    getVectorFromCamera(
                                                     existing->currentPos.x, 
                                                     existing->currentPos.y ) );
                                                otherSoundPlayed = true;
                                                }
                                            }
                                        }
                                    }
                                }
                            
                            // otherwise, don't touch frame count
                        

                            if( existing->holdingID < 0 ) {
                                // picked up a baby
                                int babyID = - existing->holdingID;
                                
                                // save ALL of these for later, after
                                // we've processed all PU lines, 
                                // because a held baby may not exist
                                // locally yet.
                                unusedHolderID.push_back( existing->id );
                                unusedHeldID.push_back( babyID );
                                }
                            }
                        

                        
                        if( existing->holdingID >= 0 &&
                            oldHeld >= 0 &&
                            oldHeld != existing->holdingID &&
                            ! heldOriginValid &&
                            heldTransitionSourceID > 0 &&
                            ! creationSoundPlayed &&
                            ! clothingSoundPlayed &&
                            ! otherSoundPlayed ) {
                            
                            
                            // what we're holding changed
                            // but no sound has been played for it yet

                            // check for special case where target
                            // of transition didn't change
                            // or is a use dummy
                            
                            TransRecord *tr = 
                                getTrans( oldHeld, 
                                          heldTransitionSourceID );
                            
                            if( tr != NULL &&
                                tr->newActor == existing->holdingID &&
                                tr->target == heldTransitionSourceID &&
                                ( tr->newTarget == tr->target 
                                  ||
                                  ( getObject( tr->target ) != NULL
                                    && 
                                    ( getObject( tr->target )->isUseDummy 
                                      ||
                                      getObject( tr->target )->numUses > 1 )
                                    ) 
                                  ||
                                  ( getObject( tr->newTarget ) != NULL
                                    && 
                                    ( getObject( tr->newTarget )->isUseDummy
                                      ||
                                      getObject( tr->newTarget )->numUses > 1 )
                                    )
                                  ) ) {
                                
                                // what about "using" sound
                                // of the target of our transition?
                                
                                char creationWillPlay = false;
                                
                                if( tr->newTarget > 0 && tr->target > 0 &&
                                    shouldCreationSoundPlay( tr->target,
                                                             tr->newTarget ) ) {
                                    creationWillPlay = true;
                                    }
                                

                                if( !creationWillPlay ) {
                                    
                                    ObjectRecord *targetObject = 
                                        getObject( heldTransitionSourceID );
                                    
                                    if( targetObject->usingSound.numSubSounds 
                                        > 0 ) {
                                        
                                        playSound( 
                                            targetObject->usingSound,
                                            getVectorFromCamera(
                                                existing->currentPos.x, 
                                                existing->currentPos.y ) );
                                        }
                                    }
                                }
                            }
                        
                        

                        
                        existing->displayID = o.displayID;
                        existing->age = o.age;
                        existing->lastAgeSetTime = o.lastAgeSetTime;
                        
                        existing->heat = o.heat;

                        if( existing->containedIDs != NULL ) {
                            delete [] existing->containedIDs;
                            }
                        if( existing->subContainedIDs != NULL ) {
                            delete [] existing->subContainedIDs;
                            }

                        existing->containedIDs = o.containedIDs;
                        existing->numContained = o.numContained;
                        existing->subContainedIDs = o.subContainedIDs;
                        
                        existing->xServer = o.xServer;
                        existing->yServer = o.yServer;
                        
                        existing->lastSpeed = o.lastSpeed;
                        
                        char babyDropped = false;
                        
                        if( done_moving && existing->heldByAdultID != -1 ) {
                            babyDropped = true;
                            }
                        
                        if( babyDropped ) {
                            // got an update for a player that's being held
                            // this means they've been dropped
                            printf( "Baby dropped\n" );
                            
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                            
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;
                            playPendingReceivedMessages( existing );
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            
                            if( existing->lastHeldByRawPosSet ) {    
                                existing->heldByDropOffset =
                                    sub( existing->lastHeldByRawPos,
                                         existing->currentPos );
                                if( length( existing->heldByDropOffset ) > 3 ) {
                                    // too far to fly during drop
                                    // snap instead
                                    existing->heldByDropOffset.x = 0;
                                    existing->heldByDropOffset.y = 0;
                                    }
                                }
                            else {
                                // held pos not known
                                // maybe this holding parent has never
                                // been drawn on the screen
                                // (can happen if they drop us during map load)
                                existing->heldByDropOffset.x = 0;
                                existing->heldByDropOffset.y = 0;
                                }
                            
                            existing->lastHeldByRawPosSet = false;
                            
                            
                            LiveObject *adultO = 
                                getGameObject( existing->heldByAdultID );
                            
                            if( adultO != NULL ) {
                                // fade from held animation
                                existing->lastAnim = 
                                    adultO->curHeldAnim;
                                        
                                existing->animationFrameCount =
                                    adultO->heldAnimationFrameCount;

                                existing->lastAnimFade = 1;
                                existing->curAnim = ground;
                                
                                adultO->holdingID = 0;
                                }

                            existing->heldByAdultID = -1;
                            }
                        else if( done_moving && forced ) {
                            
                            // don't ever force-update these for
                            // our locally-controlled object
                            // give illusion of it being totally responsive
                            // to move commands

                            // nor do we force-update remote players
                            // don't want glitches at the end of their moves

                            // UNLESS server tells us to force update
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                        
                            existing->currentSpeed = 0;
                            existing->currentGridSpeed = 0;

                            // clear an existing path, since they may no
                            // longer be on it
                            if( existing->pathToDest != NULL ) {
                                delete [] existing->pathToDest;
                                existing->pathToDest = NULL;
                                }

                            if( ! existing->somePendingMessageIsMoreMovement ) {
                                addNewAnim( existing, ground );
                                }
                            playPendingReceivedMessages( existing );

                            existing->xd = o.xd;
                            existing->yd = o.yd;

                            }
                        
                        if( existing->id == ourID ) {
                            // update for us
                            
                            if( !existing->inMotion ) {
                                // this is an update post-action, not post-move
                                
                                // ready to execute next action
                                playerActionPending = false;
                                playerActionTargetNotAdjacent = false;

                                existing->pendingAction = false;
                                }

                            if( forced ) {
                                existing->pendingActionAnimationProgress = 0;
                                existing->pendingAction = false;
                                
                                playerActionPending = false;
                                playerActionTargetNotAdjacent = false;

                                if( nextActionMessageToSend != NULL ) {
                                    delete [] nextActionMessageToSend;
                                    nextActionMessageToSend = NULL;
                                    }
                                }
                            }
                        
                        // in motion until update received, now done
                        existing->inMotion = false;
                        
                        existing->moveTotalTime = 0;

                        for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                            
                            int oldNumCont = 
                                existing->clothingContained[c].size();

                            existing->clothingContained[c].deleteAll();

                            int newNumClothingCont = 
                                o.clothingContained[c].size();
                            int *newClothingCont = 
                                o.clothingContained[c].getElementArray();
                        
                            existing->clothingContained[c].appendArray(
                                newClothingCont, newNumClothingCont );
                            delete [] newClothingCont;
                            
                            if( newNumClothingCont > oldNumCont ) {
                                // insertion
                                
                                char soundPlayed = false;
                                
                                SoundUsage contSound = 
                                    clothingByIndex( 
                                        existing->clothing, c )->usingSound;
                            
                                if( contSound.numSubSounds > 0 ) {
                                    playSound( contSound, 
                                               getVectorFromCamera( 
                                                   existing->xd,
                                                   existing->yd ) );
                                    soundPlayed = true;
                                    }

                                if( ! soundPlayed ) {
                                    // no container using sound defined
                                    // play player's using sound
                            
                                    SoundUsage s = getObject( 
                                        existing->displayID )->
                                        usingSound;

                                    if( s.numSubSounds > 0 ) {
                                        playSound( 
                                            s, 
                                            getVectorFromCamera(
                                                existing->xd,
                                                existing->yd ) );
                                        }
                                    }
                                }
                            }
                        }
                    else {    
                        o.displayChar = lastCharUsed + 1;
                    
                        lastCharUsed = o.displayChar;
                        
                        o.lastHoldingID = o.holdingID;
                        
                        if( o.holdingID < 0 ) {
                            // picked up a baby
                            int babyID = - o.holdingID;
                            
                            // save ALL of these for later, after
                            // we've processed all PU lines, 
                            // because a held baby may not exist
                            // locally yet.
                            unusedHolderID.push_back( o.id );
                            unusedHeldID.push_back( babyID );
                            }

                        o.curAnim = ground;
                        o.lastAnim = ground;
                        o.lastAnimFade = 0;

                        o.curHeldAnim = held;
                        o.lastHeldAnim = held;
                        o.lastHeldAnimFade = 0;
                        
                        o.hide = false;
                        
                        o.inMotion = false;
                        
                        o.holdingFlip = false;
                        
                        o.lastHeldByRawPosSet = false;

                        o.pendingAction = false;
                        o.pendingActionAnimationProgress = 0;
                        
                        o.currentPos.x = o.xd;
                        o.currentPos.y = o.yd;

                        o.heldPosOverride = false;
                        o.heldPosOverrideAlmostOver = false;
                        o.heldObjectPos = o.currentPos;
                        o.heldObjectRot = 0;

                        o.currentSpeed = 0;
                        o.numFramesOnCurrentStep = 0;
                        o.currentGridSpeed = 0;
                        
                        o.moveTotalTime = 0;
                        
                        o.futureAnimStack = 
                            new SimpleVector<AnimType>();
                        o.futureHeldAnimStack = 
                            new SimpleVector<AnimType>();
                        
                        
                        ObjectRecord *obj = getObject( o.displayID );
                        
                        if( obj->creationSound.numSubSounds > 0 ) {
                                
                            playSound( obj->creationSound,
                                       getVectorFromCamera( 
                                           o.currentPos.x,
                                           o.currentPos.y ) );
                            }
                        gameObjects.push_back( o );
                        }
                    }
                else if( o.id == ourID && 
                         strstr( lines[i], "X X" ) != NULL  ) {
                    // we died

                    printf( "Got X X death message for our ID %d\n",
                            ourID );

                    // get age after X X
                    char *xxPos = strstr( lines[i], "X X" );
                    
                    LiveObject *ourLiveObject = getOurLiveObject();
                    
                    if( xxPos != NULL ) {
                        sscanf( xxPos, "X X %lf", &( ourLiveObject->age ) );
                        }
                    ourLiveObject->finalAgeSet = true;
                    

                    if( mDeathReason != NULL ) {
                        delete [] mDeathReason;
                        }
                    char *reasonPos = strstr( lines[i], "reason" );
                    
                    if( reasonPos == NULL ) {
                        mDeathReason = stringDuplicate( 
                            translate( "reasonUnknown" ) );
                        }
                    else {
                        char reasonString[100];
                        reasonString[0] = '\0';
                        
                        sscanf( reasonPos, "reason_%99s", reasonString );
                        
                        if( apocalypseInProgress ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonApocalypse" ) );
                            }
                        else if( strcmp( reasonString, "nursing_hunger" ) == 
                                 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonNursingHunger" ) );
                            }
                        else if( strcmp( reasonString, "hunger" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonHunger" ) );
                            }
                        else if( strcmp( reasonString, "age" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonOldAge" ) );
                            }
                        else if( strcmp( reasonString, "disconnected" ) == 0 ) {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonDisconnected" ) );
                            }
                        else if( strstr( reasonString, "killed" ) != NULL ) {
                            
                            int weaponID = 0;
                            
                            sscanf( reasonString, "killed_%d", &weaponID );
                            
                            ObjectRecord *weaponO = NULL;
                            
                            if( weaponID > 0 ) {
                                weaponO = getObject( weaponID );
                                }
                            

                            if( weaponO == NULL ) {
                                mDeathReason = stringDuplicate( 
                                    translate( "reasonKilledUnknown" ) );
                                }
                            else {

                                char *stringUpper = stringToUpperCase( 
                                    weaponO->description );

                                stripDescriptionComment( stringUpper );


                                mDeathReason = autoSprintf( 
                                    "%s%s",
                                    translate( "reasonKilled" ),
                                    stringUpper );
                                
                                delete [] stringUpper;
                                }
                            }
                        else {
                            mDeathReason = stringDuplicate( 
                                translate( "reasonUnknown" ) );
                            }
                        }
                    
                    if( ! anyLiveTriggersLeft() ) {
                        // if we're in live trigger mode, leave 
                        // server connection open so we can see what happens
                        // next
                        closeSocket( mServerSocket );
                        mServerSocket = -1;
                        handleOurDeath();
                        }
                    else {
                        // just hide our object, but leave it around
                        getOurLiveObject()->hide = true;
                        }
                    }
                else if( strstr( lines[i], "X X" ) != NULL  ) {
                    // object deleted
                        
                    numRead = sscanf( lines[i], "%d %d",
                                      &( o.id ),
                                      &( o.holdingID ) );
                    
                    
                    for( int i=0; i<gameObjects.size(); i++ ) {
                        
                        LiveObject *nextObject =
                            gameObjects.getElement( i );
                        
                        if( nextObject->id == o.id ) {
                            
                            if( nextObject->heldByAdultID > 0 ) {
                                // baby died while held, tell
                                // parent to drop them
                                LiveObject *parent = getGameObject(
                                    nextObject->heldByAdultID );
                                
                                if( parent != NULL &&
                                    parent->holdingID == - o.id ) {
                                    parent->holdingID = 0;
                                    }
                                }
                            
                            // drop any pending messages about them
                            // we don't want these to play back later
                            for( int j=0; j<gameObjects.size(); j++ ) {
                                LiveObject *otherObject = 
                                    gameObjects.getElement( j );
                                
                                dropPendingReceivedMessagesRegardingID(
                                    otherObject, o.id );
                                }
                            
                            // play any pending messages that are
                            // waiting for them to finish their move
                            playPendingReceivedMessagesRegardingOthers( 
                                nextObject );

                            if( nextObject->containedIDs != NULL ) {
                                delete [] nextObject->containedIDs;
                                }
                            if( nextObject->subContainedIDs != NULL ) {
                                delete [] nextObject->subContainedIDs;
                                }
                            
                            if( nextObject->pathToDest != NULL ) {
                                delete [] nextObject->pathToDest;
                                }
                            
                            if( nextObject->currentSpeech != NULL ) {
                                delete [] nextObject->currentSpeech;
                                }
                            
                            if( nextObject->relationName != NULL ) {
                                delete [] nextObject->relationName;
                                }

                            if( nextObject->name != NULL ) {
                                delete [] nextObject->name;
                                }

                            delete nextObject->futureAnimStack;
                            delete nextObject->futureHeldAnimStack;

                            gameObjects.deleteElement( i );
                            break;
                            }
                        }
                    }
                
                delete [] holdingIDBuffer;
                delete [] clothingBuffer;
                
                delete [] lines[i];
                }
            
            for( int i=0; i<unusedHolderID.size(); i++ ) {
                LiveObject *existing = 
                    getGameObject( unusedHolderID.getElementDirect( i ) );
                
                LiveObject *babyO = 
                    getGameObject( unusedHeldID.getElementDirect( i ) );
                
                if( babyO != NULL && existing != NULL ) {
                    babyO->heldByAdultID = existing->id;
                    // stop crying when held
                    babyO->tempAgeOverrideSet = false;
                    
                    if( babyO->pathToDest != NULL ) {
                        // forget baby's old path
                        // they are going to be set down elsewhere
                        // far away from path
                        delete [] babyO->pathToDest;
                        babyO->pathToDest = NULL;
                        }
                    babyO->currentSpeed = 0;
                    babyO->inMotion = false;
                    
                    if( babyO->id == ourID ) {
                        if( nextActionMessageToSend != NULL ) {
                            // forget pending action, we've been interrupted
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
                        playerActionPending = false;
                        }
                    

                    existing->heldFrozenRotFrameCount =
                        babyO->frozenRotFrameCount;
                                    
                    existing->heldFrozenRotFrameCountUsed =
                        false;
                                    

                    if( babyO->lastAnimFade == 0 ) {
                        
                        existing->lastHeldAnim = 
                            babyO->curAnim;
                        
                        existing->heldAnimationFrameCount =
                            babyO->animationFrameCount;
                        
                        existing->lastHeldAnimationFrameCount =
                            babyO->lastAnimationFrameCount;

                        existing->lastHeldAnimFade = 1;
                        existing->curHeldAnim = held;
                        }
                    else {
                        // baby that we're picking up
                        // in the middle of an existing fade
                        
                        existing->lastHeldAnim = 
                            babyO->lastAnim;
                        existing->lastHeldAnimationFrameCount =
                            babyO->lastAnimationFrameCount;
                        
                        existing->curHeldAnim = 
                            babyO->curAnim;
                        
                        existing->heldAnimationFrameCount =
                            babyO->animationFrameCount;
                        
                        
                        existing->lastHeldAnimFade =
                            babyO->lastAnimFade;
                        
                        existing->futureHeldAnimStack->
                            push_back( held );
                        }
                    }
                if( existing != NULL && babyO == NULL ) {
                    // existing is holding a baby that does not exist
                    // this can happen when playing back pending PU messages
                    // when we're holding a baby that dies when we're walking
                    
                    // holding nothing now instead
                    existing->holdingID = 0;
                    }
                
                }
            


            delete [] lines;


            if( ( mFirstServerMessagesReceived & 2 ) == 0 ) {
            
                LiveObject *ourObject = 
                    gameObjects.getElement( gameObjects.size() - 1 );
                
                ourID = ourObject->id;
                
                remapRandSource.reseed( ourID );

                printf( "Got first PLAYER_UPDATE message, our ID = %d\n",
                        ourID );

                ourObject->displayChar = 'A';
                }
            
            mFirstServerMessagesReceived |= 2;
            }
        else if( type == PLAYER_MOVES_START ) {
            
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;

                double etaSec;
                
                int startX, startY;
                
                int truncated;
                
                int numRead = sscanf( lines[i], "%d %d %d %lf %lf %d",
                                      &( o.id ),
                                      &( startX ),
                                      &( startY ),
                                      &( o.moveTotalTime ),
                                      &etaSec,
                                      &truncated );

                SimpleVector<char *> *tokens =
                    tokenizeString( lines[i] );
                

                applyReceiveOffset( &startX, &startY );
                

                o.pathLength = 0;
                o.pathToDest = NULL;
                
                // require an even number at least 8
                if( tokens->size() < 8 || tokens->size() % 2 != 0 ) {
                    }
                else {                    
                    int numTokens = tokens->size();
        
                    o.pathLength = (numTokens - 6) / 2 + 1;
        
                    o.pathToDest = new GridPos[ o.pathLength ];

                    o.pathToDest[0].x = startX;
                    o.pathToDest[0].y = startY;

                    for( int e=1; e<o.pathLength; e++ ) {
            
                        char *xToken = 
                            tokens->getElementDirect( 6 + (e-1) * 2 );
                        char *yToken = 
                            tokens->getElementDirect( 6 + (e-1) * 2 + 1 );
                        
        
                        sscanf( xToken, "%d", &( o.pathToDest[e].x ) );
                        sscanf( yToken, "%d", &( o.pathToDest[e].y ) );
                        
                        // make them absolute
                        o.pathToDest[e].x += startX;
                        o.pathToDest[e].y += startY;
                        }
        
                    }

                tokens->deallocateStringElements();
                delete tokens;
                    
                
                
                
                o.moveEtaTime = etaSec + game_getCurrentTime();
                

                if( numRead == 6 && o.pathLength > 0 ) {
                
                    o.xd = o.pathToDest[ o.pathLength -1 ].x;
                    o.yd = o.pathToDest[ o.pathLength -1 ].y;
                
    
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->
                                pendingReceivedMessages.size() > 0 ) {
                                
                                // we've got older messages pending
                                // make this pending too
                                
                                printf( 
                                    "Holding PM message for %d "
                                    "until later, "
                                    "%d other messages pending for them\n",
                                    existing->id,
                                    existing->pendingReceivedMessages.size() );

                                existing->pendingReceivedMessages.push_back(
                                    autoSprintf( "PM\n%s\n#",
                                                 lines[i] ) );
                                existing->somePendingMessageIsMoreMovement =
                                    true;
                                
                                break;
                                }
                            
                            // actually playing the PM message
                            // that means nothing else is pending yet
                            existing->somePendingMessageIsMoreMovement = false;
                                
                            // receiving a PM means they aren't out of
                            // range anymore
                            existing->outOfRange = false;
                            


                            double timePassed = 
                                o.moveTotalTime - etaSec;
                            
                            double fractionPassed = 
                                timePassed / o.moveTotalTime;
                            
                            
                            // stays in motion until we receive final
                            // PLAYER_UPDATE from server telling us
                            // that move is over
                            existing->inMotion = true;
                            
                            int oldPathLength = 0;
                            GridPos oldCurrentPathPos;
                            int oldCurrentPathIndex = -1;
                            SimpleVector<GridPos> oldPath;
                            
                            if( existing->currentSpeed != 0
                                &&
                                existing->pathToDest != NULL ) {
                                
                                // an interrupted move
                                oldPathLength = existing->pathLength;
                                oldCurrentPathPos = 
                                    existing->pathToDest[
                                        existing->currentPathStep ];
                                
                                oldPath.appendArray( existing->pathToDest,
                                                     existing->pathLength );
                                oldCurrentPathIndex = existing->currentPathStep;
                                }
                            

                            if( existing->id != ourID ) {
                                // remove any double-backs from path
                                // because they confuse smooth path following
                                
                                removeDoubleBacksFromPath( &( o.pathToDest ),
                                                           &( o.pathLength ) );
                                
                                }
                            



                            if( existing->id != ourID ||
                                truncated ) {
                                // always replace path for other players
                                // with latest from server

                                // only replace OUR path if we
                                // learn that a path we submitted
                                // was truncated

                                existing->pathLength = o.pathLength;

                                if( existing->pathToDest != NULL ) {
                                    delete [] existing->pathToDest;
                                    }
                                existing->pathToDest = 
                                    new GridPos[ o.pathLength ];
                                
                                memcpy( existing->pathToDest,
                                        o.pathToDest,
                                        sizeof( GridPos ) * o.pathLength );

                                existing->xd = o.xd;
                                existing->yd = o.yd;
                                
                                if( existing->id != ourID ) {
                                    // look at how far we think object is
                                    // from current fractional position
                                    // along path

                                    int b = 
                                        (int)floor( 
                                            fractionPassed * 
                                            ( existing->pathLength - 1 ) );
                                    
                                    if( b >= existing->pathLength ) {
                                        b = existing->pathLength - 1;
                                        }

                                    // we may be getting a move for
                                    // an object that has been off-chunk
                                    // for a while and is now moving on-chunk

                                    doublePair curr;
                                    curr.x = existing->pathToDest[ b ].x;
                                    curr.y = existing->pathToDest[ b ].y;
                                    
                                    if( distance( curr, 
                                                  existing->currentPos ) >
                                        5 ) {
                                        
                                        // 5 is too far

                                        // jump right to current loc
                                        // on new path
                                        existing->currentPos = curr;
                                        }
                                    }
                                
                                }
                            


                            if( existing->id != ourID ) {
                                // don't force-update these
                                // for our object
                                // we control it locally, to keep
                                // illusion of full move interactivity
                            
                                char usingOldPathStep = false;
                                char appendingLeadPath = false;
                                
                                if( oldPathLength != 0 ) {
                                    // this move interrupts or truncates
                                    // the move we were already on

                                    // look to see if the old path step
                                    // is on our new path
                                    char found = false;
                                    int foundStep = -1;

                                    for( int p=0; 
                                         p<existing->pathLength - 1;
                                         p++ ) {
                                        
                                        if( equal( existing->pathToDest[p],
                                                   oldCurrentPathPos ) ) {
                                            
                                            found = true;
                                            foundStep = p;
                                            }
                                        }
                                    
                                    if( found ) {
                                        usingOldPathStep = true;
                                        
                                        existing->currentPathStep =
                                            foundStep;
                                        
                                        doublePair foundWorld = 
                                            gridToDouble(
                                                existing->
                                                pathToDest[ foundStep ] );

                                        // where we should move toward
                                        doublePair nextWorld;
                                        
                                        if( foundStep < 
                                            existing->pathLength - 1 ) {
                                            
                                            // point from here to our
                                            // next step
                                            nextWorld = 
                                                gridToDouble(
                                                    existing->
                                                    pathToDest[ 
                                                        foundStep + 1 ] );    
                                            }
                                        else {
                                            // at end of path, point right
                                            // to it
                                            nextWorld = foundWorld;
                                            }

                                        existing->currentMoveDirection =
                                            normalize( 
                                                sub( nextWorld, 
                                                     existing->currentPos ) );
                                        }
                                    else {
                                        // other case
                                        // check if new start on old path

                                        // maybe this new path branches
                                        // off old path before or after 
                                        // where we are
                                        
                                        int foundStartIndex = -1;
                                        
                                        for( int i=0; i<oldPathLength; i++ ) {
                                            GridPos p = 
                                                oldPath.getElementDirect( i );
                                            
                                            if( p.x == startX && 
                                                p.y == startY ) {
                                                
                                                foundStartIndex = i;
                                                break;
                                                }
                                            }
                                        
                                        if( foundStartIndex != -1 ) {
                                            
                                            int step = 1;
                                            
                                            if( foundStartIndex > 
                                                oldCurrentPathIndex ) {
                                                step = 1;
                                                }
                                            else if( foundStartIndex < 
                                                oldCurrentPathIndex ) {
                                                step = -1;
                                                }
                                            appendingLeadPath = true;
                                            
                                            SimpleVector<GridPos> newPath;
                                            
                                            for( int i=oldCurrentPathIndex;
                                                 i != foundStartIndex;
                                                 i += step ) {
                                                
                                                newPath.push_back(
                                                    oldPath.
                                                    getElementDirect( i ) );
                                                }
                                            
                                            for( int i=0; 
                                                 i<existing->pathLength;
                                                 i++ ) {
                                                // now add rest of new path
                                                newPath.push_back(
                                                    existing->pathToDest[i] );
                                                }
                                            
                                            printf( "    OLD PATH:  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );


                                            // now replace path
                                            // with new, lead-appended path
                                            existing->pathLength = 
                                                newPath.size();
                                            
                                            delete [] existing->pathToDest;
                                            
                                            existing->pathToDest =
                                                newPath.getElementArray();
                                            existing->currentPathStep = 0;
                                            existing->numFramesOnCurrentStep 
                                                = 0;
                                            

                                            printf( "    NEW PATH:  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );

                                            removeDoubleBacksFromPath( 
                                                &( existing->pathToDest ),
                                                &( existing->pathLength ) );
                                
                                            printf( 
                                                "    NEW PATH (DB free):  " );
                                            printPath( existing->pathToDest,
                                                       existing->pathLength );

                                            if( existing->pathLength == 1 ) {
                                                
                                                // trimmed path too short
                                                // needs to have at least
                                                // a start and end pos
                                                
                                                // give it an artificial
                                                // start pos
                                                

                                                doublePair nextWorld =
                                                    gridToDouble( 
                                                     existing->pathToDest[0] );

                                                
                                                doublePair vectorAway;

                                                if( ! equal( 
                                                        existing->currentPos,
                                                        nextWorld ) ) {
                                                        
                                                    vectorAway = normalize(
                                                        sub( 
                                                            existing->
                                                            currentPos,
                                                            nextWorld ) );
                                                    }
                                                else {
                                                    vectorAway.x = 1;
                                                    vectorAway.y = 0;
                                                    }
                                                
                                                GridPos oldPos =
                                                    existing->pathToDest[0];
                                                
                                                delete [] existing->pathToDest;
                                                existing->pathLength = 2;
                                                
                                                existing->pathToDest =
                                                    new GridPos[2];
                                                existing->pathToDest[0].x =
                                                    oldPos.x + vectorAway.x;
                                                existing->pathToDest[0].y =
                                                    oldPos.y + vectorAway.y;
                                                
                                                existing->pathToDest[1] =
                                                    oldPos;
                                                }
                                            
                                            
                                            doublePair nextWorld =
                                                gridToDouble( 
                                                    existing->pathToDest[1] );
                                            
                                            // point toward next path pos
                                            existing->currentMoveDirection =
                                                normalize( 
                                                    sub( nextWorld, 
                                                         existing->
                                                         currentPos ) );
                                            }    
                                        }
                                    
                                    }
                                
                                
                                if( ! usingOldPathStep && 
                                    ! appendingLeadPath ) {

                                    // we don't have enough info
                                    // to patch path
                                    
                                    // change to walking toward next
                                    // path step from wherever we are
                                    // but DON'T jump existing obj's
                                    // possition suddenly
                                    
                                    printf( "Manually forced\n" );
                                    
                                    // prev step
                                    int b = 
                                        (int)floor( 
                                            fractionPassed * 
                                            ( existing->pathLength - 1 ) );
                                    // next step
                                    int n =
                                        (int)ceil( 
                                            fractionPassed *
                                            ( existing->pathLength - 1 ) );
                                    
                                    if( n == b ) {
                                        if( n < existing->pathLength - 1 ) {
                                            n ++ ;
                                            }
                                        else {
                                            b--;
                                            }
                                        }
                                    
                                    existing->currentPathStep = b;
                                    
                                    doublePair nWorld =
                                        gridToDouble( 
                                            existing->pathToDest[n] );
                                    
                                    // point toward next path pos
                                    existing->currentMoveDirection =
                                        normalize( 
                                            sub( nWorld,
                                                 existing->currentPos ) );
                                    
                                    }
                                
                                
                                existing->moveTotalTime = o.moveTotalTime;
                                existing->moveEtaTime = o.moveEtaTime;

                                if( usingOldPathStep ) {
                                    // we're ignoring where server
                                    // says we should be

                                    // BUT, we should change speed to
                                    // compensate for the difference

                                    double oldFractionPassed =
                                        measurePathLength( 
                                            existing,
                                            existing->currentPathStep + 1 )
                                        /
                                        measurePathLength( 
                                            existing, 
                                            existing->pathLength );
                                    
                                    
                                    // if this is positive, we are
                                    // farther along than we should be
                                    // we need to slow down (moveEtaTime
                                    // should get bigger)

                                    // if negative, we are behind
                                    // we should speed up (moveEtaTime
                                    // should get smaller)
                                    double fractionDiff =
                                        oldFractionPassed - fractionPassed;
                                    
                                    double timeAdjust =
                                        existing->moveTotalTime * fractionDiff;
                                    
                                    existing->moveEtaTime += timeAdjust;
                                    }
                                

                                updateMoveSpeed( existing );
                                }
                            else if( truncated ) {
                                // adjustment to our own movement
                                
                                // cancel pending action upon arrival
                                // (no longer possible, since truncated)

                                existing->pendingActionAnimationProgress = 0;
                                existing->pendingAction = false;
                                
                                playerActionPending = false;
                                playerActionTargetNotAdjacent = false;

                                if( nextActionMessageToSend != NULL ) {
                                    delete [] nextActionMessageToSend;
                                    nextActionMessageToSend = NULL;
                                    }

                                // this path may be different
                                // from what we actually requested
                                // from sever

                                char stillOnPath = false;
                                    
                                if( oldPathLength > 0 ) {
                                    // on a path, perhaps some other one
                                    
                                    // check if our current pos
                                    // is on this new, truncated path
                                    
                                    char found = false;
                                    int foundStep = -1;
                                    for( int p=0; 
                                         p<existing->pathLength - 1;
                                         p++ ) {
                                        
                                        if( equal( existing->pathToDest[p],
                                                   oldCurrentPathPos ) ) {
                                            
                                            found = true;
                                            foundStep = p;
                                            }
                                        }
                                    
                                        if( found ) {
                                            stillOnPath = true;
                                            
                                            existing->currentPathStep =
                                                foundStep;
                                            }
                                        
                                    }
                                    


                                // only jump around if we must

                                if( ! stillOnPath ) {
                                    // path truncation from what we last knew
                                    // for ourselves, and we're off the end
                                    // of the new path
                                    
                                    // hard jump back
                                    existing->currentSpeed = 0;
                                    existing->currentGridSpeed = 0;
                                    
                                    playPendingReceivedMessages( existing );

                                    existing->currentPos.x =
                                        existing->xd;
                                    existing->currentPos.y =
                                        existing->yd;
                                    }
                                }
                            
                            

                            

                            break;
                            }
                        }
                    }
                
                if( o.pathToDest != NULL ) {
                    delete [] o.pathToDest;
                    }

                delete [] lines[i];
                }
            

            delete [] lines;
            }
        else if( type == PLAYER_SAYS ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->currentSpeech != NULL ) {
                                delete [] existing->currentSpeech;
                                existing->currentSpeech = NULL;
                                }
                            
                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {
                                existing->currentSpeech = 
                                    stringDuplicate( &( firstSpace[1] ) );
                                
                                existing->speechFade = 1.0;
                                
                                // longer time for longer speech
                                existing->speechFadeETATime = 
                                    game_getCurrentTime() + 3 +
                                    strlen( existing->currentSpeech ) / 5;

                                if( existing->age < 1 && 
                                    existing->heldByAdultID == -1 ) {
                                    // make 0-y-old unheld baby revert to 
                                    // crying age every time they speak
                                    existing->tempAgeOverrideSet = true;
                                    existing->tempAgeOverride = 0;
                                    existing->tempAgeOverrideSetTime = 
                                        game_getCurrentTime();
                                    }
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == LINEAGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->lineage.deleteAll();

                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *linStart = &( firstSpace[1] );
                                
                                SimpleVector<char *> *tokens = 
                                    tokenizeString( linStart );

                                for( int t=0; t<tokens->size(); t++ ) {
                                    char *tok = tokens->getElementDirect( t );
                                    
                                    int mID = 0;
                                    sscanf( tok, "%d", &mID );
                                    
                                    if( mID != 0 ) {
                                        existing->lineage.push_back( mID );
                                        }
                                    }
                                

                                
                                tokens->deallocateStringElements();
                                delete tokens;
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;

            // after a LINEAGE message, we should have lineage for all
            // players
            
            // now process each one and generate relation string
            LiveObject *ourObject = getOurLiveObject();
            
            for( int j=0; j<gameObjects.size(); j++ ) {
                if( gameObjects.getElement(j)->id != ourID ) {
                            
                    LiveObject *other = gameObjects.getElement(j);
                 
                    if( other->relationName == NULL ) {
                        
                        /*
                        // test
                        ourObject->lineage.deleteAll();
                        other->lineage.deleteAll();
                        
                        int cousinNum = 25;
                        int removeNum = 5;
                        
                        for( int i=0; i<=cousinNum; i++ ) {    
                            ourObject->lineage.push_back( i );
                            }

                        for( int i=0; i<=cousinNum - removeNum; i++ ) {    
                            other->lineage.push_back( 100 + i );
                            }
                        other->lineage.push_back( cousinNum );
                        */
                        other->relationName = getRelationName( ourObject,
                                                               other );
                        }
                    }
                }
            }
        else if( type == NAMES ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            if( existing->name != NULL ) {
                                delete [] existing->name;
                                }
                            
                            char *firstSpace = strstr( lines[i], " " );
        
                            if( firstSpace != NULL ) {

                                char *nameStart = &( firstSpace[1] );
                                
                                existing->name = stringDuplicate( nameStart );
                                }
                            
                            break;
                            }
                        }
                    
                    }
                
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == DYING ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->dying = true;
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == PLAYER_OUT_OF_RANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip first
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {

                int id;
                int numRead = sscanf( lines[i], "%d ",
                                      &( id ) );

                if( numRead == 1 ) {
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            existing->outOfRange = true;
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            delete [] lines;
            }
        else if( type == FOOD_CHANGE ) {
            
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( ourLiveObject != NULL ) {
                
                
                int lastAteID, lastAteFillMax;
                
                int responsiblePlayerID = -1;
                
                int foodStore;
                int foodCapacity;
                double lastSpeed;

                sscanf( message, "FX\n%d %d %d %d %lf %d", 
                        &( foodStore ),
                        &( foodCapacity ),
                        &( lastAteID ),
                        &( lastAteFillMax ),
                        &( lastSpeed ),
                        &responsiblePlayerID );
                
                if( responsiblePlayerID != -1 &&
                    getLiveObject( responsiblePlayerID ) != NULL &&
                    getLiveObject( responsiblePlayerID )->
                        pendingReceivedMessages.size() > 0 ) {
                    // someone else fed us, and they're still in the
                    // middle of a local walk with updates pending after
                    // they finish
                    
                    // defer this food update too
                    
                    LiveObject *rO = getLiveObject( responsiblePlayerID );
                    
                    printf( "Holding FX message caused by %d until later, "
                            "%d other messages pending for them\n",
                            responsiblePlayerID,
                            rO->pendingReceivedMessages.size() );
                    rO->pendingReceivedMessages.push_back(
                        stringDuplicate( message ) );
                    }
                else {
                    char foodIncreased = false;
                    int oldFoodStore = ourLiveObject->foodStore;
                    
                    if( foodCapacity == ourLiveObject->foodCapacity &&
                        foodStore > ourLiveObject->foodStore ) {
                        foodIncreased = true;
                        }
                    
                    ourLiveObject->foodStore = foodStore;
                    ourLiveObject->foodCapacity = foodCapacity;
                    ourLiveObject->lastSpeed = lastSpeed;
                    

                    if( mCurrentLastAteString != NULL ) {                    
                    

                        // one to add to erased list
                        // fade older ones first

                        for( int i=0; i<mOldLastAteStrings.size(); i++ ) {
                            float fade =
                                mOldLastAteFades.getElementDirect( i );
                        
                            if( fade > 0.5 ) {
                                fade -= 0.20;
                                }
                            else {
                                fade -= 0.1;
                                }
                        
                            *( mOldLastAteFades.getElement( i ) ) = fade;
                        

                            // bar must fade slower (different blending mode)
                            float barFade =
                                mOldLastAteBarFades.getElementDirect( i );
                        
                            barFade -= 0.01;
                        
                            *( mOldLastAteBarFades.getElement( i ) ) = barFade;
                        

                            if( fade <= 0 ) {
                                mOldLastAteStrings.deallocateStringElement( i );
                                mOldLastAteFillMax.deleteElement( i );
                                mOldLastAteFades.deleteElement( i );
                                mOldLastAteBarFades.deleteElement( i );
                                i--;
                                }

                            else if( 
                                strcmp( 
                                    mCurrentLastAteString, 
                                    mOldLastAteStrings.getElementDirect(i) )
                                == 0 ) {
                                // already in stack, move to top
                                mOldLastAteStrings.deallocateStringElement( i );
                                mOldLastAteFillMax.deleteElement( i );
                                mOldLastAteFades.deleteElement( i );
                                mOldLastAteBarFades.deleteElement( i );
                                i--;
                                }
                            }
                    
                        mOldLastAteStrings.push_back( mCurrentLastAteString );
                        mOldLastAteFillMax.push_back( mCurrentLastAteFillMax );
                        mOldLastAteFades.push_back( 1.0f );
                        mOldLastAteBarFades.push_back( 1.0f );

                        mCurrentLastAteString = NULL;
                        mCurrentLastAteFillMax = 0;
                        }
                
                    if( lastAteID != 0 ) {
                        ObjectRecord *lastAteObj = getObject( lastAteID );
                        
                        char *strUpper = stringToUpperCase(
                            lastAteObj->description );

                        const char *key = "lastAte";
                        
                        if( lastAteObj->permanent ) {
                            key = "lastAtePermanent";
                            }
                        
                        mCurrentLastAteString = 
                            autoSprintf( "%s %s",
                                         translate( key ),
                                         strUpper );
                        delete [] strUpper;
                    
                        mCurrentLastAteFillMax = lastAteFillMax;
                        }
                    else if( foodIncreased ) {
                        // we were fed, but we didn't eat anything
                        // must have been breast milk
                        mCurrentLastAteString =
                            stringDuplicate( translate( "breastMilk" ) );
                    
                        mCurrentLastAteFillMax = oldFoodStore;
                        }
                    

                    printf( "Our food = %d/%d\n", 
                            ourLiveObject->foodStore,
                            ourLiveObject->foodCapacity );
                

                    if( ourLiveObject->foodStore > 
                        ourLiveObject->maxFoodStore ) {
                        
                        ourLiveObject->maxFoodStore = ourLiveObject->foodStore;
                        }
                    if( ourLiveObject->foodCapacity > 
                        ourLiveObject->maxFoodCapacity ) {
                    
                        ourLiveObject->maxFoodCapacity = 
                            ourLiveObject->foodCapacity;
                        }
                    if( ourLiveObject->foodStore == 
                        ourLiveObject->foodCapacity ) {
                        
                        mPulseHungerSound = false;

                        mHungerSlipVisible = 0;
                        }
                    else if( ourLiveObject->foodStore <= 3 &&
                             computeCurrentAge( ourLiveObject ) < 57 ) {
                        
                        // don't play hunger sounds at end of life
                        // because it interrupts our end-of-life song
                        // currently it's 2:37 long


                        // quiet music so hunger sound can be heard
                        setMusicLoudness( 0 );
                        mHungerSlipVisible = 2;
                    
                        if( ourLiveObject->foodStore > 0 ) {
                        
                            if( ourLiveObject->foodStore > 1 ) {
                                if( mHungerSound != NULL ) {
                                    // make sure it can be heard
                                    // even if paused
                                    setSoundLoudness( 1.0 );
                                    playSoundSprite( mHungerSound );
                                    }
                                mPulseHungerSound = false;
                                }
                            else {
                                mPulseHungerSound = true;
                                }
                            }
                        }
                    else if( ourLiveObject->foodStore <= 6 ) {
                        mHungerSlipVisible = 1;
                        mPulseHungerSound = false;
                        }
                    else {
                        mHungerSlipVisible = -1;
                        }

                    if( ourLiveObject->foodStore > 3 ||
                        computeCurrentAge( ourLiveObject ) >= 57 ) {
                        // restore music
                        setMusicLoudness( musicLoudness );
                        
                        mPulseHungerSound = false;
                        }
                    
                    }
                }
            }
        
        

        delete [] message;

        // process next message if there is one
        message = getNextServerMessage();
        }
    

    if( mapPullMode && mapPullCurrentSaved && ! mapPullCurrentSent ) {
        char *message = autoSprintf( "MAP %d %d#",
                                     sendX( mapPullCurrentX ),
                                     sendY( mapPullCurrentY ) );
        
        sendToServerSocket( message );
        
        
        delete [] message;
        mapPullCurrentSent = true;
        }

        
    // check if we're about to move off the screen
    LiveObject *ourLiveObject = getOurLiveObject();


    
    if( !mapPullMode && 
        mDoneLoadingFirstObjectSet && ourLiveObject != NULL ) {
        

        // current age
        double age = computeCurrentAge( ourLiveObject );

        int sayCap = (int)( floor( age ) + 1 );
        
        if( ourLiveObject->lineage.size() == 0  && sayCap < 30 ) {
            // eve has a larger say limit
            sayCap = 30;
            }

        mSayField.setMaxLength( sayCap );



        LiveObject *cameraFollowsObject = ourLiveObject;
        
        if( ourLiveObject->heldByAdultID != -1 ) {
            cameraFollowsObject = 
                getGameObject( ourLiveObject->heldByAdultID );
            
            if( cameraFollowsObject == NULL ) {
                ourLiveObject->heldByAdultID = -1;
                cameraFollowsObject = ourLiveObject;
                }
            }
        

        doublePair screenTargetPos = 
            mult( cameraFollowsObject->currentPos, CELL_D );
        

        if( cameraFollowsObject->currentPos.x != cameraFollowsObject->xd
            ||
            cameraFollowsObject->currentPos.y != cameraFollowsObject->yd ) {
            
            // moving

            // push camera out in front
            

            double moveScale = 40 * cameraFollowsObject->currentSpeed;
            if( ( screenCenterPlayerOffsetX < 0 &&
                  cameraFollowsObject->currentMoveDirection.x < 0 )
                ||
                ( screenCenterPlayerOffsetX > 0 &&
                  cameraFollowsObject->currentMoveDirection.x > 0 ) ) {
                
                moveScale += abs( screenCenterPlayerOffsetX );
                }
            
            screenCenterPlayerOffsetX -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.x );
            
            moveScale = 40 * cameraFollowsObject->currentSpeed;
            if( ( screenCenterPlayerOffsetY < 0 &&
                  cameraFollowsObject->currentMoveDirection.y < 0 )
                ||
                ( screenCenterPlayerOffsetY > 0 &&
                  cameraFollowsObject->currentMoveDirection.y > 0 ) ) {
                
                moveScale += abs( screenCenterPlayerOffsetY );
                }
            

            screenCenterPlayerOffsetY -= 
                lrint( moveScale * 
                       cameraFollowsObject->currentMoveDirection.y );
 
            if( screenCenterPlayerOffsetX < -viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  -viewWidth / 3;
                }
            if( screenCenterPlayerOffsetX >  viewWidth / 3 ) {
                screenCenterPlayerOffsetX =  viewWidth / 3;
                }
            if( screenCenterPlayerOffsetY < -viewHeight / 5 ) {
                screenCenterPlayerOffsetY =  -viewHeight / 5;
                }
            if( screenCenterPlayerOffsetY >  viewHeight / 6 ) {
                screenCenterPlayerOffsetY =  viewHeight / 6;
                }
            }
        else if( false ) { // skip for now
            // stopped moving
            
            if( screenCenterPlayerOffsetX > 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            if( screenCenterPlayerOffsetX < 0 ) {
                int speed = screenCenterPlayerOffsetX / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetX -= speed;
                }
            
            if( screenCenterPlayerOffsetY > 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed > 2 ) {
                    speed = 1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            if( screenCenterPlayerOffsetY < 0 ) {
                int speed = screenCenterPlayerOffsetY / 10;
                
                if( speed == 0 || speed < -2 ) {
                    speed = -1;
                    }
                screenCenterPlayerOffsetY -= speed;
                }
            
            }


        screenTargetPos.x = 
            CELL_D * cameraFollowsObject->currentPos.x - 
            screenCenterPlayerOffsetX;
        
        screenTargetPos.y = 
            CELL_D * cameraFollowsObject->currentPos.y - 
            screenCenterPlayerOffsetY;
        

        // whole pixels
        screenTargetPos.x = round( screenTargetPos.x );
        screenTargetPos.y = round( screenTargetPos.y );
        

        doublePair dir = sub( screenTargetPos, lastScreenViewCenter );
        
        char viewChange = false;
        
        int maxRX = viewWidth / 15;
        int maxRY = viewHeight / 15;
        int maxR = 0;
        double moveSpeedFactor = 20 * cameraFollowsObject->currentSpeed;
        
        if( moveSpeedFactor < 1 ) {
            moveSpeedFactor = 1 * frameRateFactor;
            }

        if( abs( dir.x ) > maxRX ) {
            double moveScale = moveSpeedFactor * sqrt( abs(dir.x) - maxRX );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
                        
            if( abs( moveStep.x ) > 0 ) {
                lastScreenViewCenter.x += moveStep.x;
                viewChange = true;
                }
            }
        if( abs( dir.y ) > maxRY ) {
            double moveScale = moveSpeedFactor * sqrt( abs(dir.y) - maxRY );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.y = lrint( moveStep.y );
                        
            if( abs( moveStep.y ) > 0 ) {
                lastScreenViewCenter.y += moveStep.y;
                viewChange = true;
                }
            }
        

        if( false && length( dir ) > maxR ) {
            double moveScale = moveSpeedFactor * sqrt( length( dir ) - maxR );

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
            moveStep.y = lrint( moveStep.y );
                        
            if( length( moveStep ) > 0 ) {
                lastScreenViewCenter = add( lastScreenViewCenter, 
                                            moveStep );
                viewChange = true;
                }
            }
        

        if( viewChange ) {
            //lastScreenViewCenter.x = screenTargetPos.x;
            //lastScreenViewCenter.y = screenTargetPos.y;
            
            setViewCenterPosition( lastScreenViewCenter.x, 
                                   lastScreenViewCenter.y );
            
            }

        }
    
    
    // update all positions for moving objects
    if( !mapPullMode )
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->currentSpeech != NULL ) {
            if( game_getCurrentTime() > o->speechFadeETATime ) {
                
                o->speechFade -= 0.05 * frameRateFactor;

                if( o->speechFade <= 0 ) {
                    delete [] o->currentSpeech;
                    o->currentSpeech = NULL;
                    o->speechFade = 1.0;
                    }
                }
            }
        
        double animSpeed = o->lastSpeed;
        

        if( o->holdingID > 0 ) {
            ObjectRecord *heldObj = getObject( o->holdingID );
            
            if( heldObj->speedMult > 1.0 ) {
                // don't speed up animations just because movement
                // speed has increased
                // but DO slow animations down
                animSpeed /= heldObj->speedMult;
                }
            }

        double oldFrameCount = o->animationFrameCount;
        o->animationFrameCount += animSpeed / BASE_SPEED;
        o->lastAnimationFrameCount += animSpeed / BASE_SPEED;
        

        if( o->curAnim == moving ) {
            o->frozenRotFrameCount += animSpeed / BASE_SPEED;
            
            }
            
        
        char holdingRideable = false;
            
        if( o->holdingID > 0 &&
            getObject( o->holdingID )->rideable ) {
            holdingRideable = true;
            }
            
        
        if( o->curAnim != moving || !holdingRideable ) {
            // don't play player moving sound if riding something

            AnimType t = o->curAnim;
            doublePair pos = o->currentPos;
            
            if( o->heldByAdultID != -1 ) {
                t = held;
                

                for( int j=0; j<gameObjects.size(); j++ ) {
                    
                    LiveObject *parent = gameObjects.getElement( j );
                    
                    if( parent->id == o->heldByAdultID ) {
                        
                        pos = parent->currentPos;
                        }
                    }
                }
            
            
            handleAnimSound( o->displayID,
                             computeCurrentAge( o ),
                             t,
                             oldFrameCount, o->animationFrameCount,
                             pos.x,
                             pos.y );                
            }
            
        
        
        
        if( o->lastAnimFade > 0 ) {
            
            if( o->lastAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                if( isAnimFadeNeeded( o->displayID, 
                                      o->lastAnim, o->curAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastAnimFade = 0;
                    }
                }
            

            o->lastAnimFade -= 0.05 * frameRateFactor;
            if( o->lastAnimFade < 0 ) {
                o->lastAnimFade = 0;

                if( o->futureAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewAnimDirect( 
                        o, 
                        o->futureAnimStack->getElementDirect( 0 ) );

                    // pop from stack                    
                    o->futureAnimStack->deleteElement( 0 );
                    }
                
                }
            }

        oldFrameCount = o->heldAnimationFrameCount;
        o->heldAnimationFrameCount += animSpeed / BASE_SPEED;
        o->lastHeldAnimationFrameCount += animSpeed / BASE_SPEED;
        
        if( o->curHeldAnim == moving ) {
            o->heldFrozenRotFrameCount += animSpeed / BASE_SPEED;
            }
        
        if( o->holdingID > 0 ) {
            handleAnimSound( o->holdingID, 0, o->curHeldAnim,
                             oldFrameCount, o->heldAnimationFrameCount,
                             o->currentPos.x,
                             o->currentPos.y );
            }
        

        if( o->lastHeldAnimFade > 0 ) {
            
            if( o->lastHeldAnimFade == 1 ) {
                // fade just started
                // check if it's necessary
                
                int heldDisplayID = o->holdingID;
                

                if( o->holdingID < 0 ) {
                    LiveObject *babyO = getGameObject( - o->holdingID );
            
                    if( babyO != NULL ) {    
                        heldDisplayID = babyO->displayID;
                        }
                    else {
                        heldDisplayID = 0;
                        }
                    }
                

                if( heldDisplayID > 0 &&
                    isAnimFadeNeeded( heldDisplayID, 
                                      o->lastHeldAnim, o->curHeldAnim ) ) {
                    // fade needed, do nothing
                    }
                else {
                    // fade not needed
                    // jump to end of it
                    o->lastHeldAnimFade = 0;
                    }
                }
            

            o->lastHeldAnimFade -= 0.05 * frameRateFactor;
            if( o->lastHeldAnimFade < 0 ) {
                o->lastHeldAnimFade = 0;
                
                if( o->futureHeldAnimStack->size() > 0 ) {
                    // move on to next in stack
                    
                    addNewHeldAnimDirect( 
                        o,
                        o->futureHeldAnimStack->getElementDirect( 0 ) );
                    
                    // pop from stack
                    o->futureHeldAnimStack->deleteElement( 0 );
                    }
                
                }
            }


        if( o->currentSpeed != 0 && o->pathToDest != NULL ) {

            GridPos curStepDest = o->pathToDest[ o->currentPathStep ];
            GridPos nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
            doublePair startPos = { (double)curStepDest.x, 
                                    (double)curStepDest.y };

            doublePair endPos = { (double)nextStepDest.x, 
                                  (double)nextStepDest.y };
            
            while( distance( o->currentPos, endPos ) <= o->currentSpeed &&
                   o->currentPathStep < o->pathLength - 2 ) {
                
                // speed too great, overshooting next step
                
                o->currentPathStep ++;
                o->numFramesOnCurrentStep = 0;
                
                nextStepDest = o->pathToDest[ o->currentPathStep + 1 ];
            
                endPos.x = nextStepDest.x;
                endPos.y = nextStepDest.y;
                }


            doublePair dir = normalize( sub( endPos, o->currentPos ) );
            

            double turnFactor = 0.35;
            
            if( o->currentPathStep == o->pathLength - 2 ) {
                // last segment
                // speed up turn toward final spot so that we
                // don't miss it and circle around it forever
                turnFactor = 0.5;
                }

            if( o->currentGridSpeed > 4 ) {
                // tighten turns as speed increases to avoid
                // circling around forever
                turnFactor *= o->currentGridSpeed / 4;
                }
            
            
            if( dot( dir, o->currentMoveDirection ) >= 0 ) {
                // a right angle turn or smaller

            
                o->currentMoveDirection = 
                    normalize( add( o->currentMoveDirection, 
                                    mult( dir, 
                                          turnFactor * frameRateFactor ) ) );
                }
            else {
                // a double-back in the path
                // don't tot smooth turn through this, because
                // it doesn't resolve
                // instead, just turn sharply
                o->currentMoveDirection = dir;
                }
            
            if( o->numFramesOnCurrentStep * o->currentSpeed  * frameRateFactor
                > 2 ) {
                // spent twice as much time reaching this tile as we should
                // may be circling
                // go directly there instead
                o->currentMoveDirection = dir;
                }

            if( o->currentGridSpeed * frameRateFactor > 12 ) {
                // at high speed, can't round turns at all without circling
                o->currentMoveDirection = dir;
                }
            
            
            

            // don't change flip unless moving substantially in x
            if( fabs( o->currentMoveDirection.x ) > 0.5 ) {
                if( o->currentMoveDirection.x > 0 ) {
                    o->holdingFlip = false;
                    }
                else {
                    o->holdingFlip = true;
                    }         
                }
            

            if( o->currentPathStep < o->pathLength - 2 ) {

                o->currentPos = add( o->currentPos,
                                     mult( o->currentMoveDirection,
                                           o->currentSpeed ) );
                
                addNewAnim( o, moving );
                
                
                if( pathStepDistFactor * distance( o->currentPos,
                                    startPos )
                    >
                    distance( o->currentPos,
                              endPos ) ) {
                    
                    o->currentPathStep ++;
                    o->numFramesOnCurrentStep = 0;
                    }
                }
            else {

                if( o->id == ourID && mouseDown ) {
                    float worldMouseX, worldMouseY;
                    
                    screenToWorld( lastScreenMouseX,
                                   lastScreenMouseY,
                                   &worldMouseX,
                                   &worldMouseY );

                    doublePair worldMouse = { worldMouseX, worldMouseY };
                    
                    doublePair worldCurrent = mult( o->currentPos,
                                                    CELL_D );
                    doublePair delta = sub( worldMouse, worldCurrent );

                    // if player started by clicking on nothing
                    // allow continued movement right away
                    // however, if they started by clicking on something
                    // make sure they are really holding the mouse down
                    // (give them time to unpress the mouse)
                    if( nextActionMessageToSend == NULL ||
                        mouseDownFrames >  
                        minMouseDownFrames / frameRateFactor ) {
                        
                        double absX = abs( delta.x );
                        double absY = abs( delta.y );
                        

                        if( absX > CELL_D * 1 
                            ||
                            absY > CELL_D * 1 ) {
                            
                            if( ( absX < CELL_D * 4 ||
                                  absY < CELL_D * 4 ) 
                                &&
                                mouseDownFrames >  
                                minMouseDownFrames / frameRateFactor ) {
                                
                                // they're holding mouse down very close
                                // to to their character
                                
                                // throw mouse way out, further in the same
                                // direction
                                
                                // we don't want to repeatedly find a bunch
                                // of short-path moves when mouse
                                // is held down
                            
                                doublePair mouseVector =
                                    mult( 
                                        normalize( 
                                            sub( worldMouse, worldCurrent ) ),
                                        CELL_D * 4 );
                                
                                doublePair fakeClick = add( worldCurrent,
                                                            mouseVector );
                                
                                o->useWaypoint = true;
                                o->maxWaypointPathLength = 5;
                                
                                o->waypointX = lrint( worldMouseX / CELL_D );
                                o->waypointY = lrint( worldMouseY / CELL_D );

                                pointerDown( fakeClick.x, fakeClick.y );
                               
                                o->useWaypoint = false;
                                }
                            else {
                                pointerDown( worldMouseX, worldMouseY );
                                }
                            }
                        }
                    }

                if( distance( endPos, o->currentPos )
                    < o->currentSpeed ) {

                    // reached destination
                    o->currentPos = endPos;
                    o->currentSpeed = 0;
                    o->currentGridSpeed = 0;
                    
                    playPendingReceivedMessages( o );
                    
                    //trailColor.r = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.g = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.b = randSource.getRandomBoundedDouble( 0, .5 );
                    

                    if( ( o->id != ourID && 
                          ! o->somePendingMessageIsMoreMovement ) 
                        ||
                        ( o->id == ourID && 
                          nextActionMessageToSend == NULL ) ) {
                        
                        // simply stop walking
                        if( o->holdingID != 0 ) {
                            addNewAnim( o, ground2 );
                            }
                        else {
                            addNewAnim( o, ground );
                            }
                        }

                    printf( "Reached dest %f seconds early\n",
                            o->moveEtaTime - game_getCurrentTime() );
                    }
                else {

                    addNewAnim( o, moving );
                    
                    o->currentPos = add( o->currentPos,
                                         mult( o->currentMoveDirection,
                                               o->currentSpeed ) );

                    if( 1.5 * distance( o->currentPos,
                                        startPos )
                        >
                        distance( o->currentPos,
                                  endPos ) ) {
                        
                        o->onFinalPathStep = true;
                        }
                    }
                }
            
            // correct move speed based on how far we have left to go
            // and eta wall-clock time
            
            // make this correction once per second
            if( game_getCurrentTime() - o->timeOfLastSpeedUpdate
                > .25 ) {
    
                //updateMoveSpeed( o );
                }
            
            o->numFramesOnCurrentStep++;
            }
        
        
        

        if( o->id == ourID &&
            ( o->pendingAction || o->pendingActionAnimationProgress != 0 ) ) {
            
            o->pendingActionAnimationProgress += 0.025 * frameRateFactor;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                if( o->pendingAction ) {
                    // still pending, wrap around smoothly
                    o->pendingActionAnimationProgress -= 1;
                    }
                else {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
                    o->actionTargetTweakX = 0;
                    o->actionTargetTweakY = 0;
                    }
                }
            }
        else if( o->id != ourID && o->pendingActionAnimationProgress != 0 ) {
            
            o->pendingActionAnimationProgress += 0.025 * frameRateFactor;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
                    o->actionTargetTweakX = 0;
                    o->actionTargetTweakY = 0;
                    }
                }
            }
    

    if( nextActionMessageToSend != NULL
        && ourLiveObject != NULL
        && ourLiveObject->currentSpeed == 0 
        && (
            isGridAdjacent( ourLiveObject->xd, ourLiveObject->yd,
                            playerActionTargetX, playerActionTargetY )
            ||
            ( ourLiveObject->xd == playerActionTargetX &&
              ourLiveObject->yd == playerActionTargetY ) 
            ||
            playerActionTargetNotAdjacent ) ) {
        
        // done moving on client end
        // can start showing pending action animation, even if 
        // end of motion not received from server yet

        if( !playerActionPending ) {
            playerActionPending = true;
            ourLiveObject->pendingAction = true;
            
            // start on first frame to force at least one cycle no
            // matter how fast the server responds
            ourLiveObject->pendingActionAnimationProgress = 
                0.025 * frameRateFactor;

            ourLiveObject->pendingActionAnimationStartTime = 
                game_getCurrentTime();
            
            if( nextActionEating ) {
                addNewAnim( ourLiveObject, eating );
                }
            else {
                addNewAnim( ourLiveObject, doing );
                }
            }
        
        
        // wait until 
        // we've stopped moving locally
        // AND animation has played for a bit
        // AND server agrees with our position
        if( ! ourLiveObject->inMotion && 
            ourLiveObject->pendingActionAnimationProgress > 0.25 &&
            ourLiveObject->xd == ourLiveObject->xServer &&
            ourLiveObject->yd == ourLiveObject->yServer ) {
            
 
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            sendToServerSocket( nextActionMessageToSend );
            

            if( nextActionEating ) {
                // don't play eating sound here until 
                // we hear from server that we actually ate    
                if( false && ourLiveObject->holdingID > 0 ) {
                    ObjectRecord *held = getObject( ourLiveObject->holdingID );
                    
                    if( held->eatingSound.numSubSounds > 0 ) {
                        playSound( held->eatingSound,
                                   getVectorFromCamera( 
                                       ourLiveObject->currentPos.x, 
                                       ourLiveObject->currentPos.y ) );
                        }       
                    }
                }
            

            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
        }



    

    if( mFirstServerMessagesReceived == 3 ) {

        if( mStartedLoadingFirstObjectSet && ! mDoneLoadingFirstObjectSet ) {
            mDoneLoadingFirstObjectSet = 
                isLiveObjectSetFullyLoaded( &mFirstObjectSetLoadingProgress );
            
            if( mDoneLoadingFirstObjectSet ) {
                printf( "First map load done\n" );
                
                restartMusic( computeCurrentAge( ourLiveObject ),
                              ourLiveObject->ageRate );
                setSoundLoudness( 1.0 );
                resumePlayingSoundSprites();
                setMusicLoudness( musicLoudness );

                // center view on player's starting position
                lastScreenViewCenter.x = CELL_D * ourLiveObject->xd;
                lastScreenViewCenter.y = CELL_D * ourLiveObject->yd;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );

                mapPullMode = 
                    SettingsManager::getIntSetting( "mapPullMode", 0 );
                mapPullStartX = 
                    SettingsManager::getIntSetting( "mapPullStartX", -10 );
                mapPullStartY = 
                    SettingsManager::getIntSetting( "mapPullStartY", -10 );
                mapPullEndX = 
                    SettingsManager::getIntSetting( "mapPullEndX", 10 );
                mapPullEndY = 
                    SettingsManager::getIntSetting( "mapPullEndY", 10 );
                
                mapPullCurrentX = mapPullStartX;
                mapPullCurrentY = mapPullStartY;

                if( mapPullMode ) {
                    mMapGlobalOffset.x = mapPullCurrentX;
                    mMapGlobalOffset.y = mapPullCurrentY;
                    mMapGlobalOffsetSet = true;
                    
                    applyReceiveOffset( &mapPullCurrentX, &mapPullCurrentY );
                    applyReceiveOffset( &mapPullStartX, &mapPullStartY );
                    applyReceiveOffset( &mapPullEndX, &mapPullEndY );
                

                    mapPullCurrentSaved = true;
                    mapPullModeFinalImage = false;
                    
                    char *message = autoSprintf( "MAP %d %d#",
                                                 sendX( mapPullCurrentX ),
                                                 sendY( mapPullCurrentY ) );

                    sendToServerSocket( message );
               
                    mapPullCurrentSent = true;

                    delete [] message;

                    int screenWidth, screenHeight;
                    getScreenDimensions( &screenWidth, &screenHeight );

                    double scale = screenWidth / (double)screenW;

                    mapPullTotalImage = 
                        new Image( lrint(
                                       ( 10 + mapPullEndX - mapPullStartX ) 
                                       * CELL_D * scale ),
                                   lrint( ( 6 + mapPullEndY - mapPullStartY ) 
                                          * CELL_D * scale ),
                                   3, false );
                    numScreensWritten = 0;
                    }
                }
            }
        else {
            
            clearLiveObjectSet();
            
            // FIXME:
            // push all objects from grid, live players, what they're holding
            // and wearing into live set

            // first, players
            for( int i=0; i<gameObjects.size(); i++ ) {
                LiveObject *o = gameObjects.getElement( i );
                
                addBaseObjectToLiveObjectSet( o->displayID );
                
                
                if( ! o->allSpritesLoaded ) {
                    // check if they're loaded yet
                    
                    int numLoaded = 0;

                    ObjectRecord *displayObj = getObject( o->displayID );
                    for( int s=0; s<displayObj->numSprites; s++ ) {
                        
                        if( markSpriteLive( displayObj->sprites[s] ) ) {
                            numLoaded ++;
                            }
                        }
                    
                    if( numLoaded == displayObj->numSprites ) {
                        o->allSpritesLoaded = true;
                        }
                    }
                

                // and what they're holding
                if( o->holdingID > 0 ) {
                    addBaseObjectToLiveObjectSet( o->holdingID );

                    // and what it contains
                    for( int j=0; j<o->numContained; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            o->containedIDs[j] );
                        
                        for( int s=0; s<o->subContainedIDs[j].size(); s++ ) {
                            addBaseObjectToLiveObjectSet(
                                o->subContainedIDs[j].getElementDirect( s ) );
                            }
                        }
                    }
                
                // and their clothing
                for( int c=0; c<NUM_CLOTHING_PIECES; c++ ) {
                    ObjectRecord *cObj = clothingByIndex( o->clothing, c );
                    
                    if( cObj != NULL ) {
                        addBaseObjectToLiveObjectSet( cObj->id );

                        // and what it containes
                        for( int cc=0; 
                             cc< o->clothingContained[c].size(); cc++ ) {
                            int ccID = 
                                o->clothingContained[c].getElementDirect( cc );
                            addBaseObjectToLiveObjectSet( ccID );
                            }
                        }
                    }
                }
            
            
            // next all objects in grid
            int numMapCells = mMapD * mMapD;
            
            for( int i=0; i<numMapCells; i++ ) {
                if( mMapFloors[i] > 0 ) {
                    addBaseObjectToLiveObjectSet( mMapFloors[i] );
                    }
                
                if( mMap[i] > 0 ) {
                    
                    addBaseObjectToLiveObjectSet( mMap[i] );
            
                    // and what is contained in each object
                    int numCont = mMapContainedStacks[i].size();
                    
                    for( int j=0; j<numCont; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            mMapContainedStacks[i].getElementDirect( j ) );
                        
                        SimpleVector<int> *subVec =
                            mMapSubContainedStacks[i].getElement( j );
                        
                        int numSub = subVec->size();
                        for( int s=0; s<numSub; s++ ) {
                            addBaseObjectToLiveObjectSet( 
                                subVec->getElementDirect( s ) );
                            }
                        }
                    }
                }

            finalizeLiveObjectSet();
            
            mStartedLoadingFirstObjectSet = true;
            }
        }
    
    }


  
void LivingLifePage::makeActive( char inFresh ) {
    // unhold E key
    mEKeyDown = false;
    mouseDown = false;
    
    screenCenterPlayerOffsetX = 0;
    screenCenterPlayerOffsetY = 0;
    

    mLastMouseOverID = 0;
    mCurMouseOverID = 0;
    mCurMouseOverFade = 0;
    mCurMouseOverBehind = false;
    mCurMouseOverPerson = false;
    mCurMouseOverSelf = false;

    mPrevMouseOverSpots.deleteAll();
    mPrevMouseOverSpotFades.deleteAll();
    mPrevMouseOverSpotsBehind.deleteAll();
    
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    mCurMouseOverCellFade = 0;
    mLastClickCell.x = -1;
    mLastClickCell.y = -1;
    
    
    mPrevMouseOverCells.deleteAll();
    mPrevMouseOverCellFades.deleteAll();

    mPrevMouseClickCells.deleteAll();
    mPrevMouseClickCellFades.deleteAll();

    

    if( !inFresh ) {
        return;
        }

    mRemapDelay = 0;
    mRemapPeak = 0;
    mRemapDirection = 1.0;
    mCurrentRemapFraction = 0;
    
    apocalypseInProgress = false;

    clearMap();
    
    mMapExtraMovingObjects.deleteAll();
    mMapExtraMovingObjectsDestWorldPos.deleteAll();
    mMapExtraMovingObjectsDestObjectIDs.deleteAll();
            

    mMapGlobalOffsetSet = false;
    mMapGlobalOffset.x = 0;
    mMapGlobalOffset.y = 0;
    
    
    mNotePaperPosOffset = mNotePaperHideOffset;

    mNotePaperPosTargetOffset = mNotePaperPosOffset;

    
    mHomeSlipPosOffset = mHomeSlipHideOffset;
    mHomeSlipPosTargetOffset = mHomeSlipPosOffset;
    

    mLastKnownNoteLines.deallocateStringElements();
    mErasedNoteChars.deleteAll();
    mErasedNoteCharOffsets.deleteAll();
    mErasedNoteCharFades.deleteAll();
    
    mSentChatPhrases.deallocateStringElements();

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        mHungerSlipWiggleTime[i] = 0;
        }
    mHungerSlipVisible = -1;

    mSayField.setText( "" );
    mSayField.unfocus();

    mOldArrows.deleteAll();
    mOldDesStrings.deallocateStringElements();
    mOldDesFades.deleteAll();

    mOldLastAteStrings.deallocateStringElements();
    mOldLastAteFillMax.deleteAll();
    mOldLastAteFades.deleteAll();
    mOldLastAteBarFades.deleteAll();
    
    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    if( mCurrentDes != NULL ) {
        delete [] mCurrentDes;
        }
    mCurrentDes = NULL;

    if( mCurrentLastAteString != NULL ) {
        delete [] mCurrentLastAteString;
        }
    mCurrentLastAteString = NULL;


    lastScreenViewCenter.x = 0;
    lastScreenViewCenter.y = 0;
    
    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );
    
    mPageStartTime = game_getCurrentTime();
    
    setWaiting( true, false );

    readyPendingReceivedMessages.deallocateStringElements();

    if( pendingMapChunkMessage != NULL ) {
        delete [] pendingMapChunkMessage;
        pendingMapChunkMessage = NULL;
        }
    pendingCMData = false;
    

    clearLiveObjects();
    mFirstServerMessagesReceived = 0;
    mStartedLoadingFirstObjectSet = false;
    mDoneLoadingFirstObjectSet = false;
    mFirstObjectSetLoadingProgress = 0;

    playerActionPending = false;
    
    serverSocketBuffer.deleteAll();

    if( nextActionMessageToSend != NULL ) {    
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    
    homePosStack.deleteAll();

    for( int i=0; i<NUM_HOME_ARROWS; i++ ) {
        mHomeArrowStates[i].solid = false;
        mHomeArrowStates[i].fade = 0;
        }
    }







void LivingLifePage::checkForPointerHit( PointerHitRecord *inRecord,
                                         float inX, float inY ) {
    
    LiveObject *ourLiveObject = getOurLiveObject();

    double minDistThatHits = 2.0;

    int clickDestX = lrintf( ( inX ) / CELL_D );
    
    int clickDestY = lrintf( ( inY ) / CELL_D );
    
    float clickExtraX = inX - clickDestX * CELL_D;
    float clickExtraY = inY - clickDestY * CELL_D;

    PointerHitRecord *p = inRecord;
    
    p->closestCellX = clickDestX;
    p->closestCellY = clickDestY;

    
    int clickDestMapX = clickDestX - mMapOffsetX + mMapD / 2;
    int clickDestMapY = clickDestY - mMapOffsetY + mMapD / 2;
    
    int clickDestMapI = clickDestMapY * mMapD + clickDestMapX;
    
    if( clickDestMapY >= 0 && clickDestMapY < mMapD &&
        clickDestMapX >= 0 && clickDestMapX < mMapD ) {
                
        int oID = mMap[ clickDestMapI ];

        if( ! ourLiveObject->inMotion 
            &&
            // don't allow them to mouse over clicked cell after click
            // until they mouse out and back in again
            ( clickDestMapX != mLastClickCell.x ||
              clickDestMapY != mLastClickCell.y )  
            &&
            ( clickDestMapX != mCurMouseOverCell.x ||
              clickDestMapY != mCurMouseOverCell.y ) ) {

            GridPos newPos = { clickDestMapX, clickDestMapY };

            float oldFade = -0.8f;
            
            for( int i=0; i<mPrevMouseOverCells.size(); i++ ) {
                GridPos old = mPrevMouseOverCells.getElementDirect( i );
            
                if( equal( old, mCurMouseOverCell ) ) {
                    
                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                else if( equal( old, newPos ) ) {
                    oldFade = mPrevMouseOverCellFades.getElementDirect( i );

                    mPrevMouseOverCells.deleteElement( i );
                    mPrevMouseOverCellFades.deleteElement( i );
                    }
                }

            mPrevMouseOverCells.push_back( mCurMouseOverCell );
            mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );

            mCurMouseOverCell.x = clickDestMapX;
            mCurMouseOverCell.y = clickDestMapY;
            
            // moused somewhere new, clear last click
            mLastClickCell.x = -1;
            mLastClickCell.y = -1;
            

            mCurMouseOverCellFadeRate = 0.04;
            
            if( oldFade < 0 && oID > 0 ) {
                // show cell instantly when mousing over and occupied space
                oldFade = 0;
                mCurMouseOverCellFadeRate = 0.2;
                }
            
            mCurMouseOverCellFade = oldFade;
            }
        
        
        // check this cell first

        // all short objects are mouse-through-able
        
        if( oID > 0 && 
            getObjectHeight( oID ) < CELL_D ) {
            ObjectRecord *obj = getObject( oID );
            

            float thisObjClickExtraX = clickExtraX;
            float thisObjClickExtraY = clickExtraY;
            
            if( mMapMoveSpeeds[ clickDestMapI ] > 0 ) {
                thisObjClickExtraX -= 
                    mMapMoveOffsets[ clickDestMapI ].x * CELL_D;
                thisObjClickExtraY -= 
                    mMapMoveOffsets[ clickDestMapI ].y * CELL_D;
                }
            
            int sp, cl, sl;
                
            double dist = getClosestObjectPart( 
                obj,
                NULL,
                &( mMapContainedStacks[clickDestMapI] ),
                NULL,
                false,
                -1,
                -1,
                mMapTileFlips[ clickDestMapI ],
                thisObjClickExtraX,
                thisObjClickExtraY,
                &sp, &cl, &sl,
                // ignore transparent parts
                // allow objects behind smoke to be picked up
                false );
            
            if( dist < minDistThatHits ) {
                p->hitOurPlacement = true;
                p->closestCellX = clickDestX;
                p->closestCellY = clickDestY;
                
                p->hitSlotIndex = sl;
                
                p->hitAnObject = true;
                }
            }
        }
    

    // need to check 3 around cell in all directions because
    // of moving object offsets

    // start in front row
    // right to left
    // (check things that are in front first
    for( int y=clickDestY-3; y<=clickDestY+3 && ! p->hit; y++ ) {
        float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;

        // first all non drawn-behind map objects in this row
        // (in front of people)
        for( int x=clickDestX+3; x>=clickDestX-3  && 
                 ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;
            
            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }
            

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                ! getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                
                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;

                        p->hitAnObject = true;
                        }
                    }
                }
            }

        // don't worry about p->hitOurPlacement when checking them
        // next, people in this row
        // recently dropped babies are in front and tested first
        for( int d=0; d<2 && ! p->hit; d++ )
        for( int x=clickDestX+1; x>=clickDestX-1 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;
            
            for( int i=gameObjects.size()-1; i>=0 && ! p->hit; i-- ) {
        
                LiveObject *o = gameObjects.getElement( i );

                if( o->outOfRange ) {
                    // out of range, but this was their last known position
                    // don't draw now
                    continue;
                    }
                
                if( o->heldByAdultID != -1 ) {
                    // held by someone else, don't draw now
                    continue;
                    }

                if( d == 1 &&
                    ( o->heldByDropOffset.x != 0 ||
                      o->heldByDropOffset.y != 0 ) ) {
                    // recently dropped baby, skip
                    continue;
                    }
                else if( d == 0 &&
                         o->heldByDropOffset.x == 0 &&
                         o->heldByDropOffset.y == 0 ) {
                    // not a recently-dropped baby, skip
                    continue;
                    }
                
                
                int oX = o->xd;
                int oY = o->yd;
                
                if( o->currentSpeed != 0 && o->pathToDest != NULL ) {
                    if( o->onFinalPathStep ) {
                        oX = o->pathToDest[ o->pathLength - 1 ].x;
                        oY = o->pathToDest[ o->pathLength - 1 ].y;
                        }
                    else {
                        oX = o->pathToDest[ o->currentPathStep ].x;
                        oY = o->pathToDest[ o->currentPathStep ].y;
                        }
                    }
                
                
                if( oY == y && oX == x ) {
                    // here!
                    ObjectRecord *obj = getObject( o->displayID );
                    
                    int sp, cl, sl;
                    
                    double dist = getClosestObjectPart( 
                        obj,
                        &( o->clothing ),
                        NULL,
                        o->clothingContained,
                        false,
                        computeCurrentAge( o ),
                        -1,
                        o->holdingFlip,
                        clickOffsetX,
                        clickOffsetY,
                        &sp, &cl, &sl );
                    
                    if( dist < minDistThatHits ) {
                        p->hit = true;
                        p->closestCellX = x;
                        p->closestCellY = y;
                        if( o == ourLiveObject ) {
                            p->hitSelf = true;

                            }
                        else {
                            p->hitOtherPerson = true;
                            p->hitOtherPersonID = o->id;
                            }
                        if( cl != -1 ) {
                            p->hitClothingIndex = cl;
                            p->hitSlotIndex = sl;
                            }
                        }
                    }
                }
            
            }
        
        // now drawn-behind objects in this row

        for( int x=clickDestX+1; x>=clickDestX-1  
                 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;

            if( mapY < 0 || mapY >= mMapD || 
                mapX < 0 || mapX >= mMapD ) { 
                // skip any that are out of map bounds
                continue;
                }

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                

                float thisObjClickOffsetX = clickOffsetX;
                float thisObjClickOffsetY = clickOffsetY;
            
                if( mMapMoveSpeeds[ mapI ] > 0 ) {
                    thisObjClickOffsetX -= 
                        mMapMoveOffsets[ mapI ].x * CELL_D;
                    thisObjClickOffsetY -= 
                        mMapMoveOffsets[ mapI ].y * CELL_D;
                    }


                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    false,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    thisObjClickOffsetX,
                    thisObjClickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    
                                        
                    // already hit a short object
                    // AND this object is tall
                    // (don't click through short behind short)
                    if( p->hitOurPlacement &&
                        getObjectHeight( oID ) > CELL_D ) {
                        
                        if( p->closestCellY > y ) {
                            p->hitOurPlacementBehind = true;
                            }
                        }
                    else {
                        p->closestCellX = x;
                        p->closestCellY = y;
                        
                        p->hitSlotIndex = sl;
                        
                        p->hitAnObject = true;
                        }
                    }
                }
            }
        }

    if( p->hitOurPlacement &&
        ( p->hitOtherPerson || p->hitSelf ) ) {
        p->hitAnObject = false;
        }
    else {
        // count it now, if we didn't hit a person that blocks it
        p->hit = true;
        }
    
    }

    


        





void LivingLifePage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
        
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    

    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;

    p.hitOtherPerson = false;

    checkForPointerHit( &p, inX, inY );

    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;
    
    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;
    

    int destID = 0;
        
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    if( p.hitAnObject && mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        }



    if( mCurMouseOverID > 0 
        && 
        ! mCurMouseOverSelf
        &&
        ( mCurMouseOverID != destID
          ||
          mCurMouseOverSpot.x != mapX ||
          mCurMouseOverSpot.y != mapY ) ) {
        
        GridPos prev = { mCurMouseOverSpot.x, mCurMouseOverSpot.y };
        
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, prev ) ) {
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        
        mPrevMouseOverSpots.push_back( prev );
        mPrevMouseOverSpotFades.push_back( mCurMouseOverFade );
        mPrevMouseOverSpotsBehind.push_back( mCurMouseOverBehind );
        }

    char overNothing = true;
    
    if( destID == 0 ) {
        if( p.hitSelf ) {
            mCurMouseOverSelf = true;
            
            // clear when mousing over bare parts of body
            // show YOU
            mCurMouseOverID = -99;
            
            overNothing = false;
            
            LiveObject *ourLiveObject = getOurLiveObject();
            
            if( p.hitClothingIndex != -1 ) {
                if( p.hitSlotIndex != -1 ) {
                    mCurMouseOverID = 
                        ourLiveObject->clothingContained[ p.hitClothingIndex ].
                        getElementDirect( p.hitSlotIndex );
                    }
                else {
                    ObjectRecord *c = 
                        clothingByIndex( ourLiveObject->clothing, 
                                         p.hitClothingIndex );
                    mCurMouseOverID = c->id;
                    }
                }
            }
        if( p.hitOtherPerson ) {
            // store negative in place so that we can show their relation
            // string
            mCurMouseOverID = - p.hitOtherPersonID;
            }
        }
    

    if( destID > 0 ) {
        mCurMouseOverSelf = false;
        
        if( ( destID != mCurMouseOverID ||
              mCurMouseOverSpot.x != mapX ||
              mCurMouseOverSpot.y != mapY  ) ) {
            
            // start new fade-in
            mCurMouseOverFade = 0;
            }

        mCurMouseOverID = destID;
        overNothing = false;
        
        if( p.hitSlotIndex != -1 ) {
            mCurMouseOverID = 
                mMapContainedStacks[ mapY * mMapD + mapX ].
                getElementDirect( p.hitSlotIndex );
            }
        
        
        mCurMouseOverSpot.x = mapX;
        mCurMouseOverSpot.y = mapY;
        
        mCurMouseOverBehind = p.hitOurPlacementBehind;
        

        // check if we already have a partial-fade-out for this cell
        // from previous mouse-overs
        for( int i=0; i<mPrevMouseOverSpots.size(); i++ ) {
            GridPos old = mPrevMouseOverSpots.getElementDirect( i );
            
            if( equal( old, mCurMouseOverSpot ) ) {
                mCurMouseOverFade = 
                    mPrevMouseOverSpotFades.getElementDirect( i );
                
                mPrevMouseOverSpots.deleteElement( i );
                mPrevMouseOverSpotFades.deleteElement( i );
                mPrevMouseOverSpotsBehind.deleteElement( i );
                break;
                }
            }
        }
    
    
    if( overNothing && mCurMouseOverID != 0 ) {
        mLastMouseOverID = mCurMouseOverID;
        
        if( mCurMouseOverSelf ) {
            // don't keep YOU or EAT or clothing tips around after we mouse 
            // outside of them
            mLastMouseOverID = 0;
            }
        mCurMouseOverSelf = false;
        
        mCurMouseOverID = 0;
        mCurMouseOverBehind = false;
        mLastMouseOverFade = 1.0f;
        }
    
    }




char LivingLifePage::getCellBlocksWalking( int inMapX, int inMapY ) {
    
    if( inMapY >= 0 && inMapY < mMapD &&
        inMapX >= 0 && inMapX < mMapD ) {
        
        int destID = mMap[ inMapY * mMapD + inMapX ];
        
        
        if( destID > 0 && getObject( destID )->blocksWalking ) {
            return true;
            }
        else {
            // check for wide neighbors
            
            int r = getMaxWideRadius();
            
            int startX = inMapX - r;
            int endX = inMapX + r;
            
            if( startX < 0 ) {
                startX = 0;
                }
            if( endX >= mMapD ) {
                endX = mMapD - 1;
                }
            for( int x=startX; x<=endX; x++ ) {
                int nID = mMap[ inMapY * mMapD + x ];

                if( nID > 0 ) {
                    ObjectRecord *nO = getObject( nID );
                    
                    if( nO->blocksWalking && nO->wide &&
                        ( x - inMapX <= nO->rightBlockingRadius || 
                          inMapX - x >= nO->leftBlockingRadius ) ) {
                        return true;
                        }
                    }
                }
            }
        
        return false;
        }
    else {
        // off map blocks
        return true;
        }
    }




void LivingLifePage::pointerDown( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    char modClick = false;
    
    if( ( mEKeyDown && mEKeyEnabled ) || isLastMouseButtonRight() ) {
        modClick = true;
        }
    
    mLastMouseOverID = 0;
    
    // detect cases where mouse is held down already
    // this is for long-distance player motion, and we don't want
    // this to result in an action by accident if they mouse over
    // something actionable along the way
    char mouseAlreadyDown = mouseDown;
    
    mouseDown = true;
    if( !mouseAlreadyDown ) {
        mouseDownFrames = 0;
        }
    
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        printf( "Skipping click, action pending\n" );
        
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject = getOurLiveObject();

    if( ourLiveObject->heldByAdultID != -1 ) {
        // click from a held baby
        
        // send dummy move message right away to make baby jump out of arms

        // we don't need to use baby's true position here... they are
        // in arms
        char *moveMessage = autoSprintf( "MOVE %d %d 1 0#",
                                         ourLiveObject->xd,
                                         ourLiveObject->yd );
        sendToServerSocket( moveMessage );
        delete [] moveMessage;
        return;
        }
    

    // consider 3x4 area around click and test true object pixel
    // collisions in that area
    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    
    p.hit = false;
    p.hitSelf = false;
    p.hitOurPlacement = false;
    p.hitOurPlacementBehind = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;
    
    p.hitOtherPerson = false;
    

    checkForPointerHit( &p, inX, inY );
    
    mCurMouseOverPerson = p.hitOtherPerson || p.hitSelf;

    // don't allow clicking on object during continued motion
    if( mouseAlreadyDown ) {
        p.hit = false;
        p.hitSelf = false;
        p.hitOurPlacement = false;
        p.hitAnObject = false;
        p.hitOtherPerson = false;
        }
    
    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    
    if( mCurMouseOverCell.x != -1 ) {    
        mLastClickCell = mCurMouseOverCell;
        }

    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;

    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;

    int destID = 0;
    int floorDestID = 0;
    
    int destNumContained = 0;
    
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;    
    
    
    if( mouseAlreadyDown && 
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // continuous movement mode

        // watch out for case where they mouse over a blocked spot by accident
        
        if( getCellBlocksWalking( mapX, mapY ) ) {
            printf( "Blocked at cont move click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            double xDelta = clickDestX - ourLiveObject->currentPos.x;
            double yDelta = clickDestY - ourLiveObject->currentPos.y;

            int step = 1;
            int limit = 10;
                
            if( fabs( xDelta ) > fabs( yDelta ) ) {
                // push out further in x direction
                if( xDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int xd=step; xd != limit; xd += step ) {
                    if( ! getCellBlocksWalking( mapX + xd, mapY ) ) {
                        // found
                        clickDestX += xd;
                        break;
                        }
                    }
                }
            else {
                // push out further in y direction
                if( yDelta < 0 ) {
                    step = -step;
                    limit = -limit;
                    }

                for( int yd=step; yd != limit; yd += step ) {
                    if( ! getCellBlocksWalking( mapX, mapY + yd ) ) {
                        // found
                        clickDestY += yd;
                        break;
                        }
                    }
                }
            
            printf( "Pushing out to click dest %d,%d\n",
                    clickDestX, clickDestY );
            
            // recompute
            mapX = clickDestX - mMapOffsetX + mMapD / 2;
            mapY = clickDestY - mMapOffsetY + mMapD / 2;
            }
        }
    

    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        floorDestID = mMapFloors[ mapY * mMapD + mapX ];
        
        destNumContained = mMapContainedStacks[ mapY * mMapD + mapX ].size();
        

        // if holding something, and this is a set-down action
        // show click reaction
        if( modClick &&
            ourLiveObject->holdingID != 0 ) {
        
            int mapI = mapY * mMapD + mapX;
            
            int id = mMap[mapI];
            
            if( id == 0 || ! getObject( id )->permanent ) {
                
                // empty cell, or something we can swap held with
                
                GridPos clickPos = { mapX, mapY };
                
                mPrevMouseClickCells.push_back( clickPos );
                mPrevMouseClickCellFades.push_back( 1 );
                
                // instantly fade current cell to get it out of the way
                // of our click indicator
                mCurMouseOverCellFade = 0;
                }
            
            }
        }



    nextActionEating = false;
    nextActionDropping = false;
    


    if( p.hitSelf ) {
        // click on self

        // ignore unless it's a use-on-self action and standing still

        if( ! ourLiveObject->inMotion ) {
            
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }

            if( !modClick ) {
                
                if( ourLiveObject->holdingID > 0 &&
                    getObject( ourLiveObject->holdingID )->foodValue > 0 ) {
                    nextActionEating = true;
                    }
    
                nextActionMessageToSend = 
                    autoSprintf( "SELF %d %d %d#",
                                 sendX( clickDestX ), sendY( clickDestY ), 
                                 p.hitClothingIndex );
                printf( "Use on self\n" );
                }
            else {
                if( ourLiveObject->holdingID > 0 ) {
                    nextActionMessageToSend = 
                        autoSprintf( "DROP %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex  );
                    nextActionDropping = true;
                    }
                else {
                    nextActionMessageToSend = 
                        autoSprintf( "SREMV %d %d %d %d#",
                                     sendX( clickDestX ), sendY( clickDestY ), 
                                     p.hitClothingIndex,
                                     p.hitSlotIndex );
                    printf( "Remove from own clothing container\n" );
                    }
                }

            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            }
        

        return;
        }
    
    
    // may change to empty adjacent spot to click
    int moveDestX = clickDestX;
    int moveDestY = clickDestY;
    
    char mustMove = false;


    


    if( destID > 0 && p.hitAnObject ) {
        
        if( ourLiveObject->holdingID > 0 ) {
            TransRecord *tr = getTrans( ourLiveObject->holdingID, destID );
            
            if( tr == NULL ) {
                // try defaul transition
                // tr = getTrans( -2, destID );
                
                // for now, DO NOT consider default transition
                // no main transition for this held object applies
                // so we should probably give a hint about what CAN apply
                // to the target object.

                // Default transitions are currently just used to make
                // something react to the player (usually for animals getting
                // startled), not to actually accomplish
                // something

                // so we should show hints about the target object BEFORE it
                // went into its (temporary) reaction state. 
                }
            
            if( tr == NULL || tr->newTarget == destID ) {
                // give hint about dest object which will be unchanged 
                mNextHintObjectID = destID;
                mNextHintIndex = mHintBookmarks[ destID ];
                }
            else if( tr->newActor > 0 && 
                     ourLiveObject->holdingID != tr->newActor ) {
                // give hint about how what we're holding will change
                mNextHintObjectID = tr->newActor;
                mNextHintIndex = mHintBookmarks[ tr->newTarget ];
                }
            else if( tr->newTarget > 0 ) {
                // give hint about changed target after we act on it
                mNextHintObjectID = tr->newTarget;
                mNextHintIndex = mHintBookmarks[ tr->newTarget ];
                }
            }
        else {
            // bare hand
            // only give hint about hit if no bare-hand action applies

            if( getTrans( 0, destID ) == NULL ) {
                mNextHintObjectID = destID;
                mNextHintIndex = mHintBookmarks[ destID ];
                }
            }
        }

    
    if( destID > 0 && ! p.hitAnObject ) {
        
        // clicked on empty space near an object
        
        if( ! getObject( destID )->blocksWalking ) {
            
            // object in this space not blocking
            
            // count as an attempt to walk to the spot where the object is
            destID = 0;
            }
        else {
            // clicked on grid cell around blocking object
            destID = 0;
            
            // search for closest non-blocking and walk there
            
            GridPos closestP = { -1, -1 };
            
            double startDist = 99999;
            
            double closestDist = startDist;
            GridPos clickP = { clickDestX, clickDestY };
            GridPos ourP = { ourLiveObject->xd, ourLiveObject->yd };
            
            for( int y=-5; y<5; y++ ) {
                for( int x=-5; x<5; x++ ) {
                    
                    GridPos p = { clickDestX + x, clickDestY + y };
                    
                    int mapPX = p.x - mMapOffsetX + mMapD / 2;
                    int mapPY = p.y - mMapOffsetY + mMapD / 2;

                    if( mapPY >= 0 && mapPY < mMapD &&
                        mapPX >= 0 && mapPX < mMapD ) {
                        
                        int oID = mMap[ mapPY * mMapD + mapPX ];

                        if( oID == 0 
                            ||
                            ( oID > 0 && ! getObject( oID )->blocksWalking ) ) {


                            double d2 = distance2( p, clickP );
                            
                            if( d2 < closestDist ) {
                                closestDist = d2;
                                closestP = p;
                                }
                            else if( d2 == closestDist ) {
                                // break tie by whats closest to player
                                
                                double dPlayerOld = distance2( closestP,
                                                               ourP );
                                double dPlayerNew = distance2( p, ourP );
                                
                                if( dPlayerNew < dPlayerOld ) {
                                    closestDist = d2;
                                    closestP = p;
                                    }
                                }
                            
                            }
                        }
                    
                    }
                }
            
            if( closestDist < startDist ) {
                // found one
                // walk there instead
                moveDestX = closestP.x;
                moveDestY = closestP.y;
                
                clickDestX = moveDestX;
                clickDestY = moveDestY;
                
                mapX = clickDestX - mMapOffsetX + mMapD / 2;
                mapY = clickDestY - mMapOffsetY + mMapD / 2;
                }
            }
        }

    printf( "DestID = %d\n", destID );
    
    if( destID < 0 ) {
        return;
        }

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    

    // true if we're too far away to kill BUT we should execute
    // kill once we get to destination

    // if we're close enough to kill, we'll kill from where we're standing
    // and return
    char killLater = false;
    

    if( destID == 0 &&
        modClick && ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance > 0 ) {
        
        // special case

        // check for possible kill attempt at a distance

        // if it fails (target too far away or no person near),
        // then we resort to standard drop code below


        double d = sqrt( ( clickDestX - ourLiveObject->xd ) * 
                         ( clickDestX - ourLiveObject->xd )
                         +
                         ( clickDestY - ourLiveObject->yd ) * 
                         ( clickDestY - ourLiveObject->yd ) );

        doublePair targetPos = { (double)clickDestX, (double)clickDestY };
        


        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID ) {
                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone
                    
                    if( getObject( ourLiveObject->holdingID )->deadlyDistance 
                        >= d ) {
                        // close enough to use deadly object right now

                        
                        if( nextActionMessageToSend != NULL ) {
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
            
                        nextActionMessageToSend = 
                            autoSprintf( "KILL %d %d#",
                                         sendX( clickDestX ), 
                                         sendY( clickDestY ) );
                        
                        
                        playerActionTargetX = clickDestX;
                        playerActionTargetY = clickDestY;
                        
                        playerActionTargetNotAdjacent = true;
                        
                        printf( "KILL with target player %d\n", o->id );

                        return;
                        }
                    else {
                        // too far away, but try to kill later,
                        // once we walk there, using standard path-to-adjacent
                        // code below
                        killLater = true;
                        
                        break;
                        }
                    }
                }
            }
    
        }
    

    if( ! killLater &&
        destID != 0 &&
        ! modClick &&
        ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->useDistance > 1 &&
        getTrans( ourLiveObject->holdingID, destID ) != NULL ) {
        // check if we're close enough to use this from here 
        
        double d = sqrt( ( clickDestX - ourLiveObject->xd ) * 
                         ( clickDestX - ourLiveObject->xd )
                         +
                         ( clickDestY - ourLiveObject->yd ) * 
                         ( clickDestY - ourLiveObject->yd ) );

        if( getObject( ourLiveObject->holdingID )->useDistance >= d ) {
            // close enough to use object right now

                        
            if( nextActionMessageToSend != NULL ) {
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            nextActionMessageToSend = 
                autoSprintf( "USE %d %d#",
                             sendX( clickDestX ), 
                             sendY( clickDestY ) );
            
                        
            playerActionTargetX = clickDestX;
            playerActionTargetY = clickDestY;
            
            playerActionTargetNotAdjacent = true;
                        
            printf( "USE from a distance, src=(%d,%d), dest=(%d,%d)\n",
                    ourLiveObject->xd, ourLiveObject->yd,
                    clickDestX, clickDestY );
            
            return;
            }
        }
    

    
    // true if we're too far away to use on baby BUT we should execute
    // UBABY once we get to destination

    // if we're close enough to use on baby, we'll use on baby from where 
    // we're standing
    // and return
    char useOnBabyLater = false;
    
    if( !killLater &&
        p.hitOtherPerson &&
        ! modClick && 
        destID == 0 &&
        ourLiveObject->holdingID > 0 &&
        getObject( ourLiveObject->holdingID )->deadlyDistance == 0 &&
        ( getObject( ourLiveObject->holdingID )->clothing != 'n' ||
          getObject( ourLiveObject->holdingID )->foodValue > 0 ) ) {


        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID ) {
                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone
                    
                    if( ! ourLiveObject->inMotion && 
                        ( isGridAdjacent( clickDestX, clickDestY,
                                        ourLiveObject->xd, 
                                        ourLiveObject->yd ) ||
                          ( clickDestX == ourLiveObject->xd && 
                            clickDestY == ourLiveObject->yd ) ) ) {

                        
                        if( nextActionMessageToSend != NULL ) {
                            delete [] nextActionMessageToSend;
                            nextActionMessageToSend = NULL;
                            }
            
                        nextActionMessageToSend = 
                            autoSprintf( "UBABY %d %d %d#",
                                         sendX( clickDestX ), 
                                         sendY( clickDestY ), 
                                         p.hitClothingIndex );
                        
                        
                        playerActionTargetX = clickDestX;
                        playerActionTargetY = clickDestY;
                        
                        playerActionTargetNotAdjacent = true;
                        
                        printf( "UBABY with target player %d\n", o->id );

                        return;
                        }
                    else {
                        // too far away, but try to use on baby later,
                        // once we walk there, using standard path-to-adjacent
                        // code below
                        useOnBabyLater = true;
                        
                        break;
                        }
                    }
                }
            }
    
        }




    char tryingToPickUpBaby = false;
    
    double ourAge = computeCurrentAge( ourLiveObject );

    if( destID == 0 &&
        p.hit &&
        ! p.hitAnObject &&
        ! modClick && ourLiveObject->holdingID == 0 &&
        // only adults can pick up babies
        ourAge > 13 ) {
        

        doublePair targetPos = { (double)clickDestX, (double)clickDestY };

        for( int i=0; i<gameObjects.size(); i++ ) {
        
            LiveObject *o = gameObjects.getElement( i );
            
            if( o->id != ourID ) {
                if( distance( targetPos, o->currentPos ) < 1 ) {
                    // clicked on someone

                    if( computeCurrentAge( o ) < 5 ) {

                        // they're a baby
                        
                        tryingToPickUpBaby = true;
                        break;
                        }
                    }
                }
            }
        }

    

    if( destID == 0 && !modClick && !tryingToPickUpBaby && !useOnBabyLater && 
        ! ( clickDestX == ourLiveObject->xd && 
            clickDestY == ourLiveObject->yd ) ) {
        // a move to an empty spot where we're not already standing
        // can interrupt current move
        
        mustMove = true;
        }
    else if( ( modClick && ourLiveObject->holdingID != 0 )
             || tryingToPickUpBaby
             || useOnBabyLater
             || destID != 0
             || ( modClick && ourLiveObject->holdingID == 0 &&
                  destNumContained > 0 ) ) {
        // use/drop modifier
        // OR pick up action
            
        
        char canExecute = false;
        
        // direct click on adjacent cells or self cell?
        if( isGridAdjacent( clickDestX, clickDestY,
                            ourLiveObject->xd, ourLiveObject->yd )
            || 
            ( clickDestX == ourLiveObject->xd && 
              clickDestY == ourLiveObject->yd ) ) {
            
            canExecute = true;
            }
        else {
            // need to move to empty adjacent first, if it exists
            
            
            int nDX[4] = { -1, +1, 0, 0 };
            int nDY[4] = { 0, 0, -1, +1 };
            
            char foundEmpty = false;
            
            int closestDist = 9999999;

            char oldPathExists = ( ourLiveObject->pathToDest != NULL );

            for( int n=0; n<4; n++ ) {
                int x = mapX + nDX[n];
                int y = mapY + nDY[n];

                if( y >= 0 && y < mMapD &&
                    x >= 0 && x < mMapD ) {
                 
                    
                    int mapI = y * mMapD + x;
                    
                    if( mMap[ mapI ] == 0
                        ||
                        ( mMap[ mapI ] != -1 && 
                          ! getObject( mMap[ mapI ] )->blocksWalking ) ) {
                        
                        int emptyX = clickDestX + nDX[n];
                        int emptyY = clickDestY + nDY[n];

                        // check if there's a path there
                        int oldXD = ourLiveObject->xd;
                        int oldYD = ourLiveObject->yd;
                        
                        // set this temporarily for pathfinding
                        ourLiveObject->xd = emptyX;
                        ourLiveObject->yd = emptyY;
                        
                        computePathToDest( ourLiveObject );
                        
                        if( ourLiveObject->pathToDest != NULL &&
                            ourLiveObject->pathLength < closestDist ) {
                            
                            // can get there
                            
                            moveDestX = emptyX;
                            moveDestY = emptyY;
                            
                            closestDist = ourLiveObject->pathLength;
                            
                            foundEmpty = true;
                            }
                        
                        // restore our old dest
                        ourLiveObject->xd = oldXD;
                        ourLiveObject->yd = oldYD;    
                        }
                    
                    }
                }
            
            if( oldPathExists ) {
                // restore it
                computePathToDest( ourLiveObject );
                }

            if( foundEmpty ) {
                canExecute = true;
                }
            // always try to move as close as possible, even
            // if we can't actually get close enough to execute action
            mustMove = true;
            }
        

        if( canExecute ) {
            
            const char *action = "";
            char *extra = stringDuplicate( "" );
            
            char send = false;
            
            if( tryingToPickUpBaby ) {
                action = "BABY";
                send = true;
                }
            else if( useOnBabyLater ) {
                action = "UBABY";
                delete [] extra;
                extra = autoSprintf( " %d", p.hitClothingIndex );
                send = true;
                }
            else if( modClick && destID == 0 && 
                     ourLiveObject->holdingID != 0 ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;

                // special case:  we're too far away to kill someone
                // but we've right clicked on them from a distance
                // walk up and execute KILL once we get there.
                
                if( killLater ) {
                    action = "KILL";
                    }
                else {
                    // check for other special case
                    // a use-on-ground transition or use-on-floor transition

                    if( ourLiveObject->holdingID > 0 ) {
                        
                        ObjectRecord *held = 
                            getObject( ourLiveObject->holdingID );
                        
                        char foundAlt = false;
                        
                        if( held->foodValue == 0 ) {
                            
                            TransRecord *r = 
                                getTrans( ourLiveObject->holdingID,
                                          -1 );
                            
                            if( r != NULL &&
                                r->newTarget != 0 ) {
                                
                                // a use-on-ground transition exists!
                                
                                // override the drop action
                                action = "USE";
                                foundAlt = true;
                                }
                            }

                        if( !foundAlt && floorDestID > 0 ) {
                            // check if use on floor exists
                            TransRecord *r = 
                                getTrans( ourLiveObject->holdingID, 
                                          floorDestID );
                                
                            if( r != NULL ) {
                                // a use-on-floor transition exists!
                                
                                // override the drop action
                                action = "USE";
                                
                                }
                            }

                        }
                    }
                }
            else if( modClick && ourLiveObject->holdingID == 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) > 0 ) {
                action = "REMV";
                send = true;
                delete [] extra;
                extra = autoSprintf( " %d", p.hitSlotIndex );
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) > 0 &&
                     destNumContained < getNumContainerSlots( destID ) ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( modClick && ourLiveObject->holdingID != 0 &&
                     destID != 0 &&
                     getNumContainerSlots( destID ) == 0 &&
                     ! getObject( destID )->permanent ) {
                action = "DROP";
                nextActionDropping = true;
                send = true;
                }
            else if( destID != 0 ) {
                // allow right click and default to USE if nothing
                // else applies
                action = "USE";
                send = true;
                }
            
            if( strcmp( action, "DROP" ) == 0 ) {
                delete [] extra;
                nextActionDropping = true;
                extra = stringDuplicate( " -1" );
                }
            
            
            if( send ) {
                // queue this until after we are done moving, if we are
                nextActionMessageToSend = 
                    autoSprintf( "%s %d %d%s#", action,
                                 sendX( clickDestX ), 
                                 sendY( clickDestY ), extra );
                
                int trueClickDestX = clickDestX;
                int trueClickDestY = clickDestY;
                
                if( mapY >= 0 && mapY < mMapD &&
                    mapX >= 0 && mapX < mMapD ) {
                    
                    int mapI = mapY * mMapD + mapX;
                    
                    if( mMapMoveSpeeds[ mapI ] > 0 ) {        
                        
                        trueClickDestX = 
                            lrint( clickDestX + mMapMoveOffsets[mapI].x );
                        trueClickDestY = 
                            lrint( clickDestY + mMapMoveOffsets[mapI].y );
                        }
                    }

                ourLiveObject->actionTargetX = clickDestX;
                ourLiveObject->actionTargetY = clickDestY;

                ourLiveObject->actionTargetTweakX = trueClickDestX - clickDestX;
                ourLiveObject->actionTargetTweakY = trueClickDestY - clickDestY;
                

                playerActionTargetX = clickDestX;
                playerActionTargetY = clickDestY;
                }

            delete [] extra;
            }
        }
    


    
    if( mustMove ) {
        
        int oldXD = ourLiveObject->xd;
        int oldYD = ourLiveObject->yd;
        
        ourLiveObject->xd = moveDestX;
        ourLiveObject->yd = moveDestY;
        
        ourLiveObject->inMotion = true;


        GridPos *oldPathToDest = NULL;
        int oldPathLength = 0;
        int oldCurrentPathStep = 0;
        
        if( ourLiveObject->pathToDest != NULL ) {
            oldPathLength = ourLiveObject->pathLength;
            oldPathToDest = new GridPos[ oldPathLength ];

            memcpy( oldPathToDest, ourLiveObject->pathToDest,
                    sizeof( GridPos ) * ourLiveObject->pathLength );
            oldCurrentPathStep = ourLiveObject->currentPathStep;
            }
        

        computePathToDest( ourLiveObject );

        
        if( ourLiveObject->pathToDest == NULL ) {
            // adjust move to closest possible
            ourLiveObject->xd = ourLiveObject->closestDestIfPathFailedX;
            ourLiveObject->yd = ourLiveObject->closestDestIfPathFailedY;
            
            if( ourLiveObject->xd == oldXD && ourLiveObject->yd == oldYD ) {
                // completely blocked in, no path at all toward dest
                
                if( oldPathToDest != NULL ) {
                    // restore it
                    ourLiveObject->pathToDest = oldPathToDest;
                    ourLiveObject->pathLength = oldPathLength;
                    ourLiveObject->currentPathStep = oldCurrentPathStep;
                    oldPathToDest = NULL;
                    }
                
                    

                // ignore click
                
                if( nextActionMessageToSend != NULL ) {
                    delete [] nextActionMessageToSend;
                    nextActionMessageToSend = NULL;
                    }
                ourLiveObject->inMotion = false;
                return;
                }

            if( oldPathToDest != NULL ) {
                delete [] oldPathToDest;
                oldPathToDest = NULL;
                }
            

            computePathToDest( ourLiveObject );
            
            if( ourLiveObject->pathToDest == NULL &&
                ourLiveObject->useWaypoint ) {
                // waypoint itself may be blocked
                // try again with no waypoint at all
                ourLiveObject->useWaypoint = false;
                computePathToDest( ourLiveObject );
                }

            if( ourLiveObject->pathToDest == NULL ) {
                // this happens when our curPos is slightly off of xd,yd
                // but not a full cell away

                // make a fake path
                doublePair dest = { (double) ourLiveObject->xd,
                                    (double) ourLiveObject->yd };
                
                doublePair dir = normalize( sub( dest, 
                                                 ourLiveObject->currentPos ) );

                // fake start, one grid step away
                doublePair fakeStart = dest;

                if( fabs( dir.x ) > fabs( dir.y ) ) {
                    
                    if( dir.x < 0 ) {
                        fakeStart.x += 1;
                        }
                    else {
                        fakeStart.x -= 1;
                        }
                    }
                else {
                    if( dir.y < 0 ) {
                        fakeStart.y += 1;
                        }
                    else {
                        fakeStart.y -= 1;
                        }
                    
                    }
                
                
                ourLiveObject->pathToDest = new GridPos[2];
                
                ourLiveObject->pathToDest[0].x = (int)fakeStart.x;
                ourLiveObject->pathToDest[0].y = (int)fakeStart.y;
                
                ourLiveObject->pathToDest[1].x = ourLiveObject->xd;
                ourLiveObject->pathToDest[1].y = ourLiveObject->yd;
                
                ourLiveObject->pathLength = 2;
                ourLiveObject->currentPathStep = 0;
                ourLiveObject->numFramesOnCurrentStep = 0;
                ourLiveObject->onFinalPathStep = false;
                
                ourLiveObject->currentMoveDirection =
                    normalize( 
                        sub( gridToDouble( ourLiveObject->pathToDest[1] ), 
                             gridToDouble( ourLiveObject->pathToDest[0] ) ) );
                }

            if( ourLiveObject->xd == oldXD 
                &&
                ourLiveObject->yd == oldYD ) {
                
                // truncated path is where we're already going
                // don't send new move message
                return;
                }


            moveDestX = ourLiveObject->xd;
            moveDestY = ourLiveObject->yd;
            
            if( nextActionMessageToSend != NULL ) {
                // abort the action, because we can't reach the spot we
                // want to reach
                delete [] nextActionMessageToSend;
                nextActionMessageToSend = NULL;
                }
            
            }
        
        
        if( oldPathToDest != NULL ) {
            delete [] oldPathToDest;
            oldPathToDest = NULL;
            }
            


        // send move right away
        //Thread::staticSleep( 2000 );
        SimpleVector<char> moveMessageBuffer;
        
        moveMessageBuffer.appendElementString( "MOVE" );
        // start is absolute
        char *startString = 
            autoSprintf( " %d %d", 
                         sendX( ourLiveObject->pathToDest[0].x ),
                         sendY( ourLiveObject->pathToDest[0].y ) );
        moveMessageBuffer.appendElementString( startString );
        delete [] startString;
        
        for( int i=1; i<ourLiveObject->pathLength; i++ ) {
            // rest are relative to start
            char *stepString = autoSprintf( " %d %d", 
                                         ourLiveObject->pathToDest[i].x
                                            - ourLiveObject->pathToDest[0].x,
                                         ourLiveObject->pathToDest[i].y
                                            - ourLiveObject->pathToDest[0].y );
            
            moveMessageBuffer.appendElementString( stepString );
            delete [] stepString;
            }
        moveMessageBuffer.appendElementString( "#" );
        

        char *message = moveMessageBuffer.getElementString();

        sendToServerSocket( message );


        delete [] message;

        // start moving before we hear back from server



        
        

        ourLiveObject->moveTotalTime = 
            measurePathLength( ourLiveObject, ourLiveObject->pathLength ) / 
            ourLiveObject->lastSpeed;

        ourLiveObject->moveEtaTime = game_getCurrentTime() +
            ourLiveObject->moveTotalTime;

            
        updateMoveSpeed( ourLiveObject );
        }    
    }




void LivingLifePage::pointerDrag( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    }


void LivingLifePage::pointerUp( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;


    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        mouseDown = false;
        return;
        }

    if( mouseDown && 
        getOurLiveObject()->inMotion 
        &&
        mouseDownFrames >  
        minMouseDownFrames / frameRateFactor ) {
        
        // treat the up as one final click
        pointerDown( inX, inY );
        }

    mouseDown = false;


    // clear mouse over cell
    mPrevMouseOverCells.push_back( mCurMouseOverCell );
    mPrevMouseOverCellFades.push_back( mCurMouseOverCellFade );
    mCurMouseOverCell.x = -1;
    mCurMouseOverCell.y = -1;
    }


void LivingLifePage::keyDown( unsigned char inASCII ) {
    
    registerTriggerKeyCommand( inASCII, this );


    if( mServerSocket == -1 ) {
        // dead
        return;
        }
    
    switch( inASCII ) {
        /*
        case 'b':
            blackBorder = true;
            whiteBorder = false;
            break;
        case 'B':
            blackBorder = false;
            whiteBorder = true;
            break;
        */
        /*
        case 'a':
            drawAdd = ! drawAdd;
            break;
        case 'm':
            drawMult = ! drawMult;
            break;
        case 'n':
            multAmount += 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 'b':
            multAmount -= 0.05;
            printf( "Mult amount = %f\n", multAmount );
            break;
        case 's':
            addAmount += 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        case 'd':
            addAmount -= 0.05;
            printf( "Add amount = %f\n", addAmount );
            break;
        */
        /*
        case 'd':
            pathStepDistFactor += .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'D':
            pathStepDistFactor += .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 's':
            pathStepDistFactor -= .1;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'S':
            pathStepDistFactor -= .5;
            printf( "Path step dist factor = %f\n", pathStepDistFactor );
            break;
        case 'f':
            trail.deleteAll();
            trailColors.deleteAll();
            break;
        */
        case 'e':
        case 'E':
            mEKeyDown = true;
            break;
        case 9: // tab
            if( mCurrentHintObjectID != 0 ) {
                
                int num = getNumHints( mCurrentHintObjectID );
                
                int skip = 1;
                
                if( isShiftKeyDown() ) {
                    skip = -1;
                    }
                if( isCommandKeyDown() ) {
                    if( num > 5 ) {
                        skip *= 5;
                        }
                    }

                if( num > 0 ) {
                    mNextHintIndex += skip;
                    if( mNextHintIndex >= num ) {
                        mNextHintIndex -= num;
                        }
                    if( mNextHintIndex < 0 ) {
                        mNextHintIndex += num;
                        }
                    }
                }
            break;
        case 13:  // enter
            // speak
            if( ! mSayField.isFocused() ) {
                
                mSayField.setText( "" );
                mSayField.focus();
                }
            else {
                char *typedText = mSayField.getText();
                
                if( strcmp( typedText, "" ) == 0 ) {
                    mSayField.unfocus();
                    }
                else {
                    // send text to server
                    char *message = 
                        autoSprintf( "SAY 0 0 %s#",
                                     typedText );
                    for( int i=0; i<mSentChatPhrases.size(); i++ ) {
                        if( strcmp( 
                                typedText, 
                                mSentChatPhrases.getElementDirect(i) ) == 0 ) {
                            delete [] mSentChatPhrases.getElementDirect(i);
                            mSentChatPhrases.deleteElement(i);
                            i--;
                            }
                        }
                    mSentChatPhrases.push_back( stringDuplicate( typedText ) );
                    
                    sendToServerSocket( message );
            

                    
                    mSayField.setText( "" );
                    mSayField.unfocus();

                    delete [] message;
                    }
                
                delete [] typedText;
                }
            break;
        }
    }



void LivingLifePage::specialKeyDown( int inKeyCode ) {
    
    if( mServerSocket == -1 ) {
        // dead
        return;
        }

    if( inKeyCode == MG_KEY_UP ||
        inKeyCode == MG_KEY_DOWN ) {
        if( ! mSayField.isFocused() && inKeyCode == MG_KEY_UP ) {
            if( mSentChatPhrases.size() > 0 ) {
                mSayField.setText( 
                    mSentChatPhrases.getElementDirect( 
                        mSentChatPhrases.size() - 1 ) );
                }
            else {
                mSayField.setText( "" );
                }
            mSayField.focus();
            }
        else {
            char *curText = mSayField.getText();
                
            int curBufferIndex = -1;
            
            if( strcmp( curText, "" ) != 0 ) {
                for( int i=mSentChatPhrases.size()-1; i>=0; i-- ) {
                    if( strcmp( curText, mSentChatPhrases.getElementDirect(i) )
                        == 0 ) {
                        curBufferIndex = i;
                        break;
                        }
                    }
                }
            delete [] curText;
            
            int newIndex = mSentChatPhrases.size();
            
            if( curBufferIndex != -1 ) {
                newIndex = curBufferIndex;
                
                }

            if( inKeyCode == MG_KEY_UP ) {
                newIndex --;
                }
            else {
                newIndex ++;
                }

            
            if( newIndex >= 0 ) {
                
                if( mCurrentNoteChars.size() > 0 ) {
                    // fade older erased chars 

                    for( int i=0; i<mErasedNoteCharFades.size(); i++ ) {
                        if( mErasedNoteCharFades.getElementDirect( i ) > 0.5 ) {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.2;
                            }
                        else {
                            *( mErasedNoteCharFades.getElement( i ) ) -= 0.1;
                            }
                        
                        if( mErasedNoteCharFades.getElementDirect( i ) <= 0 ) {
                            mErasedNoteCharFades.deleteElement( i );
                            mErasedNoteChars.deleteElement( i );
                            mErasedNoteCharOffsets.deleteElement( i );
                            i--;
                            }
                        }
                    }
                
                // first, remove exact duplicates from erased
                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    char c = mCurrentNoteChars.getElementDirect( i );
                    doublePair pos = 
                        mCurrentNoteCharOffsets.getElementDirect( i );
                    
                    for( int j=0; j<mErasedNoteChars.size(); j++ ) {
                        if( mErasedNoteChars.getElementDirect(j) == c 
                            &&
                            equal( mErasedNoteCharOffsets.getElementDirect(j),
                                   pos ) ) {
                            
                            mErasedNoteChars.deleteElement( j );
                            mErasedNoteCharOffsets.deleteElement( j );
                            mErasedNoteCharFades.deleteElement( j );
                            j--;
                            }
                        }
                    }
                

                for( int i=0; i<mCurrentNoteChars.size(); i++ ) {
                    mErasedNoteChars.push_back( 
                        mCurrentNoteChars.getElementDirect( i ) );
                    
                    mErasedNoteCharOffsets.push_back(
                        mCurrentNoteCharOffsets.getElementDirect( i ) );
                    
                    mErasedNoteCharFades.push_back( 1.0f );
                    }
                
                if( newIndex >= mSentChatPhrases.size() ) {
                    mSayField.setText( "" );
                    }
                else {
                    mSayField.setText( 
                        mSentChatPhrases.getElementDirect( newIndex ) );
                    }
                }
            }
        }
    }


        
void LivingLifePage::keyUp( unsigned char inASCII ) {

    switch( inASCII ) {
        case 'e':
        case 'E':
            mEKeyDown = false;
            break;
        }

    }

        



ExtraMapObject LivingLifePage::copyFromMap( int inMapI ) {
    ExtraMapObject o = {
        mMap[ inMapI ],
        mMapMoveSpeeds[ inMapI ],
        mMapMoveOffsets[ inMapI ],
        mMapAnimationFrameCount[ inMapI ],
        mMapAnimationLastFrameCount[ inMapI ],
        mMapAnimationFrozenRotFrameCount[ inMapI ],
        mMapCurAnimType[ inMapI ],
        mMapLastAnimType[ inMapI ],
        mMapLastAnimFade[ inMapI ],
        mMapTileFlips[ inMapI ],
        mMapContainedStacks[ inMapI ],
        mMapSubContainedStacks[ inMapI ] };
    
    return o;
    }

        
void LivingLifePage::putInMap( int inMapI, ExtraMapObject *inObj ) {
    mMap[ inMapI ] = inObj->objectID;
    
    mMapMoveSpeeds[ inMapI ] = inObj->moveSpeed;
    mMapMoveOffsets[ inMapI ] = inObj->moveOffset;
    mMapAnimationFrameCount[ inMapI ] = inObj->animationFrameCount;
    mMapAnimationLastFrameCount[ inMapI ] = inObj->animationLastFrameCount;
    mMapAnimationFrozenRotFrameCount[ inMapI ] = 
        inObj->animationFrozenRotFrameCount;
    
    mMapCurAnimType[ inMapI ] = inObj->curAnimType;
    mMapLastAnimType[ inMapI ] = inObj->lastAnimType;
    mMapLastAnimFade[ inMapI ] = inObj->lastAnimFade;
    mMapTileFlips[ inMapI ] = inObj->flip;
    
    mMapContainedStacks[ inMapI ] = inObj->containedStack;
    mMapSubContainedStacks[ inMapI ] = inObj->subContainedStack;
    }

