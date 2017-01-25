#include "SettingsPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"

#include "minorGems/game/game.h"

#include "musicPlayer.h"
#include "soundBank.h"
#include "objectBank.h"


extern Font *mainFont;


SettingsPage::SettingsPage()
        : mBackButton( mainFont, 0, -250, translate( "backButton" ) ),
          mRestartButton( mainFont, 128, 128, translate( "restartButton" ) ),
          mFullscreenBox( 0, 128, 4 ),
          mMusicLoudnessSlider( mainFont, 0, 0, 4, 200, 30,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -128, 4, 200, 30,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ) {
    
    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );

    addComponent( &mRestartButton );
    mRestartButton.addActionListener( this );
    
    mRestartButton.setVisible( false );
    
    mOldFullscreenSetting = 
        SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mTestSound.id = -1;

    mFullscreenBox.setToggled( mOldFullscreenSetting );

    

    addComponent( &mMusicLoudnessSlider );
    addComponent( &mSoundEffectsLoudnessSlider );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );

    }



void SettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        setSignal( "back" );
        }
    else if( inTarget == &mFullscreenBox ) {
        int newSetting = mFullscreenBox.getToggled();
        
        SettingsManager::setSetting( "fullscreen", newSetting );
        
        mRestartButton.setVisible( mOldFullscreenSetting != newSetting );
        }
    else if( inTarget == &mRestartButton ) {
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
        if( ! mSoundEffectsLoudnessSlider.isPointerDown() ) {
            
            setSoundEffectsLoudness( 
                mSoundEffectsLoudnessSlider.getValue() );
        
            if( mTestSound.id != -1 ) {
                doublePair pos = { 0, 0 };
                
                playSound( mTestSound, pos );
                }
            }
        }    
    }



void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );
    }



void SettingsPage::step() {
    if( mTestSound.id != -1 ) {
        markSoundLive( mTestSound.id );
        }
    }





void SettingsPage::makeActive( char inFresh ) {
    if( inFresh ) {        
        mMusicLoudnessSlider.setValue( getMusicLoudness() );
        mSoundEffectsLoudnessSlider.setValue( getSoundEffectsLoudness() );


        int tryCount = 0;
        
        while( mTestSound.id == -1 && tryCount < 10 ) {

            int oID = getRandomPersonObject();

            if( oID > 0 ) {
                ObjectRecord *r = getObject( oID );
                mTestSound = r->usingSound;
                }
            }
        

        }
    }



void SettingsPage::makeNotActive() {
    }

