
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


    // first, split into words
    SimpleVector<char*> words;
    
    // -1 if not present, or index in word
    SimpleVector<int> cursorInWord;
    

    int index = 0;
    
    int textLen = strlen( mText );
    
    while( index < textLen ) {
        // word includes all spaces afterward, up to first non-space
        // copied this behavior from FireFox text area

        while( index < textLen && mText[ index ] == '\r' ) {
            // newlines are separate words
            words.push_back( autoSprintf( "\n" ) );
            
            if( mCursorPosition == index ) {
                cursorInWord.push_back( 0 );
                }
            else {
                cursorInWord.push_back( -1 );
                }
            index++;
            }

        SimpleVector<char> thisWord;
        int thisWordCursorPos = -1;
        
        while( index < textLen && mText[ index ] != ' ' &&  
               mText[ index ] != '\r' ) {
            
            if( mCursorPosition == index ) {
                thisWordCursorPos = thisWord.size();
                }

            thisWord.push_back( mText[ index ] );
        
            
            index ++;
            }

        while( index < textLen && mText[ index ] == ' ' ) {
            if( mCursorPosition == index ) {
                thisWordCursorPos = thisWord.size();
                }
            thisWord.push_back( mText[ index ] );
            
            
            index ++;
            }

        words.push_back( thisWord.getElementString() );
        cursorInWord.push_back( thisWordCursorPos );
        }
    
    // now split words into lines
    SimpleVector<char*> lines;
    
    // same as case for cursor in word, with -1 if cursor not in line
    SimpleVector<int> cursorInLine;
    
    index = 0;
    
    while( index < words.size() ) {
        
        SimpleVector<char> thisLine;
        int thisLineCursorPos = -1;

        double lineLength = 0;
        
        while( index < words.size()
               && 
               strcmp( words.getElementDirect( index ), "\n" ) != 0
               &&
               lineLength + 
               mFont->measureString( words.getElementDirect( index ) ) < 
               mWide ) {

            int oldNumChars = thisLine.size();
            
            thisLine.appendElementString( words.getElementDirect( index ) );
            
            
            if( cursorInWord.getElementDirect( index ) != -1 ) {
                thisLineCursorPos = 
                    oldNumChars + cursorInWord.getElementDirect( index );
                }
            

            char *lineText = thisLine.getElementString();
            
            lineLength = mFont->measureString( lineText );
            
            delete [] lineText;

            index ++;
            }

        lines.push_back( thisLine.getElementString() );
        cursorInLine.push_back( thisLineCursorPos );
        
        if( index < words.size() &&
            strcmp( words.getElementDirect( index ), "\n" ) == 0 ) {
            // eat one newline per line, possibly 
            index ++;
            }
        }
    

    char anyLineHasCursor = false;
    for( int i=0; i<lines.size(); i++ ) {
        if( cursorInLine.getElementDirect( i ) != -1 ) {
            anyLineHasCursor = true;
            break;
            }
        }
    
    if( lines.size() == 0 ) {
        lines.push_back( autoSprintf( "" ) );
        cursorInLine.push_back( 0 );
        }
    
    if( !anyLineHasCursor ) {
        // stick cursor at end of last line
        char *lastLine = lines.getElementDirect( lines.size() - 1 );
    
        *( cursorInLine.getElement( lines.size() - 1 ) ) = strlen( lastLine );
        }
    


    words.deallocateStringElements();

    
    pos.x -= mWide / 2;
    
    pos.y += mHigh / 2;
    pos.y -= mFont->getFontHeight() * .5;
    
    for( int i=0; i<lines.size(); i++ ) {

        setDrawColor( 1, 1, 1, 1 );

        mFont->drawString( lines.getElementDirect( i ), pos, alignLeft );

        if( cursorInLine.getElementDirect( i ) != -1 ) {
            
            // okay to modify it without copying
            char *beforeCursor = lines.getElementDirect( i );
            
            beforeCursor[ cursorInLine.getElementDirect( i ) ] = '\0';
            
            setDrawColor( 0, 0, 0, 0.5 );
        
            int cursorXOffset = mFont->measureString( beforeCursor );
            
            

            drawRect( pos.x + cursorXOffset, 
                      pos.y - mFont->getFontHeight() / 2,
                      pos.x + cursorXOffset + pixWidth, 
                      pos.y + mFont->getFontHeight() / 2 );
            
            }
        
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
