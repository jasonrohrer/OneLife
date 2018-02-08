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


#include "SoundWidget.h"



#define NUM_OBJECT_CHECKBOXES 3

#define NUM_CLOTHING_CHECKBOXES 5



class EditorObjectPage : public GamePage, public ActionListener {
        
    public:
        EditorObjectPage();
        ~EditorObjectPage();


        void setClipboardColor( FloatRGB inColor ) {
            mColorClipboard = inColor;
            }

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
        
        TextField mSlotTimeStretchField;

        TextField mDeadlyDistanceField;
        TextField mUseDistanceField;
        TextField mMinPickupAgeField;
        
        TextField mRaceField;

        TextButton mSaveObjectButton;
        TextButton mReplaceObjectButton;

        TextButton mClearObjectButton;

        TextButton mClearRotButton;
        TextButton mRot45ForwardButton;
        TextButton mRot45BackwardButton;
        

        TextButton mFlipHButton;
        
        TextButton mBakeButton;


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

        CheckboxButton mInvisibleWhenWornCheckbox;
        CheckboxButton mInvisibleWhenUnwornCheckbox;

        CheckboxButton mBehindSlotsCheckbox;
        
        TextField mAgeInField;
        TextField mAgeOutField;
        
        TextButton mAgePunchInButton;
        TextButton mAgePunchOutButton;
        
        

        CheckboxButton *mCheckboxes[NUM_OBJECT_CHECKBOXES];
        const char *mCheckboxNames[NUM_OBJECT_CHECKBOXES];

        CheckboxButton mPersonNoSpawnCheckbox;
        CheckboxButton mMaleCheckbox;
        CheckboxButton mDeathMarkerCheckbox;
        CheckboxButton mHomeMarkerCheckbox;
        CheckboxButton mFloorCheckbox;

        CheckboxButton mHeldInHandCheckbox;
        CheckboxButton mRideableCheckbox;
        CheckboxButton mBlocksWalkingCheckbox;
        CheckboxButton mDrawBehindPlayerCheckbox;
        CheckboxButton mFloorHuggingCheckbox;
        

        TextField mLeftBlockingRadiusField;
        TextField mRightBlockingRadiusField;
        

        CheckboxButton *mClothingCheckboxes[NUM_CLOTHING_CHECKBOXES];
        const char *mClothingCheckboxNames[NUM_CLOTHING_CHECKBOXES];

        
        TextField mNumUsesField;
        CheckboxButton mUseVanishCheckbox;
        CheckboxButton mUseAppearCheckbox;
        
        CheckboxButton mSimUseCheckbox;
        ValueSlider mSimUseSlider;
        

        TextButton mDemoClothesButton;
        TextButton mEndClothesDemoButton;
        

        TextButton mDemoSlotsButton;
        TextButton mClearSlotsDemoButton;

        TextButton mSetHeldPosButton;
        TextButton mEndSetHeldPosButton;

        TextButton mNextHeldDemoButton;
        TextButton mPrevHeldDemoButton;

        TextButton mCopyHeldPosButton;
        TextButton mPasteHeldPosButton;
        
        
        TextButton mDemoVertRotButton;
        TextButton mResetVertRotButton;
        char mDemoVertRot;
        

        Picker mSpritePicker;
        Picker mObjectPicker;

        ValueSlider mPersonAgeSlider;

        ValueSlider mHueSlider;
        ValueSlider mSaturationSlider;
        ValueSlider mValueSlider;
        
        CheckboxButton mSlotVertCheckbox;

        SoundWidget mCreationSoundWidget;
        SoundWidget mUsingSoundWidget;
        SoundWidget mEatingSoundWidget;
        SoundWidget mDecaySoundWidget;
        
        CheckboxButton  mCreationSoundInitialOnlyCheckbox;
        
        char mPrintRequested;
        char mSavePrintOnly;
        

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
        
        int mStepsSinceDescriptionChange;
        
        double getClosestSpriteOrSlot( float inX, float inY,
                                       int *outSprite,
                                       int *outSlot );
        
        
        // ends demo and hides the buttons
        void hideVertRotButtons();
        
        void showVertRotButtons();
        
        void endVertRotDemo();
        


        void pickedLayerChanged();
        
        void updateSliderColors();


        void recomputeNumNonAgingSprites();
        

        void updateAgingPanel();
        
        char anyClothingToggled();

        void addNewSprite( int inSpriteID );

        
        void drawSpriteLayers( doublePair inDrawOffset,
                               char inBehindSlots );
        
        void recursiveRotate( ObjectRecord *inObject,
                              int inRotatingParentIndex,
                              doublePair inRotationCenter,
                              double inRotationDelta );

        void recursiveMove( ObjectRecord *inObject,
                            int inMovingParentIndex,
                            doublePair inMoveDelta );
            


        FloatRGB mColorClipboard;
        doublePair mHeldOffsetClipboard;

        doublePair mSaveDeltaPosClipboard;
        double mSaveDeltaRotClipboard;

        int mHoverObjectLayer;
        int mHoverSlot;
        
        // fades when mouse stops moving
        float mHoverStrength;
        
        // alpha for blinking on hover
        float mHoverFlash;

        int mHoverFrameCount;

        char mRotAdjustMode;
        float mRotStartMouseX;
        
        char mIgnoreParentLinkMode;

        double mLayerOldRot;


        KeyLegend mKeyLegend;
        KeyLegend mKeyLegendB;
        KeyLegend mKeyLegendC;
        KeyLegend mKeyLegendD;
    };



#endif
