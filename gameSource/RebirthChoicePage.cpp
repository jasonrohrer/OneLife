#include "RebirthChoicePage.h"

#include "buttonStyle.h"
#include "message.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;





RebirthChoicePage::RebirthChoicePage()
        : mQuitButton( mainFont, -150, -128, 
                       translate( "quit" ) ),
          mRebornButton( mainFont, 150, -128, 
                         translate( "reborn" ) ) {

    addComponent( &mQuitButton );
    addComponent( &mRebornButton );
    
    setButtonStyle( &mQuitButton );
    setButtonStyle( &mRebornButton );
    
    mQuitButton.addActionListener( this );
    mRebornButton.addActionListener( this );
    }



void RebirthChoicePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mQuitButton ) {
        setSignal( "quit" );
        }
    else if( inTarget == &mRebornButton ) {
        setSignal( "reborn" );
        }
    }



void RebirthChoicePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    //doublePair pos = { 0, 200 };
    
    // no message for now
    //drawMessage( "", pos );
    }

