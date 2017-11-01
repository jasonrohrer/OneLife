#include "ReviewPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"

#include "minorGems/game/game.h"
#include "minorGems/system/Time.h"

#include "musicPlayer.h"
#include "soundBank.h"
#include "objectBank.h"
#include "buttonStyle.h"


extern Font *mainFont;

extern float musicLoudness;


ReviewPage::ReviewPage()
        : mBackButton( mainFont, 0, -250, translate( "backButton" ) ) {
    
    setButtonStyle( &mBackButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );
    }



void ReviewPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        setSignal( "back" );
        setMusicLoudness( 0 );
        }
    }



void ReviewPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {
    }



void ReviewPage::step() {
    }





void ReviewPage::makeActive( char inFresh ) {
    if( inFresh ) {        
        }
    }



void ReviewPage::makeNotActive() {
    }

