
#include "TextArea.h"

#include "minorGems/game/drawUtils.h"
#include "minorGems/game/game.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/graphics/openGL/KeyboardHandlerGL.h"

#include <math.h>
#include <stdlib.h>

#include "spellCheck.h"

extern double frameRateFactor;


TextArea::TextArea( Font *inLabelFont, Font *inDisplayFont, 
                    double inX, double inY, double inWide, double inHigh,
                    char inForceCaps,
                    const char *inLabelText,
                    const char *inAllowedChars,
                    const char *inForbiddenChars ) 
        : TextField( inDisplayFont, inX, inY, 1, inForceCaps, inLabelText,
                     inAllowedChars, inForbiddenChars ),
          mWide( inWide ), mHigh( inHigh ),
          mLabelFont( inLabelFont ),
          mSpellCheckOn( false ),
          mCurrentLine( 0 ),
          mRecomputeCursorPositions( false ),
          mLastComputedCursorText( stringDuplicate( "" ) ),
          mVertSlideOffset( 0 ),
          mSmoothSlidingUp( false ),
          mSmoothSlidingDown( false ),
          mTopShadingFade( 0 ),
          mBottomShadingFade( 0 ),
          mMaxLinesShown( 0 ),
          mFirstVisibleLine( 0 ), 
          mLastVisibleLine( 0 ),
          mPointerDownInside( false ),
          mStepsSinceTextChanged( 0 ),
          mEverDrawn( false ) {
    
    mLastDrawnText = stringDuplicate( "" );
    
    mSnapMove = false;
    
    clearVertArrowRepeat();

    setShiftArrowsCanSelect( true );
    }


        
TextArea::~TextArea() {
    delete [] mLastComputedCursorText;
    mLineStrings.deallocateStringElements();
    delete [] mLastDrawnText;
    }



void TextArea::enableSpellCheck( char inSpellCheckOn ) {
    mSpellCheckOn = inSpellCheckOn;
    }



void TextArea::step() {
    TextField::step();
    
    for( int i=0; i<2; i++ ) {
        
        if( mHoldVertArrowSteps[i] > -1 ) {
            mHoldVertArrowSteps[i] ++;
            
            int stepsBetween = sDeleteFirstDelaySteps;
        
            if( mFirstVertArrowRepeatDone[i] ) {
                // vert arrows move cursor more slowly
                stepsBetween = sDeleteNextDelaySteps * 2 ;
                }
        
            if( mHoldVertArrowSteps[i] > stepsBetween ) {
                // arrow repeat
                mHoldVertArrowSteps[i] = 0;
                mFirstVertArrowRepeatDone[i] = true;
            
                switch( i ) {
                    case 0:
                        upHit();
                        break;
                    case 1:
                        downHit();
                        break;
                    }
                }
            }
        }

    if( mVertSlideOffset > 0 ) {
        double speedFactor = 4;
        
        if( mHoldVertArrowSteps[0] == -1 ) {
            // arrow not held down    
            // ease toward end of move
            if( mVertSlideOffset <= mFont->getFontHeight() / 4 ) {
                speedFactor = 8;
                }
            if( mVertSlideOffset <= mFont->getFontHeight() / 8 ) {
                speedFactor = 16;
                }
            if( mVertSlideOffset <= mFont->getFontHeight() / 16 ) {
                speedFactor = 32;
                }
            }
        else {
            speedFactor = 5;
            }

        mVertSlideOffset -= 
            mFont->getFontHeight() / speedFactor * frameRateFactor;
        
        if( mVertSlideOffset < 0 ) {
            mVertSlideOffset = 0;
            }
        }
    else if( mVertSlideOffset < 0 ) {

        double speedFactor = 4;
        
        if( mHoldVertArrowSteps[1] == -1 ) {
            // arrow not held down    
            // ease toward end of move
            if( -mVertSlideOffset <= mFont->getFontHeight() / 4 ) {
                speedFactor = 8;
                }
            if( -mVertSlideOffset <= mFont->getFontHeight() / 8 ) {
                speedFactor = 16;
                }
            if( -mVertSlideOffset <= mFont->getFontHeight() / 16 ) {
                speedFactor = 32;
                }
            }
        else {
            speedFactor = 5;
            }
        
        mVertSlideOffset += 
            mFont->getFontHeight() / speedFactor * frameRateFactor;
        
        if( mVertSlideOffset > 0 ) {
            mVertSlideOffset = 0;
            }
        }
    
    mStepsSinceTextChanged++;
    }



typedef struct SelRect {
        double startX, startY, endX, endY;
    } SelRect;




void TextArea::draw() {

    int stepsPerFlash = 30 / frameRateFactor;
    
    int flashState = mCursorFlashSteps / stepsPerFlash;
    
    char cursorFlashOn = true;
    
    if( flashState % 2 == 1 ) {
        cursorFlashOn = false;
        }

    
    if( mFocused ) {    
        setDrawColor( 1, 1, 1, 1 );
        }
    else {
        setDrawColor( 0.5, 0.5, 0.5, 1 );
        }

    doublePair pos = { 0, 0 };

    double pixWidth = mCharWidth / 8;
    
    drawRect( pos, mWide / 2 + 3 * pixWidth, mHigh / 2 + 3 * pixWidth );

    
    setDrawColor( 1, 1, 1, 1 );
        
    doublePair labelPos = pos;
    labelPos.y += mHigh / 2 + 7 * pixWidth;
    
    labelPos.x -= mWide / 2 + 2 * pixWidth;
    
    mLabelFont->drawString( mLabelText, labelPos, alignLeft );
    


    setDrawColor( 0.25, 0.25, 0.25, 1 );

    startAddingToStencil( true, true );
    drawRect( pos, mWide / 2 + 2 * pixWidth, mHigh / 2 + 2 * pixWidth );

    startDrawingThroughStencil();

    // first, split into words
    SimpleVector<char*> words;
    
    // -1 if not present, or index in word
    SimpleVector<int> cursorInWord;
    SimpleVector<int> selectionStartInWord;
    SimpleVector<int> selectionEndInWord;
    

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

            if( mSelectionStart == index ) {
                selectionStartInWord.push_back( 0 );
                }
            else {
                selectionStartInWord.push_back( -1 );
                }

            if( mSelectionEnd == index ) {
                selectionEndInWord.push_back( 0 );
                }
            else {
                selectionEndInWord.push_back( -1 );
                }
            index++;
            }

        SimpleVector<char> thisWord;
        int thisWordCursorPos = -1;
        int thisWordSelectionStartPos = -1;
        int thisWordSelectionEndPos = -1;
        
        while( index < textLen && mText[ index ] != ' ' &&  
               mText[ index ] != '\r' ) {
            
            if( mCursorPosition == index ) {
                thisWordCursorPos = thisWord.size();
                }
            if( mSelectionStart == index ) {
                thisWordSelectionStartPos = thisWord.size();
                }
            if( mSelectionEnd == index ) {
                thisWordSelectionEndPos = thisWord.size();
                }

            thisWord.push_back( mText[ index ] );
        
            
            index ++;
            }

        while( index < textLen && mText[ index ] == ' ' ) {
            if( mCursorPosition == index ) {
                thisWordCursorPos = thisWord.size();
                }
            if( mSelectionStart == index ) {
                thisWordSelectionStartPos = thisWord.size();
                }
            if( mSelectionEnd == index ) {
                thisWordSelectionEndPos = thisWord.size();
                }
            thisWord.push_back( mText[ index ] );
            
            
            index ++;
            }
        
        char *wordString = thisWord.getElementString();
        
        if( mFont->measureString( wordString ) < mWide ) {
            words.push_back( wordString );
            cursorInWord.push_back( thisWordCursorPos );
            selectionStartInWord.push_back( thisWordSelectionStartPos );
            selectionEndInWord.push_back( thisWordSelectionEndPos );            
            }
        else {
            delete [] wordString;
            
            // need to break up this long word
            SimpleVector<char> curSplitWord;
            int cursorOffset = 0;

            int curSplitWordCursorPos = -1;
            int curSplitWordSelectionStartPos = -1;
            int curSplitWordSelectionEndPos = -1;

            for( int i=0; i<thisWord.size(); i++ ) {
                
                curSplitWord.push_back( thisWord.getElementDirect( i ) );

                char *curTestWord = curSplitWord.getElementString();
                
                
                if( i == thisWordCursorPos ) {
                    curSplitWordCursorPos = i - cursorOffset;
                    }
                if( i == thisWordSelectionStartPos ) {
                    curSplitWordSelectionStartPos = i - cursorOffset;
                    }
                if( i == thisWordSelectionEndPos ) {
                    curSplitWordSelectionEndPos = i - cursorOffset;
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
                    
                    selectionStartInWord.push_back( 
                        curSplitWordSelectionStartPos );
                    
                    selectionEndInWord.push_back( 
                        curSplitWordSelectionEndPos );

                    curSplitWord.deleteAll();
                    curSplitWordCursorPos = -1;
                    curSplitWordSelectionStartPos = -1;
                    curSplitWordSelectionEndPos = -1;
                    
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
                selectionStartInWord.push_back( curSplitWordSelectionStartPos );
                selectionEndInWord.push_back( curSplitWordSelectionEndPos );
                }
            
            }
        
        }
    
    // now split words into lines
    SimpleVector<char*> lines;
    
    // same as case for cursor in word, with -1 if cursor not in line
    SimpleVector<int> cursorInLine;
    SimpleVector<int> selectionStartInLine;
    SimpleVector<int> selectionEndInLine;

    SimpleVector<char> newlineEatenInLine;
    
    index = 0;
    
    while( index < words.size() ) {
        
        SimpleVector<char> thisLine;
        int thisLineCursorPos = -1;
        int thisLineSelectionStartPos = -1;
        int thisLineSelectionEndPos = -1;

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
            if( selectionStartInWord.getElementDirect( index ) != -1 ) {
                thisLineSelectionStartPos = 
                    oldNumChars + 
                    selectionStartInWord.getElementDirect( index );
                }
            if( selectionEndInWord.getElementDirect( index ) != -1 ) {
                thisLineSelectionEndPos = 
                    oldNumChars + 
                    selectionEndInWord.getElementDirect( index );
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
            if( selectionStartInWord.getElementDirect( index ) != -1 ) {
                // start in newline word
                
                // put start at end of line
                thisLineSelectionStartPos = thisLine.size();
                }
            if( selectionEndInWord.getElementDirect( index ) != -1 ) {
                // end in newline word
                
                // put end at end of line
                thisLineSelectionEndPos = thisLine.size();
                }
            
            index ++;
        
            newlineEatenInLine.push_back( true );
            }
        else {
            newlineEatenInLine.push_back( false );
            }
        
        
        cursorInLine.push_back( thisLineCursorPos );
        selectionStartInLine.push_back( thisLineSelectionStartPos );
        selectionEndInLine.push_back( thisLineSelectionEndPos );
        }
    

    char anyLineHasCursor = false;
    char anyLineHasSelectionEnd = false;
    for( int i=0; i<lines.size(); i++ ) {
        if( cursorInLine.getElementDirect( i ) != -1 ) {
            anyLineHasCursor = true;
            break;
            }
        }

    int selStartLine = -1;
    int selEndLine = -1;
    
    for( int i=0; i<lines.size(); i++ ) {
        if( selectionStartInLine.getElementDirect( i ) != -1 ) {
            selStartLine = i;
            }
        if( selectionEndInLine.getElementDirect( i ) != -1 ) {
            anyLineHasSelectionEnd = true;
            selEndLine = i;
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

    if( !anyLineHasSelectionEnd && mSelectionEnd != -1 ) {
        // stick selection end at end of last line
        char *lastLine = lines.getElementDirect( lines.size() - 1 );
    
        *( selectionEndInLine.getElement( lines.size() - 1 ) ) = 
            strlen( lastLine );
        selEndLine = lines.size() - 1;
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
    
    mMaxLinesShown = linesPossible;

    int linesBeforeCursor = linesPossible / 2;
    int linesAfterCursor = linesPossible - linesBeforeCursor - 1;
    
    if( lines.size() > linesPossible ) {
            
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
    

    


    int drawFirstLine = firstLine;
    int drawLastLine = lastLine;
    
    if( firstLine > 0 ) {
        drawFirstLine --;
        pos.y += mFont->getFontHeight();
        }

    if( lastLine < lines.size() - 1 ) {
        drawLastLine ++;
        }


    char textChange = ( strcmp( mLastDrawnText, mText ) != 0 );
    
    delete [] mLastDrawnText;
    mLastDrawnText = stringDuplicate( mText );

    if( textChange ) {
        mStepsSinceTextChanged = 0;
        }
    
    // if same text, check for cursor changes that should
    // result in additional smooth movement
    if( !textChange && !mSnapMove )
    for( int i=firstLine; i<=lastLine; i++ ) {
        if( cursorInLine.getElementDirect( i ) != -1 ) {
            if( mCurrentLine != i && mVertSlideOffset == 0 && 
                abs( mCurrentLine - i ) < linesPossible ) {
                
                // switched lines through horizontal cursor movement
                // or mouse clicks
                
                // AND not already sliding
                // AND jump isn't too big
                
                if( i < mCurrentLine && 
                    ( mSmoothSlidingUp ||
                      lastLine != mLastVisibleLine ) ) {
                    mVertSlideOffset += 
                        mFont->getFontHeight() * ( mCurrentLine - i );
                    if( firstLine == 0 ) {
                        mVertSlideOffset -= 
                            mFont->getFontHeight() *
                            ( linesBeforeCursor - i );
                        }
                    if( mLastVisibleLine == lines.size() - 1 ) {
                        mVertSlideOffset -= 
                            mFont->getFontHeight() *
                            ( linesAfterCursor - 
                              ( lines.size() - 1 - mCurrentLine ) );
                        }
                    }
                else if( i > mCurrentLine && 
                         ( mSmoothSlidingDown ||
                           firstLine != mFirstVisibleLine ) ) {
                    mVertSlideOffset -= 
                        mFont->getFontHeight() * ( i - mCurrentLine );

                    if( lastLine == lines.size() - 1 ) {
                        mVertSlideOffset += 
                            mFont->getFontHeight() *
                            ( linesAfterCursor - ( lastLine - i ) );
                        }
                    if( mFirstVisibleLine == 0 ) {
                        mVertSlideOffset += 
                            mFont->getFontHeight() *
                            ( linesBeforeCursor - mCurrentLine );
                        }
                    }
                }
            }
        }


    if( firstLine > 0 && lastLine < lines.size() - 1 ) {
        mSmoothSlidingUp = true;
        mSmoothSlidingDown = true;
        }
    else if( firstLine == 0 && lineWithCursor == linesBeforeCursor &&
             lastLine < lines.size() - 1 ) {
        mSmoothSlidingUp = false;
        mSmoothSlidingDown = true;
        }
    else if( lastLine == lines.size() - 1 && 
             lineWithCursor == lines.size() - linesAfterCursor - 1 &&
             firstLine > 0 ) {
        mSmoothSlidingUp = true;
        mSmoothSlidingDown = false;
        }
    else {
        mSmoothSlidingUp = false;
        mSmoothSlidingDown = false;
        }


    
    if( mVertSlideOffset > mFont->getFontHeight() ) {
        
        drawLastLine += lrint( mVertSlideOffset / mFont->getFontHeight() );
        if( drawLastLine >= lines.size() ) {
            drawLastLine = lines.size() - 1;
            }
        }
    else if( mVertSlideOffset < - mFont->getFontHeight() ) {

        // diff is negative
        int diff = lrint( mVertSlideOffset / mFont->getFontHeight() );
        drawFirstLine += diff;

        pos.y -= mFont->getFontHeight() * diff;

        if( drawFirstLine < 0 ) {
            int extraDiff = 0 - drawFirstLine;
            
            pos.y -= mFont->getFontHeight() * extraDiff;
            
            drawFirstLine = 0;
            }
        }
    
                    

    pos.y += mVertSlideOffset;


    mFirstVisibleLine = firstLine;
    mLastVisibleLine = lastLine;
    
    SimpleVector<SelRect> selRects;

    for( int i=drawFirstLine; i<=drawLastLine; i++ ) {


        if( mSpellCheckOn ) {
            char *lineString = lines.getElementDirect( i );
            int lineLen = strlen( lineString );
            
            int wordStartIndex = 0;
            double beforeWordX = 0;
            
            while( wordStartIndex < lineLen ) {
                int wordEndIndex = wordStartIndex;
                
                while( wordEndIndex < lineLen &&
                       lineString[ wordEndIndex ] != ' ' ) {
                    wordEndIndex++;
                    }
                
                char *wordPointer = &( lineString[ wordStartIndex ] );
                int wordLen = wordEndIndex - wordStartIndex;
                
                char *wordCopy = stringDuplicate( wordPointer );
                wordCopy[ wordLen ] = '\0';
                
                char inDict = checkWord( wordCopy );

                int cursorPos = cursorInLine.getElementDirect( i );
                
                if( cursorPos != -1 &&
                    cursorPos >= wordStartIndex &&
                    cursorPos <= wordEndIndex &&
                    mStepsSinceTextChanged < 30 / frameRateFactor ) {
                    
                    // don't spell check word we're typing in
                    inDict = true;
                    }
                

                if( !inDict ) {
                    
                    double afterWordX = mFont->measureString( lineString,
                                                              wordEndIndex );
                    
                    setDrawColor( 1, 0, 0, 1.0 );
                    double y = pos.y - 
                        mFont->getFontHeight() / 4 - 
                        mFont->getFontHeight() / 16;
                    
                    drawRect( pos.x + beforeWordX,
                              y,
                              pos.x + afterWordX,
                              y + mFont->getFontHeight() / 16 );
                    }
                delete [] wordCopy;
                
                
                wordStartIndex = wordEndIndex;
                
                // skip spaces until start of next word
                while( wordStartIndex < lineLen &&
                       lineString[ wordStartIndex ] == ' ' ) {
                    wordStartIndex++;
                    }
                
                if( wordStartIndex < lineLen ) {
                    beforeWordX = mFont->measureString( lineString,
                                                        wordStartIndex );
                    }
                }
            }
        
        setDrawColor( 1, 1, 1, 1 );

        mFont->drawString( lines.getElementDirect( i ), pos, alignLeft );

        
        if( cursorInLine.getElementDirect( i ) != -1 ) {
            
            char *beforeCursor = stringDuplicate( lines.getElementDirect( i ) );
            
            beforeCursor[ cursorInLine.getElementDirect( i ) ] = '\0';
            
            if( cursorFlashOn ) {
                setDrawColor( 1, 1, 0, .75 );
                }
            else {
                setDrawColor( 0, 0, 0, .75 );
                }

            double cursorXOffset = mFont->measureString( beforeCursor );
            
            double extra = 0;
            if( cursorXOffset == 0 ) {
                extra = -pixWidth;
                }
            
            delete [] beforeCursor;
            
            if( mFocused && 
                ! isAnythingSelected() &&
                ! ( mSelectionStart != -1 && mPointerDownInside ) ) {    
                drawRect( pos.x + cursorXOffset + extra, 
                          pos.y - mFont->getFontHeight() / 2,
                          pos.x + cursorXOffset + pixWidth + extra, 
                          pos.y + 0.55 * mFont->getFontHeight() );
                }
            
            
            mCurrentLine = i;

            if( mRecomputeCursorPositions ||
                strcmp( mLastComputedCursorText, mText ) != 0 ) {
                
                // recompute cursor offsets for every line
                
                int totalTextLen = strlen( mText );

                delete [] mLastComputedCursorText;
                mLastComputedCursorText = stringDuplicate( mText );
                mRecomputeCursorPositions = false;
                
                mCursorTargetPositions.deleteAll();
                mCursorTargetLinePositions.deleteAll();
                
                int totalLineLengthSoFar = 0;
                
                for( int j=0; j<lines.size(); j++ ) {
                    
                    totalLineLengthSoFar += 
                        strlen( lines.getElementDirect( j ) );
                    
                    int cursorPos = totalLineLengthSoFar;
                    
                    char *line = 
                        stringDuplicate( lines.getElementDirect( j ) );
                    int remainingLength = strlen( line );
                    
                    double bestUpDiff = 9999999;
                    double bestX = 0;
                    int bestPos = 0;
                    int bestLinePos = 0;
                    
                    while( fabs( mFont->measureString( line ) - 
                                 cursorXOffset ) < bestUpDiff ) {
                        
                        bestX = mFont->measureString( line );
                        
                        bestUpDiff = fabs( bestX - cursorXOffset );
                        
                        bestPos = cursorPos;
                        bestLinePos = remainingLength;
                        
                        cursorPos --;
                        remainingLength --;
                        
                        if( remainingLength < 0 ) {
                            remainingLength = 0;
                            }
                        
                        line[ remainingLength ] = '\0';
                        }
                    
                    delete [] line;

                    if( newlineEatenInLine.getElementDirect( j ) ) {
                        totalLineLengthSoFar++;
                        }

                    if( bestPos == totalLineLengthSoFar && 
                        bestPos != 0 &&
                        bestPos != totalTextLen ) {
                        // best is at end of line, which will wrap
                        // around to next line
                        bestPos --;
                        bestLinePos --;
                        }
                    
                    mCursorTargetPositions.push_back( bestPos );
                    mCursorTargetLinePositions.push_back( bestLinePos );
                    }
                }
            }
        if( mSelectionStart != mSelectionEnd ) {
            
            char drawSelThisLine = false;
            
            // start and end of sell on this line
            double selRectStartX = 0;
            double selRectEndX = 0;
            
            if( selStartLine < i && selEndLine > i ) {
                drawSelThisLine = true;
                selRectEndX = 
                    mFont->measureString( lines.getElementDirect( i ) );
                }
            if( selStartLine == i ) {
                drawSelThisLine = true;                
                char *beforeSel = 
                    stringDuplicate( lines.getElementDirect( i ) );
            
                beforeSel[ selectionStartInLine.getElementDirect( i ) ] = '\0';
            
                selRectStartX = mFont->measureString( beforeSel );
                selRectEndX = 
                    mFont->measureString( lines.getElementDirect( i ) );
                
                delete [] beforeSel;
                }
            if( selEndLine == i ) {
                drawSelThisLine = true;
                char *beforeSel = 
                    stringDuplicate( lines.getElementDirect( i ) );
            
                beforeSel[ selectionEndInLine.getElementDirect( i ) ] = '\0';
            
                selRectEndX = mFont->measureString( beforeSel );
                
                delete [] beforeSel;
                }
            
            
            if( drawSelThisLine ) {
                
                setDrawColor( 1, 1, 0, 0.25 );
            
                if( mFocused ) {    
                    double extra = mFont->getCharSpacing() / 2;
                    double extraStart = extra;
                    double extraEnd = extra;

                    if( selRectStartX == 0 ) {
                        extraStart = -extraStart;
                        }

                    SelRect r = { pos.x + selRectStartX + extraStart, 
                                  pos.y - mFont->getFontHeight() / 2,
                                  pos.x + selRectEndX + extraEnd, 
                                  pos.y + 0.5 * mFont->getFontHeight() };
                    
                    selRects.push_back( r );
                    
                    drawRect( r.startX, r.startY, r.endX, r.endY );
                    }
                }
            }
        else if( mSelectionStart != -1 && mSelectionEnd != -1 &&
                 mPointerDownInside &&
                 selStartLine == i &&
                 selEndLine == i ) {
            // 0-char selection in progress in this line
            setDrawColor( 0, 0, 0, 0.75 );

            char *beforeSel = 
                stringDuplicate( lines.getElementDirect( i ) );
            
            beforeSel[ selectionStartInLine.getElementDirect( i ) ] = '\0';
            
            double dummyCursorXOffset = mFont->measureString( beforeSel );
        
            double extra = 0;
            if( dummyCursorXOffset == 0 ) {
                extra = -pixWidth;
                }
            
            delete [] beforeSel;
            
            if( mFocused ) {    
                drawRect( pos.x + dummyCursorXOffset + extra, 
                          pos.y - mFont->getFontHeight() / 2,
                          pos.x + dummyCursorXOffset + pixWidth + extra, 
                          pos.y + 0.55 * mFont->getFontHeight() );
                }

            }
        
        
        pos.y -= mFont->getFontHeight();
        }
    
    stopStencil();



    double xRad = mWide / 2 + 2 * pixWidth;
    double yRad = mHigh / 2 + 2 * pixWidth;
    
    pos.x = 0;
    pos.y = 0;
    
    double rectStartX = pos.x - xRad;
    double rectEndX = pos.x + xRad;

    double rectStartY = pos.y + yRad;
    double rectEndY = pos.y - yRad;


    
    
    if( selRects.size() > 0 ) {
        double extra = mFont->getCharSpacing() / 2;
        
        setDrawColor( 1, 1, 1, 1 );
        
        // border
        startAddingToStencil( false, true );
        for( int i=0; i<selRects.size(); i++ ) {
            SelRect r = selRects.getElementDirect( i );
            drawRect( r.startX - extra, r.startY - extra, 
                      r.endX + extra, r.endY + extra );
            }
        // clear center
        startAddingToStencil( false, false );
        for( int i=0; i<selRects.size(); i++ ) {
            SelRect r = selRects.getElementDirect( i );
            drawRect( r.startX, r.startY, 
                      r.endX, r.endY );
            }
        
        startDrawingThroughStencil();
        
        setDrawColor( 0, 0, 0, .3 );

        drawRect( rectStartX, rectStartY, rectEndX, rectEndY );

        stopStencil();
        }
    

    
    
    double charHeight = mFont->getFontHeight();

    double cover = 2;

    if( firstLine != 0 ) {
        // fade top shading in
        if( !mEverDrawn ) {
            // start faded in
            mTopShadingFade = 1;
            }
        else if( mTopShadingFade < 1 ) {
            mTopShadingFade += 0.1 * frameRateFactor;
            if( mTopShadingFade > 1 ) {
                mTopShadingFade = 1;
                }
            }
        }
    else {
        // fade top shading out
        if( mTopShadingFade > 0 ) {
            mTopShadingFade -= 0.1 * frameRateFactor;
            if( mTopShadingFade < 0 ) {
                mTopShadingFade = 0;
                }
            }
        }


    if( lastLine != lines.size() - 1 ) {
        // fade bottom shading in
        if( !mEverDrawn ) {
            // start faded in
            mBottomShadingFade = 1;
            }
        else if( mBottomShadingFade < 1 ) {
            mBottomShadingFade += 0.1 * frameRateFactor;
            if( mBottomShadingFade > 1 ) {
                mBottomShadingFade = 1;
                }
            }
        }
    else {
        // fade bottom shading out
        if( mBottomShadingFade > 0 ) {
            mBottomShadingFade -= 0.1 * frameRateFactor;
            if( mBottomShadingFade < 0 ) {
                mBottomShadingFade = 0;
                }
            }
        }


    if( mTopShadingFade > 0 ) {
        // draw shaded overlay over top of text area
        

        double verts[] = { rectStartX, rectStartY,
                           rectEndX, rectStartY,
                           rectEndX, rectStartY - cover * charHeight,
                           rectStartX, rectStartY -  cover * charHeight };
        float vertColors[] = { 0.25, 0.25, 0.25, mTopShadingFade,
                               0.25, 0.25, 0.25, mTopShadingFade,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0 };

        drawQuads( 1, verts , vertColors );
        }
    
    if( mBottomShadingFade > 0 ) {
        // draw shaded overlay over bottom of text area


        
        double verts[] = { rectStartX, rectEndY,
                           rectEndX, rectEndY,
                           rectEndX, rectEndY + cover * charHeight,
                           rectStartX, rectEndY +  cover * charHeight };
        float vertColors[] = { 0.25, 0.25, 0.25, mBottomShadingFade,
                               0.25, 0.25, 0.25, mBottomShadingFade,
                               0.25, 0.25, 0.25, 0,
                               0.25, 0.25, 0.25, 0 };

        drawQuads( 1, verts , vertColors );
        }
    
    

    mLineStrings.deallocateStringElements();
    
    for( int i=0; i<lines.size(); i++ ) {
        mLineStrings.push_back( lines.getElementDirect( i ) );
        }
    

    
    if( ! mActive ) {
        setDrawColor( 0, 0, 0, 0.5 );
        // dark overlay

        drawRect( pos, mWide / 2 + 3 * pixWidth, mHigh / 2 + 3 * pixWidth );
        }


    mSnapMove = false;
    mEverDrawn = true;
    }



void TextArea::upHit() {
    mCursorFlashSteps = 0;
    
    cursorUpFromKey();

    if( isCommandKeyDown() ) {
        // up to end of previous paragraph
        
        // skip non-newline characters
        while( mCursorPosition > 0 &&
               mText[ mCursorPosition - 1 ] != '\r' ) {
            mCursorPosition --;
            }
        
        // skip space and newline characters
        while( mCursorPosition > 0 &&
               ( mText[ mCursorPosition - 1 ] == ' ' ||
                 mText[ mCursorPosition - 1 ] == '\r' ) ) {
            mCursorPosition --;
            }
        mSnapMove = true;
        mRecomputeCursorPositions = true;
        }
    else {
        // up one line
    
        if( mCurrentLine > 0 ) {
            // check for match before moving up
            // otherwise, we may have moved up from top of selection already
            if( mCursorPosition == 
                mCursorTargetPositions.getElementDirect( mCurrentLine ) ) {
            
                mCurrentLine--;
                mCursorPosition = 
                    mCursorTargetPositions.getElementDirect( mCurrentLine );
                
                if( mSmoothSlidingUp ) {
                    mVertSlideOffset += mFont->getFontHeight();
                    }
                }
            }
        else {
            mCursorPosition = 0;
            mRecomputeCursorPositions = true;
            }
        }
        
    if( isShiftKeyDown() ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
        }
    }



void TextArea::downHit() {
    mCursorFlashSteps = 0;
    
    cursorDownFromKey();
    
    if( isCommandKeyDown() ) {
        // down to end of previous paragraph
        int textLen = strlen( mText );
        
        // skip space and newline characters
        while( mCursorPosition < textLen &&
               ( mText[ mCursorPosition ] == ' ' ||
                 mText[ mCursorPosition ] == '\r' ) ) {
            mCursorPosition ++;
            }

        // skip non-newline characters
        while( mCursorPosition < textLen &&
               mText[ mCursorPosition ] != '\r' ) {
            mCursorPosition ++;
            }
        
        mSnapMove = true;
        mRecomputeCursorPositions = true;
        }
    else {
        // down one line

        if( mCurrentLine < mCursorTargetPositions.size() - 1 ) {
            // check for match before moving down
            // otherwise, we may have moved down from bottom of 
            // selection already
            if( mCursorPosition == 
                mCursorTargetPositions.getElementDirect( mCurrentLine ) ) {

                mCurrentLine++;
                mCursorPosition = 
                    mCursorTargetPositions.getElementDirect( mCurrentLine );
            
                if( mSmoothSlidingDown ) {
                    mVertSlideOffset -= mFont->getFontHeight();
                    }
                }
            }
        else {
            mCursorPosition = strlen( mText );
            mRecomputeCursorPositions = true;
            }

        }
    
    if( isShiftKeyDown() ) {
        *mSelectionAdjusting = mCursorPosition;
        fixSelectionStartEnd();
        }
    }



void TextArea::cursorUpFromKey() {
    if( isShiftKeyDown() ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionStart;
            }
        }
    else {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionStart;
            mRecomputeCursorPositions = true;
            }
        mSelectionStart = -1;
        mSelectionEnd = -1;
        }
    }



void TextArea::cursorDownFromKey() {
    if( isShiftKeyDown() ) {
        if( !isAnythingSelected() ) {
            mSelectionStart = mCursorPosition;
            mSelectionEnd = mCursorPosition;
            mSelectionAdjusting = &mSelectionEnd;
            }
        }
    else {
        if( isAnythingSelected() ) {
            mCursorPosition = mSelectionEnd;
            mRecomputeCursorPositions = true;
            }
        mSelectionStart = -1;
        mSelectionEnd = -1;
        }
    }



void TextArea::specialKeyDown( int inKeyCode ) {
    TextField::specialKeyDown( inKeyCode );

     if( !mFocused ) {
        return;
        }         
    
    switch( inKeyCode ) {
        case MG_KEY_UP:
            if( ! mIgnoreArrowKeys ) {
                upHit();
                clearVertArrowRepeat();
                mHoldVertArrowSteps[0] = 0;
                }
            break;
        case MG_KEY_DOWN:
            if( ! mIgnoreArrowKeys ) {
                downHit();
                clearVertArrowRepeat();
                mHoldVertArrowSteps[1] = 0;
                }
            break;
        case MG_KEY_PAGE_UP: {
            mCursorFlashSteps = 0;
            cursorUpFromKey();
            
            int old = mCurrentLine;
            
            char overshoot = false;
            
            mCurrentLine -= mMaxLinesShown - 1;
            if( mCurrentLine < 0 ) {
                mCurrentLine = 0;
                overshoot = true;
                }

            // disable smooth pg up/down for now
            if( false && mSmoothSlidingUp ) {
                mVertSlideOffset += 
                    (old - mCurrentLine ) * mFont->getFontHeight();
                }
            
            mCursorPosition = 
                mCursorTargetPositions.getElementDirect( mCurrentLine );
            if( overshoot ) {
                mCursorPosition = 0;
                }
            mSnapMove = true;
            if( isShiftKeyDown() ) {
                *mSelectionAdjusting = mCursorPosition;
                fixSelectionStartEnd();
                }
            break;
            }
        case MG_KEY_PAGE_DOWN: {
            mCursorFlashSteps = 0;
            cursorDownFromKey();
            
            int old = mCurrentLine;
            
            char overshoot = false;

            mCurrentLine += mMaxLinesShown - 1;
            if( mCurrentLine >= mCursorTargetPositions.size() ) {
                mCurrentLine = mCursorTargetPositions.size() - 1;
                overshoot = true;
                }
            
            // disable smooth pg up/down for now
            if( false && mSmoothSlidingDown ) {
                mVertSlideOffset -= 
                    (mCurrentLine - old ) * mFont->getFontHeight();
                }
            
            mCursorPosition = 
                mCursorTargetPositions.getElementDirect( mCurrentLine );
            
            if( overshoot ) {
                mCursorPosition = strlen( mText );
                }
            
            mSnapMove = true;
            if( isShiftKeyDown() ) {
                *mSelectionAdjusting = mCursorPosition;
                fixSelectionStartEnd();
                }
            break;
            }
        case MG_KEY_HOME:
            mCursorFlashSteps = 0;
            cursorUpFromKey();

            mCursorPosition = 0;
            mSnapMove = true;
            if( isShiftKeyDown() ) {
                *mSelectionAdjusting = mCursorPosition;
                fixSelectionStartEnd();
                }
            break;
        case MG_KEY_END:
            mCursorFlashSteps = 0;
            cursorDownFromKey();

            mCursorPosition = strlen( mText );
            mSnapMove = true;
            if( isShiftKeyDown() ) {
                *mSelectionAdjusting = mCursorPosition;
                fixSelectionStartEnd();
                }
            break;
        default:
            break;
        }
    }



void TextArea::specialKeyUp( int inKeyCode ) {
    TextField::specialKeyUp( inKeyCode );

    if( inKeyCode == MG_KEY_RIGHT ||
         inKeyCode == MG_KEY_LEFT ) {
         mRecomputeCursorPositions = true;
         }


    if( inKeyCode == MG_KEY_UP ) {
        mHoldVertArrowSteps[0] = -1;
        mFirstVertArrowRepeatDone[0] = false;
        }
    else if( inKeyCode == MG_KEY_DOWN ) {
        mHoldVertArrowSteps[1] = -1;
        mFirstVertArrowRepeatDone[1] = false;
        }
    }



int TextArea::getClickHitCursorIndex( float inX, float inY ) {
    float pixelHitY = mHigh / 2 - inY;

    float pixelHitX = inX + mWide / 2;
    
    int lineHit = lrint( pixelHitY / mFont->getFontHeight() -
                         0.5 );
    
    if( lineHit < 0 ||
        lineHit >= mMaxLinesShown ) {
        return -1;
        }

    if( mLastVisibleLine == 0 &&
        strlen( mLineStrings.getElementDirect( 0 ) ) == 0 ) {
        // empty text area
        return -1;
        }
    
    lineHit += mFirstVisibleLine;
    
    if( lineHit <= mLastVisibleLine ) {
        // in range
        
        
        
        int bestCursorLinePos = 0;
        double bestDistance = mWide * 2;
        
        char *lineString = mLineStrings.getElementDirect( lineHit );
        

        int drawnTextLength = strlen( lineString );
        
        // find gap between drawn letters that is closest to clicked x

        // don't consider placing cursor after space at end of line
        int limit = drawnTextLength - 1;
        
        if( lineString[ limit ] != ' ' ) {
            limit++;
            }

        for( int i=0; i<=limit; i++ ) {
            
            char *textCopy = stringDuplicate( lineString );
            
            textCopy[i] = '\0';

            double thisGapX = 
                mFont->measureString( textCopy ) +
                mFont->getCharSpacing() / 2;
            
            delete [] textCopy;
            
            double thisDistance = fabs( thisGapX - pixelHitX );
            
            if( thisDistance < bestDistance ) {
                bestCursorLinePos = i;
                bestDistance = thisDistance;
                }
            }
        
        int delta = bestCursorLinePos - 
            mCursorTargetLinePositions.getElementDirect( lineHit );

        
        return mCursorTargetPositions.getElementDirect( lineHit ) + delta;
        }
    else {
        return -1;
        }
    }






void TextArea::pointerDown( float inX, float inY ) {
    
    double pixWidth = mCharWidth / 8;

    if( inX > -mWide / 2 - 3 * pixWidth &&
        inX < mWide / 2 + 3 * pixWidth &&
        inY > -mHigh / 2 - 3 * pixWidth &&
        inY < mHigh / 2 + 3 * pixWidth ) {
        mPointerDownInside = true;
        }
    else {
        mPointerDownInside = false;
        return;
        }
        
    focus();
    
    if( mVertSlideOffset != 0 ) {
        // disable click in middle of slide
        return;
        }
    
    int newCursor = getClickHitCursorIndex( inX, inY );
    
    if( newCursor != -1 ) {
        
        if( ! isShiftKeyDown() ) {
            mSelectionStart = newCursor;
            mSelectionEnd = newCursor;
            mSelectionAdjusting = &mSelectionEnd;
            }
        else {
            // shift click
            if( ! isAnythingSelected() ) {
                // create new selection from here to cursor
                if( newCursor > mCursorPosition ) {
                    mSelectionStart = mCursorPosition;
                    mSelectionEnd = newCursor;
                    mSelectionAdjusting = &mSelectionEnd;
                    }
                else {
                    mSelectionStart = newCursor;
                    mSelectionEnd = mCursorPosition;
                    mSelectionAdjusting = &mSelectionStart;
                    }
                }
            else {
                // shift click to add to existing selection
                if( newCursor > mSelectionEnd ) {
                    mSelectionEnd = newCursor;
                    mSelectionAdjusting = &mSelectionEnd;
                    }
                if( newCursor < mSelectionStart ) {
                    mSelectionStart = newCursor;
                    mSelectionAdjusting = &mSelectionStart;
                    }
                }
            }
        }
    }



void TextArea::pointerDrag( float inX, float inY ) {
    if( ! mPointerDownInside ) {
        return;
        }
    
    double pixWidth = mCharWidth / 8;

    if( inX > -mWide / 2 - 3 * pixWidth &&
        inX < mWide / 2 + 3 * pixWidth &&
        inY > -mHigh / 2 - 3 * pixWidth &&
        inY < mHigh / 2 + 3 * pixWidth ) {
        }
    else {
        return;
        }
        
    
    if( mVertSlideOffset != 0 ) {
        // disable click in middle of slide
        return;
        }
    
    if( mSelectionStart != -1 ) {
        
        int newCursor = getClickHitCursorIndex( inX, inY );
    
        if( newCursor != -1 ) {
            *mSelectionAdjusting = newCursor;
            
            fixSelectionStartEnd();
            }
        }
    }



void TextArea::pointerUp( float inX, float inY ) {
    mPointerDownInside = false;
    
    double pixWidth = mCharWidth / 8;

    if( inX > -mWide / 2 - 3 * pixWidth &&
        inX < mWide / 2 + 3 * pixWidth &&
        inY > -mHigh / 2 - 3 * pixWidth &&
        inY < mHigh / 2 + 3 * pixWidth ) {
        focus();
        }
    else {
        return;
        }
    
    mCursorFlashSteps = 0;
    
    
    if( mVertSlideOffset != 0 ) {
        // disable click in middle of slide
        return;
        }

    if( isAnythingSelected() ) {
        // cursor in middle of selection, so selection is centered
        // in text area
        mCursorPosition = ( mSelectionEnd + mSelectionStart ) / 2;
        mRecomputeCursorPositions = true;
        }
    else {
        mSelectionStart = -1;
        mSelectionEnd = -1;
        
        int newCursor = getClickHitCursorIndex( inX, inY );
        
        if( newCursor != -1 ) {
            mCursorPosition = newCursor;
            mRecomputeCursorPositions = true;
            }
        }
    }




void TextArea::clearVertArrowRepeat() {
    for( int i=0; i<2; i++ ) {
        mHoldVertArrowSteps[i] = -1;
        mFirstVertArrowRepeatDone[i] = false;
        }
    }
