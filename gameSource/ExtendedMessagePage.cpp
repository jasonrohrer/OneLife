#include "ExtendedMessagePage.h"

#include "buttonStyle.h"
#include "message.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/game.h"

#include "minorGems/util/stringUtils.h"


extern Font *mainFont;


extern char *userEmail;
extern char *accountKey;





ExtendedMessagePage::ExtendedMessagePage()
        : mOKButton( mainFont, 0, -128, 
                     translate( "okay" ) ),
          mMessageKey( "" )  {

    addComponent( &mOKButton );
    
    setButtonStyle( &mOKButton );
    
    mOKButton.addActionListener( this );
    }



void ExtendedMessagePage::setMessageKey( const char *inMessageKey ) {
    mMessageKey = inMessageKey;
    }



        
void ExtendedMessagePage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mOKButton ) {
        setSignal( "done" );
        }
    }



void ExtendedMessagePage::draw( doublePair inViewCenter, 
                                  double inViewSize ) {
    
    doublePair pos = { 0, 200 };
    
    drawMessage( mMessageKey, pos );
    }

