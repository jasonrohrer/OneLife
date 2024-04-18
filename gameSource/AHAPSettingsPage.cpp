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
        : ServerActionPage( inAHAPGateServerURL, "get_sequence_number", false ),
          mSequenceNumber( -1 ),
          mCurrentLeaderEmail( NULL ),
          mPosting( false ),
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
    }



void AHAPSettingsPage::draw( doublePair inViewCenter, 
                             double inViewSize ) {
    }




void AHAPSettingsPage::setupRequest( const char *inActionName ) {
    clearActionParameters();
    
    setActionName( inActionName );
                    
                    
    char *encodedEmail = URLUtils::urlEncode( userEmail );
    setActionParameter( "email", encodedEmail );
    delete [] encodedEmail;
    
    setActionParameter( "sequence_number", mSequenceNumber );
    
        
    
    char *stringToHash = autoSprintf( "%d", mSequenceNumber );
    
    
    char *pureKey = getPureAccountKey();
    
    char *hash = hmac_sha1( pureKey, stringToHash );
    
    setActionParameter( "hash_value", hash );
    
    delete [] hash;
    
    
    delete [] pureKey;
    delete [] stringToHash;                
    }



void AHAPSettingsPage::step() {
    
    ServerActionPage::step();

    if( isResponseReady() ) {

        if( mSequenceNumber == -1 ) {
            
            char *seqString = getResponse( 0 );
            
            if( seqString != NULL ) {
                int seq = 0;
                int numRead = sscanf( seqString, "%d", &seq );
                
                delete [] seqString;
                
                if( numRead == 1 ) {
                    mSequenceNumber = seq;
                    }
                }


            if( mSequenceNumber != -1 ) {
                
                if( mCurrentLeaderEmail == NULL ) {
                    /*
                    server.php
                        ?action=get_content_leader
                        &email=[email address]
                        &sequence_number=[int]
                        &hash_value=[hash value]
                    */
                    setupRequest( "get_content_leader" );
                    
                    startRequest();
                    }
                else if( mPosting ) {

                    // save settings again here, just in case
                    // user fiddled with field while sequence number loading
                    saveSettings();
                    
                    setupRequest( "register_github" );

                    char *username = mGithubAccountNameField.getText();

                    setActionParameter( "github_username", username );

                    delete [] username;
                    
                    
                    startRequest();
                    }
                }
            }
        else if( mCurrentLeaderEmail == NULL ) {
            char *responseString = getResponse( 0 );
            
            if( responseString != NULL ) {
                
                char email[200];

                int numRead = sscanf( responseString, "%199s", email );
                
                delete [] responseString;
                
                if( numRead == 1 ) {
                    mCurrentLeaderEmail = stringDuplicate( email );
                    }
                }
            
            mPostButton.setVisible( true );
            }
        }
    }





void AHAPSettingsPage::makeActive( char inFresh ) {

    if( ! inFresh ) {        
        return;
        }
    

    setStatus( NULL, false );
    mResponseReady = false;


    char *username = SettingsManager::getStringSetting( "githubUsername", "" );
    
    mGithubAccountNameField.setText( username );
    
    delete [] username;
    

    char *leaderEmail = 
        SettingsManager::getStringSetting( "chosenContentLeader", "" );
    
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
