#include "AHAPResultPage.h"

#include "message.h"
#include "buttonStyle.h"
#include "musicPlayer.h"

#include "minorGems/game/game.h"

#include "minorGems/util/SettingsManager.h"



extern char *ahapAccountURL;
extern char *ahapSteamKey;

extern Font *mainFont;

extern float musicLoudness;



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
        setMusicLoudness( 0 );
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

    if( inFresh ) {

        instantStopMusic();

        setSoundLoudness( 1.0 );
        setMusicLoudness( musicLoudness );

        // this will trigger music_99.ogg
        restartMusic( 494.0, 1.0/60.0, true );
        }
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

