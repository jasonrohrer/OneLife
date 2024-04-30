#include "SettingsPage.h"


#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"
#include "minorGems/system/Time.h"

#include "musicPlayer.h"
#include "soundBank.h"
#include "objectBank.h"
#include "buttonStyle.h"


extern Font *mainFont;

extern float musicLoudness;

extern int targetFramesPerSecond;



SettingsPage::SettingsPage()
        : mBackButton( mainFont, -542, -280, translate( "backButton" ) ),
          mEditAccountButton( mainFont, -463, 129, translate( "editAccount" ) ),
          mRestartButton( mainFont, 128, 128, translate( "restartButton" ) ),
          mRedetectButton( mainFont, 173, 249, translate( "redetectButton" ) ),
          mVsyncBox( 0, 208, 4 ),
          mFullscreenBox( 0, 128, 4 ),
          mBorderlessBox( 0, 168, 4 ),
          mTargetFrameRateField( mainFont, 22, 268, 3, false, 
                                 "", // no label
                                 "0123456789",
                                 NULL ),
          mMusicLoudnessSlider( mainFont, 0, 40, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -48, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ),
          mUseCustomServerBox( -168, -148, 4 ),
          mCustomServerAddressField( mainFont, 306, -150, 14, false, 
                                     translate( "address" ),
                                     NULL,
                                     // forbid spaces
                                     " " ),
          mCustomServerPortField( mainFont, 84, -208, 4, false, 
                                  translate( "port" ),
                                  "0123456789", NULL ),
          mCopyButton( mainFont, 381, -216, translate( "copy" ) ),
          mPasteButton( mainFont, 518, -216, translate( "paste" ) ),
          mCursorScaleSlider( mainFont, 297, 155, 4, 200, 30,
                                       1.0, 10.0, 
                                       translate( "scale" ) ) {
                            

    
    const char *choiceList[3] = { translate( "system" ),
                                  translate( "drawn" ),
                                  translate( "both" ) };
    
    mCursorModeSet = 
        new RadioButtonSet( mainFont, 561, 275,
                            3, choiceList,
                            false, 4 );
    addComponent( mCursorModeSet );
    mCursorModeSet->addActionListener( this );

    addComponent( &mCursorScaleSlider );
    mCursorScaleSlider.addActionListener( this );

    mCursorScaleSlider.toggleField( false );


    setButtonStyle( &mBackButton );
    setButtonStyle( &mEditAccountButton );
    setButtonStyle( &mRestartButton );
    setButtonStyle( &mRedetectButton );
    setButtonStyle( &mCopyButton );
    setButtonStyle( &mPasteButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mEditAccountButton );
    mEditAccountButton.addActionListener( this );

    addComponent( &mVsyncBox );
    mVsyncBox.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );

    addComponent( &mTargetFrameRateField );
    mTargetFrameRateField.addActionListener( this );
    
    mTargetFrameRateField.setFireOnAnyTextChange( true );

    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );

    addComponent( &mUseCustomServerBox );
    addComponent( &mCustomServerAddressField );
    addComponent( &mCustomServerPortField );
    
    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    
    if( ! isClipboardSupported() ) {
        mCopyButton.setVisible( false );
        mPasteButton.setVisible( false );
        }

    mRestartButton.setVisible( false );
    
    mOldFullscreenSetting = 
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mTestSound = blankSoundUsage;

    mMusicStartTime = 0;

    
    mVsyncBox.setToggled( getCountingOnVsync() );

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    mBorderlessBox.setVisible( mOldFullscreenSetting );


    mOldBorderlessSetting = 
        SettingsManager::getIntSetting( "borderless", 0 );

    mBorderlessBox.setToggled( mOldBorderlessSetting );
    
    

    addComponent( &mMusicLoudnessSlider );
    addComponent( &mSoundEffectsLoudnessSlider );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );


    mTargetFrameRateField.setInt( targetFramesPerSecond );

    if( getCountingOnVsync() ) {
        mTargetFrameRateField.setVisible( false );
        }
    }



SettingsPage::~SettingsPage() {
    clearSoundUsage( &mTestSound );

    delete mCursorModeSet;
    }



void SettingsPage::checkRestartButtonVisibility() {
    int newVsyncSetting = 
        SettingsManager::getIntSetting( "countingOnVsync", -1 );
    
    int newFullscreenSetting =
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    int newBorderlessSetting =
        SettingsManager::getIntSetting( "borderless", 0 );

    if( getCountingOnVsync() != newVsyncSetting
        ||
        mOldFullscreenSetting != newFullscreenSetting
        ||
        mOldBorderlessSetting != newBorderlessSetting 
        ||
        ( mTargetFrameRateField.isVisible() && 
          mTargetFrameRateField.getInt() != targetFramesPerSecond ) ) {

        mRestartButton.setVisible( true );
        }
    else {
        mRestartButton.setVisible( false );
        }
    }




void SettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        
        int useCustomServer = 0;
        if( mUseCustomServerBox.getToggled() ) {
            useCustomServer = 1;
            }
        
        SettingsManager::setSetting( "useCustomServer", useCustomServer );
        char *address = mCustomServerAddressField.getText();
        
        SettingsManager::setSetting( "customServerAddress", address );
        delete [] address;
        
        SettingsManager::setSetting( "customServerPort",
                                     mCustomServerPortField.getInt() );
        
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mEditAccountButton ) {
        
        setSignal( "editAccount" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mTargetFrameRateField ) {
        
        int newValue = mTargetFrameRateField.getInt();
        
        if( newValue > 0 ) {
            
            // if we're manually manipulating target frame rate
            // any tweaked halfFrameRate setting should be cleared
            SettingsManager::setSetting( "halfFrameRate", 0 );

            SettingsManager::setSetting( "targetFrameRate", newValue );

            checkRestartButtonVisibility();
            }
        }
    else if( inTarget == &mVsyncBox ) {
        int newSetting = mVsyncBox.getToggled();
        
        SettingsManager::setSetting( "countingOnVsync", newSetting );
        
        checkRestartButtonVisibility();

        if( ! newSetting ) {
            mTargetFrameRateField.setVisible( true );
            mTargetFrameRateField.setInt( targetFramesPerSecond );
            }
        else {
            mTargetFrameRateField.setVisible( false );
            
            // if we're manually manipulating target frame rate
            // any tweaked halfFrameRate setting should be cleared
            SettingsManager::setSetting( "halfFrameRate", 0 );
            SettingsManager::setSetting( "targetFramesPerSecond",
                                         targetFramesPerSecond );
            }
        }
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        
        if( ! newSetting ) {
            // can't be borderless if not fullscreen
            SettingsManager::setSetting( "borderless", 0 );
            mBorderlessBox.setToggled( false );
            }
        
            
        checkRestartButtonVisibility();
        
        mBorderlessBox.setVisible( newSetting );
        }
    else if( inTarget == &mBorderlessBox ) {
        int newSetting = mBorderlessBox.getToggled();
        
        SettingsManager::setSetting( "borderless", newSetting );
        
        checkRestartButtonVisibility();
        }
    else if( inTarget == &mRedetectButton ) {
        // redetect means start from scratch, detect vsync, etc.
        SettingsManager::setSetting( "targetFrameRate", -1 );
        SettingsManager::setSetting( "countingOnVsync", -1 );
        
        char relaunched = relaunchGame();
        
        if( !relaunched ) {
            printf( "Relaunch failed\n" );
            setSignal( "relaunchFailed" );
            }
        else {
            printf( "Relaunched... but did not exit?\n" );
            setSignal( "relaunchFailed" );
            }
        }
    else if( inTarget == &mRestartButton ) {
             
        int newVsyncSetting = 
            SettingsManager::getIntSetting( "countingOnVsync", -1 );
        
        int newFullscreenSetting =
            SettingsManager::getIntSetting( "fullscreen", 1 );
        
        int newBorderlessSetting =
            SettingsManager::getIntSetting( "borderless", 0 );
        

        if( ( newVsyncSetting == 1 && ! getCountingOnVsync() )
            ||
            ( newVsyncSetting == 1 && 
              newFullscreenSetting != mOldFullscreenSetting )
            ||
            ( newVsyncSetting == 1 && 
              newBorderlessSetting != mOldBorderlessSetting ) ) {
            
            // re-measure frame rate after relaunch
            // but only if counting on vsync AND if one of these things changed
            
            // 1.  We weren't counting on vsync before
            // 2.  We've toggled fullscreen mode
            // 3.  We've toggled borderless mode
            
            // if we're not counting on vsync, we shouldn't re-measure
            // because measuring is only sensible if we're trying to detect
            // our vsync-enforced frame rate
            SettingsManager::setSetting( "targetFrameRate", -1 );
            }
        

        // now that end-user can toggle vsync here, never clear
        // the vsync setting when we re-launch


        char relaunched = relaunchGame();
        
        if( !relaunched ) {
            printf( "Relaunch failed\n" );
            setSignal( "relaunchFailed" );
            }
        else {
            printf( "Relaunched... but did not exit?\n" );
            setSignal( "relaunchFailed" );
            }
        }
    else if( inTarget == &mSoundEffectsLoudnessSlider ) {
        setMusicLoudness( 0 );
        mMusicStartTime = 0;
        
        if( ! mSoundEffectsLoudnessSlider.isPointerDown() ) {
            
            setSoundEffectsLoudness( 
                mSoundEffectsLoudnessSlider.getValue() );
        
            if( mTestSound.numSubSounds > 0 ) {
                doublePair pos = { 0, 0 };
                
                playSound( mTestSound, pos );
                }
            }
        }    
    else if( inTarget == &mMusicLoudnessSlider ) {
            

        if( ! mSoundEffectsLoudnessSlider.isPointerDown() ) {
            musicLoudness = mMusicLoudnessSlider.getValue();
            SettingsManager::setSetting( "musicLoudness", musicLoudness );
            }
            
        
        if( Time::getCurrentTime() - mMusicStartTime > 25 ) {

            instantStopMusic();
            

            restartMusic( 9.0, 1.0/60.0, true );
            
            setMusicLoudness( mMusicLoudnessSlider.getValue(), true );


            mMusicStartTime = Time::getCurrentTime();
            }
        else {
            setMusicLoudness( mMusicLoudnessSlider.getValue(), true );
            }
        }
    else if( inTarget == &mCopyButton ) {
        char *address = mCustomServerAddressField.getText();
        
        char *fullAddress = autoSprintf( "%s:%d", address,
                                         mCustomServerPortField.getInt() );
        delete [] address;
        
        setClipboardText( fullAddress );
        
        delete [] fullAddress;
        }
    else if( inTarget == &mPasteButton ) {
        char *text = getClipboardText();

        char *trimmed = trimWhitespace( text );
        
        delete [] text;
        

        char setWithPort = false;
        
        if( strstr( trimmed, ":" ) != NULL ) {
            char addressBuff[100];
            int port = 0;
            
            int numRead = sscanf( trimmed, "%99[^:]:%d", addressBuff, &port );
            
            if( numRead == 2 ) {
                setWithPort = true;
                
                char *trimmedAddr = trimWhitespace( addressBuff );
                
                // terminate at first space, if any
                char *spacePos = strstr( trimmedAddr, " " );
                if( spacePos != NULL ) {
                    spacePos[0] = '\0';
                    }

                mCustomServerAddressField.setText( trimmedAddr );

                delete [] trimmedAddr;
                
                mCustomServerPortField.setInt( port );
                }
            }
        
        if( ! setWithPort ) {
            // treat the whole thing as an address
            
            // terminate at first space, if any
            char *spacePos = strstr( trimmed, " " );
            
            if( spacePos != NULL ) {
                spacePos[0] = '\0';
                }
            mCustomServerAddressField.setText( trimmed );
            }
        delete [] trimmed;
        }
    else if( inTarget == mCursorModeSet ) {
        setCursorMode( mCursorModeSet->getSelectedItem() );
        
        int mode = getCursorMode();
        
        mCursorScaleSlider.setVisible( mode > 0 );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );
        }
    else if( inTarget == &mCursorScaleSlider ) {
        setEmulatedCursorScale( mCursorScaleSlider.getValue() );
        }
    
    
    }




void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );

    
    doublePair pos = mVsyncBox.getPosition();
    
    pos.x -= 24;
    pos.y -= 2;
    
    mainFont->drawString( translate( "vsyncOn" ), pos, alignRight );

    
    
    pos = mFullscreenBox.getPosition();
    
    pos.x -= 24;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );


    pos = mUseCustomServerBox.getPosition();
    
    pos.x -= 24;
    pos.y -= 2;
    
    mainFont->drawString( translate( "useCustomServer" ), pos, alignRight );
    

    if( mBorderlessBox.isVisible() ) {
        pos = mBorderlessBox.getPosition();
    
        pos.x -= 24;
        pos.y -= 2;
        
        mainFont->drawString( translate( "borderless" ), pos, alignRight );
        }
    

    pos = mFullscreenBox.getPosition();
    
    pos.y += 96;
    pos.x -= 8;
    
    pos.y += 44;


    if( ! mTargetFrameRateField.isVisible() ) {
        char *fpsString = autoSprintf( "%d", targetFramesPerSecond );
        
        mainFont->drawString( fpsString, pos, alignLeft );
        delete [] fpsString;
        }
    

    pos.y += 44;

    char *currentFPSString = autoSprintf( "%.2f", getRecentFrameRate() );
    
    mainFont->drawString( currentFPSString, pos, alignLeft );
    delete [] currentFPSString;
    

    pos = mFullscreenBox.getPosition();
    pos.x -= 24;

    pos.y += 96;

    pos.y += 44;
    mainFont->drawString( translate( "targetFPS" ), pos, alignRight );
    pos.y += 44;
    mainFont->drawString( translate( "currentFPS" ), pos, alignRight );


    pos = mCursorModeSet->getPosition();
    
    pos.y += 37;
    mainFont->drawString( translate( "cursor"), pos, alignRight );
    
    if( mCursorScaleSlider.isVisible() ) {
        
        pos = mCursorScaleSlider.getPosition();
        
        pos.x += 72;
        pos.y -= 2;
        
        mainFont->drawString( translate( "scale"), pos, alignRight );
        }
    
    }



void SettingsPage::step() {
    if( mTestSound.numSubSounds > 0 ) {
        markSoundUsageLive( mTestSound );
        }
    stepMusicPlayer();
    }





void SettingsPage::makeActive( char inFresh ) {
    if( inFresh ) {        
        setSoundLoudness( 1.0 );
        resumePlayingSoundSprites();

        int mode = getCursorMode();
        
        mCursorModeSet->setSelectedItem( mode );
        
        mCursorScaleSlider.setVisible( mode > 0 );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );


        int useCustomServer = 
            SettingsManager::getIntSetting( "useCustomServer", 0 );
        
        mUseCustomServerBox.setToggled( useCustomServer );
        

        char *address = 
            SettingsManager::getStringSetting( "customServerAddress",
                                               "localhost" );
        
        int port = SettingsManager::getIntSetting( "customServerPort", 8005 );
        
        mCustomServerAddressField.setText( address );
        mCustomServerPortField.setInt( port );
        
        delete [] address;
        


        mMusicLoudnessSlider.setValue( musicLoudness );
        mSoundEffectsLoudnessSlider.setValue( getSoundEffectsLoudness() );
        setMusicLoudness( 0 );
        mMusicStartTime = 0;
        
        int tryCount = 0;
        
        while( mTestSound.numSubSounds == 0 && tryCount < 10 ) {

            int oID = getRandomPersonObject();

            if( oID > 0 ) {
                ObjectRecord *r = getObject( oID );
                if( r->usingSound.numSubSounds >= 1 ) {
                    mTestSound = copyUsage( r->usingSound );
                    // constrain to only first subsound                    
                    mTestSound.numSubSounds = 1;
                    // play it at full volume
                    mTestSound.volumes[0] = 1.0;
                    }
                }
            tryCount ++;
            }
        
        if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
            mEditAccountButton.setVisible( true );
            }
        else {
            mEditAccountButton.setVisible( false );
            }

        }
    }



void SettingsPage::makeNotActive() {
    }

