#include "TextButton.h"

#include "minorGems/util/stringUtils.h"



TextButton::TextButton( Font *inDisplayFont, 
                        double inX, double inY,
                        const char *inLabelText ) 
        : Button( inX, inY, 
                  // leave w and h unset for now
                  0, 0,
                  // pix width
                  inDisplayFont->getFontHeight() / 8 ), 
          mFont( inDisplayFont ),
          mLabelText( NULL ) {

    // this will set button width and height
    setLabelText( inLabelText );
    }



TextButton::~TextButton() {
    delete [] mLabelText;
    }



void TextButton::setLabelText( const char *inLabelText ) {
    if( mLabelText != NULL ) {
        delete [] mLabelText;
        }
    
    mLabelText = stringDuplicate( inLabelText );
    

    // button width
    mWide = mFont->measureString( inLabelText ) + mFont->getFontHeight();
    
    // button height
    mHigh = 2 * mFont->getFontHeight();
    }




        
void TextButton::drawContents() {
    // leave draw color set by Button
    
    doublePair textPos = { 0, 0 };
    
    mFont->drawString( mLabelText, textPos, alignCenter );
    }

