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
        : mInfoSeeds( mainFont, 542, -150, "?" ),
		  mBackButton( mainFont, -542, -280, translate( "backButton" ) ),
          mEditAccountButton( mainFont, -463, 129, translate( "editAccount" ) ),
          mRestartButton( mainFont, 128, 128, translate( "restartButton" ) ),
          mRedetectButton( mainFont, 153, 249, translate( "redetectButton" ) ),
          mFullscreenBox( 0, 128, 4 ),
          mBorderlessBox( 0, 168, 4 ),
          mEnableNudeBox( -335, 148, 4 ),
          mMusicLoudnessSlider( mainFont, 0, 40, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -48, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ),
          mSpawnSeed( mainFont, 226, -150, 14, false, 
                                     translate( "spawnSeed" ),
                                     NULL,
                                     // forbid spaces
                                     " " ),
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


	setButtonStyle( &mInfoSeeds );
    setButtonStyle( &mBackButton );
    setButtonStyle( &mEditAccountButton );
    setButtonStyle( &mRestartButton );
    setButtonStyle( &mRedetectButton );

	addComponent( &mInfoSeeds);
	mInfoSeeds.addActionListener( this );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mEditAccountButton );
    mEditAccountButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mBorderlessBox );
    mBorderlessBox.addActionListener( this );

    addComponent( &mEnableNudeBox );
    mEnableNudeBox.addActionListener( this );

    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    
    addComponent( &mRedetectButton );
    mRedetectButton.addActionListener( this );

    addComponent( &mSpawnSeed);
    
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

    mEnableNudeSetting =
        SettingsManager::getIntSetting( "nudeEnabled", 1 );

    mEnableNudeBox.setToggled( mEnableNudeSetting );
    
    

    addComponent( &mMusicLoudnessSlider );
    addComponent( &mSoundEffectsLoudnessSlider );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );

    }



SettingsPage::~SettingsPage() {
    clearSoundUsage( &mTestSound );

    delete mCursorModeSet;
    }




void SettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        
        char *seed = mSpawnSeed.getText();
        
        SettingsManager::setSetting( "spawnSeed", seed );
        delete [] seed;
        
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
	else if( inTarget == &mInfoSeeds ) {
		 char *url = strdup("https://twohoursonelife.fandom.com/wiki/Spawn_seeds");
		 
		 if( strcmp( url, "" ) != 0 ) {
			 launchURL( url );
			}
		}
    else if( inTarget == &mEditAccountButton ) {
        
        setSignal( "editAccount" );
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
	else if( inTarget == &mEnableNudeBox ) {
        int newSetting = mEnableNudeBox.getToggled();
        
        SettingsManager::setSetting( "nudeEnabled", newSetting );
        
        mRestartButton.setVisible( mEnableNudeSetting != newSetting );
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


extern int targetFramesPerSecond;


void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );


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


    pos = mEnableNudeBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;

    mainFont->drawString( "Enable Nudity", pos, alignRight );


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

        int mode = getCursorMode();
        
        mCursorModeSet->setSelectedItem( mode );
        
        mCursorScaleSlider.setVisible( mode > 0 );
        
        mCursorScaleSlider.setValue( getEmulatedCursorScale() );


        char *seed = 
            SettingsManager::getStringSetting( "spawnSeed",
                                               "" );
        
        mSpawnSeed.setText( seed );
        
        delete [] seed;
        


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

