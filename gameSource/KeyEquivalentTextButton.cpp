#include "KeyEquivalentTextButton.h"


#include "minorGems/game/game.h"



KeyEquivalentTextButton::KeyEquivalentTextButton( Font *inDisplayFont, 
                                                  double inX, double inY,
                                                  const char *inLabelText,
                                                  unsigned char inKeyA, 
                                                  unsigned char inKeyB )
        : TextButton( inDisplayFont, inX, inY, inLabelText ),
          mKeyA( inKeyA ), mKeyB( inKeyB ) {

    }


void KeyEquivalentTextButton::keyDown( unsigned char inASCII ) {
    
    char commandDown = isCommandKeyDown();

    // 26 is "SUB" which is ctrl-z on some platforms
    // 24 is "CAN" which is ctrl-x on some platforms
    // 23 is "ETB" which is ctrl-w on some platforms
    // 22 is "SYN" which is ctrl-v on some platforms
    //  5 is "ENQ" which is ctrl-e on some platforms
    //  1 is "SOH" which is ctrl-a on some platforms
    // but if alt-z or meta-z held, SUB won't be passed through
    if( ( (mKeyA == 'z' ) && inASCII == 26 ) ||
        ( (mKeyA == 'x' ) && inASCII == 24 ) ||
        ( (mKeyA == 'w' ) && inASCII == 23 ) ||
        ( (mKeyA == 'v' ) && inASCII == 22 ) ||
        ( (mKeyA == 'e' ) && inASCII ==  5 ) ||
        ( (mKeyA == 'a' ) && inASCII ==  1 ) ||
        ( commandDown && ( inASCII == mKeyA || inASCII == mKeyB ) ) ) {
        
        fireActionPerformed( this );
        }
    
    }



