int versionNumber = 26;

// retain an older version number here if server is compatible
// with older client versions.
// Change this number (and number on server) if server has changed
// in a way that breaks old clients.
int accountHmacVersionNumber = 25;



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif


#include "minorGems/graphics/Color.h"





#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/CustomRandomSource.h"

#include "minorGems/system/Time.h"


// static seed
CustomRandomSource randSource( 34957197 );



#include "minorGems/util/log/AppLog.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"











// position of view in world
doublePair lastScreenViewCenter = {0, 0 };



// world with of one view
double viewWidth = 666;

// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;

int screenW, screenH;

char initDone = false;

float mouseSpeed;

int musicOff;
float musicLoudness;

int webRetrySeconds;


double frameRateFactor = 1;
int baseFramesPerSecond = 60;

char firstDrawFrameCalled = false;
int firstServerMessagesReceived = 0;


char upKey = 'w';
char leftKey = 'a';
char downKey = 's';
char rightKey = 'd';



#define GRID_D 20
int gridD = GRID_D;
double heatGrid[ GRID_D * GRID_D ];
double tempHeatGrid[ GRID_D * GRID_D ];

double rGrid[ GRID_D * GRID_D ];

double heatOutputGrid[ GRID_D * GRID_D ];



char doesOverrideGameImageSize() {
    return true;
    }



void getGameImageSize( int *outWidth, int *outHeight ) {
    *outWidth = 666;
    *outHeight = 666;
    }



const char *getWindowTitle() {
    return "Thermal Sim";
    }


const char *getAppName() {
    return "thermalSim";
    }

const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "thermalSimApp";
    }



const char *getFontTGAFileName() {
    return "font_32_32.tga";
    }


char isDemoMode() {
    return false;
    }


const char *getDemoCodeSharedSecret() {
    return "fundamental_right";
    }


const char *getDemoCodeServerURL() {
    return "http://FIXME/demoServer/server.php";
    }



char gamePlayingBack = false;


Font *mainFont;
Font *mainFontFixed;
Font *numbersFontFixed;

char *shutdownMessage = NULL;








static char wasPaused = false;
static float pauseScreenFade = 0;

static char *currentUserTypedMessage = NULL;



// for delete key repeat during message typing
static int holdDeleteKeySteps = -1;
static int stepsBetweenDeleteRepeat;





#define SETTINGS_HASH_SALT "another_loss"


static const char *customDataFormatWriteString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";

static const char *customDataFormatReadString = 
    "version%d_mouseSpeed%f_musicOff%d_musicLoudness%f"
    "_webRetrySeconds%d";


char *getCustomRecordedGameData() {    
    
    float mouseSpeedSetting = 
        SettingsManager::getFloatSetting( "mouseSpeed", 1.0f );
    int musicOffSetting = 
        SettingsManager::getIntSetting( "musicOff", 0 );
    float musicLoudnessSetting = 
        SettingsManager::getFloatSetting( "musicLoudness", 1.0f );
    int webRetrySecondsSetting = 
        SettingsManager::getIntSetting( "webRetrySeconds", 10 );
    

    char * result = autoSprintf(
        customDataFormatWriteString,
        versionNumber, mouseSpeedSetting, musicOffSetting, 
        musicLoudnessSetting,
        webRetrySecondsSetting );
    

    return result;
    }



char showMouseDuringPlayback() {
    // since we rely on the system mouse pointer during the game (and don't
    // draw our own pointer), we need to see the recorded pointer position
    // to make sense of game playback
    return true;
    }



char *getHashSalt() {
    return stringDuplicate( SETTINGS_HASH_SALT );
    }




void initDrawString( int inWidth, int inHeight ) {
    mainFont = new Font( getFontTGAFileName(), 6, 16, false, 8 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    // square window for this game
    viewWidth = 666 * 1.0 / viewHeightFraction;
    
    
    setViewSize( viewWidth );
    }


void freeDrawString() {
    delete mainFont;
    }



void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
                      const char *inCustomRecordedGameData,
                      char inPlayingBack ) {
    
    gamePlayingBack = inPlayingBack;
    
    screenW = inWidth;
    screenH = inHeight;
    
    if( inTargetFrameRate != baseFramesPerSecond ) {
        frameRateFactor = 
            (double)baseFramesPerSecond / (double)inTargetFrameRate;
        }
    
    


    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    
    // square window for this game
    viewWidth = 666 * 1.0 / viewHeightFraction;
    
    
    setViewSize( viewWidth );


    
    

    

    setCursorVisible( true );
    grabInput( false );
    
    // world coordinates
    setMouseReportingMode( true );
    
    
    
    
    mainFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16 );
    numbersFontFixed = new Font( getFontTGAFileName(), 6, 16, true, 16, 16 );
    
    mainFontFixed->setMinimumPositionPrecision( 1 );
    numbersFontFixed->setMinimumPositionPrecision( 1 );
    

    float mouseSpeedSetting = 1.0f;
    
    int musicOffSetting = 0;
    float musicLoudnessSetting = 1.0f;
    int webRetrySecondsSetting = 10;

    
    int readVersionNumber;
    
    int numRead = sscanf( inCustomRecordedGameData, 
                          customDataFormatReadString, 
                          &readVersionNumber,
                          &mouseSpeedSetting, 
                          &musicOffSetting,
                          &musicLoudnessSetting,
                          &webRetrySecondsSetting );
    if( numRead != 5 ) {
        // no recorded game?
        }
    else {

        if( readVersionNumber != versionNumber ) {
            AppLog::printOutNextMessage();
            AppLog::warningF( 
                "WARNING:  version number in playback file is %d "
                "but game version is %d...",
                readVersionNumber, versionNumber );
            }
        }
    
    

    
    double mouseParam = 0.000976562;

    mouseParam *= mouseSpeedSetting;

    mouseSpeed = mouseParam * inWidth / viewWidth;

    musicOff = musicOffSetting;
    musicLoudness = musicLoudnessSetting;
    webRetrySeconds = webRetrySecondsSetting;

    



    setSoundLoudness( musicLoudness );
    setSoundPlaying( false );

    initDone = true;


    for( int i=0; i<gridD*gridD; i++ ) {
        heatGrid[i] = 0;
        rGrid[i] = 0;
        heatOutputGrid[i] = 0;
        }
    
    }



void freeFrameDrawer() {

    
    delete mainFontFixed;
    delete numbersFontFixed;
    
    if( currentUserTypedMessage != NULL ) {
        delete [] currentUserTypedMessage;
        currentUserTypedMessage = NULL;
        }

    

    if( shutdownMessage != NULL ) {
        delete [] shutdownMessage;
        shutdownMessage = NULL;
        }
    }





    


// draw code separated from updates
// some updates are still embedded in draw code, so pass a switch to 
// turn them off
static void drawFrameNoUpdate( char inUpdate );




static void drawPauseScreen() {

    double viewHeight = viewHeightFraction * viewWidth;

    setDrawColor( 1, 1, 1, 0.5 * pauseScreenFade );
        
    drawSquare( lastScreenViewCenter, 1.05 * ( viewHeight / 3 ) );
        

    setDrawColor( 0.2, 0.2, 0.2, 0.85 * pauseScreenFade  );
        
    drawSquare( lastScreenViewCenter, viewHeight / 3 );
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    doublePair messagePos = lastScreenViewCenter;

    messagePos.y += 4.5  * (viewHeight / 15);

    mainFont->drawString( translate( "pauseMessage1" ), 
                           messagePos, alignCenter );
        
    messagePos.y -= 1.25 * (viewHeight / 15);
    mainFont->drawString( translate( "pauseMessage2" ), 
                           messagePos, alignCenter );

    if( currentUserTypedMessage != NULL ) {
            
        messagePos.y -= 1.25 * (viewHeight / 15);
            
        double maxWidth = 0.95 * ( viewHeight / 1.5 );
            
        int maxLines = 9;

        SimpleVector<char *> *tokens = 
            tokenizeString( currentUserTypedMessage );


        // collect all lines before drawing them
        SimpleVector<char *> lines;
        
            
        while( tokens->size() > 0 ) {

            // build up a a line

            // always take at least first token, even if it is too long
            char *currentLineString = 
                stringDuplicate( *( tokens->getElement( 0 ) ) );
                
            delete [] *( tokens->getElement( 0 ) );
            tokens->deleteElement( 0 );
            
            

            

            
            char nextTokenIsFileSeparator = false;
                
            char *nextLongerString = NULL;
                
            if( tokens->size() > 0 ) {

                char *nextToken = *( tokens->getElement( 0 ) );
                
                if( nextToken[0] == 28 ) {
                    nextTokenIsFileSeparator = true;
                    }
                else {
                    nextLongerString =
                        autoSprintf( "%s %s ",
                                     currentLineString,
                                     *( tokens->getElement( 0 ) ) );
                    }
                
                }
                
            while( !nextTokenIsFileSeparator 
                   &&
                   nextLongerString != NULL 
                   && 
                   mainFont->measureString( nextLongerString ) 
                   < maxWidth 
                   &&
                   tokens->size() > 0 ) {
                    
                delete [] currentLineString;
                    
                currentLineString = nextLongerString;
                    
                nextLongerString = NULL;
                    
                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                    
                if( tokens->size() > 0 ) {
                    
                    char *nextToken = *( tokens->getElement( 0 ) );
                
                    if( nextToken[0] == 28 ) {
                        nextTokenIsFileSeparator = true;
                        }
                    else {
                        nextLongerString =
                            autoSprintf( "%s%s ",
                                         currentLineString,
                                         *( tokens->getElement( 0 ) ) );
                        }
                    }
                }
                
            if( nextLongerString != NULL ) {    
                delete [] nextLongerString;
                }
                
            while( mainFont->measureString( currentLineString ) > 
                   maxWidth ) {
                    
                // single token that is too long by itself
                // simply trim it and discard part of it 
                // (user typing nonsense anyway)
                    
                currentLineString[ strlen( currentLineString ) - 1 ] =
                    '\0';
                }
                
            if( currentLineString[ strlen( currentLineString ) - 1 ] 
                == ' ' ) {
                // trim last bit of whitespace
                currentLineString[ strlen( currentLineString ) - 1 ] = 
                    '\0';
                }

                
            lines.push_back( currentLineString );

            
            if( nextTokenIsFileSeparator ) {
                // file separator

                // put a paragraph separator in
                lines.push_back( stringDuplicate( "---" ) );

                // token consumed
                delete [] *( tokens->getElement( 0 ) );
                tokens->deleteElement( 0 );
                }
            }   


        // all tokens deleted above
        delete tokens;


        double messageLineSpacing = 0.625 * (viewHeight / 15);
        
        int numLinesToSkip = lines.size() - maxLines;

        if( numLinesToSkip < 0 ) {
            numLinesToSkip = 0;
            }
        
        
        for( int i=0; i<numLinesToSkip-1; i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            delete [] currentLineString;
            }
        
        int lastSkipLine = numLinesToSkip - 1;

        if( lastSkipLine >= 0 ) {
            
            char *currentLineString = *( lines.getElement( lastSkipLine ) );

            // draw above and faded out somewhat

            doublePair lastSkipLinePos = messagePos;
            
            lastSkipLinePos.y += messageLineSpacing;

            setDrawColor( 1, 1, 0.5, 0.125 * pauseScreenFade );

            mainFont->drawString( currentLineString, 
                                   lastSkipLinePos, alignCenter );

            
            delete [] currentLineString;
            }
        

        setDrawColor( 1, 1, 0.5, pauseScreenFade );

        for( int i=numLinesToSkip; i<lines.size(); i++ ) {
            char *currentLineString = *( lines.getElement( i ) );
            
            if( false && lastSkipLine >= 0 ) {
            
                if( i == numLinesToSkip ) {
                    // next to last
                    setDrawColor( 1, 1, 0.5, 0.25 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 1 ) {
                    // next after that
                    setDrawColor( 1, 1, 0.5, 0.5 * pauseScreenFade );
                    }
                else if( i == numLinesToSkip + 2 ) {
                    // rest are full fade
                    setDrawColor( 1, 1, 0.5, pauseScreenFade );
                    }
                }
            
            mainFont->drawString( currentLineString, 
                                   messagePos, alignCenter );

            delete [] currentLineString;
                
            messagePos.y -= messageLineSpacing;
            }
        }
        
        

    setDrawColor( 1, 1, 1, pauseScreenFade );

    messagePos = lastScreenViewCenter;

    messagePos.y -= 3.75 * ( viewHeight / 15 );
    //mainFont->drawString( translate( "pauseMessage3" ), 
    //                      messagePos, alignCenter );

    messagePos.y -= 0.625 * (viewHeight / 15);

    const char* quitMessageKey = "pauseMessage3";
    
    if( isQuittingBlocked() ) {
        quitMessageKey = "pauseMessage3b";
        }

    mainFont->drawString( translate( quitMessageKey ), 
                          messagePos, alignCenter );

    }



void deleteCharFromUserTypedMessage() {
    if( currentUserTypedMessage != NULL ) {
                    
        int length = strlen( currentUserTypedMessage );
        
        char fileSeparatorDeleted = false;
        if( length > 2 ) {
            if( currentUserTypedMessage[ length - 2 ] == 28 ) {
                // file separator with spaces around it
                // delete whole thing with one keypress
                currentUserTypedMessage[ length - 3 ] = '\0';
                fileSeparatorDeleted = true;
                }
            }
        if( !fileSeparatorDeleted && length > 0 ) {
            currentUserTypedMessage[ length - 1 ] = '\0';
            }
        }
    }




int frameCount = 0;
int mouseOverSqaure = -1;


void drawFrame( char inUpdate ) {    


    if( !inUpdate ) {

        if( isQuittingBlocked() ) {
            // unsafe NOT to keep updating here, because pending network
            // requests can stall

            // keep stepping current page, but don't do any other processing
            // (and still block user events from reaching current page)
            //if( currentGamePage != NULL ) {
            //    currentGamePage->base_step();
            //    }
            }

        drawFrameNoUpdate( false );
            
        drawPauseScreen();
        
        if( !wasPaused ) {
            //if( currentGamePage != NULL ) {
            //    currentGamePage->base_makeNotActive();
            //    }

            // fade out music during pause
            //setMusicLoudness( 0 );
            }
        wasPaused = true;

        // handle delete key repeat
        if( holdDeleteKeySteps > -1 ) {
            holdDeleteKeySteps ++;
            
            if( holdDeleteKeySteps > stepsBetweenDeleteRepeat ) {        
                // delete repeat

                // platform layer doesn't receive event for key held down
                // tell it we are still active so that it doesn't
                // reduce the framerate during long, held deletes
                wakeUpPauseFrameRate();
                


                // subtract from messsage
                deleteCharFromUserTypedMessage();
                
                            

                // shorter delay for subsequent repeats
                stepsBetweenDeleteRepeat = (int)( 2/ frameRateFactor );
                holdDeleteKeySteps = 0;
                }
            }

        // fade in pause screen
        if( pauseScreenFade < 1 ) {
            pauseScreenFade += ( 1.0 / 30 ) * frameRateFactor;
        
            if( pauseScreenFade > 1 ) {
                pauseScreenFade = 1;
                }
            }
        

        return;
        }


    // not paused


    // fade pause screen out
    if( pauseScreenFade > 0 ) {
        pauseScreenFade -= ( 1.0 / 30 ) * frameRateFactor;
        
        if( pauseScreenFade < 0 ) {
            pauseScreenFade = 0;

            if( currentUserTypedMessage != NULL ) {

                // make sure it doesn't already end with a file separator
                // (never insert two in a row, even when player closes
                //  pause screen without typing anything)
                int lengthCurrent = strlen( currentUserTypedMessage );

                if( lengthCurrent < 2 ||
                    currentUserTypedMessage[ lengthCurrent - 2 ] != 28 ) {
                         
                        
                    // insert at file separator (ascii 28)
                    
                    char *oldMessage = currentUserTypedMessage;
                    
                    currentUserTypedMessage = autoSprintf( "%s %c ", 
                                                           oldMessage,
                                                           28 );
                    delete [] oldMessage;
                    }
                }
            }
        }    
    
    

    if( !firstDrawFrameCalled ) {
        
        // do final init step... stuff that shouldn't be done until
        // we have control of screen
        
        char *moveKeyMapping = 
            SettingsManager::getStringSetting( "upLeftDownRightKeys" );
    
        if( moveKeyMapping != NULL ) {
            char *temp = stringToLowerCase( moveKeyMapping );
            delete [] moveKeyMapping;
            moveKeyMapping = temp;
        
            if( strlen( moveKeyMapping ) == 4 &&
                strcmp( moveKeyMapping, "wasd" ) != 0 ) {
                // different assignment

                upKey = moveKeyMapping[0];
                leftKey = moveKeyMapping[1];
                downKey = moveKeyMapping[2];
                rightKey = moveKeyMapping[3];
                }
            delete [] moveKeyMapping;
            }




        firstDrawFrameCalled = true;
        }

    if( wasPaused ) {
        //if( currentGamePage != NULL ) {
        //    currentGamePage->base_makeActive( false );
        //    }

        // fade music in
        //if( ! musicOff ) {
        //    setMusicLoudness( 1.0 );
        //    }
        wasPaused = false;
        }



    frameCount ++;
    
    if( frameCount % 2 == 0 ) {
        
        // updates here
    
        memcpy( tempHeatGrid, heatGrid, gridD * gridD * sizeof( double ) );
        
        
        int numNeighbors = 8;
        int ndx[8] = { -1, 0, 1, 1,  1,  0, -1, -1 };
        int ndy[8] = {  1, 1, 1, 0, -1, -1, -1,  0 };
        
        // diag weights found here:
        // http://demonstrations.wolfram.com/
        //        ACellularAutomatonBasedHeatEquation/
        double nWeights[8] = { 1, 4, 1, 4, 1, 4, 1, 4 };
        double totalNWeight = 20;
        
        /*
        int numNeighbors = 4;
        int ndx[4] = { 0, 1,  0, -1 };
        int ndy[4] = { 1, 0, -1,  0 };

        double nWeights[4] = { 1, 1, 1, 1 };
        double totalNWeight = 4;
        */

        for( int y=1; y<gridD-1; y++ ) {
            for( int x=1; x<gridD-1; x++ ) {
                int i = y * gridD + x;
            
                double heatDelta = 0;
                
                double centerLeak = 1 - rGrid[i];

                double centerOldHeat = tempHeatGrid[i];

                for( int n=0; n<numNeighbors; n++ ) {
                
                    int nx = x + ndx[n];
                    int ny = y + ndy[n];
                
                    int ni = ny * gridD + nx;
                
                    double nLeak = 1 - rGrid[ ni ];
                    
                    heatDelta += nWeights[n] * centerLeak * nLeak *
                        ( tempHeatGrid[ ni ] - centerOldHeat );
                    }
                
                heatGrid[i] = tempHeatGrid[i] + heatDelta / totalNWeight;
                
                heatGrid[i] += heatOutputGrid[ i ];
                }
            }
        }
    
        


    // now draw stuff AFTER all updates
    drawFrameNoUpdate( true );



    // draw tail end of pause screen, if it is still visible
    if( pauseScreenFade > 0 ) {
        drawPauseScreen();
        }
    }




void drawFrameNoUpdate( char inUpdate ) {
    for( int y=0; y<gridD; y++ ) {
        for( int x=0; x<gridD; x++ ) {
            int i = y * gridD + x;
            
            doublePair pos = { 32.0 * ( x - gridD/2 ), 32.0 * ( y - gridD/2 ) };
            
            pos.x += lastScreenViewCenter.x;
            pos.y += lastScreenViewCenter.y;
            
            
            setDrawColor( 1, 1, 1, .5 );
            drawSquare( pos, 16 );
            
            float red = ( heatGrid[i] - 60 ) / (60);
            if( red < 0 ) {
                red = 0;
                }

            float green = ( heatGrid[i] - 120 ) / (255 - 120);
            
            if( green < 0 ) {
                green = 0;
                }

            float blue = 0;

            if( heatGrid[i] < 60 ) {
                blue = heatGrid[i] / 60;
                }
            else if( heatGrid[i] < 90 ) {
                blue = 1;
                }
            else if( heatGrid[i] < 120 ) {
                blue = (120 - heatGrid[i] ) / 30;
                }
            
            
            

            setDrawColor( red, green, blue, 1 );
            drawSquare( pos, 15 );

            if( heatOutputGrid[i] > 0 ) {
                setDrawColor( 0.5, 0.5, 0.5, 1 );
                
                char *string = autoSprintf( "%.1f", heatOutputGrid[i] );
                
                mainFont->drawString( string, pos, alignCenter );
                
                delete [] string;
                }
            
            if( rGrid[i] > 0 ) {
                setDrawColor( 0, 1, 0, 1 );
                
                char *string = autoSprintf( "%.1f", rGrid[i] );
                
                mainFont->drawString( string, pos, alignCenter );
                
                delete [] string;
                }
            
            }
        }
    
    if( mouseOverSqaure != -1 ) {
        doublePair pos = { 0, 320 };
        setDrawColor( 1, 1, 1, 1 );
                
        char *string = autoSprintf( "Heat = %.1f", 
                                    heatGrid[ mouseOverSqaure ] );
                
        mainFont->drawString( string, pos, alignCenter );
        }
    
        
    }




// store mouse data for use as unguessable randomizing data
// for key generation, etc.
#define MOUSE_DATA_BUFFER_SIZE 20
int mouseDataBufferSize = MOUSE_DATA_BUFFER_SIZE;
int nextMouseDataIndex = 0;
// ensure that stationary mouse data (same value over and over)
// doesn't overwrite data from actual motion
float lastBufferedMouseValue = 0;
float mouseDataBuffer[ MOUSE_DATA_BUFFER_SIZE ];



void pointerMove( float inX, float inY ) {

    // save all mouse movement data for key generation
    float bufferValue = inX + inY;
    // ignore mouse positions that are the same as the last one
    // only save data when mouse actually moving
    if( bufferValue != lastBufferedMouseValue ) {
        
        mouseDataBuffer[ nextMouseDataIndex ] = bufferValue;
        lastBufferedMouseValue = bufferValue;
        
        nextMouseDataIndex ++;
        if( nextMouseDataIndex >= mouseDataBufferSize ) {
            nextMouseDataIndex = 0;
            }
        }
    

    if( isPaused() ) {
        return;
        }

    int gridX = (int)( lrint( inX / 32 ) + gridD/2 );
    int gridY = (int)( lrint( inY / 32 ) + gridD/2 );
    

    if( gridX >= 0 && gridX < gridD &&
        gridY >= 0 && gridY < gridD ) {
        
        mouseOverSqaure = gridY * gridD + gridX;
        }
    else {
        mouseOverSqaure = -1;
        }
    }





void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    int gridX = (int)( lrint( inX / 32 ) + gridD/2 );
    int gridY = (int)( lrint( inY / 32 ) + gridD/2 );
    

    if( gridX >= 0 && gridX < gridD &&
        gridY >= 0 && gridY < gridD ) {
        
        
        int i = gridY * gridD + gridX;
        
        if( isCommandKeyDown() ) {
            // heat sources
            
            if( isLastMouseButtonRight() ) {
                heatOutputGrid[i] -= 1;
                if( heatOutputGrid[i] < 0 ) {
                    heatOutputGrid[i] = 0;
                    }
                }
            else {
                heatOutputGrid[i] += 1;
                }
            }
        else {
            // insulation
            if( isLastMouseButtonRight() ) {
                rGrid[i] -= 0.1;
                if( rGrid[i] < 0 ) {
                    rGrid[i] = 0;
                    }
                }
            else {
                rGrid[i] += 0.1;
                if( rGrid[i] > 1 ) {
                    rGrid[i] = 1;
                    }
                }
            }
        
        }
    
    }



void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    }



void pointerUp( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }
    }







void keyDown( unsigned char inASCII ) {

    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }
    

    
    if( isPaused() ) {
        // block general keyboard control during pause


        switch( inASCII ) {
            case 13:  // enter
                // unpause
                pauseGame();
                break;
            }
        
        
        if( inASCII == 127 || inASCII == 8 ) {
            // subtract from it

            deleteCharFromUserTypedMessage();

            holdDeleteKeySteps = 0;
            // start with long delay until first repeat
            stepsBetweenDeleteRepeat = (int)( 30 / frameRateFactor );
            }
        else if( inASCII >= 32 ) {
            // add to it
            if( currentUserTypedMessage != NULL ) {
                
                char *oldMessage = currentUserTypedMessage;

                currentUserTypedMessage = autoSprintf( "%s%c", 
                                                       oldMessage, inASCII );
                delete [] oldMessage;
                }
            else {
                currentUserTypedMessage = autoSprintf( "%c", inASCII );
                }
            }
        
        return;
        }
    


    
    switch( inASCII ) {
        case 'm':
        case 'M': {
#ifdef USE_MALLINFO
            struct mallinfo meminfo = mallinfo();
            printf( "Mem alloc: %d\n",
                    meminfo.uordblks / 1024 );
#endif
            }
            break;
        }
    }



void keyUp( unsigned char inASCII ) {
    if( inASCII == 127 || inASCII == 8 ) {
        // delete no longer held
        // even if pause screen no longer up, pay attention to this
        holdDeleteKeySteps = -1;
        }

    if( ! isPaused() ) {
        }

    }







void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
	}



void specialKeyUp( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
	} 




char getUsesSound() {
    
    return ! musicOff;
    }









void drawString( const char *inString, char inForceCenter ) {
    
    setDrawColor( 1, 1, 1, 0.75 );

    doublePair messagePos = lastScreenViewCenter;

    TextAlignment align = alignCenter;
    
    if( initDone && !inForceCenter ) {
        // transparent message
        setDrawColor( 1, 1, 1, 0.75 );

        // stick messages in corner
        messagePos.x -= viewWidth / 2;
        
        messagePos.x +=  20;
    

    
        messagePos.y += (viewWidth * viewHeightFraction) /  2;
    
        messagePos.y -= 32;

        align = alignLeft;
        }
    else {
        // fully opaque message
        setDrawColor( 1, 1, 1, 1 );

        // leave centered
        }
    

    int numLines;
    
    char **lines = split( inString, "\n", &numLines );
    
    for( int i=0; i<numLines; i++ ) {
        

        mainFont->drawString( lines[i], messagePos, align );
        messagePos.y -= 32;
        
        delete [] lines[i];
        }
    delete [] lines;
    }



char shouldNativeScreenResolutionBeUsed() {
    return true;
    }


char isNonIntegerScalingAllowed() {
    return true;
    }


void hintBufferSize( int inSize ) {
    }


// called by platform to get more samples
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    // for now, do nothing (no sound)
    }




