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
          mCopySteamKeyButton( mainFont, 
                               0, -128, translate( "ahapCopySteamKey" ) ),
          mOKButton( mainFont, 0, -300, 
                     translate( "okay" ) ) {
    
    
    setButtonStyle( &mCopyAccountURLButton );
    setButtonStyle( &mCopySteamKeyButton );
    setButtonStyle( &mOKButton );

    addComponent( &mCopyAccountURLButton );
    addComponent( &mCopySteamKeyButton );
    addComponent( &mOKButton );

    mCopyAccountURLButton.addActionListener( this );
    mCopySteamKeyButton.addActionListener( this );
    mOKButton.addActionListener( this );
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
    }



void AHAPResultPage::makeActive( char inFresh ) {
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
    
    pos.y += 64;
    

    drawMessage( ahapSteamKey, pos );


    pos = mCopySteamKeyButton.getPosition();
    
    pos.y -= 128;

    

    }

