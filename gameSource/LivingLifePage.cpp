#include "LivingLifePage.h"

#include "objectBank.h"
#include "transitionBank.h"
#include "soundBank.h"
#include "whiteSprites.h"
#include "message.h"
#include "groundSprites.h"

#include "accountHmac.h"

#include "liveObjectSet.h"
#include "ageControl.h"
#include "musicPlayer.h"

#include "../commonSource/fractalNoise.h"

#include "minorGems/util/SimpleVector.h"


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


extern double frameRateFactor;

extern Font *mainFont;
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


static int lastScreenMouseX, lastScreenMouseY;
static char mouseDown = false;

static int screenCenterPlayerOffsetX, screenCenterPlayerOffsetY;






// base speed for animations that aren't speeded up or slowed down
// when player moving at a different speed, anim speed is modified
#define BASE_SPEED 4.0


int numServerBytesRead = 0;
int numServerBytesSent = 0;

int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;


static char hideGuiPanel = false;



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
        return false;
        }
    
    return true;
    }



static double computeCurrentAge( LiveObject *inObj ) {
    return inObj->age + 
        inObj->ageRate * ( game_getCurrentTime() - inObj->lastAgeSetTime );
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
    PLAYER_SAYS,
    FOOD_CHANGE,
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
    else if( strcmp( copy, "PS" ) == 0 ) {
        returnValue = PLAYER_SAYS;
        }
    else if( strcmp( copy, "FX" ) == 0 ) {
        returnValue = FOOD_CHANGE;
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


// NULL if there's no full message available
char *getNextServerMessage() {
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
    


    // find first terminal character #

    int index = serverSocketBuffer.getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    
    char *message = new char[ index + 1 ];
    
    for( int i=0; i<index; i++ ) {
        message[i] = (char)( serverSocketBuffer.getElementDirect( 0 ) );
        serverSocketBuffer.deleteElement( 0 );
        }
    // delete message terminal character
    serverSocketBuffer.deleteElement( 0 );
    
    message[ index ] = '\0';

    if( getMessageType( message ) == MAP_CHUNK ) {
        pendingMapChunkMessage = message;
        
        int sizeX, sizeY, x, y, binarySize;
        sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                &sizeX, &sizeY,
                &x, &y, &binarySize, &pendingCompressedChunkSize );


        return getNextServerMessage();
        }
    else {
        return message;
        }
    }




doublePair gridToDouble( GridPos inGridPos ) {
    doublePair d = { (double) inGridPos.x, (double) inGridPos.y };
    
    return d;
    }



char equal( GridPos inA, GridPos inB ) {
    if( inA.x == inB.x && inA.y == inB.y ) {
        return true;
        }
    return false;
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
        measurePathLength( inObject, inObject->currentPathStep );
    

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
    
    double speedPerSec = moveLeft / etaSec;

    if( autoAdjustFramerate ) {
        inObject->currentSpeed = speedPerSec / getRecentFrameRate();
        printf( "autoAdjustFramerate = %d, speed = %f\n", 
                autoAdjustFramerate, inObject->currentSpeed );
        }
    else {
        // hold frame rate constant
        double fps = baseFramesPerSecond / frameRateFactor;
        
        inObject->currentSpeed = speedPerSec / fps;
        printf( "fixed speed = %f\n", inObject->currentSpeed );
        }
    

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }


// should match limit on server
static int pathFindingD = 32;


void LivingLifePage::computePathToDest( LiveObject *inObject ) {
    
    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        inObject->pathToDest = NULL;
        }

    GridPos start;
    start.x = lrint( inObject->currentPos.x );
    start.y = lrint( inObject->currentPos.y );

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

    // now add extra blocked spots for wide objects
    for( int y=0; y<pathFindingD; y++ ) {
        int mapY = ( y - pathOffsetY ) + mMapD / 2 - mMapOffsetY;
        
        for( int x=0; x<pathFindingD; x++ ) {
            int mapX = ( x - pathOffsetX ) + mMapD / 2 - mMapOffsetX;
            
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
    
    
    start.x += pathOffsetX;
    start.y += pathOffsetY;
    
    end.x += pathOffsetX;
    end.y += pathOffsetY;
    
    double startTime = game_getCurrentTime();

    GridPos closestFound;
    
    char pathFound = 
        pathFind( pathFindingD, pathFindingD,
                  blockedMap, 
                  start, end, 
                  &( inObject->pathLength ),
                  &( inObject->pathToDest ),
                  &closestFound );
    
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
            else if( inObject->pathToDest[i].x < inObject->pathToDest[i-1].x ) {
                dirsInPath[2]++;
                }
            else if( inObject->pathToDest[i].y > inObject->pathToDest[i-1].y ) {
                dirsInPath[1]++;
                }
            else if( inObject->pathToDest[i].y < inObject->pathToDest[i-1].y ) {
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





LivingLifePage::LivingLifePage() 
        : mServerSocket( -1 ), 
          mFirstServerMessagesReceived( 0 ),
          mMapD( 64 ),
          mMapOffsetX( 0 ),
          mMapOffsetY( 0 ),
          mEKeyDown( false ),
          mGuiPanelSprite( loadSprite( "guiPanel.tga", false ) ),
          mNotePaperSprite( loadSprite( "notePaper.tga", false ) ),
          mLastMouseOverID( 0 ),
          mCurMouseOverID( 0 ),
          mChalkBlotSprite( loadWhiteSprite( "chalkBlot.tga" ) ),
          mPathMarkSprite( loadWhiteSprite( "pathMark.tga" ) ),
          mGroundOverlaySprite( loadSprite( "ground.tga" ) ),
          mSayField( handwritingFont, 0, 1000, 10, true, NULL,
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-,'?! " ) {
    
    hideGuiPanel = SettingsManager::getIntSetting( "hideGameUI", 0 );

    mHungerSlipSprites[0] = loadSprite( "fullSlip.tga", false );
    mHungerSlipSprites[1] = loadSprite( "hungrySlip.tga", false );
    mHungerSlipSprites[2] = loadSprite( "starvingSlip.tga", false );
    

    // not visible, drawn under world at 0, 0, and doesn't move with camera
    // still, we can use it to receive/process/filter typing events
    addComponent( &mSayField );
    
    mSayField.unfocus();
    
    
    mNotePaperHideOffset.x = 0;
    mNotePaperHideOffset.y = -420;

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

    

    for( int i=0; i<3; i++ ) {    
        mHungerSlipPosOffset[i] = mHungerSlipHideOffsets[i];
        mHungerSlipPosTargetOffset[i] = mHungerSlipPosOffset[i];
        }
    
    mHungerSlipVisible = -1;

    

    mMap = new int[ mMapD * mMapD ];
    mMapBiomes = new int[ mMapD * mMapD ];
    
    mMapCellDrawnFlags = new char[ mMapD * mMapD ];

    mMapContainedStacks = new SimpleVector<int>[ mMapD * mMapD ];
    
    mMapAnimationFrameCount =  new int[ mMapD * mMapD ];
    mMapAnimationLastFrameCount =  new int[ mMapD * mMapD ];
    mMapAnimationFrozenRotFrameCount =  new int[ mMapD * mMapD ];
    
    mMapCurAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimType =  new AnimType[ mMapD * mMapD ];
    mMapLastAnimFade =  new double[ mMapD * mMapD ];
    
    mMapDropOffsets = new doublePair[ mMapD * mMapD ];
    mMapDropRot = new double[ mMapD * mMapD ];
    mMapDropSounds = new SoundUsage[ mMapD * mMapD ];
    mMapTileFlips = new char[ mMapD * mMapD ];
    
    mMapOurPlayerPlacedFlags = new char[ mMapD * mMapD ];
    

    for( int i=0; i<mMapD *mMapD; i++ ) {
        // -1 represents unknown
        // 0 represents known empty
        mMap[i] = -1;
        mMapBiomes[i] = -1;
        
        mMapAnimationFrameCount[i] = randSource.getRandomBoundedInt( 0, 10000 );
        mMapAnimationLastFrameCount[i] = 
            randSource.getRandomBoundedInt( 0, 10000 );
        
        mMapAnimationFrozenRotFrameCount[i] = 0;

        mMapCurAnimType[i] = ground;
        mMapLastAnimType[i] = ground;
        mMapLastAnimFade[i] = 0;

        mMapDropOffsets[i].x = 0;
        mMapDropOffsets[i].y = 0;
        mMapDropRot[i] = 0;
        mMapDropSounds[i] = blankSoundUsage;
        
        mMapTileFlips[i] = false;
        
        mMapOurPlayerPlacedFlags[i] = false;
        }
    


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

    
    mCurrentArrowI = 0;
    mCurrentArrowHeat = -1;
    mCurrentDes = NULL;
    mCurrentLastAteString = NULL;
    }




void LivingLifePage::clearLiveObjects() {
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *nextObject =
            gameObjects.getElement( i );
        
        if( nextObject->containedIDs != NULL ) {
            delete [] nextObject->containedIDs;
            }
        
        if( nextObject->pathToDest != NULL ) {
            delete [] nextObject->pathToDest;
            }

        if( nextObject->currentSpeech != NULL ) {
            delete [] nextObject->currentSpeech;
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

    delete [] mMapCurAnimType;
    delete [] mMapLastAnimType;
    delete [] mMapLastAnimFade;
    
    delete [] mMapDropOffsets;
    delete [] mMapDropRot;
    delete [] mMapDropSounds;

    delete [] mMapTileFlips;
    
    delete [] mMapContainedStacks;
    
    delete [] mMap;
    delete [] mMapBiomes;

    delete [] mMapCellDrawnFlags;

    delete [] mMapOurPlayerPlacedFlags;

    delete [] nextActionMessageToSend;

    for( int i=0; i<3; i++ ) {
        freeSprite( mHungerSlipSprites[i] );
        }

    freeSprite( mGuiPanelSprite );
    freeSprite( mNotePaperSprite );
    freeSprite( mChalkBlotSprite );
    freeSprite( mPathMarkSprite );
    freeSprite( mGroundOverlaySprite );
    
    

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
    
    LiveObject *ourLiveObject = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == ourID ) {
            ourLiveObject = o;
            break;
            }
        }
    return ourLiveObject;
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
    

    setDrawColor( 1, 1, 1, inFade );

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
            }
        }
    
    
    setDrawColor( 0, 0, 0, inFade );

    for( int i=0; i<lines->size(); i++ ) {
        char *line = lines->getElementDirect( i );
        
        doublePair lineStart = 
            { inPos.x, firstLineY - i * lineSpacing};
        
        handwritingFont->drawString( line, lineStart, alignLeft );
        delete [] line;
        }

    delete lines;
    }




static void handleAnimSound( int inObjectID, double inAge, AnimType inType,
                             int inOldFrameCount, int inNewFrameCount,
                             double inPosX, double inPosY ) {    
    
            
    double oldTimeVal = frameRateFactor * inOldFrameCount / 60.0;
            
    double newTimeVal = frameRateFactor * inNewFrameCount / 60.0;
                

    AnimationRecord *anim = getAnimation( inObjectID, inType );
    if( anim != NULL ) {
                    
        for( int s=0; s<anim->numSounds; s++ ) {
            
            if( anim->soundAnim[s].sound.id == -1 ) {
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
                    playSound( anim->soundAnim[s].sound,
                               getVectorFromCamera( inPosX, inPosY ) );
                    
                    }
                }
            }
        }
    }




void LivingLifePage::drawMapCell( int inMapI, 
                                  int inScreenX, int inScreenY ) {
            
    int oID = mMap[ inMapI ];
            
    if( oID > 0 ) {
        
        if( !mapPullMode ) {
            
            int oldFrameCount = mMapAnimationFrameCount[ inMapI ];
            mMapAnimationFrameCount[ inMapI ] ++;
            mMapAnimationLastFrameCount[ inMapI ] ++;
                
            handleAnimSound( oID, 0, ground, oldFrameCount, 
                             mMapAnimationFrameCount[ inMapI ],
                             (double)inScreenX / CELL_D,
                             (double)inScreenY / CELL_D );
            
            if( mMapLastAnimFade[ inMapI ] > 0 ) {
                mMapLastAnimFade[ inMapI ] -= 0.05 * frameRateFactor;
                if( mMapLastAnimFade[ inMapI ] < 0 ) {
                    mMapLastAnimFade[ inMapI ] = 0;
                    
                    if( mMapCurAnimType[ inMapI ] != ground ) {
                        // transition to ground now
                        mMapLastAnimType[ inMapI ] = mMapCurAnimType[ inMapI ];
                        mMapCurAnimType[ inMapI ] = ground;
                        mMapLastAnimFade[ inMapI ] = 1;
                        
                        mMapAnimationLastFrameCount[ inMapI ] =
                            mMapAnimationFrameCount[ inMapI ];
                        
                        mMapAnimationFrameCount[ inMapI ] = 0;
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
                if( mMapDropSounds[ inMapI ].id != -1 ) {
                    
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
                

        setDrawColor( 1, 1, 1, 1 );
                
        AnimType curType = ground;
        AnimType fadeTargetType = ground;
        double animFade = 1;


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
        if( obj->permanent && obj->blocksWalking ) {
            // permanent, blocking objects (e.g., walls) are never drawn flipped
            flip = false;
            }

        if( mMapContainedStacks[ inMapI ].size() > 0 ) {
            int *stackArray = 
                mMapContainedStacks[ inMapI ].getElementArray();
                    
            drawObjectAnim( oID, 
                            curType, timeVal,
                            animFade,
                            fadeTargetType,
                            targetTimeVal,
                            frozenRotTimeVal,
                            &used,
                            endAnimType,
                            endAnimType,
                            pos, rot, false, flip,
                            -1,
                            false, false, false,
                            getEmptyClothingSet(),
                            NULL,
                            mMapContainedStacks[ inMapI ].size(),
                            stackArray );
            delete [] stackArray;
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
                            pos, rot,
                            false,
                            flip, -1,
                            false, false, false,
                            getEmptyClothingSet(), NULL );
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




ObjectAnimPack LivingLifePage::drawLiveObject( 
    LiveObject *inObj,
    SimpleVector<LiveObject *> *inSpeakers,
    SimpleVector<doublePair> *inSpeakersPos ) {    

    inObj->onScreen = true;

    // current pos
                    
    doublePair pos = mult( inObj->currentPos, CELL_D );
    
    ObjectAnimPack returnPack;
    returnPack.inObjectID = -1;

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
            
            if( displayObj->usingSound.id != -1 ) {
                // play baby's using sound as they are put down
                // we no longer have acces
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
                    
        float xDir = 0;
        float yDir = 0;
                    
        if( inObj->currentPos.x < targetX ) {
            xDir = 1;
            if( ! inObj->inMotion ) {
                inObj->holdingFlip = false;
                }
            }
        if( inObj->currentPos.x > targetX ) {
            xDir = -1;
            if( ! inObj->inMotion ) {
                inObj->holdingFlip = true;
                }
            }
        if( inObj->currentPos.y < targetY ) {
            yDir = 1;
            }
        if( inObj->currentPos.y > targetY ) {
            yDir = -1;
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
    
        
    
    if( inObj->holdingID != 0  && heldObject != NULL ) {
                    
        if( heldObject->heldInHand ) {
            
            hideClosestArm = 0;
            }
        else if( heldObject->rideable ) {
            hideClosestArm = 0;
            hideAllLimbs = true;
            }
        else {
            // try hiding no arms, but freezing them instead
            // -2 means body position still returned as held pos
            // instead of hand pos
            // -2 also causes frozen arm animation to be set below
            hideClosestArm = -2;
            hideAllLimbs = false;
            }
        }
    else if( inObj->holdingID < 0 ) {
        // carrying baby
        //hideClosestArm = true;
        
        // try hiding no arms, but freezing them instead
        // -2 means body position still returned as held pos
        // instead of hand pos
        // -2 also causes frozen arm animation to be set below
        hideClosestArm = -2;
        hideAllLimbs = false;
        }
    

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

        if( holdingPos.valid ) {
            holdPos = holdingPos.pos;
            }
        else {
            holdPos = pos;
            }

        

        
        if( heldObject != NULL ) {
            
            doublePair heldOffset = heldObject->heldOffset;

            heldOffset = sub( heldOffset, 
                              getObjectCenterOffset( heldObject ) );

            if( inObj->holdingFlip ) {
                heldOffset.x *= -1;
                }
                        
            if( holdingPos.valid && holdingPos.rot != 0  &&
                ! heldObject->rideable ) {
            
                if( inObj->holdingFlip ) {
                    heldOffset = rotate( heldOffset, 
                                         2 * M_PI * holdingPos.rot );
                    }
                else {
                    heldOffset = rotate( heldOffset, 
                                         -2 * M_PI * holdingPos.rot );
                    }
                if( inObj->holdingFlip ) {
                    holdRot = -holdingPos.rot;
                    }
                else {        
                    holdRot = holdingPos.rot;
                    }

                if( holdRot > 1 ) {
                    while( holdRot > 1 ) {
                        holdRot -= 1;
                        }
                    }
                else if( holdRot < -1 ) {
                    while( holdRot < -1 ) {
                        holdRot += 1;
                        }
                    }
                }
            
            

            

            holdPos.x += heldOffset.x;

            holdPos.y += heldOffset.y;
            }
                
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

            double step = frameRateFactor * 0.0625;
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
        
        if( ! heldObject->rideable ) {
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
                                0, NULL );

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
                            0, NULL );
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
                            inObj->containedIDs );
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






char drawAdd = true;
char drawMult = true;

double multAmount = 0.15;
double addAmount = 0.25;


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


    setDrawColor( 1, 1, 1, 1 );
    drawSquare( lastScreenViewCenter, viewWidth );
    

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
    int yEnd = gridCenterY + 3;

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


    int yStartFloor = gridCenterY - 3;
    int yEndFloor = gridCenterY + 3;

    int xStartFloor = gridCenterX - 5;
    int xEndFloor = gridCenterX + 6;

    
    // give us extra border from edge so we can safely check neighbors
    if( xStartFloor < 1 ) {
        xStartFloor = 1;
        }
    if( xStartFloor >= mMapD - 1 ) {
        xStartFloor = mMapD - 2;
        }
    
    if( yStartFloor < 1 ) {
        yStartFloor = 1;
        }
    if( yStartFloor >= mMapD - 1 ) {
        yStartFloor = mMapD - 2;
        }

    if( xEndFloor < 1 ) {
        xEndFloor = 1;
        }
    if( xEndFloor >= mMapD - 1 ) {
        xEndFloor = mMapD - 2;
        }
    
    if( yEndFloor < 1 ) {
        yEndFloor = 1;
        }
    if( yEndFloor >= mMapD - 1 ) {
        yEndFloor = mMapD - 2;
        }



    int numCells = mMapD * mMapD;

    memset( mMapCellDrawnFlags, false, numCells );

    // draw underlying ground biomes
    for( int y=yEndFloor; y>=yStartFloor; y-- ) {

        int screenY = CELL_D * ( y + mMapOffsetY - mMapD / 2 );

        int tileY = -lrint( screenY / CELL_D );

        
        for( int x=xStartFloor; x<=xEndFloor; x++ ) {
            int mapI = y * mMapD + x;
            
            if( mMapCellDrawnFlags[mapI] ) {
                continue;
                }

            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );
            
            int tileX = lrint( screenX / CELL_D );

            
            int b = mMapBiomes[mapI];
                        
            GroundSpriteSet *s = NULL;
            
            if( b >= 0 && b < groundSpritesArraySize ) {
                s = groundSprites[ b ];
                }
            
            if( s == NULL ) {
                // find another
                for( int i=0; i<groundSpritesArraySize && s == NULL; i++ ) {
                    s = groundSprites[ i ];
                    }
                }
            
            
            if( s != NULL ) {
                
                setDrawColor( 1, 1, 1, 1 );
                            
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
                                    
                                    if( mMapBiomes[nI] != b ) {
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

                
                if( ! mMapCellDrawnFlags[mapI] ) {
                    // not drawn as whole sheet
                    
                    if( mMapBiomes[ mapI - 1 ] == b &&
                        mMapBiomes[ mapI + mMapD ] == b &&
                        mMapBiomes[ mapI + mMapD + 1 ] == b ) {
                        
                        // surrounded by same biome above and to left
                        // AND diagonally to the above-right
                        // draw square tile here to save pixel fill time
                        drawSprite( s->squareTiles[setY][setX], pos );
                        }
                    else {
                        drawSprite( s->tiles[setY][setX], pos );
                        }
                    mMapCellDrawnFlags[mapI] = true;
                    }
                }
            }
        }
    


    // draw overlay evenly over all biomes
    doublePair groundCenterPos;

    int groundW = getSpriteWidth( mGroundOverlaySprite );
    int groundH = getSpriteHeight( mGroundOverlaySprite );
    
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
            
            if( drawMult ) drawSprite( mGroundOverlaySprite, pos );
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

            if( drawAdd ) drawSprite( mGroundOverlaySprite, pos );
            }
        }
    //toggleAdditiveTextureColoring( false );
    toggleAdditiveBlend( false );
    

    
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
                ourLiveObject->shouldDrawPathMarks ) {
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


                        curDir = 
                            normalize( 
                                add( curDir, 
                                     mult( dir, turnFactor ) ) );
                        
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
    
    

    for( int y=yEnd; y>=yStart; y-- ) {
        
        int worldY = y + mMapOffsetY - mMapD / 2;

        int screenY = CELL_D * worldY;
        

        // draw marked objects behind everything else, including players
        
        for( int x=xStart; x<=xEnd; x++ ) {
            
            int worldX = x + mMapOffsetX - mMapD / 2;


            int mapI = y * mMapD + x;
            int screenX = CELL_D * worldX;
            
            if( mMap[ mapI ] > 0 && 
                getObject( mMap[ mapI ] )->drawBehindPlayer ) {
                
                drawMapCell( mapI, screenX, screenY );
                }

            /*
              // debugging grid
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
                    oY = lrint( o->currentPos.y - 0.15 );
                    }
                
                
                if( oY == worldY && oX == worldX ) {
                    
                    // there's a player here, draw it
                    
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
                }
            }


        // now draw non-behind-marked map objects in this row
        // OVER the player objects in this row (so that pick up and set down
        // looks correct, and so players are behind all same-row objects)
        for( int x=xStart; x<=xEnd; x++ ) {
            int mapI = y * mMapD + x;
            int screenX = CELL_D * ( x + mMapOffsetX - mMapD / 2 );


            if( mMap[ mapI ] > 0 && 
                ! getObject( mMap[ mapI ] )->drawBehindPlayer ) {
                
                drawMapCell( mapI, screenX, screenY );
                }
            }

        // now draw held flying objects on top of objects in this row
        for( int i=0; i<heldToDrawOnTop.size(); i++ ) {
            drawObjectAnim( heldToDrawOnTop.getElementDirect( i ) );
            }

        } // end loop over rows on screen

    
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
                                   o->speechFade, widthLimit );
        }
    

        
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
                // send request for our character's center

                lastScreenViewCenter.x = CELL_D * ourLiveObject->xd;
                lastScreenViewCenter.y = CELL_D * ourLiveObject->yd;
                
                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
                
                char *message = autoSprintf( "MAP %d %d#",
                                             ourLiveObject->xd,
                                             ourLiveObject->yd );
                
                printf( "Sending message to server: %s\n", message );
                sendToSocket( mServerSocket, 
                              (unsigned char*)message, 
                              strlen( message ) );
                
                numServerBytesSent += strlen( message );
                overheadServerBytesSent += 52;
                
                delete [] message;
                }
            }
        
        // skip gui
        return;
        }
    
    if( hideGuiPanel ) {
        // skip gui
        return;
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
                slipPos.y += 
                    ( 0.5 * ( 1 - cos( mHungerSlipWiggleTime[i] ) ) ) *
                    mHungerSlipWiggleAmp[i] * distFromHidden;
                }
            

            drawSprite( mHungerSlipSprites[i], slipPos );
            }
        }
    
    // info panel at bottom
    setDrawColor( 1, 1, 1, 1 );
    doublePair panelPos = lastScreenViewCenter;
    panelPos.y -= 242 + 32 + 16 + 6;
    drawSprite( mGuiPanelSprite, panelPos );


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
        
        
                
        
        doublePair pos = { lastScreenViewCenter.x + 542, 
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

            drawSprite( mTempArrowErasedSprites[a->i], pos2 );
            }
        toggleAdditiveTextureColoring( false );
        
        setDrawColor( 1, 1, 1, 1 );

        mCurrentArrowHeat = ourLiveObject->heat;
        
        pos.x += ( mCurrentArrowHeat - 0.5 ) * 120;

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
            char *des = getObject( idToDescribe )->description;

            char *stringUpper = stringToUpperCase( des );

            // pound sign is used for trailing developer comments
            // that aren't show to end user, cut them off if they exist
            char *firstPound = strstr( stringUpper, "#" );
            
            if( firstPound != NULL ) {
                firstPound[0] = '\0';
                }

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







        
void LivingLifePage::step() {
    
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
            setSignal( "died" );
            instantStopMusic();
            setSoundLoudness( 0 );
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
            int speed = 4;

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
                int speed = 4;
                
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

            sscanf( message, 
                    "SN\n"
                    "%d/%d\n"
                    "%d\n", &currentPlayers, &maxPlayers, &number );
            
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
            

            char *outMessage = autoSprintf( "LOGIN %s %s %s#",
                                         userEmail, pwHash, keyHash );
            
            delete [] pwHash;
            delete [] keyHash;

            sendToSocket( mServerSocket, 
                          (unsigned char*)outMessage, 
                          strlen( outMessage ) );
            
            delete [] outMessage;
            
            delete [] message;
            return;
            }
        else if( type == ACCEPTED ) {
            // logged in successfully, wait for next message
            
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
            
            // recenter our in-ram sub-map around this new chunk
            int newMapOffsetX = x + sizeX/2;
            int newMapOffsetY = y + sizeY/2;
            
            // move old cached map cells over to line up with new center

            int xMove = mMapOffsetX - newMapOffsetX;
            int yMove = mMapOffsetY - newMapOffsetY;
            
            int *newMap = new int[ mMapD * mMapD ];
            int *newMapBiomes = new int[ mMapD * mMapD ];
            

            int *newMapAnimationFrameCount = new int[ mMapD * mMapD ];
            int *newMapAnimationLastFrameCount = new int[ mMapD * mMapD ];

            int *newMapAnimationFrozenRotFameCount = new int[ mMapD * mMapD ];
        
            AnimType *newMapCurAnimType = new AnimType[ mMapD * mMapD ];
            AnimType *newMapLastAnimType = new AnimType[ mMapD * mMapD ];
            double *newMapLastAnimFade = new double[ mMapD * mMapD ];
            
            doublePair *newMapDropOffsets = new doublePair[ mMapD * mMapD ];
            double *newMapDropRot = new double[ mMapD * mMapD ];
            SoundUsage *newMapDropSounds = new SoundUsage[ mMapD * mMapD ];
            char *newMapTileFlips= new char[ mMapD * mMapD ];

            char *newMapOurPlayerPlacedFlags = new char[ mMapD * mMapD ];

            
            for( int i=0; i<mMapD *mMapD; i++ ) {
                // starts uknown, not empty
                newMap[i] = -1;
                newMapBiomes[i] = -1;

                int newX = i % mMapD;
                int newY = i / mMapD;

                int worldX = newX + mMapOffsetX - mMapD / 2;
                int worldY = newY + mMapOffsetY - mMapD / 2;
                
                // each cell is different, but always the same
                newMapAnimationFrameCount[i] =
                    lrint( getXYRandom( worldX, worldY ) * 10000 );
                
                newMapCurAnimType[i] = ground;
                newMapLastAnimFade[i] = ground;
                newMapLastAnimFade[i] = 0;
                newMapDropOffsets[i].x = 0;
                newMapDropOffsets[i].y = 0;

                newMapTileFlips[i] = false;
                newMapOurPlayerPlacedFlags[i] = false;
                

                
                int oldX = newX - xMove;
                int oldY = newY - yMove;
                
                if( oldX >= 0 && oldX < mMapD
                    &&
                    oldY >= 0 && oldY < mMapD ) {
                    
                    int oI = oldY * mMapD + oldX;

                    newMap[i] = mMap[oI];
                    newMapBiomes[i] = mMapBiomes[oI];

                    newMapAnimationFrameCount[i] = mMapAnimationFrameCount[oI];
                    newMapAnimationLastFrameCount[i] = 
                        mMapAnimationLastFrameCount[oI];

                    newMapAnimationFrozenRotFameCount[i] = 
                        mMapAnimationFrozenRotFrameCount[oI];

                    newMapCurAnimType[i] = mMapCurAnimType[oI];
                    newMapLastAnimFade[i] = mMapLastAnimFade[oI];
                    newMapLastAnimFade[i] = mMapLastAnimFade[oI];
                    newMapDropOffsets[i] = mMapDropOffsets[oI];
                    newMapDropRot[i] = mMapDropRot[oI];
                    newMapDropSounds[i] = mMapDropSounds[oI];

                    newMapTileFlips[i] = mMapTileFlips[oI];
                    newMapOurPlayerPlacedFlags[i] = 
                        mMapOurPlayerPlacedFlags[oI];
                    }
                }
            
            memcpy( mMap, newMap, mMapD * mMapD * sizeof( int ) );
            memcpy( mMapBiomes, newMapBiomes, mMapD * mMapD * sizeof( int ) );

            memcpy( mMapAnimationFrameCount, newMapAnimationFrameCount, 
                    mMapD * mMapD * sizeof( int ) );
            memcpy( mMapAnimationLastFrameCount, 
                    newMapAnimationLastFrameCount, 
                    mMapD * mMapD * sizeof( int ) );

            memcpy( mMapAnimationFrozenRotFrameCount, 
                    newMapAnimationFrozenRotFameCount, 
                    mMapD * mMapD * sizeof( int ) );
            
            memcpy( mMapCurAnimType, newMapCurAnimType, 
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimFade, newMapLastAnimFade,
                    mMapD * mMapD * sizeof( AnimType ) );
            memcpy( mMapLastAnimFade, newMapLastAnimFade,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropOffsets, newMapDropOffsets,
                    mMapD * mMapD * sizeof( doublePair ) );
            memcpy( mMapDropRot, newMapDropRot,
                    mMapD * mMapD * sizeof( double ) );
            memcpy( mMapDropSounds, newMapDropSounds,
                    mMapD * mMapD * sizeof( SoundUsage ) );
            
            memcpy( mMapTileFlips, newMapTileFlips,
                    mMapD * mMapD * sizeof( char ) );

            memcpy( mMapOurPlayerPlacedFlags, newMapOurPlayerPlacedFlags,
                    mMapD * mMapD * sizeof( char ) );
            
            delete [] newMap;
            delete [] newMapBiomes;
            delete [] newMapAnimationFrameCount;
            delete [] newMapAnimationLastFrameCount;
            delete [] newMapAnimationFrozenRotFameCount;
            delete [] newMapCurAnimType;
            delete [] newMapLastAnimType;
            delete [] newMapLastAnimFade;
            delete [] newMapDropOffsets;
            delete [] newMapDropRot;
            delete [] newMapDropSounds;
            delete [] newMapTileFlips;
            
            delete [] newMapOurPlayerPlacedFlags;
            
            

            mMapOffsetX = newMapOffsetX;
            mMapOffsetY = newMapOffsetY;
            
            
            unsigned char *compressedChunk = 
                new unsigned char[ compressedSize ];
    
            
            for( int i=0; i<compressedSize; i++ ) {
                compressedChunk[i] = serverSocketBuffer.getElementDirect( 0 );
                
                serverSocketBuffer.deleteElement( 0 );
                }

            
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
                                    "%d:%d", 
                                    &( mMapBiomes[mapI] ),
                                    &( mMap[mapI] ) );
                            
                            if( mMap[mapI] != oldMapID ) {
                                // our placement status cleared
                                mMapOurPlayerPlacedFlags[mapI] = false;
                                }

                            mMapContainedStacks[mapI].deleteAll();
                            
                            
                            if( strstr( tokens->getElementDirect(i), "," ) 
                                != NULL ) {
                                
                                int numInts;
                                char **ints = 
                                    split( tokens->getElementDirect(i), 
                                           ",", &numInts );
                                
                                delete [] ints[0];
                                
                                int numContained = numInts - 1;
                                
                                for( int c=0; c<numContained; c++ ) {
                                    int contained = atoi( ints[ c + 1 ] );
                                    mMapContainedStacks[mapI].push_back( 
                                        contained );
                                    
                                    delete [] ints[ c + 1 ];
                                    }
                                delete [] ints;
                                }
                            }
                        }
                    }   
                
                tokens->deallocateStringElements();
                delete tokens;

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
                
                int x, y, responsiblePlayerID;
                
                                
                char *idBuffer = new char[500];

                int numRead = sscanf( lines[i], "%d %d %499s %d",
                                      &x, &y, idBuffer, &responsiblePlayerID );
                if( numRead == 4 ) {
                    int mapX = x - mMapOffsetX + mMapD / 2;
                    int mapY = y - mMapOffsetY + mMapD / 2;
                    
                    if( mapX >= 0 && mapX < mMapD
                        &&
                        mapY >= 0 && mapY < mMapD ) {
                        
                        int mapI = mapY * mMapD + mapX;

                        int old = mMap[mapI];

                        int newID = -1;
                        
                        int oldContainedCount = 
                            mMapContainedStacks[mapI].size();
                        

                        if( strstr( idBuffer, "," ) != NULL ) {
                            int numInts;
                            char **ints = 
                                split( idBuffer, ",", &numInts );
                        
                            
                            newID = atoi( ints[0] );

                            mMap[mapI] = newID;
                            
                            delete [] ints[0];

                            mMapContainedStacks[mapI].deleteAll();
                            
                            int numContained = numInts - 1;
                            
                            for( int c=0; c<numContained; c++ ) {
                                mMapContainedStacks[mapI].push_back(
                                    atoi( ints[ c + 1 ] ) );
                                delete [] ints[ c + 1 ];
                                }
                            delete [] ints;
                            }
                        else {
                            // a single int
                            newID = atoi( idBuffer );
                            mMap[mapI] = newID;
                            mMapContainedStacks[mapI].deleteAll();
                            }
                        
                        if( old != 0 &&
                            old == newID &&
                            mMapContainedStacks[mapI].size() > 
                            oldContainedCount &&
                            responsiblePlayerID > 0 ) {
                            

                            // first, try and play the "using"
                            // sound for the container
                            char soundPlayed = false;
                            
                            SoundUsage contSound = 
                                getObject( mMap[mapI] )->usingSound;
                            
                            if( contSound.id != -1 ) {
                                playSound( contSound, 
                                           getVectorFromCamera( x, y ) );
                                soundPlayed = true;
                                }

                            if( ! soundPlayed ) {
                                // no container using sound defined
                                
                                LiveObject *responsiblePlayerObject = 
                                    getGameObject( responsiblePlayerID );
                            
                                // play player's using sound
                            
                                SoundUsage s = getObject( 
                                    responsiblePlayerObject->displayID )->
                                    usingSound;

                                if( s.id != -1 ) {
                                    playSound( s, getVectorFromCamera( x, y ) );
                                    }
                                }
                            }
                        


                        if( responsiblePlayerID == -1 ) {
                            // no one dropped this
                            
                            // our placement status cleared
                            mMapOurPlayerPlacedFlags[mapI] = false;
                            }
                        

                        if( old != newID && 
                            newID != 0 && 
                            responsiblePlayerID <= -1 ) {
                            // ID change, and not just a player setting
                            // an object down

                            if( old > 0 &&
                                responsiblePlayerID == -1 ) {
                                // object auto-decayed from some other
                                // object
                                
                                // play decay sound
                                ObjectRecord *obj = getObject( old );
                                if( obj->decaySound.id != -1 ) {    
                                    
                                    playSound( obj->decaySound,
                                               getVectorFromCamera( x, y ) );
                                    }
                                
                                
                                }
                            

                            ObjectRecord *obj = getObject( newID );
                            
                            if( obj->creationSound.id != -1 ) {
                                
                                // make sure this is really a fresh creation
                                // of newID, and not a cycling back around
                                // for a reusable object
                                if( ! obj->creationSoundInitialOnly
                                    ||
                                    ! isAncestor( old, newID, 3 ) ) {
                                    
                                    playSound( obj->creationSound,
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

                            if( old == 0 ) {
                                // set down into an empty spot
                                // reset frame count
                                mMapAnimationFrameCount[mapI] = 0;
                                mMapAnimationLastFrameCount[mapI] = 0;
                                }
                            // else, leave existing frame count alone,
                            // since object has simply gone through a
                            // transition
                            
                            mMapCurAnimType[mapI] = ground;
                            mMapLastAnimType[mapI] = ground;
                            mMapLastAnimFade[mapI] = 0;
                            
                            mMapAnimationFrozenRotFrameCount[mapI] = 0;
                            
                            LiveObject *responsiblePlayerObject = NULL;
                            
                            
                            if( responsiblePlayerID != -1 ) {
                                responsiblePlayerObject = 
                                    getGameObject( responsiblePlayerID );

                                mMapOurPlayerPlacedFlags[mapI] = 
                                    ( responsiblePlayerID == ourID );
                                }
                            
                            
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
                                        
                                mMapAnimationFrozenRotFrameCount
                                    [mapI] =
                                    lrint( 
                                        responsiblePlayerObject->
                                        heldFrozenRotFrameCount );
                                        
                                        
                                if( responsiblePlayerObject->
                                    lastHeldAnimFade == 0 ) {
                                            
                                    mMapAnimationLastFrameCount[mapI] =
                                        lrint( 
                                            responsiblePlayerObject->
                                            heldAnimationFrameCount );
                                            
                                    mMapLastAnimType[mapI] = 
                                        responsiblePlayerObject->curHeldAnim;
                                    }
                                else {
                                    // dropped object was already
                                    // in the middle of a fade
                                    mMapCurAnimType[mapI] =
                                        responsiblePlayerObject->curHeldAnim;
                                            
                                    mMapAnimationFrameCount[mapI] =
                                        lrint( 
                                            responsiblePlayerObject->
                                            heldAnimationFrameCount );
                                    
                                    mMapLastAnimType[mapI] =
                                        responsiblePlayerObject->lastHeldAnim;
                                    
                                    mMapAnimationLastFrameCount[mapI] =
                                        lrint( 
                                            responsiblePlayerObject->
                                            lastHeldAnimationFrameCount );
                                    
                                    
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
                
                o.pathToDest = NULL;
                o.containedIDs = NULL;


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
                
                int forced = 0;
                int done_moving = 0;
                
                char *holdingIDBuffer = new char[500];

                int heldOriginValid, heldOriginX, heldOriginY,
                    heldTransitionSourceID;
                
                char *clothingBuffer = new char[500];
                
                int justAte = 0;

                int facingOverride = 0;
                int actionAttempt = 0;
                int actionTargetX = 0;
                int actionTargetY = 0;
                
                double invAgeRate = 60.0;
                

                int numRead = sscanf( lines[i], 
                                      "%d %d "
                                      "%d "
                                      "%d "
                                      "%d %d "
                                      "%499s %d %d %d %d %f %d %d %d %d "
                                      "%lf %lf %lf %499s %d",
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
                                      &justAte );
                
            
                if( numRead == 21 ) {
                    printf( "PLAYER_UPDATE with orVal=%d, orx=%d, ory=%d, "
                            "pX =%d, pY=%d\n",
                            heldOriginValid, heldOriginX, heldOriginY,
                            o.xd, o.yd );
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
                        
                        for( int c=0; c<o.numContained; c++ ) {
                            o.containedIDs[c] = atoi( ints[ c + 1 ] );
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
                    
                    if( existing != NULL ) {
                        int oldHeld = existing->holdingID;
                        
                        existing->lastHoldingID = oldHeld;
                        existing->holdingID = o.holdingID;
                        

                        // what we're holding hasn't changed
                        // maybe action failed
                        if( o.id == ourID && existing->holdingID == oldHeld ) {
                            
                            LiveObject *ourObj = getOurLiveObject();
                            
                            if( ourObj->pendingActionAnimationProgress != 0 &&
                                ! ourObj->inMotion ) {
                                
                                addNewAnimPlayerOnly( existing, ground );
                                }
                            }
                        
                        if( o.id != ourID ) {
                            
                            if( actionAttempt && ! justAte ) {
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
                                        addNewAnimPlayerOnly( 
                                            existing, eating );
                                        }
                                    else {
                                        addNewAnimPlayerOnly( 
                                            existing, doing );
                                        }
                                    
                                    addNewAnimPlayerOnly( existing, ground );
                                    }
                                }
                            }
                        else if( oldHeld != existing->holdingID ) {
                            // holding something new
                            
                            // what we're holding has gone through
                            // transition.  Keep old animation going
                            // for what's held
                            
                            if( o.id == ourID ) {
                                addNewAnimPlayerOnly( existing, ground2 );
                                }
                            else {
                                if( justAte ) {
                                    addNewAnimPlayerOnly( 
                                        existing, eating );
                                    }
                                else {
                                    addNewAnimPlayerOnly( 
                                        existing, doing );
                                    }
                                addNewAnimPlayerOnly( existing, ground );
                                }
                            
                            if( heldOriginValid ) {
                                
                                // use player's using sound for pickup
                                ObjectRecord *existingObj = 
                                    getObject( existing->displayID );
                                
                                if( existingObj->usingSound.id != -1 ) {
                                    
                                    playSound( existingObj->usingSound,
                                               getVectorFromCamera(
                                                   heldOriginX,
                                                   heldOriginY ) );
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
                                    existing->heldPosOverride = true;
                                    existing->heldPosOverrideAlmostOver = false;
                                    existing->heldObjectPos.x = heldOriginX;
                                    existing->heldObjectPos.y = heldOriginY;
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
                                        if( obj->decaySound.id != -1 ) {    
                                            
                                            playSound( 
                                                obj->decaySound,
                                                getVectorFromCamera(
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                            }
                                        }
                                    
                                    
                                    char creationSoundPlayed = false;
                                    
                                    if( heldObj->creationSound.id != -1 ) {
                                        
                                        int testAncestor = oldHeld;
                                        
                                        if( oldHeld == 0 &&
                                            heldTransitionSourceID > 0 ) {
                                            
                                            testAncestor = 
                                                heldTransitionSourceID;
                                            }
                                        

                                        if( testAncestor != 0 ) {
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


                                            if( ! heldObj->
                                                creationSoundInitialOnly
                                                ||
                                                ! isAncestor( 
                                                    testAncestor, 
                                                    existing->holdingID, 
                                                    3 ) ){

                                                playSound( 
                                                heldObj->creationSound,
                                                getVectorFromCamera( 
                                                    existing->currentPos.x, 
                                                    existing->currentPos.y ) );
                                                creationSoundPlayed = true;
                                                }
                                            }
                                        }
                                    
                                    if( oldHeld == 0 ) {
                                        // we're holding something new

                                        existing->lastHeldAnim = held;
                                        existing->
                                            lastHeldAnimationFrameCount = 0;
                                        existing->curHeldAnim = held;
                                        existing->heldAnimationFrameCount = 0;
                                        existing->lastHeldAnimFade = 0;
                                            
                                        
                                        if( !creationSoundPlayed ) {
                                            // play generic pickup sound

                                            ObjectRecord *existingObj = 
                                                getObject( 
                                                    existing->displayID );
                                
                                            if( existingObj->usingSound.id != 
                                                -1 ) {
                                    
                                                playSound( 
                                                    existingObj->usingSound,
                                                    getVectorFromCamera(
                                                     existing->currentPos.x, 
                                                     existing->currentPos.y ) );
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
                        
                        existing->displayID = o.displayID;
                        existing->age = o.age;
                        existing->lastAgeSetTime = o.lastAgeSetTime;
                        
                        existing->heat = o.heat;

                        if( existing->containedIDs != NULL ) {
                            delete [] existing->containedIDs;
                            }
                        existing->containedIDs = o.containedIDs;
                        existing->numContained = o.numContained;
                        
                        existing->xServer = o.xServer;
                        existing->yServer = o.yServer;
                        
                        existing->lastSpeed = o.lastSpeed;
                        
                        existing->clothing = o.clothing;
                        

                        if( existing->heldByAdultID != -1 ) {
                            // got a move for a player that's being held
                            // this means they've been dropped
                            
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                            
                            existing->currentSpeed = 0;
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            
                            existing->heldByDropOffset =
                                sub( existing->lastHeldByRawPos,
                                     existing->currentPos );

                            
                            
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
                            
                            existing->xd = o.xd;
                            existing->yd = o.yd;

                            addNewAnim( existing, ground );
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
                            
                            existing->clothingContained[c].deleteAll();

                            int newNumClothingCont = 
                                o.clothingContained[c].size();
                            int *newClothingCont = 
                                o.clothingContained[c].getElementArray();
                        
                            existing->clothingContained[c].appendArray(
                                newClothingCont, newNumClothingCont );
                            delete [] newClothingCont;
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
                        

                        o.inMotion = false;
                        
                        o.holdingFlip = false;

                        o.pendingAction = false;
                        o.pendingActionAnimationProgress = 0;
                        
                        o.currentPos.x = o.xd;
                        o.currentPos.y = o.yd;

                        o.heldPosOverride = false;
                        o.heldPosOverrideAlmostOver = false;
                        o.heldObjectPos = o.currentPos;
                        o.heldObjectRot = 0;

                        o.currentSpeed = 0;
                                                
                        o.moveTotalTime = 0;
                        
                        o.futureAnimStack = 
                            new SimpleVector<AnimType>();
                        o.futureHeldAnimStack = 
                            new SimpleVector<AnimType>();
                        
                        
                        ObjectRecord *obj = getObject( o.displayID );
                        
                        if( obj->creationSound.id != -1 ) {
                                
                            playSound( obj->creationSound,
                                       getVectorFromCamera( 
                                           o.currentPos.x,
                                           o.currentPos.y ) );
                            }
                        gameObjects.push_back( o );
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
                            
                            if( nextObject->containedIDs != NULL ) {
                                delete [] nextObject->containedIDs;
                                }
                            
                            if( nextObject->pathToDest != NULL ) {
                                delete [] nextObject->pathToDest;
                                }
                            
                            if( nextObject->currentSpeech != NULL ) {
                                delete [] nextObject->currentSpeech;
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
                }
            


            delete [] lines;


            if( ( mFirstServerMessagesReceived & 2 ) == 0 ) {
            
                LiveObject *ourObject = 
                    gameObjects.getElement( gameObjects.size() - 1 );
                
                ourID = ourObject->id;
                
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
                            
                            if( existing->currentSpeed != 0
                                &&
                                existing->pathToDest != NULL ) {
                                
                                // an interrupted move
                                oldPathLength = existing->pathLength;
                                oldCurrentPathPos = 
                                    existing->pathToDest[
                                        existing->currentPathStep ];
                                }
                            

                            if( existing->id != ourID ) {
                                // remove any double-backs from path
                                // because they confuse smooth path following
                                
                                SimpleVector<GridPos> filteredPath;
                                
                                int dbA = -1;
                                int dbB = -1;
                                
                                int longestDB = 0;
                                
                                
                                for( int e=0; e<o.pathLength; e++ ) {
                                    
                                    for( int f=e; f<o.pathLength; f++ ) {
                                        
                                        if( equal( o.pathToDest[e],
                                                   o.pathToDest[f] ) ) {
                                            
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
                                            o.pathToDest[e] );
                                        }
                                    
                                    // skip loop between

                                    for( int e=dbB + 1; e<o.pathLength; e++ ) {
                                        filteredPath.push_back( 
                                            o.pathToDest[e] );
                                        }
                                    
                                    o.pathLength = filteredPath.size();
                                    
                                    delete [] o.pathToDest;
                                    
                                    o.pathToDest = 
                                        filteredPath.getElementArray();
                                    }
                                
                                    
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
                                }
                            



                            if( existing->id != ourID ) {
                                // don't force-update these
                                // for our object
                                // we control it locally, to keep
                                // illusion of full move interactivity
                            
                                char usingOldPathStep = false;

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
                                    }
                                
                                
                                if( ! usingOldPathStep ) {
                                    // forced to jump to exactly where
                                    // server says we are
                                    
                                    // current step
                                    int b = 
                                        (int)floor( fractionPassed * 
                                                    existing->pathLength );
                                    // next step
                                    int n =
                                        (int)ceil( fractionPassed *
                                                   existing->pathLength );
                                    
                                    if( n == b ) {
                                        if( n < existing->pathLength - 1 ) {
                                            n ++ ;
                                            }
                                        else {
                                            b--;
                                            }
                                        }
                                    
                                    existing->currentPathStep = b;
                                
                                    double nWeight =
                                        fractionPassed * existing->pathLength 
                                        - b;
                                
                                    doublePair bWorld =
                                        gridToDouble(
                                            existing->pathToDest[ b ] );
                                    
                                    doublePair nWorld =
                                        gridToDouble(
                                            existing->pathToDest[ n ] );
                                    
                                
                                    existing->currentPos =
                                        add( mult( bWorld, 1 - nWeight ), 
                                             mult( nWorld, nWeight ) );
                                    
                                    existing->currentMoveDirection =
                                        normalize( sub( nWorld, bWorld ) );
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
                                            existing->currentPathStep )
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
                                }
                            
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
                
                sscanf( message, "FX\n%d %d %d %d %lf", 
                        &( ourLiveObject->foodStore ),
                        &( ourLiveObject->foodCapacity ),
                        &( lastAteID ),
                        &( lastAteFillMax ),
                        &( ourLiveObject->lastSpeed ) );

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

                        else if( strcmp( 
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
                    char *strUpper = stringToUpperCase(
                        getObject( lastAteID )->description );

                    mCurrentLastAteString = autoSprintf( "%s %s",
                                                         translate( "lastAte" ),
                                                         strUpper );
                    delete [] strUpper;
                    
                    mCurrentLastAteFillMax = lastAteFillMax;
                    }


                printf( "Our food = %d/%d\n", 
                        ourLiveObject->foodStore,
                        ourLiveObject->foodCapacity );
                
                if( ourLiveObject->foodStore > ourLiveObject->maxFoodStore ) {
                    ourLiveObject->maxFoodStore = ourLiveObject->foodStore;
                    }
                if( ourLiveObject->foodCapacity > 
                    ourLiveObject->maxFoodCapacity ) {
                    
                    ourLiveObject->maxFoodCapacity = 
                        ourLiveObject->foodCapacity;
                    }
                if( ourLiveObject->foodStore == ourLiveObject->foodCapacity ) {
                    mHungerSlipVisible = 0;
                    }
                else if( ourLiveObject->foodStore <= 3 ) {
                    mHungerSlipVisible = 2;
                    }
                else if( ourLiveObject->foodStore <= 6 ) {
                    mHungerSlipVisible = 1;
                    }
                else {
                    mHungerSlipVisible = -1;
                    }
                }
            }
        
        

        delete [] message;

        // process next message if there is one
        message = getNextServerMessage();
        }
    

    if( mapPullMode && mapPullCurrentSaved && ! mapPullCurrentSent ) {
        char *message = autoSprintf( "MAP %d %d#",
                                     mapPullCurrentX,
                                     mapPullCurrentY );
        
        printf( "Sending message to server: %s\n", message );
        sendToSocket( mServerSocket, 
                      (unsigned char*)message, 
                      strlen( message ) );
        
        numServerBytesSent += strlen( message );
        overheadServerBytesSent += 52;
        
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
        
        mSayField.setMaxLength( sayCap );



        LiveObject *cameraFollowsObject = ourLiveObject;
        
        if( ourLiveObject->heldByAdultID != -1 ) {
            cameraFollowsObject = 
                getGameObject( ourLiveObject->heldByAdultID );
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
            moveSpeedFactor = 1;
            }

        if( abs( dir.x ) > maxRX ) {
            double moveScale = moveSpeedFactor * sqrt( abs(dir.x) - maxRX ) 
                * frameRateFactor;

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.x = lrint( moveStep.x );
                        
            if( abs( moveStep.x ) > 0 ) {
                lastScreenViewCenter.x += moveStep.x;
                viewChange = true;
                }
            }
        if( abs( dir.y ) > maxRY ) {
            double moveScale = moveSpeedFactor * sqrt( abs(dir.y) - maxRY ) 
                * frameRateFactor;

            doublePair moveStep = mult( normalize( dir ), moveScale );
            
            // whole pixels

            moveStep.y = lrint( moveStep.y );
                        
            if( abs( moveStep.y ) > 0 ) {
                lastScreenViewCenter.y += moveStep.y;
                viewChange = true;
                }
            }
        

        if( false && length( dir ) > maxR ) {
            double moveScale = moveSpeedFactor * sqrt( length( dir ) - maxR ) 
                * frameRateFactor;

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
            
            doublePair dir = normalize( sub( endPos, o->currentPos ) );
            

            double turnFactor = 0.35;
            
            if( o->currentPathStep == o->pathLength - 2 ) {
                // last segment
                // speed up turn toward final spot so that we
                // don't miss it and circle around it forever
                turnFactor = 0.5;
                }
            
            
            o->currentMoveDirection = 
                normalize( add( o->currentMoveDirection, 
                                mult( dir, turnFactor * frameRateFactor ) ) );

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

                    if( abs( delta.x ) > CELL_D * 4 
                        ||
                        abs( delta.y ) > CELL_D * 1 ) {
                        
                        pointerDown( worldMouseX, worldMouseY );
                        }
                    }

                if( distance( endPos, o->currentPos )
                    < o->currentSpeed ) {

                    // reached destination
                    o->currentPos = endPos;
                    o->currentSpeed = 0;

                    //trailColor.r = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.g = randSource.getRandomBoundedDouble( 0, .5 );
                    //trailColor.b = randSource.getRandomBoundedDouble( 0, .5 );
                    

                    if( o->id != ourID || 
                        nextActionMessageToSend == NULL ) {
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
                    }
                }
            }
        else if( o->id != ourID && o->pendingActionAnimationProgress != 0 ) {
            
            o->pendingActionAnimationProgress += 0.025 * frameRateFactor;
            
            if( o->pendingActionAnimationProgress > 1 ) {
                    // no longer pending, finish last cycle by snapping
                    // back to 0
                    o->pendingActionAnimationProgress = 0;
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
            
            printf( "Sending pending action message to server: %s\n",
                    nextActionMessageToSend );
            
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            sendToSocket( mServerSocket, 
                          (unsigned char*)nextActionMessageToSend, 
                          strlen( nextActionMessageToSend) );
            
            numServerBytesSent += strlen( nextActionMessageToSend );
            overheadServerBytesSent += 52;

            if( nextActionEating ) {
                if( ourLiveObject->holdingID > 0 ) {
                    ObjectRecord *held = getObject( ourLiveObject->holdingID );
                    
                    if( held->eatingSound.id != -1 ) {
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
                restartMusic( computeCurrentAge( ourLiveObject ),
                              ourLiveObject->ageRate );
                setSoundLoudness( 1.0 );
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
                    mapPullCurrentSaved = true;
                    mapPullModeFinalImage = false;
                    
                    char *message = autoSprintf( "MAP %d %d#",
                                                 mapPullCurrentX,
                                                 mapPullCurrentY );

                    printf( "Sending message to server: %s\n", message );

                    sendToSocket( mServerSocket, 
                                  (unsigned char*)message, 
                                  strlen( message ) );
            
                    numServerBytesSent += strlen( message );
                    overheadServerBytesSent += 52;
                    
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
                
                // and what they're holding
                if( o->holdingID > 0 ) {
                    addBaseObjectToLiveObjectSet( o->holdingID );

                    // and what it contains
                    for( int j=0; j<o->numContained; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            o->containedIDs[j] );
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
                if( mMap[i] > 0 ) {
                    
                    addBaseObjectToLiveObjectSet( mMap[i] );
            
                    // and what is contained in each object
                    int numCont = mMapContainedStacks[i].size();
                    
                    for( int j=0; j<numCont; j++ ) {
                        addBaseObjectToLiveObjectSet(
                            mMapContainedStacks[i].getElementDirect( j ) );
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

    if( !inFresh ) {
        return;
        }
    
    mNotePaperPosOffset = mNotePaperHideOffset;

    mNotePaperPosTargetOffset = mNotePaperPosOffset;

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
    
    if( mMapOurPlayerPlacedFlags[ clickDestMapI ] ) {
        // check this cell first
        
        int oID = mMap[ clickDestMapI ];
        
        if( oID > 0 ) {
            ObjectRecord *obj = getObject( oID );
                
            int sp, cl, sl;
                
            double dist = getClosestObjectPart( 
                obj,
                NULL,
                &( mMapContainedStacks[clickDestMapI] ),
                NULL,
                -1,
                -1,
                mMapTileFlips[ clickDestMapI ],
                clickExtraX,
                clickExtraY,
                &sp, &cl, &sl,
                // ignore transparent parts
                // allow objects behind smoke to be picked up
                false );
            
            if( dist < minDistThatHits ) {
                p->hit = true;
                p->closestCellX = clickDestX;
                p->closestCellY = clickDestY;
                
                p->hitSlotIndex = sl;
                
                p->hitAnObject = true;
                }
            }
        }
    


    // start in front row
    // right to left
    // (check things that are in front first
    for( int y=clickDestY-2; y<=clickDestY+1 && ! p->hit; y++ ) {
        float clickOffsetY = ( clickDestY  - y ) * CELL_D + clickExtraY;

        // first all non drawn-behind map objects in this row
        // (in front of people)
        for( int x=clickDestX+1; x>=clickDestX-1  && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                ! getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                
                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    clickOffsetX,
                    clickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    p->closestCellX = x;
                    p->closestCellY = y;
                    
                    p->hitSlotIndex = sl;

                    p->hitAnObject = true;
                    }
                }
            }
        
        // next, people in this row
        // recently dropped babies are in front and tested first
        for( int d=0; d<2 && ! p->hit; d++ )
        for( int x=clickDestX+1; x>=clickDestX-1 && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;
            
            for( int i=gameObjects.size()-1; i>=0 && ! p->hit; i-- ) {
        
                LiveObject *o = gameObjects.getElement( i );
                
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

        for( int x=clickDestX+1; x>=clickDestX-1  && ! p->hit; x-- ) {
            float clickOffsetX = ( clickDestX  - x ) * CELL_D + clickExtraX;

            int mapX = x - mMapOffsetX + mMapD / 2;
            int mapY = y - mMapOffsetY + mMapD / 2;

            int mapI = mapY * mMapD + mapX;

            int oID = mMap[ mapI ];
            
            

            if( oID > 0 &&
                getObject( oID )->drawBehindPlayer ) {
                ObjectRecord *obj = getObject( oID );
                
                int sp, cl, sl;
                
                double dist = getClosestObjectPart( 
                    obj,
                    NULL,
                    &( mMapContainedStacks[mapI] ),
                    NULL,
                    -1,
                    -1,
                    mMapTileFlips[ mapI ],
                    clickOffsetX,
                    clickOffsetY,
                    &sp, &cl, &sl,
                    // ignore transparent parts
                    // allow objects behind smoke to be picked up
                    false );
                
                if( dist < minDistThatHits ) {
                    p->hit = true;
                    p->closestCellX = x;
                    p->closestCellY = y;
                    
                    p->hitSlotIndex = sl;

                    p->hitAnObject = true;
                    }
                }
            }
        }
    }

    


        





void LivingLifePage::pointerMove( float inX, float inY ) {
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );

    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    

    p.hit = false;
    p.hitSelf = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;


    checkForPointerHit( &p, inX, inY );
    
    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;
    

    int destID = 0;
        
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    if( p.hitAnObject && mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        }

    if( destID > 0 ) {
        mCurMouseOverID = destID;
        
        }
    else if( mCurMouseOverID != 0 ) {
        mLastMouseOverID = mCurMouseOverID;
        mCurMouseOverID = 0;
        mLastMouseOverFade = 1.0f;
        }
    
    }







void LivingLifePage::pointerDown( float inX, float inY ) {
    mLastMouseOverID = 0;
    
    mouseDown = true;
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    
    if( mFirstServerMessagesReceived != 3 || ! mDoneLoadingFirstObjectSet ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject = getOurLiveObject();

    

    // consider 3x4 area around click and test true object pixel
    // collisions in that area
    PointerHitRecord p;
    
    p.hitSlotIndex = -1;
    
    p.hit = false;
    p.hitSelf = false;
    
    p.hitClothingIndex = -1;
    

    // when we click in a square, only count as hitting something
    // if we actually clicked the object there.  Else, we can walk
    // there if unblocked.
    p.hitAnObject = false;


    checkForPointerHit( &p, inX, inY );
    
    
    int clickDestX = p.closestCellX;
    int clickDestY = p.closestCellY;

    nextActionEating = false;
    nextActionDropping = false;
    
    char modClick = false;
    
    if( mEKeyDown || isLastMouseButtonRight() ) {
        modClick = true;
        }

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
                                 clickDestX, clickDestY, 
                                 p.hitClothingIndex );
                printf( "Use on self\n" );
                }
            else {
                if( ourLiveObject->holdingID > 0 ) {
                    nextActionMessageToSend = 
                        autoSprintf( "DROP %d %d %d#",
                                     clickDestX, clickDestY, 
                                     p.hitClothingIndex  );
                    nextActionDropping = true;
                    }
                else {
                    nextActionMessageToSend = 
                        autoSprintf( "SREMV %d %d %d %d#",
                                     clickDestX, clickDestY, 
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


    int destID = 0;
        
    int destNumContained = 0;
    
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    
    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
    
        destNumContained = mMapContainedStacks[ mapY * mMapD + mapX ].size();
        }


    if( destID > 0 && ! p.hitAnObject ) {
        
        // clicked on empty space near an object
        
        if( ! getObject( destID )->blocksWalking ) {
            
            // object in this space not blocking
            
            // count as an attempt to walk to the spot where the object is
            destID = 0;
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
                                         clickDestX, clickDestY );
                        
                        
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
                                         clickDestX, clickDestY, 
                                         p.hitClothingIndex );
                        
                        
                        playerActionTargetX = clickDestX;
                        playerActionTargetY = clickDestY;
                        
                        playerActionTargetNotAdjacent = true;
                        
                        printf( "UBABY with target player %d\n", o->id );

                        return;
                        }
                    else {
                        // too far away, but try to kill later,
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
             || ( ! modClick && destID != 0 )
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
                mustMove = true;
                }
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
                    // a use-on-ground transition

                    if( ourLiveObject->holdingID > 0 ) {
                        
                        ObjectRecord *held = 
                            getObject( ourLiveObject->holdingID );
                        
                        if( held->foodValue == 0 ) {
                            
                            TransRecord *r = 
                                getTrans( ourLiveObject->holdingID,
                                          -1 );
                            
                            if( r != NULL &&
                                r->newTarget != 0 ) {
                                
                                // a use-on-ground transition exists!
                                
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
            else if( ! modClick && destID != 0 ) {
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
                                 clickDestX, clickDestY, extra );
                
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

        computePathToDest( ourLiveObject );

        
        if( ourLiveObject->pathToDest == NULL ) {
            // adjust move to closest possible
            ourLiveObject->xd = ourLiveObject->closestDestIfPathFailedX;
            ourLiveObject->yd = ourLiveObject->closestDestIfPathFailedY;
            
            if( ourLiveObject->xd == oldXD && ourLiveObject->yd == oldYD ) {
                // completely blocked in, no path at all toward dest
                
                // ignore click
                
                if( nextActionMessageToSend != NULL ) {
                    delete [] nextActionMessageToSend;
                    nextActionMessageToSend = NULL;
                    }
                ourLiveObject->inMotion = false;
                return;
                }
            

            computePathToDest( ourLiveObject );
            
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
        

        // send move right away
        //Thread::staticSleep( 2000 );
        SimpleVector<char> moveMessageBuffer;
        
        moveMessageBuffer.appendElementString( "MOVE" );
        // start is absolute
        char *startString = autoSprintf( " %d %d", 
                                         ourLiveObject->pathToDest[0].x,
                                         ourLiveObject->pathToDest[0].y );
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

        printf( "Sending message to server:  %s\n", message );
        
        sendToSocket( mServerSocket, (unsigned char*)message, 
                      strlen( message ) );
            
        numServerBytesSent += strlen( message );
        overheadServerBytesSent += 52;

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
    getLastMouseScreenPos( &lastScreenMouseX, &lastScreenMouseY );
    }

void LivingLifePage::pointerUp( float inX, float inY ) {
    mouseDown = false;
    }

void LivingLifePage::keyDown( unsigned char inASCII ) {

    switch( inASCII ) {
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
                        autoSprintf( "SAY 0 0 %s\n#",
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
                    
                    sendToSocket( mServerSocket, (unsigned char*)message, 
                                  strlen( message ) );
            
                    numServerBytesSent += strlen( message );
                    overheadServerBytesSent += 52;

                    
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

        
