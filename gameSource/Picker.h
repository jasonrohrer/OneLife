#ifndef PICKER_INCLUDED
#define PICKER_INCLUDED


#include "PageComponent.h"
#include "TextButton.h"
#include "TextField.h"


#include "Pickable.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/ui/event/ActionListenerList.h"

typedef struct FilterFunction {
        char(*unclickableFunc)(int);
    } FilterFunction;
        

// fires action performed when selected object changes
class Picker : public PageComponent, ActionListener, 
                     public ActionListenerList  {
    public:
        
        // centered on inX, inY
        Picker( Pickable *inPickable, double inX, double inY );
        
        ~Picker();


        // -1 if none picked
        // pass pointer to a char if you care about right-vs-left clicks
        int getSelectedObject( char *inWasRightClick=NULL );

        void unselectObject();
        
        
        void redoSearch( char inClearPageSkip );
        
        // add a function that filters each result and returns true
        // for results that should be unclickable
        void addFilter( char(*inUnclickableFunc)( int inID ) );

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void specialKeyDown( int inKeyCode );

        
    protected:
        virtual void draw();

        virtual void pointerDown( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

        char mPressStartedHere;

        
        Pickable *mPickable;
        
        int mSkip;
        
        void **mResults;
        int mNumResults;
        char *mResultsUnclickable;


        TextButton mNextButton;
        TextButton mPrevButton;

        TextButton mDelButton;
        TextButton mDelConfirmButton;

        
        TextField mSearchField;
        
        int mSelectionIndex;
        
        char mSelectionRightClicked;


        SimpleVector<char*> mPastSearches;
        SimpleVector<int> mPastSearchSkips;
        int mPastSearchCurrentIndex;

        void addSearchToStack();
        
        
        // list of filter functions
        SimpleVector<FilterFunction> mFilterFuncions;
        
    };



#endif
