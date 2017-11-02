
#include "TextArea.h"

#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"



TextArea::TextArea( Font *inDisplayFont, 
                    double inX, double inY, double inWide, double inHigh,
                    char inForceCaps,
                    const char *inLabelText,
                    const char *inAllowedChars,
                    const char *inForbiddenChars ) 
        : TextField( inDisplayFont, inX, inY, 1, inForceCaps, inLabelText,
                     inAllowedChars, inForbiddenChars ),
          mWide( inWide ), mHigh( inHigh ) {
    
    }
        


void TextArea::draw() {
    
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };

    double pixWidth = mCharWidth / 8;
    
    drawRect( pos, mWide / 2 + 2 * pixWidth, mHigh / 2 + 2 * pixWidth );

    setDrawColor( 0.25, 0.25, 0.25, 1 );

    drawRect( pos, mWide / 2 + pixWidth, mHigh / 2 + pixWidth );

    setDrawColor( 1, 1, 1, 1 );

    // first, split into words
    SimpleVector<char*> words;
    
    int index = 0;
    
    int textLen = strlen( mText );
    
    while( index < textLen ) {
        // word includes all spaces afterward, up to first non-space
        // copied this behavior from FireFox text area

        while( index < textLen && mText[ index ] == '\r' ) {
            // newlines are separate words
            words.push_back( autoSprintf( "\n" ) );
            index++;
            }

        SimpleVector<char> thisWord;
        
        while( index < textLen && mText[ index ] != ' ' &&  
               mText[ index ] != '\r' ) {
            
            thisWord.push_back( mText[ index ] );
            index ++;
            }

        while( index < textLen && mText[ index ] == ' ' ) {
            thisWord.push_back( mText[ index ] );
            index ++;
            }

        words.push_back( thisWord.getElementString() );
        }
    
    // now split words into lines
    SimpleVector<char*> lines;
    
    index = 0;
    
    while( index < words.size() ) {
        
        SimpleVector<char> thisLine;

        double lineLength = 0;
        
        while( index < words.size()
               && 
               strcmp( words.getElementDirect( index ), "\n" ) != 0
               &&
               lineLength + 
               mFont->measureString( words.getElementDirect( index ) ) < 
               mWide ) {
            
            thisLine.appendElementString( words.getElementDirect( index ) );
            
            char *lineText = thisLine.getElementString();
            
            lineLength = mFont->measureString( lineText );
            
            delete [] lineText;

            index ++;
            }

        lines.push_back( thisLine.getElementString() );
        
        if( index < words.size() &&
            strcmp( words.getElementDirect( index ), "\n" ) == 0 ) {
            // eat one newline per line, possibly 
            index ++;
            }
        }

    words.deallocateStringElements();

    
    pos.x -= mWide / 2;
    
    pos.y += mHigh / 2;
    pos.y -= mFont->getFontHeight() * .5;
    
    for( int i=0; i<lines.size(); i++ ) {
        
        mFont->drawString( lines.getElementDirect( i ), pos, alignLeft );
        
        pos.y -= mFont->getFontHeight();
        }
    
    lines.deallocateStringElements();
    }




void TextArea::specialKeyDown( int inKeyCode ) {
    TextField::specialKeyDown( inKeyCode );
    }



void TextArea::specialKeyUp( int inKeyCode ) {
    TextField::specialKeyUp( inKeyCode );
    }
