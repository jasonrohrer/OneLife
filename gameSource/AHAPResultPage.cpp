#include "AHAPResultPage.h"

#include "message.h"
#include "buttonStyle.h"

#include "minorGems/game/game.h"



extern char *ahapAccountURL;
extern char *ahapSteamKey;

extern Font *mainFont;



AHAPResultPage::AHAPResultPage()
        : mCopyAccountURLButton( mainFont, 
                                 0, 64, translate( "ahapCopyURL" ) ),
          mSteamKeyField( mainFont, 0, -64, 15, true,
                          translate( "steamKey" ),
                          "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-" ),
          mCopySteamKeyButton( mainFont, 
                               0, -128, translate( "ahapCopySteamKey" ) ),
          mOKButton( mainFont, 0, -300, 
                     translate( "okay" ) ) {
    
    
    setButtonStyle( &mCopyAccountURLButton );
    setButtonStyle( &mCopySteamKeyButton );
    setButtonStyle( &mOKButton );

    addComponent( &mCopyAccountURLButton );
    addComponent( &mSteamKeyField );
    addComponent( &mCopySteamKeyButton );
    addComponent( &mOKButton );

    mCopyAccountURLButton.addActionListener( this );
    mSteamKeyField.addActionListener( this );
    mCopySteamKeyButton.addActionListener( this );
    mOKButton.addActionListener( this );

    mSteamKeyField.setFireOnAnyTextChange( true );
    mSteamKeyField.setCursorHidden( true );
    }


        
AHAPResultPage::~AHAPResultPage() {
    }



void AHAPResultPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mOKButton ) {
        setSignal( "done" );
        }
    else if( inTarget == &mCopyAccountURLButton ) {
        setClipboardText( ahapAccountURL );
        }
    else if( inTarget == &mCopySteamKeyButton ) {
        setClipboardText( ahapSteamKey );
        }
    else if( inTarget == &mSteamKeyField ) {
        mSteamKeyField.setText( ahapSteamKey );
        }
    }



void AHAPResultPage::makeActive( char inFresh ) {
    mSteamKeyField.setText( ahapSteamKey );
    mSteamKeyField.setContentsHidden( true );
    }

        

void AHAPResultPage::draw( doublePair inViewCenter, 
                           double inViewSize ) {
    
    doublePair pos = mCopyAccountURLButton.getPosition();
    
    pos.y += 200;

    drawMessage( translate( "youWin" ), pos );


    pos = mCopyAccountURLButton.getPosition();

    pos.y += 64;

    drawMessage( translate( "ahapAccess" ), pos );


    pos = mCopySteamKeyButton.getPosition();
    }

