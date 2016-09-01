#ifndef TEXT_FIELD_INCLUDED
#define TEXT_FIELD_INCLUDED


#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListenerList.h"

#include "PageComponent.h"


// fires action performed when ENTER hit inside field
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

        // copied internally
        void setText( const char *inText );
        

        // destroyed by caller
        char *getText();
        

        // defaults to -1 (no limit)
        void setMaxLength( int inLimit );
        
        // returns -1 if no limit
        int getMaxLength();
        
        

        // at current cursor position
        void insertCharacter( unsigned char inASCII );


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

        int mCursorPosition;
        
        
        char *mDrawnText;
        // position of cursor within text that is actually drawn
        int mCursorDrawPosition;
        // leftmost x position of drawn text
        double mDrawnTextX;
        

        int mHoldDeleteSteps;
        char mFirstDeleteRepeatDone;

        int mHoldArrowSteps[2];
        char mFirstArrowRepeatDone[2];
        

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
