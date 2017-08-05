#include "EditorAnimationPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"


#include "soundBank.h"
#include "zoomView.h"


extern Font *mainFont;
extern Font *smallFont;

extern double frameRateFactor;


#include "minorGems/util/random/JenkinsRandomSource.h"
static JenkinsRandomSource randSource;



#include "ObjectPickable.h"
#include "whiteSprites.h"

static ObjectPickable objectPickable;

extern double defaultAge;


static float lastMouseX, lastMouseY;



EditorAnimationPage::EditorAnimationPage()
        : mCenterMarkSprite( loadSprite( "centerMark.tga" ) ),
          mGroundSprite( loadWhiteSprite( "testGround.tga" ) ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mSaveButton( smallFont, 0, 200, "Save" ),
          mDeleteButton( smallFont, 140, 200, "Delete" ),
          mObjectPicker( &objectPickable, +410, 90 ),
          mSoundWidget( smallFont, -120, -200 ),
          mSoundRepeatPerSecSlider( smallFont, -120, -264, 2,
                                    100, 20,
                                    0, 6, "Loop Hz" ),
          mSoundRepeatPhaseSlider( smallFont, -120, -296, 2,
                                    100, 20,
                                    0, 1, "Loop Phase" ),
          mSoundAgeInField( smallFont, 
                            -180,  -328, 6,
                            false,
                            "In", "0123456789.", NULL ),
          mSoundAgeOutField( smallFont, 
                             -50,  -328, 6,
                             false,
                             "Out", "0123456789.", NULL ),
          mSoundAgePunchInButton( smallFont, -130, -328, "S" ),
          mSoundAgePunchOutButton( smallFont, -10, -328, "S" ),
          mPersonAgeSlider( smallFont, 100, -212, 2,
                            100, 20,
                            0, 100, "Age" ),
          mPlayAgeButton( smallFont, 264, -212, "P" ),
          mPlayingAge( false ),
          mTestSpeedSlider( smallFont, 100, -170, 2,
                            100, 20,
                            0, 1, "Test Speed" ),
          mReverseRotationCheckbox( 0, 0, 2 ),
          mRandomStartPhaseCheckbox( -80, 200, 2 ),
          mCurrentObjectID( -1 ),
          mCurrentSlotDemoID( -1 ),
          mFlipDraw( false ),
          mWiggleAnim( NULL ),
          mWiggleFade( 0.0 ),
          mWiggleSpriteOrSlot( -1 ),
          mRotCenterFade( 0.0 ),
          mCurrentType( ground ),
          mLastType( ground ),
          mLastTypeFade( 0 ),
          mFrameCount( 0 ),
          mLastTypeFrameCount( 0 ),
          mFrozenRotFrameCount( 0 ),
          mFrozenRotFrameCountUsed( false ),
          mCurrentSound( 0 ),
          mCurrentSpriteOrSlot( 0 ),
          mSettingRotCenter( false ),
          mPickSlotDemoButton( smallFont, 280, 60, "Fill Slots" ),
          mPickingSlotDemo( false ),
          mClearSlotDemoButton( smallFont, 280, -90, "Clear Slots" ),
          mPickClothingButton( smallFont, 280, 60, "+ Clothes" ),
          mPickingClothing( false ),
          mClearClothingButton( smallFont, 280, 120, "X Clothes" ),
          mPickHeldButton( smallFont, 280, -100, "+ Held" ),
          mPickingHeld( false ),
          mHeldID( -1 ),
          mClearHeldButton( smallFont, 280, -140, "X Held" ),
          mCopyButton( smallFont, -390, 230, "Copy" ),
          mCopyChainButton( smallFont, -390, 270, "Copy Child Chain" ),
          mCopyWalkButton( smallFont, -260, 270, "Copy Walk" ),
          mCopyAllButton( smallFont, -470, 230, "Copy All" ),
          mCopyUpButton( smallFont, -500, 270, "Copy Up" ),
          mPasteButton( smallFont, -330, 230, "Paste" ),
          mClearButton( smallFont, -270, 230, "Clear" ),
          mNextSpriteOrSlotButton( smallFont, 280, -270, "Next Layer" ),
          mPrevSpriteOrSlotButton( smallFont, 100, -270, "Prev Layer" ),
          mNextSoundButton( smallFont, -30, -160, ">" ),
          mPrevSoundButton( smallFont, -210, -160, "<" ),
          mCopySoundAnimButton( smallFont, -85, -160, "Copy" ),
          mCopyAllSoundAnimButton( smallFont, -85, -160, "Copy All" ),
          mPasteSoundAnimButton( smallFont, -155, -160, "Paste" ) {
    
    
    for( int i=0; i<endAnimType; i++ ) {
        mCurrentAnim[i] = NULL;
        }
    
    zeroRecord( &mCopyBuffer );
    zeroRecord( &mSoundAnimCopyBuffer );
    
    mWalkCopied = false;
    mUpCopied = false;
    
    addComponent( &mObjectEditorButton );
    addComponent( &mSaveButton );
    addComponent( &mDeleteButton );
    
    addComponent( &mObjectPicker );


    addComponent( &mSoundWidget );
    
    mSoundWidget.addActionListener( this );


    addComponent( &mSoundAgeInField );
    addComponent( &mSoundAgeOutField );
    
    addComponent( &mSoundAgePunchInButton );
    addComponent( &mSoundAgePunchOutButton );
    
    mSoundAgePunchInButton.addActionListener( this );
    mSoundAgePunchOutButton.addActionListener( this );

    mSoundAgeInField.setVisible( false );
    mSoundAgeOutField.setVisible( false );
    mSoundAgePunchInButton.setVisible( false );
    mSoundAgePunchOutButton.setVisible( false );


    mSoundAgeInField.addActionListener( this );
    mSoundAgeOutField.addActionListener( this );

    mSoundAgeInField.setFireOnLoseFocus( true );
    mSoundAgeOutField.setFireOnLoseFocus( true );

    
    mSoundAgePunchOutButton.addActionListener( this );
    mSoundAgePunchInButton.addActionListener( this );
    


    addComponent( &mNextSoundButton );
    addComponent( &mPrevSoundButton );
    mNextSoundButton.addActionListener( this );
    mPrevSoundButton.addActionListener( this );
    
    mNextSoundButton.setVisible( false );
    mPrevSoundButton.setVisible( false );

    addComponent( &mCopySoundAnimButton );
    addComponent( &mCopyAllSoundAnimButton );
    addComponent( &mPasteSoundAnimButton );
    mCopySoundAnimButton.addActionListener( this );
    mCopyAllSoundAnimButton.addActionListener( this );
    mPasteSoundAnimButton.addActionListener( this );

    mCopySoundAnimButton.setVisible( false );
    mCopyAllSoundAnimButton.setVisible( false );
    mPasteSoundAnimButton.setVisible( false );


    addComponent( &mSoundRepeatPerSecSlider );
    addComponent( &mSoundRepeatPhaseSlider );
    

    mSoundWidget.setVisible( false );
    mSoundRepeatPerSecSlider.setVisible( false );
    mSoundRepeatPhaseSlider.setVisible( false );

    
    mSoundRepeatPerSecSlider.addActionListener( this );
    mSoundRepeatPhaseSlider.addActionListener( this );


    addComponent( &mPersonAgeSlider );
    
    mPersonAgeSlider.setVisible( false );
    mPersonAgeSlider.setValue( defaultAge );
    
    addComponent( &mPlayAgeButton );
    mPlayAgeButton.addActionListener( this );
    mPlayAgeButton.setVisible( false );


    addComponent( &mTestSpeedSlider );
    
    mTestSpeedSlider.setVisible( true );

    mTestSpeedSlider.setValue( 1 );
    

    mTestSpeedSlider.addActionListener( this );
    
    mLastTestSpeed = 1.0;
    mFrameTimeOffset = 0;
    

    addComponent( &mPickSlotDemoButton );
    addComponent( &mClearSlotDemoButton );

    addComponent( &mPickClothingButton );
    addComponent( &mClearClothingButton );
    
    addComponent( &mPickHeldButton );
    addComponent( &mClearHeldButton );

    addComponent( &mCopyButton );
    addComponent( &mCopyChainButton );
    addComponent( &mCopyWalkButton );
    addComponent( &mCopyAllButton );
    addComponent( &mCopyUpButton );
    addComponent( &mPasteButton );
    addComponent( &mClearButton );
    

    addComponent( &mNextSpriteOrSlotButton );
    addComponent( &mPrevSpriteOrSlotButton );
    

    mObjectEditorButton.addActionListener( this );
    mSaveButton.addActionListener( this );
    mDeleteButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mPickSlotDemoButton.addActionListener( this );
    mClearSlotDemoButton.addActionListener( this );

    mPickClothingButton.addActionListener( this );
    mClearClothingButton.addActionListener( this );

    mPickHeldButton.addActionListener( this );
    mClearHeldButton.addActionListener( this );

    mCopyButton.addActionListener( this );
    mCopyChainButton.addActionListener( this );
    mCopyWalkButton.addActionListener( this );
    mCopyAllButton.addActionListener( this );
    mCopyUpButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mClearButton.addActionListener( this );
    

    mNextSpriteOrSlotButton.addActionListener( this );
    mPrevSpriteOrSlotButton.addActionListener( this );

    mClearSlotDemoButton.setVisible( false );

    mPickClothingButton.setVisible( false );
    mClearClothingButton.setVisible( false );
    
    mPickHeldButton.setVisible( false );
    mClearHeldButton.setVisible( false );

    checkNextPrevVisible();
    
    
    double boxY = 20;
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 310, boxY, 2 );
        addComponent( mCheckboxes[i] );

        mCheckboxes[i]->addActionListener( this );
        boxY -= 20;
        }
    mCheckboxNames[0] = "Ground";
    mCheckboxNames[1] = "Held";
    mCheckboxNames[2] = "Moving";
    mCheckboxNames[3] = "Eating";
    mCheckboxNames[4] = "Doing";

    mCheckboxAnimTypes[0] = ground;
    mCheckboxAnimTypes[1] = held;
    mCheckboxAnimTypes[2] = moving;
    mCheckboxAnimTypes[3] = eating;
    mCheckboxAnimTypes[4] = doing;

    mCheckboxes[0]->setToggled( true );
    
    mCheckboxes[3]->setVisible( false );
    mCheckboxes[4]->setVisible( false );


    boxY = 220;
    
    double space = 28;
    double x = -390;
    
    mSliders[0] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "X Osc" );
    mSliders[1] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 256, "X Amp" );
    mSliders[2] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "X Phase" );

    mSliders[3] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "Y Osc" );
    mSliders[4] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 256, "Y Amp" );
    mSliders[5] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Y Phase" );
    
    mSliders[6] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 12, "Rot" );
    
    mReverseRotationCheckbox.setPosition( x - 65, boxY );

    addComponent( &mReverseRotationCheckbox );
    mReverseRotationCheckbox.addActionListener( this );

    mSliders[7] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Rot Phase" );

    mSliders[8] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "Rock Osc" );
    mSliders[9] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Rock Amp" );
    mSliders[10] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                    100, 20,
                                    0, 1, "Rock Phase" );


    mSliders[11] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "Fade Osc" );
    mSliders[12] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Fade Hard" );
    mSliders[13] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Fade Min" );
    mSliders[14] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Fade Max" );
    mSliders[15] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Fade Phase" );

    
    mSliders[16] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                    100, 20,
                                    0, 100, "Duration Sec" );
    mSliders[17] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                    100, 20,
                                    0, 100, "Pause Sec" );




    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        addComponent( mSliders[i] );

        mSliders[i]->addActionListener( this );
        mSliders[i]->setVisible( false );
        }
    mReverseRotationCheckbox.setVisible( false );

    addComponent( &mRandomStartPhaseCheckbox );
    mRandomStartPhaseCheckbox.addActionListener( this );
    
    mClothingSet = getEmptyClothingSet();
    mNextShoeToFill = &( mClothingSet.backShoe );
    mOtherShoe = &( mClothingSet.frontShoe );

    addKeyClassDescription( &mKeyLegend, "R-Click/arrows", 
                            "Move layer rot anchor" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyDescription( &mKeyLegend, 'f', "Flip horizontally" );
    
    addKeyClassDescription( &mKeyLegendB, "R-Click", "Copy animations" );
    }



EditorAnimationPage::~EditorAnimationPage() {
    freeCurrentAnim();
    
    for( int i=0; i<mAllCopyBufferSounds.size(); i++ ) {
        unCountLiveUse( mAllCopyBufferSounds.getElementDirect(i).sound.id );
        }

    if( mSoundAnimCopyBuffer.sound.id != -1 ) {
        unCountLiveUse( mSoundAnimCopyBuffer.sound.id );
        }
        
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
        }
    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        delete mSliders[i];
        }
    
    freeSprite( mCenterMarkSprite );
    freeSprite( mGroundSprite );
    }



void EditorAnimationPage::clearClothing() {
    mClothingSet.hat = NULL;
    mClothingSet.frontShoe = NULL;
    mClothingSet.backShoe = NULL;
    mClothingSet.tunic = NULL;
    mClothingSet.bottom = NULL;
    mClothingSet.backpack = NULL;
    mClearClothingButton.setVisible( false );
    }




void EditorAnimationPage::freeCurrentAnim() {
    
    for( int i=0; i<endAnimType; i++ ) {
        if( mCurrentAnim[i] != NULL ) {
            
            freeRecord( mCurrentAnim[i] );
            mCurrentAnim[i] = NULL;
            }
        }
    if( mWiggleAnim != NULL ) {
        freeRecord( mWiggleAnim );
        mWiggleAnim = NULL;
        }
    }



static void adjustRecordList( SpriteAnimationRecord **inOldList,
                              int inOldSize,
                              int inNewSize ) {
    if( inOldSize == inNewSize ) {
        // do nothing
        return;
        }
    
    SpriteAnimationRecord *newRecords =
        new SpriteAnimationRecord[ inNewSize ];
            
    int smallerSize = inOldSize;
    if( inNewSize < inOldSize ) {
        smallerSize = inNewSize;
        }
    memcpy( newRecords, *inOldList, 
            sizeof( SpriteAnimationRecord ) * smallerSize );
    
    if( inNewSize > inOldSize ) {
        // zero any extras in new
        for( int i=inOldSize; i<inNewSize; i++ ) {
            zeroRecord( &( newRecords[i] ) );
            }
        }
    delete [] ( *inOldList );
    
    *inOldList = newRecords;    
    }



void EditorAnimationPage::populateCurrentAnim() {
    freeCurrentAnim();

    ObjectRecord *obj = getObject( mCurrentObjectID );

    mCurrentObjectFrameRateFactor = frameRateFactor;
    
    if( obj->speedMult < 1.0 ) {    
        mCurrentObjectFrameRateFactor =  frameRateFactor * obj->speedMult;
        }
    

    for( int i=0; i<endAnimType; i++ ) {

        AnimationRecord *oldRecord =
            getAnimation( mCurrentObjectID, (AnimType)i );
            
        int sprites = obj->numSprites;
        int slots = obj->numSlots;
        
        if( oldRecord == NULL ) {
            // no anim exists
            mCurrentAnim[i] = new AnimationRecord;
        
            mCurrentAnim[i]->objectID = mCurrentObjectID;
            mCurrentAnim[i]->type = (AnimType)i;
        
            mCurrentAnim[i]->randomStartPhase = false;
        
            mCurrentAnim[i]->numSounds = 0;
            mCurrentAnim[i]->soundAnim = new SoundAnimationRecord[ 0 ];
            
            mCurrentAnim[i]->numSprites = sprites;
            mCurrentAnim[i]->numSlots = slots;
            
            mCurrentAnim[i]->spriteAnim = 
                new SpriteAnimationRecord[ sprites ];
            
            mCurrentAnim[i]->slotAnim = 
                new SpriteAnimationRecord[ slots ];
            
            for( int j=0; j<sprites; j++ ) {
                zeroRecord( &( mCurrentAnim[i]->spriteAnim[j] ) );
                }
            for( int j=0; j<slots; j++ ) {
                zeroRecord( &( mCurrentAnim[i]->slotAnim[j] ) );
                }
            }
        else {
            mCurrentAnim[i] = copyRecord( oldRecord );
        
            for( int s=0; s<mCurrentAnim[i]->numSounds; s++ ) {
                countLiveUse( mCurrentAnim[i]->soundAnim[s].sound.id );
                }

            adjustRecordList( &( mCurrentAnim[i]->spriteAnim ),
                              mCurrentAnim[i]->numSprites,
                              sprites );
            
            mCurrentAnim[i]->numSprites = sprites;
            
            adjustRecordList( &( mCurrentAnim[i]->slotAnim ),
                              mCurrentAnim[i]->numSlots,
                              slots );
            
            mCurrentAnim[i]->numSlots = slots;
            }        
    
        }
    
    mRandomStartPhaseCheckbox.setToggled( 
        mCurrentAnim[ mCurrentType ]->randomStartPhase );
    
    mWiggleAnim = copyRecord( mCurrentAnim[0] );

    
    updateSlidersFromAnim();
        
    if( mCurrentAnim[ mCurrentType ]->randomStartPhase ) {
        mFrameCount = randSource.getRandomBoundedInt( 0, 10000 );
        }
    else {
        mFrameCount = 0;
        }
    }



void EditorAnimationPage::setWiggle() {
    if( mWiggleAnim == NULL ) {
        return;
        }
    
    int sprites = mWiggleAnim->numSprites;
    int slots = mWiggleAnim->numSlots;

    SpriteAnimationRecord *r = NULL;
    
    int totalCount = 0;
    
    for( int i=0; i<sprites; i++ ) {
        zeroRecord( &( mWiggleAnim->spriteAnim[i] ) );
        
        if( totalCount == mWiggleSpriteOrSlot ) {
            r = &( mWiggleAnim->spriteAnim[i] );
            }
        totalCount++;
        }

    for( int i=0; i<slots; i++ ) {
        zeroRecord( &( mWiggleAnim->slotAnim[i] ) );
        
        if( totalCount == mWiggleSpriteOrSlot ) {
            r = &( mWiggleAnim->slotAnim[i] );
            }
        totalCount++;
        }

    if( r != NULL ) {
        //r->yOscPerSec = 2;
        //r->yAmp = 16 * mWiggleFade;
        }
    }



void EditorAnimationPage::checkNextPrevVisible() {
    if( mCurrentObjectID == -1 ) {
        mNextSpriteOrSlotButton.setVisible( false );
        mPrevSpriteOrSlotButton.setVisible( false );
    
        mPickSlotDemoButton.setVisible( false );
        mPickClothingButton.setVisible( false );
        
        mPickHeldButton.setVisible( false );
        
        mCopyChainButton.setVisible( false );
        mCopyWalkButton.setVisible( false );
        mCopyUpButton.setVisible( false );
        return;
        }
    
    ObjectRecord *r = getObject( mCurrentObjectID );

    int num = r->numSprites + r->numSlots;
    
    mNextSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot < num - 1 );
    mPrevSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot > 0 );
    

    if( mCurrentSpriteOrSlot < r->numSprites ) {
        mCopyChainButton.setVisible( true );
        mCopyUpButton.setVisible( true );
        }
    else {
        mCopyChainButton.setVisible( false );
        mCopyUpButton.setVisible( false );
        mChainCopyBuffer.deleteAll();
        }

    
    if( r->person ) {
        mCopyWalkButton.setVisible( true );
        }
    else {
        mCopyWalkButton.setVisible( false );
        }


    if( r->person ) {
        mPickClothingButton.setVisible( true );
        mPickHeldButton.setVisible( true );
        }
    else {
        mPickClothingButton.setVisible( false );
        mClearClothingButton.setVisible( false );
        mPickHeldButton.setVisible( false );
        mClearHeldButton.setVisible( false );
        
        mHeldID = -1;
        }
    
    if( r->numSlots > 0 ) {
        mPickSlotDemoButton.setVisible( true );
        }
    else {
        mPickSlotDemoButton.setVisible( false );
        mClearSlotDemoButton.setVisible( false );
        }
    }



void EditorAnimationPage::updateAnimFromSliders() {
    SpriteAnimationRecord *r;
    
    AnimationRecord *anim = mCurrentAnim[ mCurrentType ];
    
    if( mCurrentSpriteOrSlot > anim->numSprites - 1 ) {
        r = &( anim->slotAnim[ mCurrentSpriteOrSlot -
                               anim->numSprites ] );
        }
    else {
        r = &( anim->spriteAnim[ mCurrentSpriteOrSlot ] );
        }
    
    
    r->xOscPerSec = mSliders[0]->getValue();
    r->xAmp = mSliders[1]->getValue();
    r->xPhase = mSliders[2]->getValue();
    r->yOscPerSec = mSliders[3]->getValue();
    r->yAmp = mSliders[4]->getValue();
    r->yPhase = mSliders[5]->getValue();
    
    r->rotPerSec = mSliders[6]->getValue();
    r->rotPhase = mSliders[7]->getValue();

    if( mReverseRotationCheckbox.getToggled() ) {
        r->rotPerSec *= -1;
        r->rotPhase *= -1;
        }
    
    r->rockOscPerSec = mSliders[8]->getValue();
    r->rockAmp = mSliders[9]->getValue();
    r->rockPhase = mSliders[10]->getValue();


    r->fadeOscPerSec = mSliders[11]->getValue();
    r->fadeHardness = mSliders[12]->getValue();
    r->fadeMin = mSliders[13]->getValue();
    r->fadeMax = mSliders[14]->getValue();
    r->fadePhase = mSliders[15]->getValue();

    r->durationSec = mSliders[16]->getValue();
    r->pauseSec = mSliders[17]->getValue();
    }



SpriteAnimationRecord *EditorAnimationPage::getRecordForCurrentSlot(
    char *outIsSprite ) {
    
    SpriteAnimationRecord *r = NULL;

    AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

    if( mCurrentSpriteOrSlot > anim->numSprites - 1 ) {
        r = &( anim->slotAnim[ mCurrentSpriteOrSlot -
                               anim->numSprites ] );
        if( outIsSprite != NULL ) {
            *outIsSprite = false;
            }
        }
    else {
        r = &( anim->spriteAnim[ mCurrentSpriteOrSlot ] );
        
        if( outIsSprite != NULL ) {
            *outIsSprite = true;
            }
        }
    return r;
    }



void EditorAnimationPage::updateSlidersFromAnim() {
    char isSprite;

    SpriteAnimationRecord *r = getRecordForCurrentSlot( &isSprite );

    
    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        mSliders[i]->setVisible( true );
        }

    if( ! isSprite ) {
        // last five sliders (rotation, rock) not available for slots
        mSliders[6]->setVisible( false );
        mSliders[7]->setVisible( false );
        mReverseRotationCheckbox.setVisible( false );
        
        mSliders[8]->setVisible( false );
        mSliders[9]->setVisible( false );
        mSliders[10]->setVisible( false );

        // no fade
        mSliders[11]->setVisible( false );
        mSliders[12]->setVisible( false );
        mSliders[13]->setVisible( false );
        mSliders[14]->setVisible( false );
        mSliders[15]->setVisible( false );
        }
    else {
        // last two sliders (rotation) are available for sprites
        mSliders[6]->setVisible( true );
        mSliders[7]->setVisible( true );
        mReverseRotationCheckbox.setVisible( true );
        
        mSliders[8]->setVisible( true );
        mSliders[9]->setVisible( true );
        mSliders[10]->setVisible( true );

        // yes fade
        mSliders[11]->setVisible( true );
        mSliders[12]->setVisible( true );
        mSliders[13]->setVisible( true );
        mSliders[14]->setVisible( true );
        mSliders[15]->setVisible( true );
        }
    
    
    mSliders[0]->setValue( r->xOscPerSec );
    mSliders[1]->setValue( r->xAmp );
    mSliders[2]->setValue( r->xPhase );
    mSliders[3]->setValue( r->yOscPerSec );
    mSliders[4]->setValue( r->yAmp );
    mSliders[5]->setValue( r->yPhase );
    
    mSliders[6]->setValue( fabs( r->rotPerSec ) );
    mSliders[7]->setValue( fabs( r->rotPhase ) );

    mReverseRotationCheckbox.setToggled( r->rotPerSec < 0 ||
                                         r->rotPhase < 0 );

    mSliders[8]->setValue( r->rockOscPerSec );
    mSliders[9]->setValue( r->rockAmp );
    mSliders[10]->setValue( r->rockPhase );
    

    mSliders[11]->setValue( r->fadeOscPerSec );
    mSliders[12]->setValue( r->fadeHardness );
    mSliders[13]->setValue( r->fadeMin );
    mSliders[14]->setValue( r->fadeMax );
    mSliders[15]->setValue( r->fadePhase );

    mSliders[16]->setValue( r->durationSec );
    mSliders[17]->setValue( r->pauseSec );
    }

    
// copies animations starting at a given index and following
// the parent chain up to, but not including, the body
// puts the results in the passed-in vector (clearing it first)
static void copyChainToBody( ObjectRecord *inObject,
                             double inAge,
                             int inStartSpriteIndex,
                             int inBodyIndex,
                             AnimationRecord *inAnim,
                             SimpleVector<SpriteAnimationRecord> *inVector ) {
    inVector->deleteAll();
    
    inVector->push_back( inAnim->spriteAnim[ inStartSpriteIndex ] );

    int parent = inObject->spriteParent[ inStartSpriteIndex ];
    
    while( parent != -1 && parent != inBodyIndex ) {
        if( isSpriteVisibleAtAge( inObject, parent, inAge ) ) {
            inVector->push_back( inAnim->spriteAnim[ parent ] );
            }
        parent = inObject->spriteParent[ parent ];
        }
    }


static void pasteChainToBody( ObjectRecord *inObject,
                              double inAge,
                              int inStartSpriteIndex,
                              int inBodyIndex,
                              AnimationRecord *inAnim,
                              SimpleVector<SpriteAnimationRecord> *inVector ) {
    
    int i = 0;
    inAnim->spriteAnim[ inStartSpriteIndex ] = inVector->getElementDirect( i );
    i++;
    
    int parent = inObject->spriteParent[ inStartSpriteIndex ];
    
    while( parent != -1 && parent != inBodyIndex ) {
        if( isSpriteVisibleAtAge( inObject, parent, inAge ) ) {
            inAnim->spriteAnim[ parent ] = inVector->getElementDirect( i );
            i++;
            }
        
        parent = inObject->spriteParent[ parent ];
        }
    }




void EditorAnimationPage::soundIndexChanged() {
    if( mCurrentSound < mCurrentAnim[ mCurrentType ]->numSounds - 1 
        ||
        ( mCurrentSound == mCurrentAnim[ mCurrentType ]->numSounds -1 &&
          mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ].sound.id 
          != -1 ) ) {
            
        mNextSoundButton.setVisible( true );
        }
    else {
        mNextSoundButton.setVisible( false );
        }

    if( mCurrentSound > 0 ) {
        mPrevSoundButton.setVisible( true );
        }
    else {
        mPrevSoundButton.setVisible( false );
        }
    
    if( mSoundWidget.isRecording() ) {
        // don't allow switching sound layers while recording
        mPrevSoundButton.setVisible( false );
        mNextSoundButton.setVisible( false );
        }
    

    if( mCurrentSound < mCurrentAnim[ mCurrentType ]->numSounds ) {
        if( !mSoundWidget.isRecording() ) {    
            mSoundWidget.setSoundUsage( 
                mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].sound );
            }
        
        mSoundRepeatPerSecSlider.setValue( 
            mCurrentAnim[ mCurrentType ]->
            soundAnim[ mCurrentSound ].repeatPerSec );
        
        mSoundRepeatPhaseSlider.setValue( 
            mCurrentAnim[ mCurrentType ]->
            soundAnim[ mCurrentSound ].repeatPhase );
        
        mSoundRepeatPerSecSlider.setVisible( true );
        mSoundRepeatPhaseSlider.setVisible( true );
        
        mCopySoundAnimButton.setVisible( true );
        mCopyAllSoundAnimButton.setVisible( false );

        char person = getObject( mCurrentObjectID )->person;
        
        mSoundAgeInField.setVisible( person );
        mSoundAgeOutField.setVisible( person );
        mSoundAgePunchInButton.setVisible( person );
        mSoundAgePunchOutButton.setVisible( person );

        if( person ) {

            double in = mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].ageStart;
            double out = mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].ageEnd;

            if( in == -1 ) {
                in = 0;
                }
            if( out == -1 ) {
                out = 999.0;
                }
            mSoundAgeInField.setFloat( in,  2 );
            
            mSoundAgeOutField.setFloat( out,  2 );
            }
        }
    else {
        mSoundWidget.setSoundUsage( blankSoundUsage );

        mSoundRepeatPerSecSlider.setVisible( false );
        mSoundRepeatPhaseSlider.setVisible( false );        

        mSoundAgeInField.setVisible( false );
        mSoundAgeOutField.setVisible( false );
        mSoundAgePunchInButton.setVisible( false );
        mSoundAgePunchOutButton.setVisible( false );
        
        mCopySoundAnimButton.setVisible( false );

        if( mCurrentAnim[ mCurrentType ]->numSounds > 0 ) {    
            mCopyAllSoundAnimButton.setVisible( true );
            }
        else {
            mCopyAllSoundAnimButton.setVisible( false );
            }
        }
    
    mPasteSoundAnimButton.setVisible( mSoundAnimCopyBuffer.sound.id != -1 ||
                                      mAllCopyBufferSounds.size() > 0 );


    mSoundWidget.setVisible( true );
    }



void EditorAnimationPage::markAllCopyBufferSoundsNotLive() {
    for( int i=0; i<mAllCopyBufferSounds.size(); i++ ) {
        unCountLiveUse( mAllCopyBufferSounds.getElementDirect( i ).sound.id );
        }
    }



void EditorAnimationPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mSaveButton ) {
        for( int i=0; i<endAnimType; i++ ) {
            addAnimation( mCurrentAnim[i] );
            }
        }
    else if( inTarget == &mDeleteButton ) {
        for( int i=0; i<endAnimType; i++ ) {
            clearAnimation( mCurrentAnim[i]->objectID, 
                            mCurrentAnim[i]->type );
            }
        mCurrentSound = 0;
        mCurrentSpriteOrSlot = 0;
                
        checkNextPrevVisible();
        populateCurrentAnim();
        soundIndexChanged();
        }    
    else if( inTarget == &mClearButton ) {
        zeroRecord( getRecordForCurrentSlot() );
        updateSlidersFromAnim();
        }
    else if( inTarget == &mCopySoundAnimButton ) {
        if( mSoundAnimCopyBuffer.sound.id != -1 ) {
            unCountLiveUse( mSoundAnimCopyBuffer.sound.id );
            }
        mSoundAnimCopyBuffer = 
            mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ];
        
        if( mSoundAnimCopyBuffer.sound.id != -1 ) {
            countLiveUse( mSoundAnimCopyBuffer.sound.id );
            mPasteSoundAnimButton.setVisible( true );
            }
        }
    else if( inTarget == &mPlayAgeButton ) {
        if( !mPlayingAge ) {
            mPlayingAge = true;
            }
        else {
            mPlayingAge = false;
            }
        }
    else if( inTarget == &mCopyAllSoundAnimButton ) {
        
        markAllCopyBufferSoundsNotLive();

        mAllCopyBufferSounds.deleteAll();
        
        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        for( int i=0; i<anim->numSounds; i++ ) {
            mAllCopyBufferSounds.push_back( anim->soundAnim[i] );
            countLiveUse( anim->soundAnim[i].sound.id );
            }
        mPasteSoundAnimButton.setVisible( mAllCopyBufferSounds.size() > 0 );
        }
    else if( inTarget == &mPasteSoundAnimButton ) {
        if( mAllCopyBufferSounds.size() > 0 ) {
            // paste beyond end

            if( mCurrentAnim[ mCurrentType ]->numSounds > 0 ) {
                mCurrentSound++;
                }
            
            for( int i=0; i<mAllCopyBufferSounds.size(); i++ ) {
                
                mCurrentAnim[ mCurrentType ]->numSounds++;
                SoundAnimationRecord *old = mCurrentAnim[ mCurrentType ]->
                    soundAnim;
                
                mCurrentAnim[ mCurrentType ]->soundAnim = 
                    new SoundAnimationRecord[ 
                        mCurrentAnim[ mCurrentType ]->numSounds ];
                
                memcpy( mCurrentAnim[ mCurrentType ]->soundAnim,
                        old,
                        sizeof( SoundAnimationRecord ) *
                        mCurrentAnim[ mCurrentType ]->numSounds - 1 );
                
                delete [] old;
                
                mCurrentAnim[ mCurrentType ]->soundAnim[ 
                    mCurrentAnim[ mCurrentType ]->numSounds - 1 ] =
                    
                    mAllCopyBufferSounds.getElementDirect( i );
                
                countLiveUse( mCurrentAnim[ mCurrentType ]->
                              soundAnim[ mCurrentSound ].sound.id );
                }
            }
        else {
            
            if( mCurrentSound <  mCurrentAnim[ mCurrentType ]->numSounds ) {
                // replace
            
                unCountLiveUse( mCurrentAnim[ mCurrentType ]->
                                soundAnim[ mCurrentSound ].sound.id );
                
                mSoundWidget.setSoundUsage( mCurrentAnim[ mCurrentType ]->
                                            soundAnim[ mCurrentSound ].sound );
                
                mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ] =
                    mSoundAnimCopyBuffer;
                }
            else {
                // add new
                mCurrentAnim[ mCurrentType ]->numSounds++;
                SoundAnimationRecord *old = mCurrentAnim[ mCurrentType ]->
                    soundAnim;
                
                mCurrentAnim[ mCurrentType ]->soundAnim = 
                    new SoundAnimationRecord[ 
                        mCurrentAnim[ mCurrentType ]->numSounds ];
                
                memcpy( mCurrentAnim[ mCurrentType ]->soundAnim,
                        old,
                        sizeof( SoundAnimationRecord ) *
                        mCurrentAnim[ mCurrentType ]->numSounds - 1 );
                
                delete [] old;

                mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ] =
                    mSoundAnimCopyBuffer;
                }
            
            if( mSoundAnimCopyBuffer.sound.id != -1 ) {
                countLiveUse( mSoundAnimCopyBuffer.sound.id );
                }
            }
        
        soundIndexChanged();
        }
    else if( inTarget == &mCopyButton ) {
        mCopyBuffer = *( getRecordForCurrentSlot() );
        mChainCopyBuffer.deleteAll();
        
        markAllCopyBufferSoundsNotLive();

        mAllCopyBufferSounds.deleteAll();
        mAllCopyBufferSprites.deleteAll();
        mAllCopyBufferSlots.deleteAll();
        mWalkCopied = false;
        mUpCopied = false;
        }
    else if( inTarget == &mCopyChainButton ) {
        mWalkCopied = false;
        mUpCopied = false;
        zeroRecord( &mCopyBuffer );
        
        mChainCopyBuffer.deleteAll();

        SpriteAnimationRecord *parentRecord = getRecordForCurrentSlot();
        
        mChainCopyBuffer.push_back( *parentRecord );
        
        int newParent = mCurrentSpriteOrSlot;
        
        ObjectRecord *r = getObject( mCurrentObjectID );

        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        while( newParent != -1 ) {
            int oldParent = newParent;
            newParent = -1;
            
            for( int i=0; i<r->numSprites; i++ ) {
                if( r->spriteParent[i] == oldParent ) {
                    // found a child

                    if( i < anim->numSprites ) {
                        mChainCopyBuffer.push_back( anim->spriteAnim[i] );
                        }
                    

                    newParent = i;
                    break;
                    }
                }
            }
        printf( "%d in copied chain\n", mChainCopyBuffer.size() );
        }
    else if( inTarget == &mCopyWalkButton ) {
        ObjectRecord *r = getObject( mCurrentObjectID );
        if( r->person ) {
            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

            double age = mPersonAgeSlider.getValue();

            int headIndex = getHeadIndex( r, age );
            int bodyIndex = getBodyIndex( r, age );
            int backFootIndex = getBackFootIndex( r, age );
            int frontFootIndex = getFrontFootIndex( r, age );

            if( headIndex != -1 ) {
                mCopiedHeadAnim = anim->spriteAnim[ headIndex ];
                }
            
            if( bodyIndex != -1 ) {
                mCopiedBodyAnim = anim->spriteAnim[ bodyIndex ];
                }
            
            if( frontFootIndex != -1 ) {
                copyChainToBody( r, age, frontFootIndex, bodyIndex, anim,
                                 &mCopiedFrontFootAnimChain );
                }
            
            if( backFootIndex != -1 ) {
                copyChainToBody( r, age, backFootIndex, bodyIndex, anim,
                                 &mCopiedBackFootAnimChain );
                }
            
            int frontHandIndex = getFrontHandIndex( r, age );
            int backHandIndex = getBackHandIndex( r, age );
            
            if( frontHandIndex != -1 ) {
                copyChainToBody( r, age, frontHandIndex, bodyIndex, anim,
                                 &mCopiedFrontHandAnimChain );
                }

            if( backHandIndex != -1 ) {
                copyChainToBody( r, age, backHandIndex, bodyIndex, anim,
                                 &mCopiedBackHandAnimChain );
                }
            

            mWalkCopied = true;
            mUpCopied = false;
            
            zeroRecord( &mCopyBuffer );
            
            mChainCopyBuffer.deleteAll();

            markAllCopyBufferSoundsNotLive();

            mAllCopyBufferSounds.deleteAll();
            mAllCopyBufferSprites.deleteAll();
            mAllCopyBufferSlots.deleteAll();
            }
        }
    else if( inTarget == &mCopyAllButton ) {
        mChainCopyBuffer.deleteAll();
        mWalkCopied = false;
        mUpCopied = false;
        zeroRecord( &mCopyBuffer );

        markAllCopyBufferSoundsNotLive();

        mAllCopyBufferSounds.deleteAll();
        mAllCopyBufferSprites.deleteAll();
        mAllCopyBufferSlots.deleteAll();
        
        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        for( int i=0; i<anim->numSounds; i++ ) {
            mAllCopyBufferSounds.push_back( anim->soundAnim[i] );
            countLiveUse( anim->soundAnim[i].sound.id );
            }
        for( int i=0; i<anim->numSprites; i++ ) {
            mAllCopyBufferSprites.push_back( anim->spriteAnim[i] );
            }
        for( int i=0; i<anim->numSlots; i++ ) {
            mAllCopyBufferSlots.push_back( anim->slotAnim[i] );
            }
        }
    else if( inTarget == &mCopyUpButton ) {
        mChainCopyBuffer.deleteAll();
        mWalkCopied = false;
        mUpCopied = true;
        zeroRecord( &mCopyBuffer );
        mChainCopyBuffer.deleteAll();


        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        for( int i=mCurrentSpriteOrSlot; i< anim->numSprites; i++ ) {
            
            mChainCopyBuffer.push_back( anim->spriteAnim[i] );
            }

        printf( "%d in copied up-chain\n", mChainCopyBuffer.size() );
        }
    else if( inTarget == &mPasteButton ) {
        ObjectRecord *r = getObject( mCurrentObjectID );
        if( mWalkCopied && r->person ) {
            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];
            
            double age = mPersonAgeSlider.getValue();

            int headIndex = getHeadIndex( r, age );
            int bodyIndex = getBodyIndex( r, age );
            int backFootIndex = getBackFootIndex( r, age );
            int frontFootIndex = getFrontFootIndex( r, age );

            if( headIndex != -1 ) {
                anim->spriteAnim[ headIndex ] = mCopiedHeadAnim;
                }
            if( bodyIndex != -1 ) {
                anim->spriteAnim[ bodyIndex ] = mCopiedBodyAnim;
                }
            
            if( frontFootIndex != -1 ) {
                pasteChainToBody( r, age, frontFootIndex, bodyIndex,
                                  anim, &mCopiedFrontFootAnimChain );
                }
            
            if( backFootIndex != -1 ) {
                pasteChainToBody( r, age, backFootIndex, bodyIndex,
                                  anim, &mCopiedBackFootAnimChain );
                }
            
                        
            int frontHandIndex = getFrontHandIndex( r, age );
            int backHandIndex = getBackHandIndex( r, age );
            
            if( frontHandIndex != -1 ) {
                pasteChainToBody( r, age, frontHandIndex, bodyIndex,
                                  anim, &mCopiedFrontHandAnimChain );
                }

            if( backHandIndex != -1 ) {
                pasteChainToBody( r, age, backHandIndex, bodyIndex,
                                  anim, &mCopiedBackHandAnimChain );
                }
            }
        else if( mUpCopied && mChainCopyBuffer.size() > 0 ) {
            // up chain copy paste

            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];
            
            int chainIndex = 0;
            for( int i=mCurrentSpriteOrSlot; i< anim->numSprites; i++ ) {
                anim->spriteAnim[i] =
                    mChainCopyBuffer.getElementDirect( 
                        chainIndex );
                
                chainIndex ++;
                }
            }
        else if( mChainCopyBuffer.size() > 0 ) {
            // child chain copy paste
            
            SpriteAnimationRecord *parentRecord = getRecordForCurrentSlot();
            int newParent = mCurrentSpriteOrSlot;
        
            *parentRecord = mChainCopyBuffer.getElementDirect( 0 );
            
            int chainIndex = 1;
            int chainLength = mChainCopyBuffer.size();

            ObjectRecord *r = getObject( mCurrentObjectID );
            
            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];
            
            while( newParent != -1 && chainIndex < chainLength ) {
                int oldParent = newParent;
                newParent = -1;
            
                for( int i=0; i<r->numSprites; i++ ) {
                    if( r->spriteParent[i] == oldParent ) {
                        // found a child
                        
                        if( i < anim->numSprites ) {
                            
                            anim->spriteAnim[i] =
                                mChainCopyBuffer.getElementDirect( 
                                    chainIndex );
                            
                            chainIndex++;
                            }   
                        
                        newParent = i;
                        break;
                        }
                    }
                }
            }
        else if( mAllCopyBufferSounds.size() > 0 ||
                 mAllCopyBufferSprites.size() > 0 ||
                 mAllCopyBufferSlots.size() > 0 ) {
            // paste all
            
            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

            for( int i=0; i<anim->numSounds; i++ ) {
                unCountLiveUse( anim->soundAnim[i].sound.id );
                }

            delete [] anim->soundAnim;
            
            mCurrentSound = 0;
            
            anim->numSounds = mAllCopyBufferSounds.size();
            anim->soundAnim = 
                new SoundAnimationRecord[ mAllCopyBufferSounds.size() ];
            
            for( int i=0; i < mAllCopyBufferSounds.size(); i++ ) {

                anim->soundAnim[i] = 
                    mAllCopyBufferSounds.getElementDirect( i );

                countLiveUse( anim->soundAnim[i].sound.id );
                }
            

            for( int i=0; 
                 i<anim->numSprites && i < mAllCopyBufferSprites.size(); 
                 i++ ) {

                anim->spriteAnim[i] = 
                    mAllCopyBufferSprites.getElementDirect( i );
                }
            for( int i=0; 
                 i<anim->numSlots && i < mAllCopyBufferSlots.size(); 
                 i++ ) {

                anim->slotAnim[i] = 
                    mAllCopyBufferSlots.getElementDirect( i );
                }
            
            soundIndexChanged();
            }
        else {
            // regular single-layer copy-paste

            char isSprite;
            SpriteAnimationRecord *r = getRecordForCurrentSlot( &isSprite );
            *r = mCopyBuffer;
            if( !isSprite ) {
                r->rotPerSec = 0;
                r->rotPhase = 0;
                r->rockOscPerSec = 0;
                r->rockAmp = 0;
                r->rockPhase = 0;
                }
            }
        updateSlidersFromAnim();
        }
    else if( inTarget == &mObjectPicker ) {
        int newPickID = mObjectPicker.getSelectedObject();

        if( newPickID != -1 ) {
            if( mPickingSlotDemo ) {
                mPickingSlotDemo = false;
                
                mCurrentSlotDemoID = newPickID;
                mPickSlotDemoButton.setVisible( true );
                mClearSlotDemoButton.setVisible( true );
                }
            else if( mPickingClothing ) {
                mPickingClothing = false;
                ObjectRecord *r = getObject( newPickID );
                switch( r->clothing ) {
                    case 's': {
                        *mNextShoeToFill = r;
                        ObjectRecord **temp = mNextShoeToFill;
                        mNextShoeToFill = mOtherShoe;
                        mOtherShoe = temp;
                        break;
                        }
                    case 'h':
                        mClothingSet.hat = r;
                        break;
                    case 't':
                        mClothingSet.tunic = r;
                        break;
                    case 'b':
                        mClothingSet.bottom = r;
                        break;
                    case 'p':
                        mClothingSet.backpack = r;
                        break;
                    }   
                        
                mPickClothingButton.setVisible( true );
                }
            else if( mPickingHeld ) {
                mHeldID = newPickID;

                ObjectRecord *obj = getObject( mHeldID );
                
                mCurrentObjectFrameRateFactor = frameRateFactor;
                
                if( obj->speedMult < 1.0 ) {
                    mCurrentObjectFrameRateFactor =  
                        frameRateFactor * obj->speedMult;
                    }
                }
            else {
                int oldID = mCurrentObjectID;
                
                mWiggleSpriteOrSlot = -1;
                
                char keepOldID = false;

                mCurrentObjectID = newPickID;
            
                if( oldID != -1 &&
                    oldID != mCurrentObjectID &&
                    isLastMouseButtonRight() ) {
                    // consider keeping old id and just copying all animations
                    ObjectRecord *oldObj = getObject( oldID );
                    ObjectRecord *newObj = getObject( mCurrentObjectID );
                    
                    if( oldObj->numSprites == newObj->numSprites &&
                        oldObj->numSlots == newObj->numSlots ) {
                        keepOldID = true;
                        }    
                    }
                

                mCurrentSound = 0;
                mCurrentSpriteOrSlot = 0;
                
                clearClothing();
                
                checkNextPrevVisible();
                
                populateCurrentAnim();
                soundIndexChanged();

                if( keepOldID ) {
                    mCurrentObjectID = oldID;
                    
                    for( int i=0; i<endAnimType; i++ ) {
                        mCurrentAnim[ i ]->objectID = mCurrentObjectID;
                        }
                    }

                if( getObject( mCurrentObjectID )->person ) {
                    mPersonAgeSlider.setVisible( true );
                    mPlayAgeButton.setVisible( true );
                    mCheckboxes[3]->setVisible( true );
                    mCheckboxes[4]->setVisible( true );
                    }
                else {
                    mPersonAgeSlider.setVisible( false );
                    mPlayAgeButton.setVisible( false );
                    mPlayingAge = false;
                    
                    if( mCheckboxes[3]->getToggled() || 
                        mCheckboxes[4]->getToggled() ) {
                        
                        actionPerformed( mCheckboxes[0] );
                        }
                    mCheckboxes[3]->setVisible( false );
                    mCheckboxes[4]->setVisible( false );
                    }
                }
            }
        }
    else if( inTarget == &mPickSlotDemoButton ) {
        mPickingSlotDemo = true;
        mPickingClothing = false;
        mPickingHeld = false;
        mCurrentSlotDemoID = -1;
        mPickSlotDemoButton.setVisible( false );
        mClearSlotDemoButton.setVisible( true );
        }
    else if( inTarget == &mClearSlotDemoButton ) {
        mPickingSlotDemo = false;
        mCurrentSlotDemoID = -1;
        mPickSlotDemoButton.setVisible( true );
        mClearSlotDemoButton.setVisible( false );
        }
    else if( inTarget == &mPickClothingButton ) {
        mPickingSlotDemo = false;
        mPickingClothing = true;
        mPickingHeld = false;
        mPickClothingButton.setVisible( false );
        mClearClothingButton.setVisible( true );
        }
    else if( inTarget == &mClearClothingButton ) {
        mPickingClothing = false;
        mPickClothingButton.setVisible( true );
        mClearClothingButton.setVisible( false );
        
        clearClothing();
        }
    else if( inTarget == &mPickHeldButton ) {
        mPickingSlotDemo = false;
        mPickingClothing = false;
        mPickingHeld = true;
        mPickHeldButton.setVisible( false );
        mClearHeldButton.setVisible( true );
        }
    else if( inTarget == &mClearHeldButton ) {
        mPickingHeld = false;
        mPickClothingButton.setVisible( true );
        mPickHeldButton.setVisible( true );
        mClearHeldButton.setVisible( false );
        mHeldID = -1;
        
        ObjectRecord *obj = getObject( mCurrentObjectID );
        
        mCurrentObjectFrameRateFactor = frameRateFactor;
        
        if( obj->speedMult < 1.0 ) {
            mCurrentObjectFrameRateFactor =  frameRateFactor * obj->speedMult;
            }
        }
    else if( inTarget == &mNextSpriteOrSlotButton ) {
        mCurrentSpriteOrSlot ++;
        
        mWiggleFade = 1.0;
        mWiggleSpriteOrSlot = mCurrentSpriteOrSlot;
        setWiggle();
        mFrameCount = 0;

        checkNextPrevVisible();
        updateSlidersFromAnim();
        }
    else if( inTarget == &mPrevSpriteOrSlotButton ) {
        mCurrentSpriteOrSlot --;
        
        mWiggleFade = 1.0;
        mWiggleSpriteOrSlot = mCurrentSpriteOrSlot;
        setWiggle();
        mFrameCount = 0;
        
        checkNextPrevVisible();
        updateSlidersFromAnim();
        }
    else if( inTarget == &mSoundWidget ) {
        SoundUsage u = mSoundWidget.getSoundUsage();
        
        if( ! mSaveButton.isVisible() && !mSoundWidget.isRecording() ) {
            // done recording now, restore save visibility
            mSaveButton.setVisible( true );
            mCopyAllButton.setVisible( true );
            }
        
        if( mCurrentSound < mCurrentAnim[ mCurrentType ]->numSounds ) {
            if( u.id != -1 ) { // just set it
                int oldID = mCurrentAnim[ mCurrentType ]->
                    soundAnim[ mCurrentSound ].sound.id;
                mCurrentAnim[ mCurrentType ]->
                    soundAnim[ mCurrentSound ].sound = u;
                countLiveUse( u.id );
                unCountLiveUse( oldID );
                }
            else if( !mSoundWidget.isRecording() ) {
                // delete this sound
                unCountLiveUse( mCurrentAnim[ mCurrentType ]->
                                soundAnim[ mCurrentSound ].sound.id );
                
                if( mCurrentAnim[ mCurrentType ]->numSounds > 
                    mCurrentSound + 1 ) {
                    
                    memcpy( &( mCurrentAnim[ mCurrentType ]->
                               soundAnim[ mCurrentSound ] ),
                            &( mCurrentAnim[ mCurrentType ]->
                               soundAnim[ mCurrentSound + 1 ] ),
                            sizeof( SoundAnimationRecord ) *
                            ( mCurrentAnim[ mCurrentType ]->numSounds -
                              mCurrentSound ) );
                    }
                mCurrentAnim[ mCurrentType ]->numSounds --;
                mCurrentSound = mCurrentAnim[ mCurrentType ]->numSounds;
                }
            else {
                // leave sound anim in place, but just clear soundID 
                // so it stops repeat-playing while we're recording
                // its replacement
                unCountLiveUse( mCurrentAnim[ mCurrentType ]->
                                soundAnim[ mCurrentSound ].sound.id );
                
                mCurrentAnim[ mCurrentType ]->
                    soundAnim[ mCurrentSound ].sound.id = -1;
                
                // don't allow save or copy-all in this weird state
                mCopyAllButton.setVisible( false );
                mSaveButton.setVisible( false );
                }
            }
        else if( u.id != -1 ) {
            // expand it to make room
            mCurrentAnim[ mCurrentType ]->numSounds++;
            SoundAnimationRecord *old = mCurrentAnim[ mCurrentType ]->soundAnim;
            
            mCurrentAnim[ mCurrentType ]->soundAnim = 
                new SoundAnimationRecord[ 
                    mCurrentAnim[ mCurrentType ]->numSounds ];
            
            memcpy( mCurrentAnim[ mCurrentType ]->soundAnim,
                    old,
                    sizeof( SoundAnimationRecord ) *
                    mCurrentAnim[ mCurrentType ]->numSounds - 1 );
            
            delete [] old;

            mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ].sound = u;
            mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].repeatPerSec = 0;
            mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].repeatPhase = 0;
            mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].ageStart = -1;
            mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].ageEnd = -1;

            countLiveUse( u.id );
            }
        
        soundIndexChanged();
        }
    else if( inTarget == &mSoundAgeInField ) {
        float value = mSoundAgeInField.getFloat();
        if( value < 0 ) {
            value = 0;
            }
        mCurrentAnim[ mCurrentType ]->
            soundAnim[ mCurrentSound ].ageStart = value;
        }
    else if( inTarget == &mSoundAgeOutField ) {
        float value = mSoundAgeOutField.getFloat();
        if( value < 0 ) {
            value = 0;
            }
        mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ].ageEnd = value;
        }
    else if( inTarget == &mSoundAgePunchInButton ) {
        double in = mPersonAgeSlider.getValue();
        mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ].ageStart = in;
        
        mSoundAgeInField.setFloat( in, 2 );
        }
    else if( inTarget == &mSoundAgePunchOutButton ) {
        double out = mPersonAgeSlider.getValue();
        mCurrentAnim[ mCurrentType ]->soundAnim[ mCurrentSound ].ageEnd = out;
        
        mSoundAgeOutField.setFloat( out, 2 );
        }
    else if( inTarget == &mNextSoundButton ) {
        mCurrentSound ++;
        soundIndexChanged();
        }
    else if( inTarget == &mPrevSoundButton ) {
        mCurrentSound --;
        
        soundIndexChanged();
        }
    else if( inTarget == &mSoundRepeatPerSecSlider ) {
        mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].repeatPerSec = 
            mSoundRepeatPerSecSlider.getValue();
        }
    else if( inTarget == &mSoundRepeatPhaseSlider ) {
        mCurrentAnim[ mCurrentType ]->
                soundAnim[ mCurrentSound ].repeatPhase = 
            mSoundRepeatPhaseSlider.getValue();
        }
    else if( inTarget == &mReverseRotationCheckbox ) {
        updateAnimFromSliders();
        }
    else if( inTarget == &mRandomStartPhaseCheckbox ) {
        mCurrentAnim[ mCurrentType ]->randomStartPhase =
            mRandomStartPhaseCheckbox.getToggled();
        }
    else if( inTarget == &mTestSpeedSlider ) {
        
        double factor = mCurrentObjectFrameRateFactor;
        
        // make sure frame time never goes backwards when we reduce speed
        // nor jumps forward when we increase speed
        double oldFrameTime = 
            ( mFrameCount / 60.0 ) * factor;
            
        oldFrameTime *= mLastTestSpeed;

        oldFrameTime += mFrameTimeOffset;

        double newFrameTime = oldFrameTime;

        if( mTestSpeedSlider.getValue() > 0 ) {
            
            newFrameTime /= mTestSpeedSlider.getValue();
            
            newFrameTime /= factor;
            
            mFrameCount = lrint( newFrameTime * 60.0 );
        
            mFrameTimeOffset = 0;
            }
        else {
            mFrameCount = 0;
            mFrameTimeOffset = oldFrameTime;
            }
        
        
        

        mLastTestSpeed = mTestSpeedSlider.getValue();
        }
    else {
        
        AnimType oldType = mCurrentType;
        
        int boxHit = -1;

        for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
            if( inTarget == mCheckboxes[i] ) {
                boxHit = i;
                mCheckboxes[i]->setToggled( true );
                mCurrentType = mCheckboxAnimTypes[i];
                break;
                }
            }
        
        if( boxHit != -1 ) {
            for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
                if( i != boxHit ) {
                    mCheckboxes[i]->setToggled( false );
                    }
                }
            }
        
        if( mCurrentObjectID != -1 &&
            oldType != mCurrentType ) {
            
            mCurrentSound = 0;
            
            soundIndexChanged();

            mLastType = oldType;
            mLastTypeFade = 1.0;
            mLastTypeFrameCount = mFrameCount;
            
            if( oldType == moving ) {
                mFrozenRotFrameCount = mFrameCount;
                mFrozenRotFrameCountUsed = false;
                }
            else if( mCurrentType == moving &&
                     oldType != moving &&
                     mFrozenRotFrameCountUsed ) {
                // switching back to moving
                // resume from where frozen
                mFrameCount = mFrozenRotFrameCount;
                }
            
            if( ! isAnimFadeNeeded( mCurrentObjectID,
                                    mCurrentAnim[ mLastType ],
                                    mCurrentAnim[ mCurrentType ] ) ) {
                
                // jump right to start of new animation with no fade
                mLastTypeFade = 0;
                }

            
            checkNextPrevVisible();
            updateSlidersFromAnim();
            }
        

        if( boxHit != -1 ) {
            return;
            }
        
        // check sliders
        for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
            if( inTarget == mSliders[i] ) {
                updateAnimFromSliders();
                break;
                }
            }
        
        }
    
    }




double EditorAnimationPage::computeFrameTime() {

    double factor = mCurrentObjectFrameRateFactor;


    double animFade = 1.0;
    
    if( mLastTypeFade != 0 ) {
        animFade = mLastTypeFade;
        }

    double frameTime = 
        ( mFrameCount / 60.0 ) * factor;
    
    if( animFade < 1 ) {
        factor = mCurrentObjectFrameRateFactor;

        frameTime = 
            ( mLastTypeFrameCount / 60.0 ) * 
            factor;
        }
    

    frameTime *= mTestSpeedSlider.getValue();
    
    frameTime += mFrameTimeOffset;
    return frameTime;
    }





void EditorAnimationPage::drawUnderComponents( doublePair inViewCenter, 
                                               double inViewSize ) {

    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };
    
    doublePair nudgedPos = pos;
    nudgedPos.y += 16;
    drawRect( nudgedPos, 228, 144 );
    

    pos.y -= 64;


    if( mCurrentObjectID != -1 ) {
        
        
        ObjectRecord *obj = getObject( mCurrentObjectID );

        int *demoSlots = NULL;
        if( mCurrentSlotDemoID != -1 && obj->numSlots > 0 ) {
            demoSlots = new int[ obj->numSlots ];
            for( int i=0; i<obj->numSlots; i++ ) {
                demoSlots[i] = mCurrentSlotDemoID;
                }
            }
        
        AnimType t = mCurrentType;
        double animFade = 1.0;
        
        if( mLastTypeFade != 0 ) {
            t = mLastType;
            animFade = mLastTypeFade;
            }

        AnimationRecord *anim = mCurrentAnim[ t ];
        AnimationRecord *fadeTargetAnim = mCurrentAnim[ mCurrentType ];
        
        AnimationRecord *animRotFrozen = mCurrentAnim[ moving ];
        
        if( mWiggleFade > 0 ) {
            anim = mWiggleAnim;
            fadeTargetAnim = mWiggleAnim;
            animFade = 1.0;
            }
        
        if( mSettingRotCenter ) {
            anim = NULL;
            }
        
        double age = -1;

        if( mPersonAgeSlider.isVisible() ) {
            age = mPersonAgeSlider.getValue();
            }

        if( anim != NULL ) {
            double factor = mCurrentObjectFrameRateFactor;
            
            double frameTime = computeFrameTime();
            
            double frozenRotFrameTime = 
                ( mFrozenRotFrameCount / 60.0 ) * factor;
            
            double fadeTargetFrameTime = frameTime;

            if( mCurrentType == moving ) {
                doublePair groundPos = pos;
                
                setDrawColor( 0, 0, 0, 1 );

                double groundFrameTime = frameTime;

                if( obj->speedMult > 1.0 ) {                    
                    groundFrameTime *= obj->speedMult;
                    }
                if( mHeldID != -1 &&  getObject( mHeldID )->speedMult > 1.0 ) {
                    groundFrameTime *= getObject( mHeldID )->speedMult;
                    }
                

                groundFrameTime = groundFrameTime - floor( groundFrameTime );
                
                int moveSign = -1;
                
                if( mFlipDraw ) {
                    moveSign = 1;
                    }
                
                groundPos.x += moveSign * groundFrameTime * 4 * 128;
                
                
                groundPos.x += moveSign * 512;

                if( mFlipDraw ) {
                    while( groundPos.x > 512 ) {
                        groundPos.x -= 128;
                        }

                    while( groundPos.x >= -512 ) {
                        
                        //setDrawColor( 1, 1, 1, 
                        //              1.0 - ( fabs( groundPos.x ) / 128.0 ) );

                        drawSprite( mGroundSprite, groundPos );
                        groundPos.x -= 128;
                        }
                    }
                else {
                    while( groundPos.x < -512 ) {
                        groundPos.x += 128;
                        }
                    
                    while( groundPos.x <= 512 ) {
                        
                        //setDrawColor( 1, 1, 1, 
                        //              1.0 - ( fabs( groundPos.x ) / 128.0 ) );
                        
                        drawSprite( mGroundSprite, groundPos );
                        groundPos.x += 128;
                        }
                    }
                
                
                }
                    
            setDrawColor( 1, 1, 1, 1 );

            if( mWiggleFade > 0 ) {
                int numLayers = obj->numSprites + obj->numSlots;
                
                float *animLayerFades = new float[ numLayers ];
                for( int i=0; i<numLayers; i++ ) {
                    animLayerFades[i] = .25f;
                    }
                if( mWiggleSpriteOrSlot != -1 ) {
                    animLayerFades[ mWiggleSpriteOrSlot ] = 1.0f;
                    }
                setAnimLayerFades( animLayerFades );
                }
            
            
            if( demoSlots != NULL ) {
                drawObjectAnim( mCurrentObjectID, 
                                anim, frameTime,
                                animFade, 
                                fadeTargetAnim, fadeTargetFrameTime, 
                                frozenRotFrameTime,
                                &mFrozenRotFrameCountUsed,
                                animRotFrozen,
                                NULL,
                                NULL,
                                pos, 0, false, mFlipDraw, age,
                                false,
                                false,
                                false,
                                mClothingSet,
                                NULL,
                                obj->numSlots, demoSlots );
                }
            else {
                
                int hideClosestArm = 0;
                char hideAllLimbs = false;
                
                ObjectRecord *heldObject = NULL;
                
                AnimationRecord *frozenArmAnim = NULL;

                if( mHeldID != -1 ) {
                    heldObject = getObject( mHeldID );
                    }
                

                if( heldObject != NULL ) {
                    
                    if( heldObject->person ) {
                        // try hiding no arms, but freezing them instead
                        // -2 means body position still returned as held pos
                        // instead of hand pos
                        hideClosestArm = -2;
                        hideAllLimbs = false;
                        frozenArmAnim = mCurrentAnim[ moving ];
                        }
                    else if( heldObject->heldInHand ) {
                        hideClosestArm = 0;
                        }
                    else if( heldObject->rideable ) {
                        hideClosestArm = 0;
                        hideAllLimbs = true;
                        frozenArmAnim = mCurrentAnim[ moving ];
                        }
                    else {
                        // try hiding no arms, but freezing them instead
                        // -2 means body position still returned as held pos
                        // instead of hand pos
                        hideClosestArm = -2;
                        hideAllLimbs = false;
                        frozenArmAnim = mCurrentAnim[ moving ];
                        }
                    }


                HoldingPos holdingPos = 
                    drawObjectAnim( mCurrentObjectID, 2, 
                                anim, frameTime,
                                animFade,
                                fadeTargetAnim, fadeTargetFrameTime, 
                                frozenRotFrameTime,
                                &mFrozenRotFrameCountUsed,
                                animRotFrozen,
                                frozenArmAnim,
                                frozenArmAnim,
                                pos, 0, false, mFlipDraw, age,
                                hideClosestArm,
                                hideAllLimbs,
                                false,
                                mClothingSet,
                                NULL );
                
                doublePair holdPos;
                
                if( holdingPos.valid ) {
                    holdPos = holdingPos.pos;
                    }
                else {
                    holdPos = pos;
                    }

        

        
                if( heldObject != NULL ) {
                    
                    doublePair heldOffset = heldObject->heldOffset;
            
                    
                    if( !heldObject->person ) {    
                        heldOffset = sub( heldOffset, 
                                          getObjectCenterOffset( heldObject ) );
                        }
                    
                    if( mFlipDraw ) {
                        heldOffset.x *= -1;
                        }

                    double heldRot = 0;
                    
                    if( holdingPos.valid && holdingPos.rot != 0  &&
                        ! heldObject->rideable ) {
                        
                        if( mFlipDraw ) {
                            heldOffset = 
                                rotate( heldOffset, 
                                        2 * M_PI * holdingPos.rot );
                            }
                        else {
                            heldOffset = 
                                rotate( heldOffset, 
                                        -2 * M_PI * holdingPos.rot );
                            }
                        
                        if( ! heldObject->person ) {
                            // baby doesn't rotate when held
                        
                            if( mFlipDraw ) {
                                heldRot = -holdingPos.rot;
                                }
                            else {
                                heldRot = holdingPos.rot;
                                }
                            }
                        }
            
            
            

                    holdPos.x += heldOffset.x;
                    holdPos.y += heldOffset.y;
                    
                    double heldAge = -1;
                    AnimType heldAnimType = t;
                    AnimType heldFadeTargetType = mCurrentType;
                    
                    if( heldObject->person ) {
                        heldAge = 0;
                        heldAnimType = held;
                        heldFadeTargetType = held;
                        }
                    
                    drawObjectAnim( mHeldID, 2,  
                                    heldAnimType, frameTime,
                                    animFade, 
                                    heldFadeTargetType, fadeTargetFrameTime, 
                                    frozenRotFrameTime,
                                    &mFrozenRotFrameCountUsed,
                                    moving,
                                    moving,
                                    holdPos, heldRot, false, mFlipDraw, heldAge,
                                    false,
                                    false,
                                    false,
                                    getEmptyClothingSet(),
                                    NULL );
                    }
                }
            }
        else {
            if( demoSlots != NULL ) {
                drawObject( obj, pos, 0, false,
                            mFlipDraw, age, false, false, false, 
                            mClothingSet,
                            obj->numSlots, demoSlots );
                }
            else {
                drawObject( obj, 2, pos, 0, false, 
                            mFlipDraw, age, false, false, false,
                            mClothingSet );
                }
            }
        
        if( demoSlots != NULL ) {
            delete [] demoSlots;
            }

        if( mWiggleFade > 0 || mSettingRotCenter || mRotCenterFade > 0 ) {
        
            ObjectRecord *r = getObject( mCurrentObjectID );

            if( mCurrentSpriteOrSlot < r->numSprites ) {
                
                doublePair center = add( r->spritePos[ mCurrentSpriteOrSlot ],
                                         pos );
                center = add( center, 
                              mCurrentAnim[ mCurrentType ]->
                              spriteAnim[mCurrentSpriteOrSlot].
                              rotationCenterOffset );
                
                setDrawColor( 1, 1, 1, 0.75 );
                drawSprite( mCenterMarkSprite, center );
                }
            }
        
            
        }
    
    setDrawColor( 1, 1, 1, 1 );

    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        if( mCheckboxes[i]->isVisible() ) {
            pos = mCheckboxes[i]->getPosition();
    
            pos.x -= 20;
            
            smallFont->drawString( mCheckboxNames[i], pos, alignRight );
            }
        }

    if( mReverseRotationCheckbox.isVisible() ) {
        pos = mReverseRotationCheckbox.getPosition();
        pos.x -= 10;
        smallFont->drawString( "CCW", pos, alignRight );
        }

    if( mRandomStartPhaseCheckbox.isVisible() ) {
        pos = mRandomStartPhaseCheckbox.getPosition();
        pos.x -= 10;
        smallFont->drawString( "Random Start Point", pos, alignRight );
        }
        
    
    if( mCurrentObjectID != -1 ) {
        
        setDrawColor( 1, 1, 1, 1 );
        
        pos = mPrevSpriteOrSlotButton.getPosition();
        
        pos.x += 60;
        
        const char *tag;
        int num;
        
        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        int spriteID = -1;
        
        if( mCurrentSpriteOrSlot > anim->numSprites - 1 ) {
            tag = "Slot";
            num = mCurrentSpriteOrSlot - anim->numSprites;
            }
        else {
            tag = "Sprite";
            num = mCurrentSpriteOrSlot;
            
            if( mCurrentSpriteOrSlot < 
                getObject( mCurrentObjectID )->numSprites ) {
                
                spriteID = getObject( mCurrentObjectID )->
                    sprites[ mCurrentSpriteOrSlot ];
                }
            }

        char *string = autoSprintf( "%s %d", tag, num );
        smallFont->drawString( string, pos, alignLeft );
        delete [] string;
    
        pos.y += 28;
        smallFont->drawString( getObject( mCurrentObjectID )->description,
                               pos, alignLeft );
        
        pos.y -= 56;
        
        if( spriteID != -1 ) {
            smallFont->drawString( getSpriteRecord( spriteID )->tag,
                                   pos, alignLeft );
            }
        

        if( mCurrentSpriteOrSlot < anim->numSprites ) {
            doublePair legendPos = mObjectEditorButton.getPosition();
            
            legendPos.x = 150;
            legendPos.y += 20;
            
            drawKeyLegend( &mKeyLegend, legendPos );
            }

        doublePair legendPos = mObjectPicker.getPosition();
        legendPos.y -= 255;

        drawKeyLegend( &mKeyLegendB, legendPos, alignCenter );
        }
    

    
    doublePair zoomPos = { lastMouseX, lastMouseY };

    pos.x = +500;
    pos.y = -290;
    
    drawZoomView( zoomPos, 16, 4, pos );
    }




void EditorAnimationPage::step() {
    double oldFrameTime = computeFrameTime();

    mFrameCount++;
    mLastTypeFrameCount++;
    

    double newFrameTime = computeFrameTime();
    
    // should any sounds play?
    if( mCurrentObjectID != -1 ) {
        
        for( int s=0; s<mCurrentAnim[ mCurrentType ]->numSounds; s++ ) {
            
            SoundAnimationRecord soundAnim =
                mCurrentAnim[ mCurrentType ]->soundAnim[s];
            
            
            if( soundAnim.sound.id == -1 ) {
                continue;
                }
            
            if( getObject( mCurrentObjectID )->person ) {
                
                double age = mPersonAgeSlider.getValue();
                    
                if( ( soundAnim.ageStart != -1 && 
                      age < soundAnim.ageStart )
                    ||
                    ( soundAnim.ageEnd != -1 && 
                      age >= soundAnim.ageEnd ) ) {
                    continue;
                    }
                }
            

            double hz = soundAnim.repeatPerSec;
            
            double phase = soundAnim.repeatPhase;

            // mark them live to keep them loaded whether they play or not
            markSoundLive( soundAnim.sound.id );
            
            if( hz != 0 ) {
                double period = 1 / hz;
                
                double startOffsetSec = phase * period;
                
                int oldPeriods = 
                    lrint( 
                        floor( ( oldFrameTime - startOffsetSec ) / period ) );
                
                int newPeriods = 
                    lrint( 
                        floor( ( newFrameTime - startOffsetSec ) / period ) );
                
                if( newPeriods > oldPeriods ) {
                    playSound( soundAnim.sound );
                    }
                }
            else {
                // play once at very beginning
                if( mFrameCount == 1 ) {
                    playSound( soundAnim.sound );
                    }
                }
            }
        }
    

    if( mCurrentType == moving ) {
        mFrozenRotFrameCount++;
        }

    if( mWiggleFade > 0 ) {
        
        mWiggleFade -= 0.025 * frameRateFactor;
        
        if( mWiggleFade < 0 ) {
            mWiggleFade = 0;
            mFrameCount = 0;
            }
        setWiggle();
        }

    if( mRotCenterFade > 0 ) {
        
        mRotCenterFade -= 0.025 * frameRateFactor;
        
        if( mRotCenterFade < 0 ) {
            mRotCenterFade = 0;
            }
        }

    if( mLastTypeFade > 0 ) {
        mLastTypeFade -= 0.05 * frameRateFactor;
        if( mLastTypeFade < 0 ) {
            mLastTypeFade = 0;
            }
        }
    
    if( mCurrentObjectID != -1 ) {
        // make sure sprites stay loaded
        // (may not be drawn each step if clothing replaces them)
        ObjectRecord *obj = getObject( mCurrentObjectID );
        
        for( int i=0; i<obj->numSprites; i++ ) {
            markSpriteLive( obj->sprites[i] );
            }
        }

    if( mPlayingAge ) {
        double old = mPersonAgeSlider.getValue();
        
        double rate = 0.05;
        
        if( old > 20 && old < 40 ) {
            rate *= 2;
            }

        double newAge = old + rate * frameRateFactor;
        
        if( newAge > 60 ) {
            newAge = 60;
            mPlayingAge = false;
            }
        mPersonAgeSlider.setValue( newAge );
        }
    
    }




void EditorAnimationPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch( true );

    mSettingRotCenter = false;
    }



int EditorAnimationPage::getClosestSpriteOrSlot( float inX, float inY ) {
    if( mCurrentObjectID == -1 ) {
        return -1;
        }
    
    int closestSpriteOrSlot = -1;
    
    
    if( inX > -228 && inX < 228
        &&
        inY > -128 && inY < 160 ) {

        
        doublePair mousePos = { inX, inY + 64 };
        
        
        ObjectRecord *obj = getObject( mCurrentObjectID );
        

        double age = 20;
        if( mPersonAgeSlider.isVisible() ) {
            age = mPersonAgeSlider.getValue();
            }
        

        int pickedSprite;
        int pickedClothing;
        int pickedSlot;
        
        getClosestObjectPart( obj,
                              NULL,
                              NULL,
                              NULL,
                              age,
                              -1,
                              mFlipDraw,
                              mousePos.x, mousePos.y,
                              &pickedSprite,
                              &pickedClothing,
                              &pickedSlot );

        

        int sprites = obj->numSprites;
       
        if( pickedSprite != -1 ) {
            closestSpriteOrSlot = pickedSprite;
            }
        else if( pickedSlot != -1 ) {
            closestSpriteOrSlot = pickedSlot + sprites;
            }
        
        }
    
    return closestSpriteOrSlot;
    }




void EditorAnimationPage::pointerMove( float inX, float inY ) {    
    lastMouseX = inX;
    lastMouseY = inY;

    int closestSpriteOrSlot = getClosestSpriteOrSlot( inX, inY );
    
    if( closestSpriteOrSlot != -1 ) {
        if( closestSpriteOrSlot != mWiggleSpriteOrSlot ) {
                
            // switch
            mWiggleFade = 1.0;
            mWiggleSpriteOrSlot = closestSpriteOrSlot;
            setWiggle();
            mFrameCount = 0;
            }
        else {
            // increase amplitude again
            mWiggleFade += 0.1;
            if( mWiggleFade > 1 ) {
                mWiggleFade = 1;
                }
            }
        }
    else if( inX < 228 && inX > -228 &&
             inY < 160 && inY > -128 ) {
        // keep frozen as long as mouse is there
        if( mWiggleSpriteOrSlot != -1 ) {
            mWiggleFade += 0.1;
            if( mWiggleFade > 1 ) {
                mWiggleFade = 1;
                }
            }
        }
    }



void EditorAnimationPage::pointerDown( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( isLastMouseButtonRight() ) {
        mSettingRotCenter = true;
        return;
        }
    
    mSettingRotCenter = false;
    
    int closestSpriteOrSlot = getClosestSpriteOrSlot( inX, inY );
    
    if( closestSpriteOrSlot != -1 ) {
        mCurrentSpriteOrSlot = closestSpriteOrSlot;
        
        mWiggleFade = 1.0;
        mWiggleSpriteOrSlot = mCurrentSpriteOrSlot;
        setWiggle();
        mFrameCount = 0;

        checkNextPrevVisible();
        updateSlidersFromAnim();
        }

    if( inX > -228 && inX < 228 &&
        inY > -128 && inY < 160 ) {
        
        TextField::unfocusAll();
        }
    
    }



void EditorAnimationPage::pointerDrag( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( mSettingRotCenter ) {
        doublePair pos = { 0, -64 };
    
        ObjectRecord *r = getObject( mCurrentObjectID );

        if( mCurrentSpriteOrSlot < r->numSprites ) {
                
            doublePair center = add( r->spritePos[ mCurrentSpriteOrSlot ],
                                     pos );
            
            doublePair delta;
            

            delta.x = inX - center.x;
            delta.y = inY - center.y;
            
            mCurrentAnim[ mCurrentType ]->
                spriteAnim[mCurrentSpriteOrSlot].rotationCenterOffset = delta;
            }
        }
    
        
    }



void EditorAnimationPage::pointerUp( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    mSettingRotCenter = false;
    }




void EditorAnimationPage::keyDown( unsigned char inASCII ) {
    if( TextField::isAnyFocused() ) {
        return;
        }

    if( inASCII == 'f' ) {
        mFlipDraw = ! mFlipDraw;
        }
    }



void EditorAnimationPage::specialKeyDown( int inKeyCode ) {
    int offset = 1;
    
    if( isCommandKeyDown() ) {
        offset = 5;
        }
    if( isShiftKeyDown() ) {
        offset = 10;
        }
    if( isCommandKeyDown() && isShiftKeyDown() ) {
        offset = 20;
        }

    if( mCurrentObjectID != -1 ) {
        ObjectRecord *obj = getObject( mCurrentObjectID );

        if( mCurrentSpriteOrSlot < obj->numSprites ) {
            
            switch( inKeyCode ) {
                case MG_KEY_LEFT:
                    mCurrentAnim[ mCurrentType ]->
                        spriteAnim[mCurrentSpriteOrSlot].
                        rotationCenterOffset.x -= offset;
                    mRotCenterFade = 1.0;
                    break;
                case MG_KEY_RIGHT:
                    mCurrentAnim[ mCurrentType ]->
                        spriteAnim[mCurrentSpriteOrSlot].
                        rotationCenterOffset.x += offset;
                    mRotCenterFade = 1.0;
                    break;
                case MG_KEY_DOWN:
                    mCurrentAnim[ mCurrentType ]->
                        spriteAnim[mCurrentSpriteOrSlot].
                        rotationCenterOffset.y -= offset;
                    mRotCenterFade = 1.0;
                    break;
                case MG_KEY_UP:
                    mCurrentAnim[ mCurrentType ]->
                        spriteAnim[mCurrentSpriteOrSlot].
                        rotationCenterOffset.y += offset;
                    mRotCenterFade = 1.0;
                    break;
                }
            }
        }
    }






void EditorAnimationPage::clearUseOfObject( int inObjectID ) {
    if( mCurrentObjectID == inObjectID ) {
        mCurrentObjectID = -1;
        freeCurrentAnim();
        }
    }



void EditorAnimationPage::objectLayersChanged( int inObjectID ) {
    if( mCurrentObjectID == inObjectID ) {

        mCurrentSound = 0;
        mCurrentSpriteOrSlot = 0;
                
        clearClothing();
                
        checkNextPrevVisible();
        
        populateCurrentAnim();
        soundIndexChanged();
        }
    }

