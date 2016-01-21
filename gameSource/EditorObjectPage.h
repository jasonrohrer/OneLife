#ifndef EDITOR_OBJECT_PAGE_INCLUDED
#define EDITOR_OBJECT_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "CheckboxButton.h"

#include "Picker.h"

#include "ValueSlider.h"


#include "objectBank.h"

#include "keyLegend.h"



#define NUM_OBJECT_CHECKBOXES 3

#define NUM_CLOTHING_CHECKBOXES 4



class EditorObjectPage : public GamePage, public ActionListener {
        
    public:
        EditorObjectPage();
        ~EditorObjectPage();


        void clearUseOfSprite( int inSpriteID );
        
        
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
        virtual void keyUp( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
    protected:

        TextField mDescriptionField;
        
        TextField mMapChanceField;
        TextField mHeatValueField;
        TextField mRValueField;
        
        TextField mFoodValueField;
        TextField mSpeedMultField;
        
        TextField mContainSizeField;
        TextField mSlotSizeField;

        TextField mDeadlyDistanceField;


        TextButton mSaveObjectButton;
        TextButton mReplaceObjectButton;

        TextButton mClearObjectButton;

        TextButton mClearRotButton;
        
        TextButton mFlipHButton;
        

        TextButton mImportEditorButton;
        TextButton mTransEditorButton;
        TextButton mAnimEditorButton;
        
        TextButton mMoreSlotsButton;
        TextButton mLessSlotsButton;
        
        CheckboxButton *mCheckboxes[NUM_OBJECT_CHECKBOXES];
        const char *mCheckboxNames[NUM_OBJECT_CHECKBOXES];

        CheckboxButton *mClothingCheckboxes[NUM_CLOTHING_CHECKBOXES];
        const char *mClothingCheckboxNames[NUM_CLOTHING_CHECKBOXES];

        TextButton mDemoClothesButton;
        TextButton mEndClothesDemoButton;
        

        TextButton mDemoSlotsButton;
        TextButton mClearSlotsDemoButton;

        TextButton mSetHeldPosButton;
        TextButton mEndSetHeldPosButton;
        

        Picker mSpritePicker;
        Picker mObjectPicker;

        ValueSlider mPersonAgeSlider;


        ObjectRecord mCurrentObject;
        
        char mDemoSlots;
        int mSlotsDemoObject;

        char mSetHeldPos;
        int mDemoPersonObject;
        
        // use same person object
        char mSetClothesPos;
        

        doublePair mSetHeldMouseStart;
        doublePair mSetHeldOffsetStart;

        doublePair mSetClothingMouseStart;
        doublePair mSetClothingOffsetStart;


        SpriteHandle mSlotPlaceholderSprite;

        int mPickedObjectLayer;
        int mPickedSlot;
        doublePair mPickedMouseOffset;

        
        double getClosestSpriteOrSlot( float inX, float inY,
                                       int *outSprite,
                                       int *outSlot );

        int mHoverObjectLayer;
        int mHoverSlot;
        
        // fades when mouse stops moving
        float mHoverStrength;
        
        // alpha for blinking on hover
        float mHoverFlash;

        int mHoverFrameCount;

        char mRotAdjustMode;
        float mRotStartMouseX;
        

        KeyLegend mKeyLegend;
    };



#endif
