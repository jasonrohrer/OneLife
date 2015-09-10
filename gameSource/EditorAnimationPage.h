#ifndef EDITOR_ANIMATION_PAGE_INCLUDED
#define EDITOR_ANIMATION_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "CheckboxButton.h"

#include "ValueSlider.h"


#include "Picker.h"

#include "animationBank.h"
#include "objectBank.h"


#define NUM_ANIM_CHECKBOXES 3
#define NUM_ANIM_SLIDERS 8


class EditorAnimationPage : public GamePage, public ActionListener {
        
    public:
        EditorAnimationPage();
        ~EditorAnimationPage();        
        
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
        
        TextButton mObjectEditorButton;
        
        Picker mObjectPicker;

        CheckboxButton *mCheckboxes[NUM_ANIM_CHECKBOXES];
        const char *mCheckboxNames[NUM_ANIM_CHECKBOXES];
        AnimType mCheckboxAnimTypes[NUM_ANIM_CHECKBOXES];
        
        ValueSlider *mSliders[ NUM_ANIM_SLIDERS ];
        
        int mCurrentObjectID;

        AnimationRecord *mCurrentAnim;
        
        AnimType mCurrentType;

        int mCurrentSpriteOrSlot;
        

        TextButton mNextSpriteOrSlotButton;
        TextButton mPrevSpriteOrSlotButton;
        
        void checkNextPrevVisible();
        

        void updateAnimFromSliders();
        void updateSlidersFromAnim();


        int mFrameCount;

        void freeCurrentAnim();
        void populateCurrentAnim();
        
    };



#endif
