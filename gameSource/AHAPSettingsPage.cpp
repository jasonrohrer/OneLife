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
#include "message.h"


extern Font *mainFont;

extern char *userEmail;



AHAPSettingsPage::AHAPSettingsPage( const char *inAHAPGateServerURL )
        : ServerActionPage( inAHAPGateServerURL, "get_sequence_number", false ),
          mSequenceNumber( -1 ),
          mCurrentLeaderEmail( NULL ),
          mPosting( false ),
          mGithubAccountNameField( mainFont, 200, 60, 10, false,
                                   translate( "githubAccountName"), 
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "1234567890"
                                   "._-" ),
          mContentLeaderVoteField( mainFont, 200, -100, 10, false,
                                   translate( "contentLeaderVote"), 
                                   NULL,
                                   // forbid only spaces and backslash and 
                                   // single/double quotes 
                                   "\"' \\" ),
          mPasteGithubButton( mainFont, 200, 0, 
                              translate( "paste" ), 'v', 'V' ),
          mPasteLeaderButton( mainFont, 200, -160, 
                              translate( "paste" ), 'v', 'V' ),
          mBackButton( mainFont, -526, -280, translate( "backButton" ) ),
          mPostButton( mainFont, 526, -280, translate( "postButton" ) ),
          mGettingSequenceNumber( false ) {

    setButtonStyle( &mBackButton );
    setButtonStyle( &mPostButton );
    
    setButtonStyle( &mPasteGithubButton );
    setButtonStyle( &mPasteLeaderButton );


    addComponent( &mBackButton );
    addComponent( &mPostButton );

    addComponent( &mPasteGithubButton );
    addComponent( &mPasteLeaderButton );

    mBackButton.addActionListener( this );
    mPostButton.addActionListener( this );
    
    mPasteGithubButton.addActionListener( this );
    mPasteLeaderButton.addActionListener( this );
    

    mGithubAccountNameField.addActionListener( this );
    mContentLeaderVoteField.addActionListener( this );
    
    mGithubAccountNameField.setFireOnAnyTextChange( true );
    mContentLeaderVoteField.setFireOnAnyTextChange( true );


    // add name field after so we can hit return in name field
    // and advance to text area without sending a return key to the text area
    addComponent( &mGithubAccountNameField );    
    addComponent( &mContentLeaderVoteField );
    

    mPostButton.setMouseOverTip( translate( "postAHAPSettingsTip" ) );

    mPasteGithubButton.setVisible( false );
    mPasteLeaderButton.setVisible( false );
    }



AHAPSettingsPage::~AHAPSettingsPage() {
    if( mCurrentLeaderEmail != NULL ) {
        delete [] mCurrentLeaderEmail;
        }
    
    mCurrentLeaderEmail = NULL;
    }



void AHAPSettingsPage::saveSettings() {
    char *chosenContentLeader = mContentLeaderVoteField.getText();

    SettingsManager::setSetting( "contentLeaderVote", chosenContentLeader );
    
    delete [] chosenContentLeader;
    

    char *githubAccountName = mGithubAccountNameField.getText();

    SettingsManager::setSetting( "githubUsername", githubAccountName );
    
    delete [] githubAccountName;
    }





void AHAPSettingsPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {        
        saveSettings();
        
        setSignal( "back" );
        }
    else if( inTarget == &mPostButton ) {
        saveSettings();
        
        mPosting = true;
        
        mPostButton.setVisible( false );
        
        clearActionParameters();
        
        setActionName( "get_sequence_number" );
        
        setActionParameter( "email", userEmail );
        
        setAttachAccountHmac( false );
        
        mSequenceNumber = -1;
        
        startRequest();
        }
    else if( inTarget == &mGithubAccountNameField ) {
        testPostVisible();
        }
    else if( inTarget == &mContentLeaderVoteField ) {
        testPostVisible();
        }
    else if( inTarget == &mPasteGithubButton ) {
        char *clipboardText = getClipboardText();
        
        mGithubAccountNameField.setText( clipboardText );
    
        delete [] clipboardText;
        }
    else if( inTarget == &mPasteLeaderButton ) {
        char *clipboardText = getClipboardText();
        
        mContentLeaderVoteField.setText( clipboardText );
    
        delete [] clipboardText;
        }
    }



void AHAPSettingsPage::draw( doublePair inViewCenter, 
                             double inViewSize ) {

    if( mCurrentLeaderEmail != NULL ) {

        doublePair pos = mGithubAccountNameField.getPosition();
        
        pos.x = 0;
        
        pos.y += 125;

        pos.y += 64;
        drawMessage( translate( "contentLeaderExplain" ), pos );
        
        pos.y -= 64;
        drawMessage( mCurrentLeaderEmail, pos );
        }
    }




void AHAPSettingsPage::setupRequest( const char *inActionName,
                                     const char *inExtraHmacConcat ) {
    clearActionParameters();
    
    setActionName( inActionName );
                    
                    
    char *encodedEmail = URLUtils::urlEncode( userEmail );
    setActionParameter( "email", encodedEmail );
    delete [] encodedEmail;
    
    setActionParameter( "sequence_number", mSequenceNumber );
    
        
    
    char *stringToHash = autoSprintf( "%d%s", mSequenceNumber,
                                      inExtraHmacConcat );
    
    
    char *pureKey = getPureAccountKey();
    
    char *hash = hmac_sha1( pureKey, stringToHash );
    
    setActionParameter( "hash_value", hash );
    
    delete [] hash;
    
    
    delete [] pureKey;
    delete [] stringToHash;                
    }



void AHAPSettingsPage::testPostVisible() {
    mPostButton.setVisible( false );
    
    if( mCurrentLeaderEmail == NULL ) {
        return;
        }
    
    char *github = mGithubAccountNameField.getText();
    
    char *leaderVote = mContentLeaderVoteField.getText();
    
    if( strlen( github ) > 0 &&
        strstr( leaderVote, "@" )  != NULL &&
        strstr( leaderVote, "." )  != NULL ) {
        
        // github name not empty, and vote resembles an email

        mPostButton.setVisible( true );
        
        setStatus( NULL, false );
        }

    delete [] github;
    delete [] leaderVote;
    }



void AHAPSettingsPage::step() {
    
    ServerActionPage::step();


    mPasteGithubButton.setVisible( isClipboardSupported() &&
                                   mGithubAccountNameField.isFocused() );
    
    mPasteLeaderButton.setVisible( isClipboardSupported() &&
                                   mContentLeaderVoteField.isFocused() );

    if( isResponseReady() ) {

        if( mSequenceNumber == -1 ) {
            
            if( getNumResponseParts() > 0 ) {
                
                char *seqString = getResponse( 0 );
                
                if( seqString != NULL ) {
                    int seq = 0;
                    int numRead = sscanf( seqString, "%d", &seq );
                    
                    delete [] seqString;
                    
                    if( numRead == 1 ) {
                        mSequenceNumber = seq;
                        }
                    }
                }

            if( mSequenceNumber != -1 ) {
                
                if( mCurrentLeaderEmail == NULL ) {
                    setupRequest( "get_content_leader" );
                    
                    startRequest();
                    }
                else if( mPosting ) {

                    // save settings again here, just in case
                    // user fiddled with field while sequence number loading
                    saveSettings();

                    char *username = mGithubAccountNameField.getText();
                    
                    setupRequest( "register_github", username );


                    setActionParameter( "github_username", username );

                    delete [] username;
                    
                    
                    startRequest();
                    }
                }
            }
        else if( mCurrentLeaderEmail == NULL ) {
            if( getNumResponseParts() > 0 ) {
                
                char *responseString = getResponse( 0 );
                
                if( responseString != NULL ) {
                    
                    char email[200];
                    
                    int numRead = sscanf( responseString, "%199s", email );
                    
                    delete [] responseString;
                    
                    if( numRead == 1 ) {
                        mCurrentLeaderEmail = stringDuplicate( email );
                        }
                    }
                
                testPostVisible();
                }
            }
        else if( mPosting ) {
            // won't get isResponseReady for posting unless success
            setStatus( "githubAccountUpdateSuccess", false );
            
            mPosting = false;
            }
        }
    }





void AHAPSettingsPage::makeActive( char inFresh ) {

    if( ! inFresh ) {        
        return;
        }
    

    setStatus( NULL, false );
    mResponseReady = false;
    
    mPostButton.setVisible( false );

    char *username = SettingsManager::getStringSetting( "githubUsername", "" );
    
    mGithubAccountNameField.setText( username );
    
    delete [] username;
    

    char *leaderEmail = 
        SettingsManager::getStringSetting( "contentLeaderVote", "" );
    
    mContentLeaderVoteField.setText( leaderEmail );
    
    delete [] leaderEmail;
    
    
    
    mPosting = false;
    

    if( mCurrentLeaderEmail != NULL ) {
        delete [] mCurrentLeaderEmail;
        mCurrentLeaderEmail = NULL;
        }
    
    clearActionParameters();
        
    setActionName( "get_sequence_number" );
    
    setActionParameter( "email", userEmail );
    
    setAttachAccountHmac( false );
    
    mSequenceNumber = -1;
    
    startRequest();    
    }



void AHAPSettingsPage::makeNotActive() {
    }


void AHAPSettingsPage::switchFields() {
    if( mGithubAccountNameField.isFocused() ) {
        mContentLeaderVoteField.focus();
        }
    else if( mContentLeaderVoteField.isFocused() ) {
        mGithubAccountNameField.focus();
        }
    }



void AHAPSettingsPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }
    }
