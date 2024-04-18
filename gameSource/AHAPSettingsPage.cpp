#include "AHAPSettingsPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"

#include "minorGems/network/web/URLUtils.h"
#include "minorGems/crypto/hashes/sha1.h"


#include "buttonStyle.h"
#include "spellCheck.h"
#include "accountHmac.h"


extern Font *mainFont;

extern char *userEmail;



AHAPSettingsPage::AHAPSettingsPage( const char *inAHAPGateServerURL )
        : ServerActionPage( inAHAPGateServerURL, "register_github", false ),
          mGithubAccountNameField( mainFont, -242, 250, 10, false,
                                   translate( "githubAccountName"), 
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "1234567890"
                                   "._-" ),
          mContentLeaderVoteField( mainFont, -242, 125, 10, false,
                                   translate( "contentLeaderVote"), 
                                   NULL,
                                   // forbid only spaces and backslash and 
                                   // single/double quotes 
                                   "\"' \\" ),
          mBackButton( mainFont, -526, -140, translate( "backButton" ) ),
          mPostButton( mainFont, 526, -140, translate( "postButton" ) ),
          mGettingSequenceNumber( false ) {

    setButtonStyle( &mBackButton );
    setButtonStyle( &mPostButton );
    

    addComponent( &mBackButton );
    addComponent( &mPostButton );

    mBackButton.addActionListener( this );
    mPostButton.addActionListener( this );
    
    // add name field after so we can hit return in name field
    // and advance to text area without sending a return key to the text area
    addComponent( &mGithubAccountNameField );    
    addComponent( &mContentLeaderVoteField );
    

    mPostButton.setMouseOverTip( translate( "postAHAPSettingsTip" ) );
    
    }



AHAPSettingsPage::~AHAPSettingsPage() {
    }






void AHAPSettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {        
        setSignal( "back" );
        }
    else if( inTarget == &mPostButton ) {
        switchFields();
        }
    }



void AHAPSettingsPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {

    doublePair pos = mRecommendChoice->getPosition();
    
    pos.y += 32;
    pos.x += 12;
    
    setDrawColor( 1, 1, 1, 1 );
    mainFont->drawString( translate( "recommend" ), pos, alignRight );


    if( mReviewTextArea.isAtLimit() ) {
        
        pos.x = 0;
        pos.y = -193;
        mainFont->drawString( translate( "charLimit" ), pos, alignCenter );
        }
    
    pos = mSpellcheckButton.getPosition();
    
    pos.x -= 24;
    pos.y -= 2;
    
    mainFont->drawString( translate( "spellCheck" ), pos, alignRight );
    }




void AHAPSettingsPage::step() {
    
    // FIXME
    // this copied from ReviewPage

    ServerActionPage::step();

    if( isResponseReady() ) {

        if( ! mGettingSequenceNumber && ! mRemoving ) {
            
            int reviewPosted = SettingsManager::getIntSetting( 
                "reviewPosted", 0 );
            
            if( reviewPosted ) {
                setStatus( "reviewUpdated", false );
                }
            else {
                setStatus( "reviewPosted", false );
                }
        
            SettingsManager::setSetting( "reviewPosted", 1 );
            }
        else if( ! mRemoving ) {
            
            char *seqString = getResponse( 0 );
            
            if( seqString != NULL ) {
                int seq = 0;
                sscanf( seqString, "%d", &seq );
                
                delete [] seqString;
                

                mResponseReady = false;
                
                setActionName( "remove_review" );
        
                clearActionParameters();
        
        
                char *encodedEmail = URLUtils::urlEncode( userEmail );
                setActionParameter( "email", encodedEmail );
                delete [] encodedEmail;
        
                setActionParameter( "sequence_number", seq );
                
        
        
                char *stringToHash = autoSprintf( "%d", seq );

                
                char *pureKey = getPureAccountKey();
                
                char *hash = hmac_sha1( pureKey, stringToHash );
                
                delete [] pureKey;
                delete [] stringToHash;
                
                
                setActionParameter( "hash_value", hash );
        
                delete [] hash;
        


                mGettingSequenceNumber = false;
                mRemoving = true;
                startRequest();
                }
            }
        else if( mRemoving ) {
            setStatus( "reviewRemoved", false );
            
            SettingsManager::setSetting( "reviewPosted", 0 );
            }
        }
    }





void AHAPSettingsPage::makeActive( char inFresh ) {
    // FIXME
    // this copied from ReviewPage


    if( inFresh ) {        
        }

    if( ! isSpellCheckReady() ) {
        initSpellCheck();
        }
    
    setStatus( NULL, false );
    mResponseReady = false;
    

    mReviewNameField.setActive( true );
    mReviewTextArea.setActive( true );
    mRecommendChoice->setActive( true );
    mSpellcheckButton.setActive( true );
    

    mCopyButton.setActive( true );
    mPasteButton.setActive( true );
    mClearButton.setActive( true );

    
    char spellCheck = SettingsManager::getIntSetting( "spellCheckOn", 1 );
    
    mReviewTextArea.enableSpellCheck( spellCheck );

    mSpellcheckButton.setToggled( spellCheck );
    

    mReviewNameField.focus();


    char *reviewName = SettingsManager::getSettingContents( "reviewName", "" );
    char *reviewText = SettingsManager::getSettingContents( "reviewText", "" );
    
    int reviewRecommend = 
        SettingsManager::getIntSetting( "reviewRecommend", 1 );
    
    if( reviewRecommend == 0 ) {    
        mRecommendChoice->setSelectedItem( 1 );
        }
    else {
        mRecommendChoice->setSelectedItem( 0 );
        }
    
    mReviewNameField.setText( reviewName );

    char *oldText = mReviewTextArea.getText();
    
    if( strcmp( oldText, reviewText ) != 0 ) {    
        // keep cursor pos if text hasn't changed
        mReviewTextArea.setText( reviewText );
        }
    
    delete [] oldText;
    

    delete [] reviewName;
    delete [] reviewText;


        
    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mPostButton.setLabelText( translate( "updateButton" ) );
        mPostButton.setMouseOverTip( translate( "updateReviewTip" ) );
        mRemoveButton.setVisible( true );
        }
    else {
        mPostButton.setLabelText( translate( "postButton" ) );
        mPostButton.setMouseOverTip( translate( "postReviewTip" ) );
        mRemoveButton.setVisible( false );
        }

    

    checkCanPost();

    
    if( mPostButton.isVisible() ) {
        // name field has something already
        // put focus on review text
        mReviewTextArea.focus();
        }
    

    checkCanPaste();
    }



void AHAPSettingsPage::makeNotActive() {
    }


void AHAPSettingsPage::switchFields() {
    if( mReviewNameField.isFocused() ) {
        mReviewTextArea.focus();
        }
    else if( mReviewTextArea.isFocused() ) {
        mReviewNameField.focus();
        }
    }



void AHAPSettingsPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }
    }
