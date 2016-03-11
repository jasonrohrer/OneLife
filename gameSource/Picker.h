#ifndef PICKER_INCLUDED
#define PICKER_INCLUDED


#include "PageComponent.h"
#include "TextButton.h"
#include "TextField.h"


#include "Pickable.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/ui/event/ActionListenerList.h"



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
        
        
        void redoSearch();


        virtual void actionPerformed( GUIComponent *inTarget );
        
        
    protected:
        virtual void draw();

        virtual void pointerUp( float inX, float inY );

        
        Pickable *mPickable;
        
        int mSkip;
        
        void **mResults;
        int mNumResults;

        TextButton mNextButton;
        TextButton mPrevButton;

        TextButton mDelButton;
        TextButton mDelConfirmButton;

        
        TextField mSearchField;
        
        int mSelectionIndex;
        
        char mSelectionRightClicked;
    };



#endif
