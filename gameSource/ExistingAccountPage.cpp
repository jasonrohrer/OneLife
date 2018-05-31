#include "ExistingAccountPage.h"

#include "message.h"
#include "buttonStyle.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"


#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"


extern Font *mainFont;


extern char gamePlayingBack;

extern char *userEmail;
extern char *accountKey;


extern SpriteHandle instructionsSprite;


ExistingAccountPage::ExistingAccountPage()
        : mEmailField( mainFont, 0, 128, 10, false, 
                       translate( "email" ),
                       NULL,
                       // forbid only spaces
                       " " ),
          mKeyField( mainFont, 0, 0, 15, true,
                     translate( "accountKey" ),
                     // allow only ticket code characters
                     "23456789ABCDEFGHJKLMNPQRSTUVWXYZ-" ),
          mAtSignButton( mainFont, 252, 128, "@" ),
          mPasteButton( mainFont, 0, -80, translate( "paste" ), 'v', 'V' ),
          mDisableCustomServerButton( mainFont, 0, 220, 
                                      translate( "disableCustomServer" ) ),
          mLoginButton( mainFont, 400, 0, translate( "loginButton" ) ),
          mLoginNoSaveButton( mainFont, 400, -280, 
                              translate( "loginNoSaveButton" ) ),
          mCancelButton( mainFont, -400, -280, 
                         translate( "quit" ) ),
          mSettingsButton( mainFont, -400, -120, 
                           translate( "settingsButton" ) ),
          mReviewButton( mainFont, -400, -200, 
                         translate( "postReviewButton" ) ),
          mRedetectButton( mainFont, 0, 220, translate( "redetectButton" ) ),
          mPageActiveStartTime( 0 ),
          mFramesCounted( 0 ),
          mFPSMeasureDone( false ) {
    
    
    // center this in free space
    /*
    mPasteButton.setPosition( ( 333 + mKeyField.getRightEdgeX() ) / 2,
                              -64 );
    */
    // align this one with the paste button
    mAtSignButton.setPosition( mEmailField.getRightEdgeX() + 48,
                               128 );
    
    
    if( userEmail != NULL && accountKey != NULL ) {
        mEmailField.setText( userEmail );
        mKeyField.setText( accountKey );
        }

    setButtonStyle( &mLoginButton );
    setButtonStyle( &mLoginNoSaveButton );
    setButtonStyle( &mCancelButton );
    setButtonStyle( &mSettingsButton );
    setButtonStyle( &mReviewButton );
    setButtonStyle( &mAtSignButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mRedetectButton );

    setButtonStyle( &mDisableCustomServerButton );
    
    mFields[0] = &mEmailField;
    mFields[1] = &mKeyField;

    
    addComponent( &mLoginButton );
    addComponent( &mLoginNoSaveButton );
    addComponent( &mCancelButton );
    addComponent( &mSettingsButton );
    addComponent( &mReviewButton );
    addComponent( &mAtSignButton );
    addComponent( &mPasteButton );
    addComponent( &mEmailField );
    addComponent( &mKeyField );
    addComponent( &mRedetectButton );
    addComponent( &mDisableCustomServerButton );
    
    mLoginButton.addActionListener( this );
    mLoginNoSaveButton.addActionListener( this );
    
    mCancelButton.addActionListener( this );
    mSettingsButton.addActionListener( this );
    mReviewButton.addActionListener( this );
    
    mAtSignButton.addActionListener( this );
    mPasteButton.addActionListener( this );

    mRedetectButton.addActionListener( this );
    
    mDisableCustomServerButton.addActionListener( this );

    mRedetectButton.setVisible( false );
    mDisableCustomServerButton.setVisible( false );
    
    mAtSignButton.setMouseOverTip( translate( "atSignTip" ) );

    mLoginButton.setMouseOverTip( translate( "saveTip" ) );
    mLoginNoSaveButton.setMouseOverTip( translate( "noSaveTip" ) );
    
    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    

    // to dodge quit message
    setTipPosition( true );
    }

          
        
ExistingAccountPage::~ExistingAccountPage() {
    }



void ExistingAccountPage::clearFields() {
    mEmailField.setText( "" );
    mKeyField.setText( "" );
    }



void ExistingAccountPage::showReviewButton( char inShow ) {
    mReviewButton.setVisible( inShow );
    }



void ExistingAccountPage::showDisableCustomServerButton( char inShow ) {
    mDisableCustomServerButton.setVisible( inShow );
    }




void ExistingAccountPage::makeActive( char inFresh ) {

    mFramesCounted = 0;
    mPageActiveStartTime = game_getCurrentTime();    
    mFPSMeasureDone = false;
    
    mLoginButton.setVisible( false );
    
    int skipFPSMeasure = SettingsManager::getIntSetting( "skipFPSMeasure", 0 );
    
    if( skipFPSMeasure ) {
        mFPSMeasureDone = true;
        mLoginButton.setVisible( true );
        }


    int pastSuccess = SettingsManager::getIntSetting( "loginSuccess", 0 );

    char *emailText = mEmailField.getText();
    char *keyText = mKeyField.getText();

    // don't hide field contents unless there is something to hide
    if( ! pastSuccess || 
        ( strcmp( emailText, "" ) == 0 
          &&
          strcmp( keyText, "" ) == 0 ) ) {

        mEmailField.focus();
        }
    else {
        mEmailField.unfocus();
        mKeyField.unfocus();
        
        mEmailField.setContentsHidden( true );
        mKeyField.setContentsHidden( true );
        }
    
    delete [] emailText;
    delete [] keyText;

    
    mPasteButton.setVisible( false );
    mAtSignButton.setVisible( false );


    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mReviewButton.setLabelText( translate( "updateReviewButton" ) );
        }
    else {
        mReviewButton.setLabelText( translate( "postReviewButton" ) );
        }
    }



void ExistingAccountPage::makeNotActive() {
    for( int i=0; i<2; i++ ) {
        mFields[i]->unfocus();
        }
    }



void ExistingAccountPage::step() {
    mPasteButton.setVisible( isClipboardSupported() &&
                             mKeyField.isFocused() );
    mAtSignButton.setVisible( mEmailField.isFocused() );
    }



void ExistingAccountPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mLoginButton ) {
        processLogin( true );
        }
    else if( inTarget == &mLoginNoSaveButton ) {
        processLogin( false );
        }
    else if( inTarget == &mCancelButton ) {
        setSignal( "quit" );
        }
    else if( inTarget == &mSettingsButton ) {
        setSignal( "settings" );
        }
    else if( inTarget == &mReviewButton ) {
        if( userEmail != NULL ) {
            delete [] userEmail;
            }
        userEmail = mEmailField.getText();
        
        if( accountKey != NULL ) {
            delete [] accountKey;
            }
        accountKey = mKeyField.getText();
        
        setSignal( "review" );
        }
    else if( inTarget == &mAtSignButton ) {
        mEmailField.insertCharacter( '@' );
        }
    else if( inTarget == &mPasteButton ) {
        char *clipboardText = getClipboardText();
        
        mKeyField.setText( clipboardText );
    
        delete [] clipboardText;
        }
    else if( inTarget == &mRedetectButton ) {
        SettingsManager::setSetting( "targetFrameRate", -1 );
        SettingsManager::setSetting( "countingOnVsync", -1 );
        
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
    else if( inTarget == &mDisableCustomServerButton ) {
        SettingsManager::setSetting( "useCustomServer", 0 );
        mDisableCustomServerButton.setVisible( false );
        processLogin( true );
        }
    }



void ExistingAccountPage::switchFields() {
    if( mFields[0]->isFocused() ) {
        mFields[1]->focus();
        }
    else if( mFields[1]->isFocused() ) {
        mFields[0]->focus();
        }
    }

    

void ExistingAccountPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }

    if( inASCII == 10 || inASCII == 13 ) {
        // enter key
        
        if( mKeyField.isFocused() ) {

            processLogin( true );
            
            return;
            }
        else if( mEmailField.isFocused() ) {
            switchFields();
            }
        }
    }



void ExistingAccountPage::specialKeyDown( int inKeyCode ) {
    if( inKeyCode == MG_KEY_DOWN ||
        inKeyCode == MG_KEY_UP ) {
        
        switchFields();
        return;
        }
    }



void ExistingAccountPage::processLogin( char inStore ) {
    if( userEmail != NULL ) {
        delete [] userEmail;
        }
    userEmail = mEmailField.getText();
        
    if( accountKey != NULL ) {
        delete [] accountKey;
        }
    accountKey = mKeyField.getText();

    if( !gamePlayingBack ) {
        
        if( inStore ) {
            SettingsManager::setSetting( "email", userEmail );
            SettingsManager::setSetting( "accountKey", accountKey );
            }
        else {
            SettingsManager::setSetting( "email", "" );
            SettingsManager::setSetting( "accountKey", "" );
            }
        }
    
                
    setSignal( "done" );
    }



void ExistingAccountPage::draw( doublePair inViewCenter, 
                                double inViewSize ) {
    
    
    if( !mFPSMeasureDone ) {
        double timePassed = game_getCurrentTime() - mPageActiveStartTime;
        double settleTime = 0.1;

        if ( timePassed > settleTime ) {
            mFramesCounted ++;
            }
        
        if( timePassed > 1 + settleTime ) {
            double fps = mFramesCounted / ( timePassed - settleTime );
            int targetFPS = 
                SettingsManager::getIntSetting( "targetFrameRate", -1 );
            char fpsFailed = true;
            
            if( targetFPS != -1 ) {
                
                double diff = fabs( fps - targetFPS );
                
                if( diff / targetFPS > 0.1 ) {
                    // more than 10% off

                    fpsFailed = true;
                    }
                else {
                    // close enough
                    fpsFailed = false;
                    }
                }

            if( !fpsFailed ) {
                mLoginButton.setVisible( true );
                }
            else {
                // show error message

                setStatus( "fpsErrorLogin", true );
                setStatusPositiion( true );
                mRedetectButton.setVisible( true );
                }
            

            mFPSMeasureDone = true;
            }
        }
    

    setDrawColor( 1, 1, 1, 1 );
    

    doublePair pos = { -9, -225 };
    
    drawSprite( instructionsSprite, pos );
    }

