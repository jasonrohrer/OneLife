#ifndef EDITOR_TRANSITION_PAGE_INCLUDED
#define EDITOR_TRANSITION_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"
#include "SpriteButton.h"

#include "CheckboxButton.h"

#include "RadioButtonSet.h"

#include "Picker.h"

#include "transitionBank.h"
#include "objectBank.h"



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

        CheckboxButton mLastUseActorCheckbox;
        CheckboxButton mLastUseTargetCheckbox;

        CheckboxButton mReverseUseActorCheckbox;
        CheckboxButton mReverseUseTargetCheckbox;
        
        RadioButtonSet mMovementButtons;
        
        TextField mDesiredMoveDistField;
        
        TextField mActorMinUseFractionField, mTargetMinUseFractionField;

        TextButton mSaveTransitionButton;
        Picker mObjectPicker;

        TextButton mObjectEditorButton;
        TextButton mCategoryEditorButton;

        TransRecord mCurrentTransition;

        int mCurrentlyReplacing;

        Button *mPickButtons[4];
        
        TextButton *mClearButtons[4];
        
        
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
        

        SpriteButton mSwapActorButton;
        SpriteButton mSwapTargetButton;

        SpriteButton mSwapTopButton;
        SpriteButton mSwapBottomButton;

        void checkIfSaveVisible();

        void redoTransSearches( int inObjectID, char inClearSkip );
        
    };



#endif
