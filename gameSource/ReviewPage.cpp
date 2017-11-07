#include "ReviewPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"

#include "minorGems/game/game.h"

#include "buttonStyle.h"


extern Font *mainFont;
extern Font *mainFontReview;

extern float musicLoudness;


ReviewPage::ReviewPage()
        : mReviewNameField( mainFont, 0, 250, 10, false,
                            translate( "reviewName"), 
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "1234567890"
                            " ._-" ),
          mReviewTextArea( 
              mainFont, mainFontReview, 0, 0, 800, 320, false, 
              translate( "reviewText" ), 
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "1234567890"
              " !?$%*&()+-='\":;,.\r", NULL ),
          mBackButton( mainFont, 0, -250, translate( "backButton" ) ) {
    
    setButtonStyle( &mBackButton );

    addComponent( &mBackButton );
    mBackButton.addActionListener( this );

    // add name field after so we can hit return in name field
    // and advance to text area without sending a return key to the text area
    addComponent( &mReviewTextArea );    
    addComponent( &mReviewNameField );


    mReviewNameField.setMaxLength( 20 );

    mReviewNameField.addActionListener( this );

    mReviewNameField.setLabelTop( true );
    }



void ReviewPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
        setSignal( "back" );
        }
    else if( inTarget == &mReviewNameField ) {
        switchFields();
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


void ReviewPage::switchFields() {
    if( mReviewNameField.isFocused() ) {
        mReviewTextArea.focus();
        }
    else if( mReviewTextArea.isFocused() ) {
        mReviewNameField.focus();
        }
    }



void ReviewPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }
    }
