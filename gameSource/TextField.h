#ifndef TEXT_FIELD_INCLUDED
#define TEXT_FIELD_INCLUDED


#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListenerList.h"

#include "PageComponent.h"


// fires action performed when ENTER hit inside field, but only if
// \r is not an allowed character.
// Note that \r has to be specifically listed in the inAllowedChars list
// otherwise, it is forbidden by default.
// (can be toggled to fire on every text change or every focus loss)
class TextField : public PageComponent, public ActionListenerList {
        
    public:
        
        // text field width based on widest allowed 
        // (or non-forbidden) character

        // label text and char maps copied internally
        TextField( Font *inDisplayFont, 
                   double inX, double inY, int inCharsWide,
                   char inForceCaps = false,
                   const char *inLabelText = NULL,
                   const char *inAllowedChars = NULL,
                   const char *inForbiddenChars = NULL );

        virtual ~TextField();

        // automatically becomes non-hidden when focused
        void setContentsHidden( char inHidden );
        

        // copied internally
        void setText( const char *inText );
        

        // destroyed by caller
        char *getText();
        

        // defaults to -1 (no limit)
        void setMaxLength( int inLimit );
        
        // returns -1 if no limit
        int getMaxLength();

        // returns true if field is full
        char isAtLimit();
        
        
        char isAnythingSelected();

        // destroyed by caller.
        // NULL if nothing selected
        char *getSelectedText();

        
        // defaults to false
        // true means shift + arrow keys can modify selection from current
        // cursor
        void setShiftArrowsCanSelect( char inCanSelect );


        // at current cursor position, or replacing current selection
        void insertCharacter( unsigned char inASCII );
        void insertString( char *inString );
        
        
        // controls whether arrow keys have an effect on cursor or not
        // (default to having an effect)
        void setIgnoreArrowKeys( char inIgnore );
        
        
        // index in string of character that cursor is in front of
        int getCursorPosition();
        

        // brings cursor back to start of string
        void cursorReset();
        
        
        double getRightEdgeX();
        

        // defaults to false
        void setFireOnAnyTextChange( char inFireOnAny );

        // defaults to false
        void setFireOnLoseFocus( char inFireOnLeave );


        int getInt();
        
        float getFloat();
        

        void setInt( int inI );
        
        // if inDigitsAfterDecimal = -1 (default), no limit
        // if inTrimZeros is true, extra zeros after decimal point are removed
        void setFloat( float inF, int inDigitsAfterDecimal=-1, 
                       char inTrimZeros = false );
        

        // defaults to label on left
        void setLabelSide( char inLabelOnRight );
        
        // defaults to side
        void setLabelTop( char inLabelOnTop );
        

        
        virtual void setActive( char inActive );
        virtual char isActive();
        
        
        
        virtual void step();
        
        virtual void draw();

        virtual void pointerUp( float inX, float inY );

        virtual void keyDown( unsigned char inASCII );
        virtual void keyUp( unsigned char inASCII );
        
        virtual void specialKeyDown( int inKeyCode );
        virtual void specialKeyUp( int inKeyCode );
        

        // makes this text field the only focused field.
        // causes it to respond to keystrokes that are passed to it
        virtual void focus();
        
        // if this field is the only focused field, this causes no
        // fields to be focused.
        // if this field is currently unfocused, this call has no effect.
        virtual void unfocus();

        virtual char isFocused();
        
        
        static char isAnyFocused();
        
        static void unfocusAll();
        

        // defaults to 30 and 2
        static void setDeleteRepeatDelays( int inFirstDelaySteps,
                                           int inNextDelaySteps );
        

        
    protected:
        char mActive;
        char mContentsHidden;
        
        SpriteHandle mHiddenSprite;
        

        Font *mFont;
        int mCharsWide;
        
        int mMaxLength;
        
        char mFireOnAnyChange;
        char mFireOnLeave;
        char mForceCaps;

        char *mLabelText;
        
        char *mAllowedChars;
        char *mForbiddenChars;

        
        double mWide, mHigh;
        
        double mBorderWide;
        
        // width of a single character
        double mCharWidth;
        
        

        char mFocused;

        char *mText;
        int mTextLen;
        
        int mCursorPosition;
        
        char mIgnoreArrowKeys;
        
        
        char *mDrawnText;
        // position of cursor within text that is actually drawn
        int mCursorDrawPosition;
        // leftmost x position of drawn text
        double mDrawnTextX;
        

        int mHoldDeleteSteps;
        char mFirstDeleteRepeatDone;

        int mHoldArrowSteps[2];
        char mFirstArrowRepeatDone[2];
        

        char mLabelOnRight;
        char mLabelOnTop;

        int mSelectionStart;
        int mSelectionEnd;
        
        // pointer to end of selection that is being adjusted
        int *mSelectionAdjusting;
        
        char mShiftPlusArrowsCanSelect;
        
        int mCursorFlashSteps;
        
        
        void fixSelectionStartEnd();


        void deleteHit();
        void leftHit();
        void rightHit();
        
        void clearArrowRepeat();
        
        
        // returns 0 if character completely forbidden by field rules
        unsigned char processCharacter( unsigned char inASCII );
        
        

        // clever (!) way of handling focus?

        // keep static pointer to focused field (there can be only one)
        static TextField *sFocusedTextField;
        

        static int sDeleteFirstDelaySteps;
        static int sDeleteNextDelaySteps;
        
    };


#endif
