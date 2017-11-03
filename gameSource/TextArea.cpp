
#include "TextArea.h"

#include "minorGems/game/drawUtils.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"

#include <math.h>


TextArea::TextArea( Font *inDisplayFont, 
                    double inX, double inY, double inWide, double inHigh,
                    char inForceCaps,
                    const char *inLabelText,
                    const char *inAllowedChars,
                    const char *inForbiddenChars ) 
        : TextField( inDisplayFont, inX, inY, 1, inForceCaps, inLabelText,
                     inAllowedChars, inForbiddenChars ),
          mWide( inWide ), mHigh( inHigh ),
          mCurrentLine( 0 ),
          mRecomputeCursorPositions( false ),
          mLastComputedCursorText( stringDuplicate( "" ) ) {
    
    }


        
TextArea::~TextArea() {
    delete [] mLastComputedCursorText;
    }



void TextArea::draw() {
    
    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };

    double pixWidth = mCharWidth / 8;
    
    drawRect( pos, mWide / 2 + 3 * pixWidth, mHigh / 2 + 3 * pixWidth );

    setDrawColor( 0.25, 0.25, 0.25, 1 );

    drawRect( pos, mWide / 2 + 2 * pixWidth, mHigh / 2 + 2 * pixWidth );


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
        
        char *wordString = thisWord.getElementString();
        
        if( mFont->measureString( wordString ) < mWide ) {
            words.push_back( wordString );
            cursorInWord.push_back( thisWordCursorPos );
            }
        else {
            delete [] wordString;
            
            // need to break up this long word
            SimpleVector<char> curSplitWord;
            int cursorOffset = 0;

            int curSplitWordCursorPos = -1;

            for( int i=0; i<thisWord.size(); i++ ) {
                
                curSplitWord.push_back( thisWord.getElementDirect( i ) );

                char *curTestWord = curSplitWord.getElementString();
                
                
                if( i == thisWordCursorPos ) {
                    curSplitWordCursorPos = i - cursorOffset;
                    }

                if( mFont->measureString( curTestWord ) < mWide ) {
                    // keep going
                    }
                else {
                    // too long

                    curSplitWord.deleteElement( curSplitWord.size() - 1 );
                    
                    char *finalSplitWord = curSplitWord.getElementString();
                    
                    words.push_back( finalSplitWord );
                    cursorInWord.push_back( curSplitWordCursorPos );

                    curSplitWord.deleteAll();
                    curSplitWordCursorPos = -1;
                    
                    cursorOffset += strlen( finalSplitWord );
                    
                    // add the too-long character to the start of the 
                    // next working split word
                    curSplitWord.push_back( thisWord.getElementDirect( i ) );
                    }
                delete [] curTestWord;
                }

            if( curSplitWord.size() > 0 ) {
                char *finalSplitWord = curSplitWord.getElementString();
                words.push_back( finalSplitWord );
                cursorInWord.push_back( curSplitWordCursorPos );
                }
            
            }
        
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
        
        if( index < words.size() &&
            strcmp( words.getElementDirect( index ), "\n" ) == 0 ) {
            // eat one newline per line, possibly 
            
            if( cursorInWord.getElementDirect( index ) != -1 ) {
                // cursor in newline word
                
                // put cursor at end of line
                thisLineCursorPos = thisLine.size();
                }
            
            index ++;
            }
        
        cursorInLine.push_back( thisLineCursorPos );
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


    int firstLine = 0;
    int lastLine = lines.size() - 1;
    
    int lineWithCursor = 0;
    
    for( int i=0; i<lines.size(); i++ ) {
        if( cursorInLine.getElementDirect( i ) != -1 ) {
            lineWithCursor = i;
            break;
            }
        }
    
    int linesPossible = floor( mHigh / mFont->getFontHeight() );
    
    if( lines.size() > linesPossible ) {
        int linesBeforeCursor = linesPossible / 2;
        int linesAfterCursor = linesPossible - linesBeforeCursor - 1;
            
        if( lineWithCursor <= linesBeforeCursor ) {
            // show beginning
            lastLine = linesPossible - 1;
            }
        else if( lines.size() - 1 - lineWithCursor <= linesAfterCursor ) {
            // show end
            firstLine = lines.size() - linesPossible;
            }
        else {
            //firstLine = lines.size() - linesPossible;
            // cursor in middle
            
            firstLine = lineWithCursor - linesBeforeCursor;
            lastLine = lineWithCursor + linesAfterCursor;
            }
        
        }
    


    for( int i=firstLine; i<=lastLine; i++ ) {

        setDrawColor( 1, 1, 1, 1 );

        mFont->drawString( lines.getElementDirect( i ), pos, alignLeft );

        if( cursorInLine.getElementDirect( i ) != -1 ) {
            
            char *beforeCursor = stringDuplicate( lines.getElementDirect( i ) );
            
            beforeCursor[ cursorInLine.getElementDirect( i ) ] = '\0';
            
            setDrawColor( 0, 0, 0, 0.5 );
        
            double cursorXOffset = mFont->measureString( beforeCursor );
            
            double extra = 0;
            if( cursorXOffset == 0 ) {
                extra = -pixWidth;
                }
            
            delete [] beforeCursor;
            
            drawRect( pos.x + cursorXOffset + extra, 
                      pos.y - mFont->getFontHeight() / 2,
                      pos.x + cursorXOffset + pixWidth + extra, 
                      pos.y + mFont->getFontHeight() / 2 );
            
            
            mCurrentLine = i;

            if( mRecomputeCursorPositions ||
                strcmp( mLastComputedCursorText, mText ) != 0 ) {
                
                // recompute cursor offsets for every line
                
                delete [] mLastComputedCursorText;
                mLastComputedCursorText = stringDuplicate( mText );
                mRecomputeCursorPositions = false;
                
                mCursorTargetPositions.deleteAll();
                
                int totalLineLengthSoFar = 0;
                
                for( int j=0; j<lines.size(); j++ ) {
                    
                    totalLineLengthSoFar += 
                        strlen( lines.getElementDirect( j ) );
                    
                    int cursorPos = totalLineLengthSoFar;
                    
                    char *line = 
                        stringDuplicate( lines.getElementDirect( j ) );
                    int remainingLength = strlen( line );
                    
                    double bestUpDiff = 9999999;
                
                    while( fabs( mFont->measureString( line ) - 
                                 cursorXOffset ) < bestUpDiff ) {
                    
                        bestUpDiff = fabs( mFont->measureString( line ) - 
                                           cursorXOffset );
                        cursorPos --;
                        remainingLength --;
                        
                        line[ remainingLength ] = '\0';
                        }
                    
                    if( cursorPos < totalLineLengthSoFar - 1 ) {
                        // not right at end of line
                        
                        // give it a nudge to make movement look better
                        cursorPos += 1;
                        }
                    
                    mCursorTargetPositions.push_back( cursorPos );
                    }
                }
            }
        
        pos.y -= mFont->getFontHeight();
        }
    
    lines.deallocateStringElements();
    }




void TextArea::specialKeyDown( int inKeyCode ) {
    TextField::specialKeyDown( inKeyCode );

     if( !mFocused ) {
        return;
        }

     if( inKeyCode == MG_KEY_RIGHT ||
         inKeyCode == MG_KEY_LEFT ) {
         mRecomputeCursorPositions = true;
         }
         
    
    switch( inKeyCode ) {
        case MG_KEY_UP:
            if( ! mIgnoreArrowKeys ) {
                if( mCurrentLine > 0 ) {
                    mCurrentLine--;
                    mCursorPosition = 
                        mCursorTargetPositions.getElementDirect( mCurrentLine );
                    }
                else {
                    mCursorPosition = 0;
                    mRecomputeCursorPositions = true;
                    }
                }
            break;
        case MG_KEY_DOWN:
            if( ! mIgnoreArrowKeys ) {
                if( mCurrentLine < mCursorTargetPositions.size() - 1 ) {
                    mCurrentLine++;
                    mCursorPosition = 
                        mCursorTargetPositions.getElementDirect( mCurrentLine );
                    }
                else {
                    mCursorPosition = strlen( mText );
                    mRecomputeCursorPositions = true;
                    }
                }
            break;
        default:
            break;
        }
    }



void TextArea::specialKeyUp( int inKeyCode ) {
    TextField::specialKeyUp( inKeyCode );
    }
