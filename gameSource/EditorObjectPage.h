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
#include "animationBank.h"

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
        doublePair mObjectCenterOnScreen;
        
        TextField mDescriptionField;
        
        TextField mBiomeField;
        TextField mMapChanceField;
        TextField mHeatValueField;
        TextField mRValueField;
        
        TextField mFoodValueField;
        TextField mSpeedMultField;
        
        TextField mContainSizeField;
        TextField mSlotSizeField;

        TextField mDeadlyDistanceField;
        
        TextField mRaceField;

        TextButton mSaveObjectButton;
        TextButton mReplaceObjectButton;

        TextButton mClearObjectButton;

        TextButton mClearRotButton;
        TextButton mRot90ForwardButton;
        TextButton mRot90BackwardButton;
        

        TextButton mFlipHButton;
        

        TextButton mImportEditorButton;
        TextButton mTransEditorButton;
        TextButton mAnimEditorButton;
        
        TextButton mMoreSlotsButton;
        TextButton mLessSlotsButton;


        CheckboxButton mAgingLayerCheckbox;

        CheckboxButton mHeadLayerCheckbox;
        CheckboxButton mBodyLayerCheckbox;
        CheckboxButton mBackFootLayerCheckbox;
        CheckboxButton mFrontFootLayerCheckbox;

        CheckboxButton mInvisibleWhenHoldingCheckbox;

        
        TextField mAgeInField;
        TextField mAgeOutField;
        
        TextButton mAgePunchInButton;
        TextButton mAgePunchOutButton;
        
        

        CheckboxButton *mCheckboxes[NUM_OBJECT_CHECKBOXES];
        const char *mCheckboxNames[NUM_OBJECT_CHECKBOXES];

        CheckboxButton mMaleCheckbox;
        CheckboxButton mDeathMarkerCheckbox;

        CheckboxButton mHeldInHandCheckbox;
        CheckboxButton mRideableCheckbox;
        CheckboxButton mBlocksWalkingCheckbox;

        CheckboxButton *mClothingCheckboxes[NUM_CLOTHING_CHECKBOXES];
        const char *mClothingCheckboxNames[NUM_CLOTHING_CHECKBOXES];

        TextButton mDemoClothesButton;
        TextButton mEndClothesDemoButton;
        

        TextButton mDemoSlotsButton;
        TextButton mClearSlotsDemoButton;

        TextButton mSetHeldPosButton;
        TextButton mEndSetHeldPosButton;

        TextButton mNextHeldDemoButton;
        TextButton mPrevHeldDemoButton;
        


        Picker mSpritePicker;
        Picker mObjectPicker;

        ValueSlider mPersonAgeSlider;

        ValueSlider mHueSlider;
        ValueSlider mSaturationSlider;
        ValueSlider mValueSlider;


        ObjectRecord mCurrentObject;
        

        SimpleVector<LayerSwapRecord> mObjectLayerSwaps;
        

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

        char mDragging;
        
        
        double getClosestSpriteOrSlot( float inX, float inY,
                                       int *outSprite,
                                       int *outSlot );
        

        void pickedLayerChanged();
        
        void updateSliderColors();


        void recomputeNumNonAgingSprites();
        

        void updateAgingPanel();
        

        FloatRGB mColorClipboard;
        

        int mHoverObjectLayer;
        int mHoverSlot;
        
        // fades when mouse stops moving
        float mHoverStrength;
        
        // alpha for blinking on hover
        float mHoverFlash;

        int mHoverFrameCount;

        char mRotAdjustMode;
        float mRotStartMouseX;
        
        double mLayerOldRot;


        KeyLegend mKeyLegend;
        KeyLegend mKeyLegendB;
        KeyLegend mKeyLegendC;
    };



#endif
