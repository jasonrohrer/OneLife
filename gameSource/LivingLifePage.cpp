#include "LivingLifePage.h"

#include "objectBank.h"

#include "minorGems/util/SimpleVector.h"


#include "minorGems/game/Font.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/formats/encodingUtils.h"


#include <stdlib.h>//#include <math.h>


extern double frameRateFactor;

extern Font *mainFont;

extern doublePair lastScreenViewCenter;



int numServerBytesRead = 0;
int numServerBytesSent = 0;

int overheadServerBytesSent = 0;
int overheadServerBytesRead = 0;



SimpleVector<unsigned char> serverSocketBuffer;


// reads all waiting data from socket and stores it in buffer
void readServerSocketFull( int inServerSocket ) {

    unsigned char buffer[512];
    
    int numRead = readFromSocket( inServerSocket, buffer, 512 );
    
    
    while( numRead > 0 ) {
        serverSocketBuffer.appendArray( buffer, numRead );
        numServerBytesRead += numRead;

        numRead = readFromSocket( inServerSocket, buffer, 512 );
        }    
    }



// NULL if there's no full message available
char *getNextServerMessage() {
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
    
    return message;
    }



typedef enum messageType {
	MAP_CHUNK,
    MAP_CHANGE,
    PLAYER_UPDATE,
    PLAYER_MOVES_START,
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

    if( strcmp( copy, "MC" ) == 0 ) {
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
    
    delete [] copy;
    return returnValue;
    }






SimpleVector<LiveObject> gameObjects;




void updateMoveSpeed( LiveObject *inObject ) {
    doublePair endPos = { (double)inObject->xd, (double)inObject->yd };
    
    double etaSec = inObject->moveEtaTime - game_getCurrentTime();
    
    doublePair moveLeft = sub( endPos, 
                               inObject->currentPos );

    doublePair speedPerSec =
        mult( moveLeft, 1.0 / etaSec );
                            
    inObject->currentSpeed =
        mult( speedPerSec, 
              1.0 / getRecentFrameRate() );

    inObject->timeOfLastSpeedUpdate = game_getCurrentTime();
    }



void LivingLifePage::computePathToDest( LiveObject *inObject ) {
    
    if( inObject->pathToDest != NULL ) {
        delete [] inObject->pathToDest;
        inObject->pathToDest = NULL;
        }

    GridPos start;
    start.x = (int)( inObject->currentPos.x );
    start.y = (int)( inObject->currentPos.y );

    GridPos end = { inObject->xd, inObject->yd };
        
    char *blockedMap = new char[ mMapD * mMapD ];
    for( int i=0; i<mMapD*mMapD; i++ ) {
        blockedMap[i] = ( mMap[ i ] != 0 );
        }
    
    int pathOffsetX = mMapD/2 - mMapOffsetX;
    int pathOffsetY = mMapD/2 - mMapOffsetY;
    
    start.x += pathOffsetX;
    start.y += pathOffsetY;
    
    end.x += pathOffsetX;
    end.y += pathOffsetY;
    
    double startTime = game_getCurrentTime();

    GridPos closestFound;
    
    char pathFound = 
        pathFind( mMapD, mMapD,
                  blockedMap, 
                  start, end, 
                  &( inObject->pathLength ),
                  &( inObject->pathToDest ),
                  &closestFound );
    
    if( pathFound ) {
        printf( "Path found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );

        // move into world coordinates
        for( int i=0; i<inObject->pathLength; i++ ) {
            inObject->pathToDest[i].x -= pathOffsetX;
            inObject->pathToDest[i].y -= pathOffsetY;
            }
        }
    else {
        printf( "Path not found in %f ms\n", 
                1000 * ( game_getCurrentTime() - startTime ) );
        
        inObject->closestDestIfPathFailedX = 
            closestFound.x - pathOffsetX;

        inObject->closestDestIfPathFailedY = 
            closestFound.y - pathOffsetY;
        
        }
    
    inObject->currentPathStep = 0;
    
    delete [] blockedMap;
    }






// if user clicks to initiate an action while still moving, we
// queue it here
static char *nextActionMessageToSend = NULL;

// block move until next PLAYER_UPDATE received after action sent
static char playerActionPending = false;
static int playerActionTargetX, playerActionTargetY;



int ourID;

char lastCharUsed = 'A';




LivingLifePage::LivingLifePage() 
        : mServerAddress( NULL ),
          mServerSocket( -1 ), 
          mFirstServerMessagesReceived( 0 ),
          mMapD( 64 ),
          mMapOffsetX( 0 ),
          mMapOffsetY( 0 ),
          mEKeyDown( false ) {

    
    mServerAddress = SettingsManager::getStringSetting( "serverAddress" );

    if( mServerAddress == NULL ) {
        mServerAddress = stringDuplicate( "127.0.0.1" );
        }

    mServerPort = SettingsManager::getIntSetting( "serverPort", 5077 );



    mMap = new int[ mMapD * mMapD ];
    
    for( int i=0; i<mMapD *mMapD; i++ ) {
        mMap[i] = 0;
        }

    }


LivingLifePage::~LivingLifePage() {
    printf( "Total received = %d bytes (+%d in headers), "
            "total sent = %d bytes (+%d in headers)\n",
            numServerBytesRead, overheadServerBytesRead,
            numServerBytesSent, overheadServerBytesSent );


    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *nextObject =
            gameObjects.getElement( i );
        
        if( nextObject->pathToDest != NULL ) {
            delete [] nextObject->pathToDest;
            }
        }
    
    gameObjects.deleteAll();
    


    if( mServerAddress != NULL ) {    
        delete [] mServerAddress;
        mServerAddress = NULL;
        }
    
    if( mServerSocket != -1 ) {
        closeSocket( mServerSocket );
        }


    delete [] mMap;

    delete [] nextActionMessageToSend;
    }





void LivingLifePage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    drawSquare( lastScreenViewCenter, 400 );
    
    //if( currentGamePage != NULL ) {
    //    currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
    //    }
    
    setDrawColor( 1, 1, 1, 1 );

    int gridCenterX = 
        lrintf( lastScreenViewCenter.x / 32 ) - mMapOffsetX + mMapD/2;
    int gridCenterY = 
        lrintf( lastScreenViewCenter.y / 32 ) - mMapOffsetY + mMapD/2;
    
    int xStart = gridCenterX - 12;
    int xEnd = gridCenterX + 12;

    int yStart = gridCenterY - 12;
    int yEnd = gridCenterY + 12;

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
    
    if( yStart < 0 ) {
        yStart = 0;
        }
    if( yEnd >= mMapD ) {
        yEnd = mMapD - 1;
        }
    

    for( int y=yEnd; y>=yStart; y-- ) {
        
        int screenY = 32 * ( y + mMapOffsetY - mMapD / 2 );
        
        for( int x=xStart; x<=xEnd; x++ ) {
            
            int screenX = 32 * ( x + mMapOffsetX - mMapD / 2 );

            int mapI = y * mMapD + x;
            
            int oID = mMap[ mapI ];
            
            if( oID > 0 ) {
                
                doublePair pos = { (double)screenX, (double)screenY };

                drawObject( getObject(oID), pos );
                }
            }
        }
        
    doublePair lastChunkCenter = { 32.0 * mMapOffsetX, 32.0 * mMapOffsetY };
    
    setDrawColor( 0, 1, 0, 1 );
    
    mainFont->drawString( "X", 
                          lastChunkCenter, alignCenter );
    
    
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );


        if( o->currentPos.x != o->xd || o->currentPos.y != o->yd ) {
            // destination
            
            char *string = autoSprintf( "[%c]", o->displayChar );
        
            doublePair pos;
            pos.x = o->xd * 32;
            pos.y = o->yd * 32;
        
            setDrawColor( 1, 0, 0, 1 );
            mainFont->drawString( string, 
                                  pos, alignCenter );
            delete [] string;


            if( o->pathToDest != NULL ) {
                // highlight path
                
                for( int p=0; p< o->pathLength; p++ ) {
                    GridPos pathSpot = o->pathToDest[ p ];
                    
                    pos.x = pathSpot.x * 32;
                    pos.y = pathSpot.y * 32;

                    setDrawColor( 1, 1, 0, 1 );
                    mainFont->drawString( "P", 
                                          pos, alignCenter );
                    }
                }
            else {
                pos.x = o->closestDestIfPathFailedX * 32;
                pos.y = o->closestDestIfPathFailedY * 32;
                
                setDrawColor( 1, 0, 1, 1 );
                mainFont->drawString( "P", 
                                      pos, alignCenter );
                }
            
            
            }



        // current pos
        char *string = autoSprintf( "%c", o->displayChar );

        doublePair pos = mult( o->currentPos, 32 );

        doublePair actionOffset = { 0, 0 };
        
        
        if( o->id == ourID && o->pendingActionAnimationProgress != 0 ) {
            // wiggle toward target

            float xDir = 0;
            float yDir = 0;
            
            if( o->xd < playerActionTargetX ) {
                xDir = 1;
                }
            if( o->xd > playerActionTargetX ) {
                xDir = -1;
                }
            if( o->yd < playerActionTargetY ) {
                yDir = 1;
                }
            if( o->yd > playerActionTargetY ) {
                yDir = -1;
                }
            
            double offset =
                8 - 
                8 * cos( 2 * M_PI * o->pendingActionAnimationProgress );
            

            actionOffset.x += xDir * offset;
            actionOffset.y += yDir * offset;
            }
        

        // bare hands action OR holding something
        // character wiggle
        if(  o->id == ourID && o->pendingActionAnimationProgress != 0 ) {
            
            pos = add( pos, actionOffset );
            }
        

        setDrawColor( 0, 0, 0, 1 );
        mainFont->drawString( string, 
                              pos, alignCenter );

        delete [] string;
        
        if( o->holdingID != 0 ) { 
            doublePair holdPos = pos;
            holdPos.x += 16;
            holdPos.y -= 16;
            
            setDrawColor( 1, 1, 1, 1 );
            drawObject( getObject( o->holdingID ), holdPos );
            }
        
        
        }


    setDrawColor( 0, 0, 0, 0.125 );
    
    int screenGridOffsetX = lrint( lastScreenViewCenter.x / 32 );
    int screenGridOffsetY = lrint( lastScreenViewCenter.y / 32 );
    
    for( int y=-11; y<12; y++ ) {
        for( int x=-11; x<12; x++ ) {
            
            doublePair pos;
            pos.x = ( x + screenGridOffsetX ) * 32;
            pos.y = ( y + screenGridOffsetY ) * 32;
            
            drawSquare( pos, 14 );
            }
        }    

    }




        
void LivingLifePage::step() {
    
    if( mServerSocket == -1 ) {
        mServerSocket = openSocketConnection( mServerAddress, mServerPort );
        return;
        }
    


    // first, read all available data from server
    readServerSocketFull( mServerSocket );
    

    char *message = getNextServerMessage();


    if( message != NULL ) {
        overheadServerBytesRead += 52;
        
        printf( "Got length %d message\n%s\n", strlen( message ), message );

        messageType type = getMessageType( message );
        

        if( type == MAP_CHUNK ) {
            
            int size = 0;
            int x = 0;
            int y = 0;
            
            int binarySize = 0;
            int compressedSize = 0;
            
            sscanf( message, "MC\n%d %d %d\n%d %d\n", 
                    &size, &x, &y, &binarySize, &compressedSize );
            
            printf( "Got map chunk with bin size %d, compressed size %d\n", 
                    binarySize, compressedSize );
            
            // recenter our in-ram sub-map around this new chunk
            int newMapOffsetX = x + size/2;
            int newMapOffsetY = y + size/2;
            
            // move old cached map cells over to line up with new center

            int xMove = mMapOffsetX - newMapOffsetX;
            int yMove = mMapOffsetY - newMapOffsetY;
            
            int *newMap = new int[ mMapD * mMapD ];
            
            for( int i=0; i<mMapD *mMapD; i++ ) {
                newMap[i] = 0;

                int newX = i % mMapD;
                int newY = i / mMapD;
                
                int oldX = newX - xMove;
                int oldY = newY - yMove;
                
                if( oldX >= 0 && oldX < mMapD
                    &&
                    oldY >= 0 && oldY < mMapD ) {
                    
                    newMap[i] = mMap[ oldY * mMapD + oldX ];
                    }
                }
            
            memcpy( mMap, newMap, mMapD * mMapD * sizeof( int ) );
            
            delete [] newMap;

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
            
            
            unsigned char *binaryChunk = new unsigned char[ binarySize + 1 ];
            
            memcpy( binaryChunk, decompressedChunk, binarySize );
            
            delete [] decompressedChunk;
 
            
            // for now, binary chunk is actually just ASCII
            binaryChunk[ binarySize ] = '\0';
            

            SimpleVector<char *> *tokens = 
                tokenizeString( (char*)binaryChunk );
            
            delete [] binaryChunk;


            int numCells = size * size;

            if( tokens->size() == numCells ) {
                
                for( int i=0; i<tokens->size(); i++ ) {
                    int cX = i % size;
                    int cY = i / size;
                    
                    int mapX = cX + x - mMapOffsetX + mMapD / 2;
                    int mapY = cY + y - mMapOffsetY + mMapD / 2;
                    
                    if( mapX >= 0 && mapX < mMapD
                        &&
                        mapY >= 0 && mapY < mMapD ) {
                        
                        
                        int mapI = mapY * mMapD + mapX;
                        
                        sscanf( tokens->getElementDirect(i),
                                "%d", &( mMap[mapI] ) );
                        }
                    }
                }   
            
            tokens->deallocateStringElements();
            delete tokens;

            mFirstServerMessagesReceived |= 1;
            }
        else if( type == MAP_CHANGE ) {
            int numLines;
            char **lines = split( message, "\n", &numLines );
            
            if( numLines > 0 ) {
                // skip fist
                delete [] lines[0];
                }
            
            
            for( int i=1; i<numLines; i++ ) {
                
                int x, y, id;
                int numRead = sscanf( lines[i], "%d %d %d",
                                      &x, &y, &id );
                if( numRead == 3 ) {
                    int mapX = x - mMapOffsetX + mMapD / 2;
                    int mapY = y - mMapOffsetY + mMapD / 2;
                    
                    if( mapX >= 0 && mapX < mMapD
                        &&
                        mapY >= 0 && mapY < mMapD ) {
                        
                        mMap[mapY * mMapD + mapX ] = id;
                        }
                    }
                
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
            
            
            for( int i=1; i<numLines; i++ ) {

                LiveObject o;
                
                o.pathToDest = NULL;
                
                int numRead = sscanf( lines[i], "%d %d %d %d %lf",
                                      &( o.id ),
                                      &( o.holdingID ),
                                      &( o.xd ),
                                      &( o.yd ),
                                      &( o.lastSpeed ) );
                
                if( numRead == 5 ) {
                    
                    LiveObject *existing = NULL;

                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            existing = gameObjects.getElement(j);
                            break;
                            }
                        }
                    
                    if( existing != NULL ) {
                        existing->holdingID = o.holdingID;
                        
                        
                        
                        existing->lastSpeed = o.lastSpeed;
                        
                        if( existing->id != ourID ) {
                            // don't ever force-update these for
                            // our locally-controlled object
                            // give illusion of it being totally responsive
                            // to move commands
                            existing->currentPos.x = o.xd;
                            existing->currentPos.y = o.yd;
                        
                            existing->currentSpeed.x = 0;
                            existing->currentSpeed.y = 0;
                        
                            existing->xd = o.xd;
                            existing->yd = o.yd;
                            }
                        else {
                            // update for us
                            
                            if( !existing->inMotion ) {
                                // this is an update post-action, not post-move
                                
                                // ready to execute next action
                                playerActionPending = false;
                                
                                existing->pendingAction = false;
                                }
                            }
                        
                        // in motion until update received, now done
                        existing->inMotion = false;
                        
                        existing->moveTotalTime = 0;
                        }
                    else {    
                        o.displayChar = lastCharUsed + 1;
                    
                        lastCharUsed = o.displayChar;
                    

                        o.inMotion = false;

                        o.pendingAction = false;
                        o.pendingActionAnimationProgress = 0;
                        
                        o.currentPos.x = o.xd;
                        o.currentPos.y = o.yd;
                        
                        o.currentSpeed.x = 0;
                        o.currentSpeed.y = 0;
                        
                        o.moveTotalTime = 0;
                        
                        
                        gameObjects.push_back( o );
                        }
                    }
                else if( numRead == 2 ) {
                    if( strstr( lines[i], "X X" ) != NULL  ) {
                        // object deleted
                        
                        numRead = sscanf( lines[i], "%d %d",
                                          &( o.id ),
                                          &( o.holdingID ) );
                        

                        for( int i=0; i<gameObjects.size(); i++ ) {
        
                            LiveObject *nextObject =
                                gameObjects.getElement( i );
                            
                            if( nextObject->id == o.id ) {

                                if( nextObject->pathToDest != NULL ) {
                                    delete [] nextObject->pathToDest;
                                    }
                                
                                gameObjects.deleteElement( i );
                                break;
                                }
                            }
                        
                        }
                    }
                
                delete [] lines[i];
                }
            

            delete [] lines;


            if( ( mFirstServerMessagesReceived & 2 ) == 0 ) {
            
                LiveObject *ourObject = 
                    gameObjects.getElement( gameObjects.size() - 1 );
                
                ourID = ourObject->id;
                
                ourObject->displayChar = 'A';

                // center view on player's starting position
                lastScreenViewCenter.x = 32 * ourObject->xd;
                lastScreenViewCenter.y = 32 * ourObject->yd;

                setViewCenterPosition( lastScreenViewCenter.x, 
                                       lastScreenViewCenter.y );
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
                int deltaX, deltaY;
                
                int numRead = sscanf( lines[i], "%d %d %d %d %d %lf %lf",
                                      &( o.id ),
                                      &( startX ),
                                      &( startY ),
                                      &( deltaX ),
                                      &( deltaY ),
                                      &( o.moveTotalTime ),
                                      &etaSec );
                
                o.xd = startX + deltaX;
                o.yd = startY + deltaY;
                
                o.moveEtaTime = etaSec + game_getCurrentTime();
                

                if( numRead == 7 ) {
                    
                    for( int j=0; j<gameObjects.size(); j++ ) {
                        if( gameObjects.getElement(j)->id == o.id ) {
                            
                            LiveObject *existing = gameObjects.getElement(j);
                            
                            double timePassed = 
                                o.moveTotalTime - etaSec;
                            
                            double fractionPassed = 
                                timePassed / o.moveTotalTime;
                            
                            doublePair startPos = { (double)startX,
                                                    (double)startY };
                            doublePair endPos = { (double)o.xd,
                                                  (double)o.yd };
                            
                            
                            
                            
                            // stays in motion until we receive final
                            // PLAYER_UPDATE from server telling us
                            // that move is over
                            existing->inMotion = true;
                            
                            if( existing->id != ourID ) {
                                // don't force-update these
                                // for our object
                                // we control it locally, to keep
                                // illusion of full move interactivity
                            
                                if( equal( existing->currentPos, startPos ) ) {
                                
                                    existing->currentPos = 
                                        add( mult( endPos, 
                                                   fractionPassed ), 
                                             mult( startPos, 
                                                   1 - fractionPassed ) );
                                    }
                                
                                
                                existing->xd = o.xd;
                                existing->yd = o.yd;
                                

                                computePathToDest( existing );
                                
                                
                                existing->moveTotalTime = o.moveTotalTime;
                                existing->moveEtaTime = o.moveEtaTime;

                                updateMoveSpeed( existing );
                                }
                            
                            break;
                            }
                        }
                    }
                delete [] lines[i];
                }
            

            delete [] lines;
            }
        

        delete [] message;
        }
    
        
    // check if we're about to move off the screen
    LiveObject *ourLiveObject = NULL;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == ourID ) {
            ourLiveObject = o;
            break;
            }
        }

    
    if( ourLiveObject != NULL ) {
        
        doublePair screenTargetPos = mult( ourLiveObject->currentPos, 32 );
        

        if( ourLiveObject->currentPos.x != ourLiveObject->xd
            ||
            ourLiveObject->currentPos.y != ourLiveObject->yd ) {
            
            // moving

            // push camera out in front
            
            doublePair moveDir = { ourLiveObject->xd -
                                   ourLiveObject->currentPos.x,
                                   ourLiveObject->yd -
                                   ourLiveObject->currentPos.y };
            
            
            if( length( moveDir ) > 10 ) {
                moveDir = mult( normalize( moveDir ), 10 );
                }
                
            moveDir = mult( moveDir, 32 );
            
            
            screenTargetPos = add( screenTargetPos, moveDir );
            }
        
        // whole pixels
        screenTargetPos.x = round( screenTargetPos.x );
        screenTargetPos.y = round( screenTargetPos.y );
        

        doublePair dir = sub( screenTargetPos, lastScreenViewCenter );
        
        char viewChange = false;
        
        int maxR = 50;
        double moveSpeedFactor = .5;
        

        if( length( dir ) > maxR ) {
            
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
    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->currentSpeed.x != 0 ||
            o->currentSpeed.y != 0 ) {

            doublePair endPos = { (double)o->xd, (double)o->yd };
            
            if( distance( endPos, o->currentPos )
                < length( o->currentSpeed ) ) {
                
                // reached destination
                o->currentPos = endPos;
                o->currentSpeed.x = 0;
                o->currentSpeed.y = 0;
                }
            else {
                // still stepping toward it
                o->currentPos =
                    add( o->currentPos,
                         o->currentSpeed );
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
            
            o->pendingActionAnimationProgress += 0.05 * frameRateFactor;
            
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
        }
    

    if( nextActionMessageToSend != NULL 
        && ourLiveObject->currentSpeed.x == 0
        && ourLiveObject->currentSpeed.y == 0 ) {
        
        // done moving on client end
        // can start showing pending action animation, even if 
        // end of motion not received from server yet

        if( !playerActionPending ) {
            playerActionPending = true;
            ourLiveObject->pendingAction = true;
            
            // start on first frame to force at least one cycle no
            // matter how fast the server responds
            ourLiveObject->pendingActionAnimationProgress = 
                0.05 * frameRateFactor;
            }
        
        
        if( ! ourLiveObject->inMotion && 
            ourLiveObject->pendingActionAnimationProgress > 0.25 ) {
            
            // move end acked by server AND action animation in progress

            // queued action waiting for our move to end
            sendToSocket( mServerSocket, 
                          (unsigned char*)nextActionMessageToSend, 
                          strlen( nextActionMessageToSend) );
            
            numServerBytesSent += strlen( nextActionMessageToSend );
            overheadServerBytesSent += 52;

            delete [] nextActionMessageToSend;
            nextActionMessageToSend = NULL;
            }
        }




    }


  
void LivingLifePage::makeActive( char inFresh ) {
    // unhold E key
    mEKeyDown = false;

    if( !inFresh ) {
        return;
        }
    
    
    
    }



static char isGridAdjacent( int inXA, int inYA, int inXB, int inYB ) {
    if( ( abs( inXA - inXB ) == 1 && inYA == inYB ) 
        ||
        ( abs( inYA - inYB ) == 1 && inXA == inXB ) ) {
        
        return true;
        }

    return false;
    }
        





void LivingLifePage::pointerMove( float inX, float inY ) {
    }






void LivingLifePage::pointerDown( float inX, float inY ) {
    
    if( mFirstServerMessagesReceived != 3 ) {
        return;
        }

    if( playerActionPending ) {
        // block further actions until update received to confirm last
        // action
        return;
        }
    

    LiveObject *ourLiveObject;

    for( int i=0; i<gameObjects.size(); i++ ) {
        
        LiveObject *o = gameObjects.getElement( i );
        
        if( o->id == ourID ) {
            ourLiveObject = o;
            break;
            }
        }
    
    int clickDestX = lrintf( ( inX ) / 32 );
    
    int clickDestY = lrintf( ( inY ) / 32 );
    

    if( clickDestX == ourLiveObject->xd && clickDestY == ourLiveObject->yd ) {
        // ignore clicks where we're already standing
        return;
        }
    
    
    // may change to empty adjacent spot to click
    int moveDestX = clickDestX;
    int moveDestY = clickDestY;
    
    char mustMove = false;


    int destID = 0;
        
    int mapX = clickDestX - mMapOffsetX + mMapD / 2;
    int mapY = clickDestY - mMapOffsetY + mMapD / 2;
    
    printf( "clickDestX,Y = %d, %d,  mapX,Y = %d, %d, curX,Y = %d, %d\n", 
            clickDestX, clickDestY, 
            mapX, mapY,
            ourLiveObject->xd, ourLiveObject->yd );
    if( mapY >= 0 && mapY < mMapD &&
        mapX >= 0 && mapX < mMapD ) {
        
        destID = mMap[ mapY * mMapD + mapX ];
        }
    
    printf( "DestID = %d\n", destID );
        

    if( nextActionMessageToSend != NULL ) {
        delete [] nextActionMessageToSend;
        nextActionMessageToSend = NULL;
        }
    

    char modClick = false;
    
    if( mEKeyDown || isLastMouseButtonRight() ) {
        modClick = true;
        }

    if( destID == 0 && ( !modClick || ourLiveObject->holdingID == 0 ) ) {
        // a move to an empty spot
        // can interrupt current move
        
        mustMove = true;
        }
    else if( modClick || destID != 0 ) {
        // use/drop modifier
        // OR pick up action
            
        
        char canExecute = false;
        
        // direct click on adjacent cells?
        if( isGridAdjacent( clickDestX, clickDestY,
                            ourLiveObject->xd, ourLiveObject->yd ) ) {
            
            canExecute = true;
            }
        else {
            // need to move to empty adjacent first, if it exists
            
            
            int nDX[4] = { -1, +1, 0, 0 };
            int nDY[4] = { 0, 0, -1, +1 };
            
            char foundEmpty = false;
            
            double closestDist = 9999999;
            

            for( int n=0; n<4; n++ ) {
                int x = mapX + nDX[n];
                int y = mapY + nDY[n];

                if( y >= 0 && y < mMapD &&
                    x >= 0 && x < mMapD ) {
                 
                    
                    if( mMap[ y * mMapD + x ] == 0 ) {
                        
                        int emptyX = clickDestX + nDX[n];
                        int emptyY = clickDestY + nDY[n];

                        doublePair emptyDest = { (double)emptyX, 
                                                 (double)emptyY };
                        
                        double emptyDist = 
                            distance( emptyDest, ourLiveObject->currentPos );
                        
                        if( emptyDist < closestDist ) {
                            
                            // check if there's a path there
                            int oldXD = ourLiveObject->xd;
                            int oldYD = ourLiveObject->yd;
                            
                            ourLiveObject->xd = emptyX;
                            ourLiveObject->yd = emptyY;
        
                            computePathToDest( ourLiveObject );
                            
                            if( ourLiveObject->pathToDest != NULL ) {
                                // can get there

                                moveDestX = emptyX;
                                moveDestY = emptyY;
                            
                                closestDist = emptyDist;
                                
                                foundEmpty = true;
                                }

                            ourLiveObject->xd = oldXD;
                            ourLiveObject->yd = oldYD;
                            }
                        }
                    
                    }
                }
            
            if( foundEmpty ) {
                canExecute = true;
                mustMove = true;
                }
            }
        
        if( canExecute ) {
            
            const char *action = "";
            
            char send = false;
            
                
            if( modClick && destID == 0 && ourLiveObject->holdingID != 0 ) {
                action = "DROP";
                send = true;
                }
            else if( modClick && destID != 0 ) {
                action = "USE";
                send = true;
                }
            else if( ! modClick && destID != 0 ) {
                action = "GRAB";
                send = true;
                }
            
            
            if( send ) {
                // queue this until after we are done moving, if we are
                nextActionMessageToSend = 
                    autoSprintf( "%s %d %d#", action,
                                 clickDestX, clickDestY );
                playerActionTargetX = clickDestX;
                playerActionTargetY = clickDestY;
                }
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
            

            if( ourLiveObject->xd == oldXD 
                &&
                ourLiveObject->yd == oldYD ) {
                
                // truncated path is where we're already standing
                return;
                }
            

            computePathToDest( ourLiveObject );
            
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
        char *message = autoSprintf( "MOVE %d %d#", moveDestX, moveDestY );
        sendToSocket( mServerSocket, (unsigned char*)message, 
                      strlen( message ) );
            
        numServerBytesSent += strlen( message );
        overheadServerBytesSent += 52;

        delete [] message;

        // start moving before we hear back from server



        
        

        doublePair endPos = { (double)moveDestX, (double)moveDestY };
            
        ourLiveObject->moveTotalTime = 
            distance( endPos, 
                      ourLiveObject->currentPos ) / 
            ourLiveObject->lastSpeed;

        ourLiveObject->moveEtaTime = game_getCurrentTime() +
            ourLiveObject->moveTotalTime;

            
        updateMoveSpeed( ourLiveObject );
        }    
    }




void LivingLifePage::pointerDrag( float inX, float inY ) {
    }

void LivingLifePage::pointerUp( float inX, float inY ) {
    }

void LivingLifePage::keyDown( unsigned char inASCII ) {

    switch( inASCII ) {
        case 'e':
        case 'E':
            mEKeyDown = true;
            break;
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

        
