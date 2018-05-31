int versionNumber = 60;



#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

//#define USE_MALLINFO

#ifdef USE_MALLINFO
#include <malloc.h>
#endif


#include "minorGems/graphics/Color.h"



// for testing
char fillAbstracts = false;




#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/random/CustomRandomSource.h"


// static seed
CustomRandomSource randSource( 34957197 );



// unused, but fulfill accountHmac extern
char *accountKey = NULL;
int serverSequenceNumber = 0;
int accountHmacVersionNumber = 0;



#include "minorGems/util/log/AppLog.h"



#include "minorGems/game/game.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/diffBundle/client/diffBundleClient.h"




#include "EditorImportPage.h"
#include "EditorSpriteTrimPage.h"
#include "EditorObjectPage.h"
#include "EditorTransitionPage.h"
#include "EditorAnimationPage.h"
#include "EditorCategoryPage.h"
#include "EditorScenePage.h"

#include "LoadingPage.h"

#include "spriteBank.h"
#include "objectBank.h"
#include "overlayBank.h"
#include "soundBank.h"

#include "SoundWidget.h"

#include "groundSprites.h"


#include "ageControl.h"

#include "minorGems/io/file/File.h"
#include "minorGems/system/Time.h"

int loadingFileHandle = -1;
int loadingFileStepCount = 0;
double loadingStartTime;




EditorImportPage *importPage;
EditorSpriteTrimPage *spriteTrimPage;
EditorObjectPage *objectPage;
EditorTransitionPage *transPage;
EditorAnimationPage *animPage;
EditorCategoryPage *categoryPage;
EditorScenePage *scenePage;

LoadingPage *loadingPage;

int loadingPhase = 0;


int loadingStepBatchSize = 1;
double loadingPhaseStartTime;


GamePage *currentGamePage = NULL;








// position of view in world
doublePair lastScreenViewCenter = {0, 0 };



// world with of one view
double viewWidth = 1024;

// fraction of viewWidth visible vertically (aspect ratio)
double viewHeightFraction;

int screenW, screenH;

char initDone = false;

float mouseSpeed;

int musicOff = false;
float musicLoudness;

int webRetrySeconds;


double frameRateFactor = 1;


char firstDrawFrameCalled = false;
char firstServerMessageReceived = false;


char upKey = 'w';
char leftKey = 'a';
char downKey = 's';
char rightKey = 'd';




char *serverAddress = NULL;
int serverPort;

int serverSocket = -1;



char doesOverrideGameImageSize() {
    return true;
    }



void getGameImageSize( int *outWidth, int *outHeight ) {
    *outWidth = 1024;
    *outHeight = 600;
    }


char shouldNativeScreenResolutionBeUsed() {
    return true;
    }

char isNonIntegerScalingAllowed() {
    return false;
    }



const char *getWindowTitle() {
    return "EDITOR - OneLife";
    }


const char *getAppName() {
    return "EditOneLife";
    }


int getAppVersion() {
    return versionNumber;
    }


const char *getLinuxAppName() {
    // no dir-name conflict here because we're using all caps for app name
    return "EditOneLifeApp";
    }



const char *getFontTGAFileName() {
    return "font_32_64.tga";
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
Font *smallFont;
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
    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );
    

    mainFont = new Font( getFontTGAFileName(), 6, 16, false, 16 );
    mainFont->setMinimumPositionPrecision( 1 );

    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    // rect window
    viewWidth = inWidth;
    
    
    setViewSize( viewWidth );
    }


void freeDrawString() {
    delete mainFont;
    }



void initFrameDrawer( int inWidth, int inHeight, int inTargetFrameRate,
                      const char *inCustomRecordedGameData,
                      char inPlayingBack ) {

    initAgeControl();

    /*
    for( int i=0; i<800; i++ ) {
        Image *spriteImage = readTGAFileBase( "sprites/94.tga" );
                                    
        if( spriteImage != NULL ) {
            SpriteHandle h =
                fillSprite( spriteImage, false );
            freeSprite( h );
            delete spriteImage;
            }
        }
    exit( 1 );
    */
    /*
    for( int i=0; i<800; i++ ) {
        RawRGBAImage *spriteImage = readTGAFileRawBase( "sprites/94.tga" );
                                    
        if( spriteImage != NULL ) {

            if( spriteImage->mNumChannels == 4 ) {
                
                SpriteHandle h =
                    fillSprite( spriteImage->mRGBABytes, spriteImage->mWidth,
                                spriteImage->mHeight );
                freeSprite( h );
                }
            delete spriteImage;
            }
        }
    exit( 1 );
    */

    
    toggleLinearMagFilter( true );
    toggleMipMapGeneration( true );
    toggleMipMapMinFilter( true );
    toggleTransparentCropping( true );

    gamePlayingBack = inPlayingBack;
    
    screenW = inWidth;
    screenH = inHeight;
    
    if( inTargetFrameRate != 60 ) {
        frameRateFactor = (double)60 / (double)inTargetFrameRate;
        }
    
    


    setViewCenterPosition( lastScreenViewCenter.x, lastScreenViewCenter.y );

    viewHeightFraction = inHeight / (double)inWidth;

    
    // square window for this game
    viewWidth = inWidth;
    
    
    setViewSize( viewWidth );


    
    

    

    setCursorVisible( true );
    grabInput( false );
    
    // world coordinates
    setMouseReportingMode( true );
    
    
    
    smallFont = new Font( getFontTGAFileName(), 3, 8, false, 8 );
    
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

    
    serverAddress = SettingsManager::getStringSetting( "serverAddress" );

    if( serverAddress == NULL ) {
        serverAddress = stringDuplicate( "127.0.0.1" );
        }

    serverPort = SettingsManager::getIntSetting( "serverPort", 5077 );



    setSoundLoudness( musicLoudness );
    setSoundPlaying( true );
    //setSoundSpriteRateRange( 0.95, 1.05 );
    setSoundSpriteVolumeRange( 0.60, 1.0 );
    
    initOverlayBankStart();
    

    importPage = new EditorImportPage;
    spriteTrimPage = new EditorSpriteTrimPage;
    objectPage = new EditorObjectPage;
    transPage = new EditorTransitionPage;
    animPage = new EditorAnimationPage;
    categoryPage = new EditorCategoryPage;
    scenePage = new EditorScenePage;
    loadingPage = new LoadingPage;
    
    loadingPage->setCurrentPhase( "OVERLAYS" );
    loadingPage->setCurrentProgress( 0 );

    currentGamePage = loadingPage;
    currentGamePage->base_makeActive( true );
    

    enableSpriteSearch( true );
    enableObjectSearch( true );

    initDone = true;


    }



void freeFrameDrawer() {
    delete smallFont;
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


    if( serverAddress != NULL ) {    
        delete [] serverAddress;
        serverAddress = NULL;
        }
    
    if( serverSocket != -1 ) {
        closeSocket( serverSocket );
        }


    delete importPage;
    delete spriteTrimPage;
    delete objectPage;
    delete transPage;
    delete animPage;
    delete categoryPage;
    delete scenePage;
    delete loadingPage;


    SoundWidget::clearClipboard();
    

    freeGroundSprites();


    freeTransBank();
    
    freeCategoryBank();

    freeObjectBank();

    freeSpriteBank();
    
    freeOverlayBank();

    freeAnimationBank();

    freeSoundBank();

    freeSoundUsagePrintBuffer();
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






void drawFrame( char inUpdate ) {    

    // test code for async file loading
    if( loadingFileHandle != -1 ) {
        char fileDone = checkAsyncFileReadDone( loadingFileHandle );
        loadingFileStepCount++;
        
        if( fileDone ) {
            int length = 0;
            unsigned char *data
                = getAsyncFileData( loadingFileHandle, &length );
            

            printf( "Done with file read (%d steps), %.2f sec\n", 
                    loadingFileStepCount,
                    Time::getCurrentTime() - loadingStartTime );
            
            if( data != NULL ) {
                delete [] data;
                }
            loadingFileHandle = -1;
            loadingFileStepCount = 0;
            }
        }
    
    

    if( !inUpdate ) {

        if( isQuittingBlocked() ) {
            // unsafe NOT to keep updating here, because pending network
            // requests can stall

            // keep stepping current page, but don't do any other processing
            // (and still block user events from reaching current page)
            if( currentGamePage != NULL ) {
                currentGamePage->base_step();
                }
            }

        drawFrameNoUpdate( false );
            
        drawPauseScreen();
        
        if( !wasPaused ) {
            if( currentGamePage != NULL ) {
                currentGamePage->base_makeNotActive();
                }

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
        if( currentGamePage != NULL ) {
            currentGamePage->base_makeActive( false );
            }

        // fade music in
        //if( ! musicOff ) {
        //    setMusicLoudness( 1.0 );
        //    }
        wasPaused = false;
        }



    // updates here
    
    stepSpriteBank();
    
    stepSoundBank();

    if( currentGamePage != NULL ) {
        currentGamePage->base_step();

        if( currentGamePage == loadingPage ) {
            
            switch( loadingPhase ) {
                case 0: {
                    float progress = initOverlayBankStep();
                    loadingPage->setCurrentProgress( progress );
                    
                    if( progress == 1.0 ) {
                        initOverlayBankFinish();
                        

                        int numReverbs = 
                            initSoundBankStart();
                        
                        if( numReverbs > 0 ) {
                            loadingPage->setCurrentPhase( 
                                "SOUNDS##(GENERATING REVERBS)" );
                            loadingPage->setCurrentProgress( 0 );
                        
                            
                            loadingStepBatchSize = numReverbs / 20;
                        
                            if( loadingStepBatchSize < 1 ) {
                                loadingStepBatchSize = 1;
                                }
                            
                            loadingPhase ++;
                            }
                        else {
                            // skip sound progress
                            initSoundBankFinish();
                            
                            // turn reverb off in editor so that we can
                            // hear raw sounds
                            disableReverb( true );

                            loadingPhaseStartTime = Time::getCurrentTime();

                            char rebuilding;
                            
                            int numSprites = 
                                initSpriteBankStart( &rebuilding );
                        
                            if( rebuilding ) {
                                loadingPage->setCurrentPhase( 
                                    "SPRITES##(REBUILDING CACHE)" );
                                }
                            else {
                                loadingPage->setCurrentPhase( "SPRITES" );
                                }
                            loadingPage->setCurrentProgress( 0 );
                            
                            
                            loadingStepBatchSize = numSprites / 20;
                            
                            if( loadingStepBatchSize < 1 ) {
                                loadingStepBatchSize = 1;
                                }
                        
                            loadingPhase += 2;
                            }
                        }
                    break;
                    }
                case 1: {
                    float progress = initSoundBankStep();
                    loadingPage->setCurrentProgress( progress );
                    
                    if( progress == 1.0 ) {
                        initSoundBankFinish();
                        
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        int numSprites = 
                            initSpriteBankStart( &rebuilding );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                "SPRITES##(REBUILDING CACHE)" );
                            }
                        else {
                            loadingPage->setCurrentPhase( "SPRITES" );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numSprites / 20;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }
                        
                        loadingPhase ++;
                        }
                    break;
                    }
                case 2: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initSpriteBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initSpriteBankFinish();
                        printf( "Finished loading Sprite bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        
                        loadingPhaseStartTime = Time::getCurrentTime();


                        char rebuilding;
                        
                        int numCats = 
                            initAnimationBankStart( &rebuilding );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                "ANIMATION##(REBUILDING CACHE)" );
                            }
                        else {
                            loadingPage->setCurrentPhase( "ANIMATION" );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numCats / 20;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 3: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initAnimationBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initAnimationBankFinish();
                        
                        printf( "Finished loading animation bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );

                        loadingPhaseStartTime = Time::getCurrentTime();
                        
                        char rebuilding;
                        
                        int numObjects = 
                            initObjectBankStart( &rebuilding, fillAbstracts,
                                                 fillAbstracts );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                "OBJECTS##(REBUILDING CACHE)" );
                            }
                        else {
                            loadingPage->setCurrentPhase( "OBJECTS" );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numObjects / 20;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }
                        
                        loadingPhase ++;
                        }
                    break;
                    }
                case 4: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initObjectBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initObjectBankFinish();
                        printf( "Finished loading object bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        int numCats = 
                            initCategoryBankStart( &rebuilding );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                "CATEGORIES##(REBUILDING CACHE)" );
                            }
                        else {
                            loadingPage->setCurrentPhase( "CATEGORIES" );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numCats / 20;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 5: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initCategoryBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initCategoryBankFinish();
                        printf( "Finished loading category bank in %f sec\n",
                                Time::getCurrentTime() - 
                                loadingPhaseStartTime );
                        
                        loadingPhaseStartTime = Time::getCurrentTime();

                        char rebuilding;
                        
                        int numTrans = 
                            initTransBankStart( &rebuilding,
                                                fillAbstracts,
                                                fillAbstracts,
                                                fillAbstracts,
                                                fillAbstracts );
                        
                        if( rebuilding ) {
                            loadingPage->setCurrentPhase( 
                                "TRANSITIONS##(REBUILDING CACHE)" );
                            }
                        else {
                            loadingPage->setCurrentPhase( "TRANSITIONS" );
                            }
                        loadingPage->setCurrentProgress( 0 );
                        

                        loadingStepBatchSize = numTrans / 20;
                        
                        if( loadingStepBatchSize < 1 ) {
                            loadingStepBatchSize = 1;
                            }

                        loadingPhase ++;
                        }
                    break;
                    }
                case 6: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initTransBankStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initTransBankFinish();
                        
                        loadingPage->setCurrentPhase( 
                            translate( "groundTextures" ) );

                        loadingPage->setCurrentProgress( 0 );
                        
                        initGroundSpritesStart();

                        loadingStepBatchSize = 1;

                        loadingPhase ++;
                        }
                    break;
                    }
                case 7: {
                    float progress;
                    for( int i=0; i<loadingStepBatchSize; i++ ) {    
                        progress = initGroundSpritesStep();
                        loadingPage->setCurrentProgress( progress );
                        }
                    
                    if( progress == 1.0 ) {
                        initGroundSpritesFinish();
                        
                        loadingPhase ++;
                        }
                    break;
                    }
                default:
                    //printOrphanedSoundReport();
                    currentGamePage = importPage;
                    loadingComplete();
                    currentGamePage->base_makeActive( true );
                }
            }
        if( currentGamePage == importPage ) {
            if( importPage->checkSignal( "objectEditor" ) ) {
                currentGamePage = objectPage;
                currentGamePage->base_makeActive( true );
                }
            else if( importPage->checkSignal( "spriteTrimEditor" ) ) {
                currentGamePage = spriteTrimPage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == spriteTrimPage ) {
            if( spriteTrimPage->checkSignal( "importEditor" ) ) {
                currentGamePage = importPage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == objectPage ) {
            if( objectPage->checkSignal( "importEditor" ) ) {
                currentGamePage = importPage;
                currentGamePage->base_makeActive( true );
                }
            else if( objectPage->checkSignal( "transEditor" ) ) {
                currentGamePage = transPage;
                currentGamePage->base_makeActive( true );
                }
            else if( objectPage->checkSignal( "animEditor" ) ) {
                currentGamePage = animPage;
                animPage->clearClothing();
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == transPage ) {
            if( transPage->checkSignal( "objectEditor" ) ) {
                currentGamePage = objectPage;
                currentGamePage->base_makeActive( true );
                }
            else if( transPage->checkSignal( "categoryEditor" ) ) {
                currentGamePage = categoryPage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == animPage ) {
            if( animPage->checkSignal( "objectEditor" ) ) {
                currentGamePage = objectPage;
                currentGamePage->base_makeActive( true );
                }
            else if( animPage->checkSignal( "sceneEditor" ) ) {
                currentGamePage = scenePage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == categoryPage ) {
            if( categoryPage->checkSignal( "transEditor" ) ) {
                currentGamePage = transPage;
                currentGamePage->base_makeActive( true );
                }
            }
        else if( currentGamePage == scenePage ) {
            if( scenePage->checkSignal( "animEditor" ) ) {
                currentGamePage = animPage;
                currentGamePage->base_makeActive( true );
                }
            }
        }
    
    

    // now draw stuff AFTER all updates
    drawFrameNoUpdate( true );

    /*
    double recentFPS = getRecentFrameRate();

    if( recentFPS < 0.90 * ( 60.0 / frameRateFactor ) 
        ||
        recentFPS > 1.10 * ( 60.0 / frameRateFactor ) ) {
        
        // slowdown or speedup of more than 10% off target

        printf( "Seeing true framerate of %f\n", recentFPS );

        // if we're seeing a speedup, this might be correcting
        // for a previous slowdown that we already adjusted for
        frameRateFactor = 60.0 / recentFPS;

        printf( "Adjusting framerate factor to %f\n", frameRateFactor );
        }
    */

    // draw tail end of pause screen, if it is still visible
    if( pauseScreenFade > 0 ) {
        drawPauseScreen();
        }
    }



void drawFrameNoUpdate( char inUpdate ) {
    if( currentGamePage != NULL ) {
        currentGamePage->base_draw( lastScreenViewCenter, viewWidth );
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
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerMove( inX, inY );
        }
    }



void pointerDown( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDown( inX, inY );
        }
    }



void pointerDrag( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerDrag( inX, inY );
        }
    }



void pointerUp( float inX, float inY ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_pointerUp( inX, inY );
        }
    }







void keyDown( unsigned char inASCII ) {
    
    // taking screen shot is ALWAYS possible
    if( inASCII == '=' ) {    
        saveScreenShot( "screen" );
        }
    
    /*
    if( ! TextField::isAnyFocused() ) {    
        if( inASCII == 'N' ) {
            toggleMipMapMinFilter( true );
            }
        if( inASCII == 'n' ) {
            toggleMipMapMinFilter( false );
            }
        }
    */

    /*
    if( inASCII == 'F' ) {
        if( loadingFileHandle == -1 ) {
            printf( "Starting file read\n" );
            loadingStartTime = Time::getCurrentTime();
            loadingFileHandle = startAsyncFileRead( "20MegFile" );
            }
        }
    if( inASCII == 'f' ) {
        File testFile( NULL, "20MegFile" );
        
        printf( "Starting file read\n" );
        double startTime = Time::getCurrentTime();
        int length = 0;
        unsigned char *data = testFile.readFileContents( &length );
        printf( "Done with file read, %.2f sec\n", 
                Time::getCurrentTime() - startTime );
        if( data != NULL ) {
            delete [] data;
            }
        }
    */
    

    
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
    
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_keyDown( inASCII );
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

        if( currentGamePage != NULL ) {
            currentGamePage->base_keyUp( inASCII );
            }
        }

    }







void specialKeyDown( int inKey ) {
    if( isPaused() ) {
        return;
        }

    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyDown( inKey );
        }
	}



void specialKeyUp( int inKey ) {
    if( isPaused() ) {
        return;
        }
    
    if( currentGamePage != NULL ) {
        currentGamePage->base_specialKeyUp( inKey );
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






void hintBufferSize( int inSize ) {
    }

void freeHintedBuffers() {
    }



// called by platform to get more samples
void getSoundSamples( Uint8 *inBuffer, int inLengthToFillInBytes ) {
    // for now, do nothing (no sound)
    }




