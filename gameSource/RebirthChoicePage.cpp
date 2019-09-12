#include "RebirthChoicePage.h"

#include "buttonStyle.h"
#include "message.h"

#include "lifeTokens.h"
#include "fitnessScore.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;


extern SpriteHandle instructionsSprite;



static doublePair tutorialButtonPos = { 522, 300 };



RebirthChoicePage::RebirthChoicePage()
        : mQuitButton( mainFont, -150, -128, 
                       translate( "quit" ) ),
          mReviewButton( mainFont, 150, 64, 
                       translate( "postReviewButton" ) ),
          mRebornButton( mainFont, 150, -128, 
                         translate( "reborn" ) ),
          mGenesButton( mainFont, -300, 64, 
                        translate( "geneticHistoryButton" ) ),
          mTutorialButton( mainFont, tutorialButtonPos.x, tutorialButtonPos.y, 
                           translate( "tutorial" ) ),
          mMenuButton( mainFont, -tutorialButtonPos.x, tutorialButtonPos.y, 
                       translate( "menu" ) ),
          // only visible in hard to quit mode
          mFriendsButton( mainFont, -360, 0, 
                          translate( "friendsButton" ) ) {
    if( !isHardToQuitMode() ) {
        addComponent( &mQuitButton );
        addComponent( &mReviewButton );
        addComponent( &mMenuButton );
	addComponent( &mTutorialButton );
	addComponent( &mGenesButton );
        }
    else {
        mRebornButton.setPosition( 0, -200 );
        }
    
    addComponent( &mRebornButton );

    setButtonStyle( &mQuitButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mRebornButton );
    setButtonStyle( &mGenesButton );
    
    setButtonStyle( &mTutorialButton );
    setButtonStyle( &mMenuButton );
    setButtonStyle( &mFriendsButton );
    
    mQuitButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    mRebornButton.addActionListener( this );
    mGenesButton.addActionListener( this );
    mTutorialButton.addActionListener( this );
    mMenuButton.addActionListener( this );
    mFriendsButton.addActionListener( this );

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
    else if( inTarget == &mGenesButton ) {
        setSignal( "genes" );
        }
    else if( inTarget == &mTutorialButton ) {
        setSignal( "tutorial" );
        }
    else if( inTarget == &mMenuButton ) {
        setSignal( "menu" );
        }
    else if( inTarget == &mFriendsButton ) {
        setSignal( "friends" );
        }
    }



void RebirthChoicePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );
    
    if( isHardToQuitMode() ) {
        pos.y = 300;
        drawMessage( "kioskMessage", pos );
        }
    else {
        drawTokenMessage( pos );

        pos.y += 104;
        drawFitnessScore( pos );
        }


    if( isHardToQuitMode() ) {
        // show instructions sprite
        setDrawColor( 1, 1, 1, 1 );
        
        
        doublePair pos = { 360, -225 };
        
        drawSprite( instructionsSprite, pos );
        }
    }



void RebirthChoicePage::makeActive( char inFresh ) {
    triggerLifeTokenUpdate();
    triggerFitnessScoreUpdate();
    
    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    else {
        mReviewButton.setLabelText( translate( "postReviewButton" ) );
        }    

    if( SettingsManager::getIntSetting( "useSteamUpdate", 0 ) ) {
        // no review button on Steam
        mReviewButton.setVisible( false );
        }
    else {
        mReviewButton.setVisible( true );
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
