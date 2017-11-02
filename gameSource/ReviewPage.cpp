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
extern Font *mainFontReview;

extern float musicLoudness;


ReviewPage::ReviewPage()
        : mReviewTextArea( 
            mainFontReview, 0, 0, 480, 320, false, 
            translate( "reviewText" ), 
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "1234567890"
            " !?$%*&()+-='\":;,.\r", NULL ),
          mBackButton( mainFont, 0, -250, translate( "backButton" ) ) {
    
    setButtonStyle( &mBackButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    addComponent( &mReviewTextArea );

    mReviewTextArea.setText( "This is a test of a very long message that we will want to edit later using the arrow keys.  It's a pain to type this long message over and over each time we test the game." );
    
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

    mReviewTextArea.focus();
    }



void ReviewPage::makeNotActive() {
    }

