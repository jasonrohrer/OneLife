#ifndef EDITOR_TRANSITION_PAGE_INCLUDED
#define EDITOR_TRANSITION_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "Picker.h"

#include "transitionBank.h"



#define NUM_TREE_TRANS_TO_SHOW 1



class EditorTransitionPage : public GamePage, public ActionListener {
        
    public:
        EditorTransitionPage();
        ~EditorTransitionPage();

        void clearUseOfObject( int inObjectID );
        

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );
        
        virtual void keyDown( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
    protected:
        
        TextField mAutoDecayTimeField;

        
        TextButton mSaveTransitionButton;
        Picker mObjectPicker;

        TextButton mObjectEditorButton;

        TransRecord mCurrentTransition;

        int mCurrentlyReplacing;

        Button *mPickButtons[4];
        
        TextButton *mClearButtons[3];
        
        
        int mLastSearchID;

        int mProducedBySkip;
        TransRecord mProducedBy[NUM_TREE_TRANS_TO_SHOW];
        Button *mProducedByButtons[NUM_TREE_TRANS_TO_SHOW];
        
        TextButton mProducedByNext;
        TextButton mProducedByPrev;
        

        int mProducesSkip;
        TransRecord mProduces[NUM_TREE_TRANS_TO_SHOW];
        Button *mProducesButtons[NUM_TREE_TRANS_TO_SHOW];
        
        TextButton mProducesNext;
        TextButton mProducesPrev;
        
        
        TextButton mDelButton;
        TextButton mDelConfirmButton;
        

        void checkIfSaveVisible();

        void redoTransSearches( int inObjectID, char inClearSkip );
        
    };



#endif
