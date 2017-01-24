#include "SettingsPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"

#include "minorGems/game/game.h"


extern Font *mainFont;


SettingsPage::SettingsPage()
        : mBackButton( mainFont, 0, -250, translate( "backButton" ) ),
          mRestartButton( mainFont, 128, 0, translate( "restartButton" ) ),
          mFullscreenBox( 0, 0, 4 ),
          mMusicLoudnessSlider( mainFont, 0, -64, 2, 200, 20,
                                0.0, 1.0, 
                                translate( "musicLoudness" ) ),
          mSoundEffectsLoudnessSlider( mainFont, 0, -128, 2, 200, 20,
                                       0.0, 1.0, 
                                       translate( "soundLoudness" ) ) {
    
    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mFullscreenBox );
    mFullscreenBox.addActionListener( this );


    mOldFullscreenSetting = SettingsManager::getIntSetting( "fullscreen", 1 );
    
    mFullscreenBox.setToggled( mOldFullscreenSetting );
    
    

    addComponent( &mMusicLoudnessSlider );
    addComponent( &mSoundEffectsLoudnessSlider );
    
    mMusicLoudnessSlider.addActionListener( this );
    mSoundEffectsLoudnessSlider.addActionListener( this );
    
    mMusicLoudnessSlider.toggleField( false );
    mSoundEffectsLoudnessSlider.toggleField( false );

    
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
    
    
    }



void SettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = mFullscreenBox.getPosition();
    
    pos.x -= 30;
    pos.y -= 2;
    
    mainFont->drawString( translate( "fullscreen" ), pos, alignRight );
    }





void SettingsPage::makeActive( char inFresh ) {
    }


void SettingsPage::makeNotActive() {
    }

