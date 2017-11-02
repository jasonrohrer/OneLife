
#include "TextArea.h"




TextArea::TextArea( Font *inDisplayFont, 
                    double inX, double inY, double inWide, double inHigh,
                    char inForceCaps,
                    const char *inLabelText,
                    const char *inAllowedChars,
                    const char *inForbiddenChars ) 
        : TextField( inDisplayFont, inX, inY, 1, inForceCaps, inLabelText,
                     inAllowedChars, inForbiddenChars ) {
    
    }
        


void TextArea::draw() {
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };
    
    mFont->drawString( mText, pos, alignLeft );
    }



void TextArea::specialKeyDown( int inKeyCode ) {
    TextField::specialKeyDown( inKeyCode );
    }



void TextArea::specialKeyUp( int inKeyCode ) {
    TextField::specialKeyUp( inKeyCode );
    }
