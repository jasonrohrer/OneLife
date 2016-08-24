#include "EditorAnimationPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;
extern Font *smallFont;

extern double frameRateFactor;




#include "ObjectPickable.h"
#include "whiteSprites.h"

static ObjectPickable objectPickable;

extern double defaultAge;


EditorAnimationPage::EditorAnimationPage()
        : mCenterMarkSprite( loadSprite( "centerMark.tga" ) ),
          mGroundSprite( loadWhiteSprite( "testGround.tga" ) ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mSaveButton( mainFont, 0, 180, "Save" ),
          mDeleteButton( mainFont, 140, 180, "Delete" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mPersonAgeSlider( smallFont, 0, -220, 2,
                            100, 20,
                            0, 100, "Age" ),
          mTestSpeedSlider( smallFont, 0, -170, 2,
                            100, 20,
                            0, 1, "Test Speed" ),
          mReverseRotationCheckbox( 0, 0, 2 ),
          mCurrentObjectID( -1 ),
          mCurrentSlotDemoID( -1 ),
          mFlipDraw( false ),
          mWiggleAnim( NULL ),
          mWiggleFade( 0.0 ),
          mWiggleSpriteOrSlot( 0 ),
          mRotCenterFade( 0.0 ),
          mCurrentType( ground ),
          mLastType( ground ),
          mLastTypeFade( 0 ),
          mFrameCount( 0 ),
          mLastTypeFrameCount( 0 ),
          mFrozenRotFrameCount( 0 ),
          mFrozenRotFrameCountUsed( false ),
          mCurrentSpriteOrSlot( 0 ),
          mSettingRotCenter( false ),
          mPickSlotDemoButton( smallFont, 180, 40, "Fill Slots" ),
          mPickingSlotDemo( false ),
          mClearSlotDemoButton( smallFont, 180, -90, "Clear Slots" ),
          mPickClothingButton( smallFont, 180, 60, "+ Clothes" ),
          mPickingClothing( false ),
          mClearClothingButton( smallFont, 180, 120, "X Clothes" ),
          mCopyButton( smallFont, -290, 210, "Copy" ),
          mCopyChainButton( smallFont, -290, 250, "Copy Child Chain" ),
          mCopyWalkButton( smallFont, -160, 250, "Copy Walk" ),
          mCopyAllButton( smallFont, -370, 210, "Copy All" ),
          mPasteButton( smallFont, -230, 210, "Paste" ),
          mClearButton( smallFont, -170, 210, "Clear" ),
          mNextSpriteOrSlotButton( smallFont, 120, -270, "Next Layer" ),
          mPrevSpriteOrSlotButton( smallFont, -120, -270, "Prev Layer" ) {
    
    
    for( int i=0; i<endAnimType; i++ ) {
        mCurrentAnim[i] = NULL;
        }
    
    zeroRecord( &mCopyBuffer );

    mWalkCopied = false;

    addComponent( &mObjectEditorButton );
    addComponent( &mSaveButton );
    addComponent( &mDeleteButton );
    
    addComponent( &mObjectPicker );

    addComponent( &mPersonAgeSlider );
    
    mPersonAgeSlider.setVisible( false );


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
    

    addComponent( &mCopyButton );
    addComponent( &mCopyChainButton );
    addComponent( &mCopyWalkButton );
    addComponent( &mCopyAllButton );
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

    mCopyButton.addActionListener( this );
    mCopyChainButton.addActionListener( this );
    mCopyWalkButton.addActionListener( this );
    mCopyAllButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mClearButton.addActionListener( this );
    

    mNextSpriteOrSlotButton.addActionListener( this );
    mPrevSpriteOrSlotButton.addActionListener( this );

    mClearSlotDemoButton.setVisible( false );

    mPickClothingButton.setVisible( false );
    mClearClothingButton.setVisible( false );
    

    checkNextPrevVisible();
    
    
    double boxY = 0;
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 210, boxY, 2 );
        addComponent( mCheckboxes[i] );

        mCheckboxes[i]->addActionListener( this );
        boxY -= 20;
        }
    mCheckboxNames[0] = "Ground";
    mCheckboxNames[1] = "Held";
    mCheckboxNames[2] = "Moving";

    mCheckboxAnimTypes[0] = ground;
    mCheckboxAnimTypes[1] = held;
    mCheckboxAnimTypes[2] = moving;

    mCheckboxes[0]->setToggled( true );



    boxY = 190;
    
    double space = 32;
    double x = -290;
    
    mSliders[0] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "X Osc" );
    mSliders[1] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 96, "X Amp" );
    mSliders[2] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "X Phase" );

    mSliders[3] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 6, "Y Osc" );
    mSliders[4] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 96, "Y Amp" );
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
                                    0, 20, "Duration Sec" );
    mSliders[12] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                    100, 20,
                                    0, 20, "Pause Sec" );
    

    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        addComponent( mSliders[i] );

        mSliders[i]->addActionListener( this );
        mSliders[i]->setVisible( false );
        }
    mReverseRotationCheckbox.setVisible( false );


    mClothingSet = getEmptyClothingSet();
    mNextShoeToFill = &( mClothingSet.backShoe );
    mOtherShoe = &( mClothingSet.frontShoe );

    addKeyClassDescription( &mKeyLegend, "R-Click", "Move layer rot anchor" );
    }



EditorAnimationPage::~EditorAnimationPage() {
    freeCurrentAnim();
    
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
    mClearClothingButton.setVisible( false );
    }




void EditorAnimationPage::freeCurrentAnim() {
    
    for( int i=0; i<endAnimType; i++ ) {
        if( mCurrentAnim[i] != NULL ) {
            delete [] mCurrentAnim[i]->spriteAnim;
            delete [] mCurrentAnim[i]->slotAnim;
            delete mCurrentAnim[i];
            mCurrentAnim[i] = NULL;    
            }
        }
    if( mWiggleAnim != NULL ) {
        delete [] mWiggleAnim->spriteAnim;
        delete [] mWiggleAnim->slotAnim;
        delete mWiggleAnim;
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

    mWiggleAnim = copyRecord( mCurrentAnim[0] );

    
    updateSlidersFromAnim();
        
    mFrameCount = 0;
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
        
        mCopyChainButton.setVisible( false );
        mCopyWalkButton.setVisible( false );
        return;
        }
    
    ObjectRecord *r = getObject( mCurrentObjectID );

    int num = r->numSprites + r->numSlots;
    
    mNextSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot < num - 1 );
    mPrevSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot > 0 );
    

    if( mCurrentSpriteOrSlot < r->numSprites ) {
        mCopyChainButton.setVisible( true );
        }
    else {
        mCopyChainButton.setVisible( false );
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
        }
    else {
        mPickClothingButton.setVisible( false );
        mClearClothingButton.setVisible( false );
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


    r->durationSec = mSliders[11]->getValue();
    r->pauseSec = mSliders[12]->getValue();
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
        }
    else {
        // last two sliders (rotation) are available for sprites
        mSliders[6]->setVisible( true );
        mSliders[7]->setVisible( true );
        mReverseRotationCheckbox.setVisible( true );
        
        mSliders[8]->setVisible( true );
        mSliders[9]->setVisible( true );
        mSliders[10]->setVisible( true );
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
    
    mSliders[11]->setValue( r->durationSec );
    mSliders[12]->setValue( r->pauseSec );
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
        mCurrentSpriteOrSlot = 0;
                
        checkNextPrevVisible();
        populateCurrentAnim();
        }    
    else if( inTarget == &mClearButton ) {
        zeroRecord( getRecordForCurrentSlot() );
        updateSlidersFromAnim();
        }
    else if( inTarget == &mCopyButton ) {
        mCopyBuffer = *( getRecordForCurrentSlot() );
        mChainCopyBuffer.deleteAll();
        mAllCopyBufferSprites.deleteAll();
        mAllCopyBufferSlots.deleteAll();
        mWalkCopied = false;
        }
    else if( inTarget == &mCopyChainButton ) {
        mWalkCopied = false;
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

            zeroRecord( &mCopyBuffer );
            
            mChainCopyBuffer.deleteAll();

            mAllCopyBufferSprites.deleteAll();
            mAllCopyBufferSlots.deleteAll();
            }
        }
    else if( inTarget == &mCopyAllButton ) {
        mChainCopyBuffer.deleteAll();
        mWalkCopied = false;
        zeroRecord( &mCopyBuffer );

        mAllCopyBufferSprites.deleteAll();
        mAllCopyBufferSlots.deleteAll();
        
        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

        for( int i=0; i<anim->numSprites; i++ ) {
            mAllCopyBufferSprites.push_back( anim->spriteAnim[i] );
            }
        for( int i=0; i<anim->numSlots; i++ ) {
            mAllCopyBufferSlots.push_back( anim->slotAnim[i] );
            }
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
        else if( mChainCopyBuffer.size() > 0 ) {
            // chain copy paste
            
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
        else if( mAllCopyBufferSprites.size() > 0 ||
                 mAllCopyBufferSlots.size() > 0 ) {
            // paste all
            
            AnimationRecord *anim = mCurrentAnim[ mCurrentType ];

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
                    }   
                        
                mPickClothingButton.setVisible( true );
                }
            else {
                mCurrentObjectID = newPickID;
            
                mCurrentSpriteOrSlot = 0;
                
                clearClothing();
                
                checkNextPrevVisible();
                
                populateCurrentAnim();

                if( getObject( mCurrentObjectID )->person ) {
                    mPersonAgeSlider.setValue( defaultAge );
                    mPersonAgeSlider.setVisible( true );
                    }
                else {
                    mPersonAgeSlider.setVisible( false );
                    }
                }
            }
        }
    else if( inTarget == &mPickSlotDemoButton ) {
        mPickingSlotDemo = true;
        mPickingClothing = false;
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
        mPickClothingButton.setVisible( false );
        mClearClothingButton.setVisible( true );
        }
    else if( inTarget == &mClearClothingButton ) {
        mPickingClothing = false;
        mPickClothingButton.setVisible( true );
        mClearClothingButton.setVisible( false );
        
        clearClothing();
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
    else if( inTarget == &mReverseRotationCheckbox ) {
        updateAnimFromSliders();
        }
    else if( inTarget == &mTestSpeedSlider ) {
        
        // make sure frame time never goes backwards when we reduce speed
        // nor jumps forward when we increase speed
        double oldFrameTime = ( mFrameCount / 60.0 ) * frameRateFactor;
            
        oldFrameTime *= mLastTestSpeed;

        oldFrameTime += mFrameTimeOffset;

        double newFrameTime = oldFrameTime;

        if( mTestSpeedSlider.getValue() > 0 ) {
            
            newFrameTime /= mTestSpeedSlider.getValue();
            
            newFrameTime /= frameRateFactor;
            
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




void EditorAnimationPage::drawUnderComponents( doublePair inViewCenter, 
                                               double inViewSize ) {

    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };
    
    drawSquare( pos, 128 );

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
            
            double frameTime = ( mFrameCount / 60.0 ) * frameRateFactor;
            
            double fadeTargetFrameTime = frameTime;
            
            double frozenRotFrameTime = 
                ( mFrozenRotFrameCount / 60.0 ) * frameRateFactor;
            
            if( animFade < 1 ) {
                frameTime = ( mLastTypeFrameCount / 60.0 ) * frameRateFactor;
                }
            

            frameTime *= mTestSpeedSlider.getValue();
            
            frameTime += mFrameTimeOffset;

            if( mCurrentType == moving ) {
                doublePair groundPos = pos;
                
                setDrawColor( 0, 0, 0, 1 );

                double groundFrameTime = frameTime - floor( frameTime );

                groundPos.x -= groundFrameTime * 4 * 128;
                
                
                groundPos.x -= 256;

                while( groundPos.x < -256 ) {
                    groundPos.x += 128;
                    }
                
                while( groundPos.x <= 256 ) {
                    
                    //setDrawColor( 1, 1, 1, 
                    //              1.0 - ( fabs( groundPos.x ) / 128.0 ) );

                    drawSprite( mGroundSprite, groundPos );
                    groundPos.x += 128;
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
                                pos, mFlipDraw, age,
                                false,
                                false,
                                false,
                                mClothingSet,
                                obj->numSlots, demoSlots );
                }
            else {
                drawObjectAnim( mCurrentObjectID, 
                                anim, frameTime,
                                animFade,
                                fadeTargetAnim, fadeTargetFrameTime, 
                                frozenRotFrameTime,
                                &mFrozenRotFrameCountUsed,
                                animRotFrozen,
                                pos, mFlipDraw, age,
                                false,
                                false,
                                false,
                                mClothingSet );
                }
            }
        else {
            if( demoSlots != NULL ) {
                drawObject( obj, pos, 0, mFlipDraw, age, false, false, false, 
                            mClothingSet,
                            obj->numSlots, demoSlots );
                }
            else {
                drawObject( obj, pos, 0, mFlipDraw, age, false, false, false,
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
        pos = mCheckboxes[i]->getPosition();
    
        pos.x -= 20;
        
        smallFont->drawString( mCheckboxNames[i], pos, alignRight );
        }

    if( mReverseRotationCheckbox.isVisible() ) {
        pos = mReverseRotationCheckbox.getPosition();
        pos.x -= 10;
        smallFont->drawString( "CCW", pos, alignRight );
        }
        
    
    if( mCurrentObjectID != -1 ) {
        
        setDrawColor( 1, 1, 1, 1 );
        
        pos = mPrevSpriteOrSlotButton.getPosition();
        
        pos.x += 80;
        
        const char *tag;
        int num;
        
        AnimationRecord *anim = mCurrentAnim[ mCurrentType ];
        
        if( mCurrentSpriteOrSlot > anim->numSprites - 1 ) {
            tag = "Slot";
            num = mCurrentSpriteOrSlot - anim->numSprites;
            }
        else {
            tag = "Sprite";
            num = mCurrentSpriteOrSlot;
            }

        char *string = autoSprintf( "%s %d", tag, num );
        smallFont->drawString( string, pos, alignLeft );
        delete [] string;
    

        if( mCurrentSpriteOrSlot < anim->numSprites ) {
            doublePair legendPos = mObjectEditorButton.getPosition();
            
            legendPos.x = 150;
            legendPos.y += 20;
            
            drawKeyLegend( &mKeyLegend, legendPos );
            }
        }
    

    
    
    }




void EditorAnimationPage::step() {
    mFrameCount++;
    mLastTypeFrameCount++;
    
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
    
    }




void EditorAnimationPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch();

    mSettingRotCenter = false;
    }



int EditorAnimationPage::getClosestSpriteOrSlot( float inX, float inY ) {
    if( mCurrentObjectID == -1 ) {
        return -1;
        }
    
    int closestSpriteOrSlot = -1;
    
    
    if( inX > -128 && inX < 128
        &&
        inY > -128 && inY < 128 ) {

        
        doublePair mousePos = { inX, inY + 64 };
        
        
        ObjectRecord *obj = getObject( mCurrentObjectID );
        

        double age = 20;
        if( mPersonAgeSlider.isVisible() ) {
            age = mPersonAgeSlider.getValue();
            }
        

        int pickedSprite;
        int pickedSlot;
        
        getClosestObjectPart( obj, 
                              age,
                              -1,
                              mousePos.x, mousePos.y,
                              &pickedSprite,
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
    else if( inX < 128 && inX > -128 &&
             inY < 128 && inY > -128 ) {
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
    }



void EditorAnimationPage::pointerDrag( float inX, float inY ) {

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
    mSettingRotCenter = false;
    }




void EditorAnimationPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 'f' ) {
        mFlipDraw = ! mFlipDraw;
        }
    }



void EditorAnimationPage::specialKeyDown( int inKeyCode ) {
    int offset = 1;
    
    if( isCommandKeyDown() ) {
        offset = 5;
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

        mCurrentSpriteOrSlot = 0;
                
        clearClothing();
                
        checkNextPrevVisible();
        
        populateCurrentAnim();
        }
    }

