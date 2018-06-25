#include "RebirthChoicePage.h"

#include "buttonStyle.h"
#include "message.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;


static doublePair tutorialButtonPos = { -522, 300 };



RebirthChoicePage::RebirthChoicePage()
        : mQuitButton( mainFont, -150, -128, 
                       translate( "quit" ) ),
          mReviewButton( mainFont, 150, 64, 
                       translate( "postReviewButton" ) ),
          mRebornButton( mainFont, 150, -128, 
                         translate( "reborn" ) ),
          mTutorialButton( mainFont, tutorialButtonPos.x, tutorialButtonPos.y, 
                           translate( "tutorial" ) ) {
    if( !isHardToQuitMode() ) {
        addComponent( &mQuitButton );
        addComponent( &mReviewButton );
        }
    else {
        mRebornButton.setPosition( 0, -128 );
        }
    
    addComponent( &mRebornButton );
    addComponent( &mTutorialButton );
    
    setButtonStyle( &mQuitButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mRebornButton );
    setButtonStyle( &mTutorialButton );
    
    mQuitButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    mRebornButton.addActionListener( this );
    mTutorialButton.addActionListener( this );


    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    }



void RebirthChoicePage::showReviewButton( char inShow ) {
    mReviewButton.setVisible( inShow );
    }



void RebirthChoicePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mQuitButton ) {
        setSignal( "quit" );
        }
    else if( inTarget == &mReviewButton ) {
        setSignal( "review" );
        }
    else if( inTarget == &mRebornButton ) {
        setSignal( "reborn" );
        }
    else if( inTarget == &mTutorialButton ) {
        setSignal( "tutorial" );
        }
    }



void RebirthChoicePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    //doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );
    }



void RebirthChoicePage::makeActive( char inFresh ) {
    
    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    else {
        mReviewButton.setLabelText( translate( "postReviewButton" ) );
        }

    int tutorialDone = SettingsManager::getIntSetting( "tutorialDone", 0 );
    

    if( !tutorialDone ) {
        mRebornButton.setVisible( false );
        doublePair rebornPos = mRebornButton.getPosition();
        mTutorialButton.setPosition( rebornPos.x, rebornPos.y );
        mTutorialButton.setLabelText( translate( "restartTutorial" ) );
        }
    else {
        mRebornButton.setVisible( true );
        mTutorialButton.setPosition( tutorialButtonPos.x, tutorialButtonPos.y );
        mTutorialButton.setLabelText( translate( "tutorial" ) );
        }
    }
