#ifndef EDITOR_ANIMATION_PAGE_INCLUDED
#define EDITOR_ANIMATION_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "CheckboxButton.h"

#include "SpriteToggleButton.h"

#include "ValueSlider.h"


#include "Picker.h"
#include "SoundWidget.h"

#include "animationBank.h"
#include "objectBank.h"
#include "keyLegend.h"


#define NUM_ANIM_CHECKBOXES 6
#define NUM_ANIM_SLIDERS 19



typedef struct WalkAnimationRecord {
    } WalkAnimationRecord;



class EditorAnimationPage : public GamePage, public ActionListener {
        
    public:
        EditorAnimationPage();
        ~EditorAnimationPage();        

        void clearClothing();
        
        void clearUseOfObject( int inObjectID );

        void objectLayersChanged( int inObjectID );
        

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void drawUnderComponents( doublePair inViewCenter, 
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
        
        SpriteHandle mCenterMarkSprite;

        SpriteHandle mGroundSprite;
        

        TextButton mObjectEditorButton;
        TextButton mSceneEditorButton;

        TextButton mSaveButton;
        TextButton mDeleteButton;
        Picker mObjectPicker;

        SoundWidget mSoundWidget;

        ValueSlider mSoundRepeatPerSecSlider;
        ValueSlider mSoundRepeatPhaseSlider;


        TextField mSoundAgeInField;
        TextField mSoundAgeOutField;
        
        TextButton mSoundAgePunchInButton;
        TextButton mSoundAgePunchOutButton;

        SpriteToggleButton mSoundFootstepButton;


        ValueSlider mPersonAgeSlider;
        TextButton mPlayAgeButton;
        
        char mPlayingAge;

        ValueSlider mTestSpeedSlider;
        double mLastTestSpeed;

        double mFrameTimeOffset;
        

        CheckboxButton *mCheckboxes[NUM_ANIM_CHECKBOXES];
        const char *mCheckboxNames[NUM_ANIM_CHECKBOXES];
        AnimType mCheckboxAnimTypes[NUM_ANIM_CHECKBOXES];
        
        TextButton mPrevExtraButton, mNextExtraButton, mDelExtraButton;

        void setNextExtraButtonColor();
        

        ValueSlider *mSliders[ NUM_ANIM_SLIDERS ];
        
        ValueSlider *mXOffsetSlider;
        ValueSlider *mYOffsetSlider;
        

        CheckboxButton mReverseRotationCheckbox;
        
        CheckboxButton mRandomStartPhaseCheckbox;
        CheckboxButton mForceZeroStartCheckbox;

        
        int mCurrentObjectID;
        int mCurrentSlotDemoID;

        char mFlipDraw;
        char mHideUI;
        
        double mCurrentObjectFrameRateFactor;
        
        // endAnimType index allways NULL
        // extra and extraB indices sometimes contain 
        //   pointers to mCurrentExtraAnim record
        //   from mCurrentExtraIndex
        AnimationRecord *mCurrentAnim[ extraB + 1 ];
        
        SimpleVector<AnimationRecord*> mCurrentExtraAnim;
        
        int mCurrentExtraIndex;
        


        AnimationRecord *mWiggleAnim;
        
        double mWiggleFade;
        int mWiggleSpriteOrSlot;

        double mRotCenterFade;
        

        AnimType mCurrentType;
        AnimType mLastType;

        double mLastTypeFade;
        
        int mFrameCount;
        int mLastTypeFrameCount;
        
        int mFrozenRotFrameCount;

        char mFrozenRotFrameCountUsed;


        int mCurrentSound;
        
        int mCurrentSpriteOrSlot;

        char mSettingRotCenter;
        
        TextButton mPickSlotDemoButton;
        
        char mPickingSlotDemo;

        TextButton mClearSlotDemoButton;
        

        TextButton mPickClothingButton;
        
        char mPickingClothing;
        
        ClothingSet mClothingSet;
        
        ObjectRecord **mNextShoeToFill;
        ObjectRecord **mOtherShoe;
        
        TextButton mClearClothingButton;


        TextButton mPickHeldButton;
        char mPickingHeld;
        int mHeldID;

        TextButton mClearHeldButton;


        TextButton mPickSceneryButton;
        char mPickingScenery;
        int mSceneryID;
        
        TextButton mClearSceneryButton;
        
        


        SpriteAnimationRecord mCopyBuffer;
        SoundAnimationRecord mSoundAnimCopyBuffer;
        
        SimpleVector<SpriteAnimationRecord> mChainCopyBuffer;

        SimpleVector<SoundAnimationRecord> mAllCopyBufferSounds;

        // ID of sprite from source of copy
        // we don't want to paste these on different sprites
        SimpleVector<int> mAllCopySpriteIDs;
        SimpleVector<SpriteAnimationRecord> mAllCopyBufferSprites;
        SimpleVector<SpriteAnimationRecord> mAllCopyBufferSlots;
        

        SimpleVector<SoundAnimationRecord> mFullSoundCopyBuffer[ endAnimType ];
        
        
        void clearAllCopyBufferSounds();

        double computeFrameTime();
        

        char mWalkCopied;
        char mUpCopied;
        SpriteAnimationRecord mCopiedHeadAnim;
        SpriteAnimationRecord mCopiedBodyAnim;
        
        SimpleVector<SpriteAnimationRecord> mCopiedBackFootAnimChain;
        SimpleVector<SpriteAnimationRecord> mCopiedFrontFootAnimChain;
        
        SimpleVector<SpriteAnimationRecord> mCopiedBackHandAnimChain;
        SimpleVector<SpriteAnimationRecord> mCopiedFrontHandAnimChain;


        TextButton mCopyButton;
        TextButton mCopyChainButton;
        TextButton mCopyWalkButton;
        TextButton mCopyAllButton;
        TextButton mCopyUpButton;

        TextButton mPasteButton;
        TextButton mClearButton;
        

        TextButton mNextSpriteOrSlotButton;
        TextButton mPrevSpriteOrSlotButton;
        
        TextButton mChildButton;
        TextButton mParentButton;
        

        TextButton mNextSoundButton;
        TextButton mPrevSoundButton;

        TextButton mCopySoundAnimButton;
        TextButton mCopyAllSoundAnimButton;
        TextButton mPasteSoundAnimButton;


        TextButton mFullSoundCopyButton;
        TextButton mFullSoundPasteButton;
        

        void checkNextPrevVisible();
        
        // if inPageChange is false, SoundWidget sound usage is not
        // reset
        void soundIndexChanged( char inPageChange=true );
        

        void updateAnimFromSliders();
        void updateSlidersFromAnim();



        void freeCurrentAnim();
        void populateCurrentAnim();
        

        void setWiggle();
        
        int getClosestSpriteOrSlot( float inX, float inY );
        

        
        SpriteAnimationRecord *getRecordForCurrentSlot(
            char *outIsSprite = NULL );
        

        KeyLegend mKeyLegend, mKeyLegendB;
    };



#endif
