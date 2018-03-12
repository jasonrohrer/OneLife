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
        : mBackButton( mainFont, 0, -250, translate( "backButton" ) ),
          mRestartButton( mainFont, 128, 128, translate( "restartButton" ) ),
          mRedetectButton( mainFont, 153, 249, translate( "redetectButton" ) ),
          mFullscreenBox( 0, 128, 4 ),
          mMusicLoudnessSlider( mainFont, 0, 0, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -128, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ) {
    
    setButtonStyle( &mBackButton );
    setButtonStyle( &mRestartButton );
    setButtonStyle( &mRedetectButton );
    
    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );
    

    mRestartButton.setVisible( false );
    
    mOldFullscreenSetting = 
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mTestSound = blankSoundUsage;

    mMusicStartTime = 0;

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    

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
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        
        mRestartButton.setVisible( mOldFullscreenSetting != newSetting );
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
    }


extern int targetFramesPerSecond;


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );
    
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

