#include "ServicesPage.h"

#include "message.h"
#include "buttonStyle.h"
#include "accountHmac.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/crc32.h"

#include "minorGems/game/game.h"
#include "minorGems/crypto/hashes/sha1.h"


#include <stdio.h>



extern Font *mainFont;
extern char *userEmail;



ServicesPage::ServicesPage()
  : mChallengeField( mainFont, 0, 0, 16, false, 
                     translate( "serviceChallengeField" ),
                     NULL,
                     NULL ),
    mCopyEmailButton( mainFont, 0, 128 +60, translate( "servicesCopyEmail" ) ),
    mViewEmailButton( mainFont, -250, 128 +60, translate( "view" ) ),
    mPasteButton( mainFont, 0, 60, translate( "servicesPaste" ) ),
    mCopyHashButton( mainFont, 0, -128 -60, translate( "servicesCopyHash" ) ),
    mBackButton( mainFont, -542, -280, translate( "backButton" ) ) {
    
    mHashString = stringDuplicate( "" );

    setButtonStyle( &mCopyEmailButton );
    setButtonStyle( &mViewEmailButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mCopyHashButton );
    setButtonStyle( &mBackButton );
    
    addComponent( &mCopyEmailButton );
    addComponent( &mViewEmailButton );
    addComponent( &mPasteButton );
    addComponent( &mCopyHashButton );
    addComponent( &mBackButton );

    addComponent( &mChallengeField );
    
    mCopyEmailButton.addActionListener( this );
    mViewEmailButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mCopyHashButton.addActionListener( this );
    mBackButton.addActionListener( this );

    
    mChallengeField.addActionListener( this );
    
    mChallengeField.setFireOnAnyTextChange( true );

    mCopyHashButton.setVisible( false );
    }


        
ServicesPage::~ServicesPage() {
    delete [] mHashString;
    }



void ServicesPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mChallengeField ) {
        char *text = mChallengeField.getText();
        
        char *trimText = trimWhitespace( text );

        if( strcmp( trimText, "" ) == 0 ) {
            delete [] mHashString;
            mHashString = stringDuplicate( "" );
            
            mCopyHashButton.setVisible( false );
            }
        else {
            char *pureKey = getPureAccountKey();
            
            char *keyHash = hmac_sha1( pureKey, trimText );

            delete [] pureKey;

            delete [] mHashString;
            
            mHashString = keyHash;
            
            mCopyHashButton.setVisible( true );
            }
        delete [] text;
        delete [] trimText;
        }
    else if( inTarget == &mBackButton ) {
        setSignal( "back" );
        }
    else if( inTarget == &mCopyEmailButton ) {
        setClipboardText( userEmail );
        }
    else if( inTarget == &mViewEmailButton ) {
        if( mEmailVisible ) {
            mEmailVisible = false;
            mViewEmailButton.setLabelText( translate( "view" ) );
            }
        else {
            mEmailVisible = true;
            mViewEmailButton.setLabelText( translate( "hide" ) );
            }
        }
    else if( inTarget == &mPasteButton ) {
        char *text = getClipboardText();

        mChallengeField.setText( text );
        actionPerformed( &mChallengeField );

        delete [] text;
        }    
    else if( inTarget == &mCopyHashButton ) {
        setClipboardText( mHashString );
        }
    }



void ServicesPage::makeActive( char inFresh ) {
    mEmailVisible = false;
    mViewEmailButton.setLabelText( translate( "view" ) );
    }

        

void ServicesPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    doublePair pos = { 0, 278 };
    
    drawMessage( translate( "servicesTip" ), pos );


    TextAlignment prev = getMessageAlign();
    


    pos.x = -400;
    pos.y = 128;
    

    setMessageAlign( alignRight );
    
    drawMessage( translate( "serviceEmailLabel" ), pos );
    

    pos.x += 16;
    
    setMessageAlign( alignLeft );

    if( mEmailVisible ) {
        if( userEmail != NULL ) {
            drawMessage( userEmail, pos );
            }
        }
    else {
        drawMessage( "xxxxxxxxxxxxxxxxxxxxx", pos );
        }
    


    pos.x = -400;
    pos.y = -128;
    

    setMessageAlign( alignRight );
    
    drawMessage( translate( "serviceHashLabel" ), pos );
    

    pos.x += 16;
    
    setMessageAlign( alignLeft );
    
    drawMessage( mHashString, pos );







    setMessageAlign( prev );
    }

