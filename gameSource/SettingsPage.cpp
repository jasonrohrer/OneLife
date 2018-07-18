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


SettingsPage::SettingsPage()
        : mBackButton( mainFont, -542, -280, translate( "backButton" ) ),
          mRestartButton( mainFont, 128, 128, translate( "restartButton" ) ),
          mRedetectButton( mainFont, 153, 249, translate( "redetectButton" ) ),
          mFullscreenBox( 0, 128, 4 ),
          mBorderlessBox( 0, 168, 4 ),
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
          mPasteButton( mainFont, 518, -216, translate( "paste" ) ) {
    
    setButtonStyle( &mBackButton );
    setButtonStyle( &mRestartButton );
    setButtonStyle( &mRedetectButton );
    setButtonStyle( &mCopyButton );
    setButtonStyle( &mPasteButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );

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

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    mBorderlessBox.setVisible( mOldFullscreenSetting );


    mOldBorderlessSetting = 
        SettingsManager::getIntSetting( "borderless", 0 );

    mBorderlessBox.setToggled( mOldBorderlessSetting );
    
    

    addComponent( &mMusicLoudnessSlider );
    addComponent( &mSoundEffectsLoudnessSlider );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );

    }



SettingsPage::~SettingsPage() {
    clearSoundUsage( &mTestSound );
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
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        
        mRestartButton.setVisible( mOldFullscreenSetting != newSetting );
        
        mBorderlessBox.setVisible( newSetting );
        }
    else if( inTarget == &mBorderlessBox ) {
        int newSetting = mBorderlessBox.getToggled();
        
        SettingsManager::setSetting( "borderless", newSetting );
        
        mRestartButton.setVisible( mOldBorderlessSetting != newSetting );
        }
    else if( inTarget == &mRestartButton ||
             inTarget == &mRedetectButton ) {
        // always re-measure frame rate after relaunch
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
    
    
    }


extern int targetFramesPerSecond;


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );


    pos = mUseCustomServerBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "useCustomServer" ), pos, alignRight );
    

    if( mBorderlessBox.isVisible() ) {
        pos = mBorderlessBox.getPosition();
    
        pos.x -= 30;
        pos.y -= 2;
        
        mainFont->drawString( translate( "borderless" ), pos, alignRight );
        }
    

    pos = mFullscreenBox.getPosition();
    
    pos.y += 96;
    pos.x -= 16;
    
    if( getCountingOnVsync() ) {
        mainFont->drawString( translate( "vsyncYes" ), pos, alignLeft );
        }
    else {
        mainFont->drawString( translate( "vsyncNo" ), pos, alignLeft );
        }
    
    pos.y += 44;

    char *fpsString = autoSprintf( "%d", targetFramesPerSecond );
    
    mainFont->drawString( fpsString, pos, alignLeft );
    delete [] fpsString;


    pos.y += 44;

    char *currentFPSString = autoSprintf( "%.2f", getRecentFrameRate() );
    
    mainFont->drawString( currentFPSString, pos, alignLeft );
    delete [] currentFPSString;
    

    pos = mFullscreenBox.getPosition();
    pos.x -= 30;

    pos.y += 96;
    mainFont->drawString( translate( "vsyncOn" ), pos, alignRight );
    pos.y += 44;
    mainFont->drawString( translate( "targetFPS" ), pos, alignRight );
    pos.y += 44;
    mainFont->drawString( translate( "currentFPS" ), pos, alignRight );
    }



void SettingsPage::step() {
    if( mTestSound.numSubSounds > 0 ) {
        markSoundUsageLive( mTestSound );
        }
    stepMusicPlayer();
    }





void SettingsPage::makeActive( char inFresh ) {
    if( inFresh ) {        

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
        

        }
    }



void SettingsPage::makeNotActive() {
    }

