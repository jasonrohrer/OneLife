#ifndef TEXT_AREA_INCLUDED
#define TEXT_AREA_INCLUDED


#include "TextField.h"


// fires action performed when ENTER hit inside area
// (can be toggled to fire on every text change or every focus loss)
class TextArea : public TextField {
        
    public:
        
        // label text and char maps copied internally
        TextArea( Font *inLabelFont, Font *inDisplayFont, 
                  double inX, double inY, double inWide, double inHigh,
                  char inForceCaps = false,
                  const char *inLabelText = NULL,
                  const char *inAllowedChars = NULL,
                  const char *inForbiddenChars = NULL );

        virtual ~TextArea();
        
        // defaults to false
        void enableSpellCheck( char inSpellCheckOn );


        virtual void draw();
        virtual void step();
        
        virtual void specialKeyDown( int inKeyCode );
        virtual void specialKeyUp( int inKeyCode );

        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

    protected:
        double mWide, mHigh;
        
        Font *mLabelFont;
        
        char mSpellCheckOn;

        int mCurrentLine;
        
        SimpleVector<char*> mLineStrings;
        
        // one per line, to help cursor move up and down evenly
        // absolute positions in mText
        SimpleVector<int> mCursorTargetPositions;
        // relative positions in each line's string
        SimpleVector<int> mCursorTargetLinePositions;


        char *mLastDrawnText;
        
        char mRecomputeCursorPositions;
        char *mLastComputedCursorText;
        

        int mHoldVertArrowSteps[2];
        char mFirstVertArrowRepeatDone[2];

        void upHit();
        void downHit();
        
        void clearVertArrowRepeat();

        double mVertSlideOffset;
        char mSmoothSlidingUp, mSmoothSlidingDown;

        // override smooth movement for HOME and END and PAGE keys
        char mSnapMove;
        

        float mTopShadingFade, mBottomShadingFade;

        int mMaxLinesShown;
        
        int mFirstVisibleLine;
        int mLastVisibleLine;
        
        char mPointerDownInside;
        
        int mStepsSinceTextChanged;
        char mEverDrawn;
        

        int getClickHitCursorIndex( float inX, float inY );

        // starts adjusting selection if shift held
        void cursorUpFromKey();
        void cursorDownFromKey();
        

    };




#endif
