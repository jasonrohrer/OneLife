#include "EditorObjectPage.h"

#include "ageControl.h"

#include "zoomView.h"
#include "spellCheck.h"


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


#include "SpritePickable.h"

static SpritePickable spritePickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;


#include "EditorAnimationPage.h"

extern EditorAnimationPage *animPage;


// label distance from checkboxes
static int checkboxSep = 12;


double defaultAge = 20;


static float lastMouseX, lastMouseY;



EditorObjectPage::EditorObjectPage()
        : mDescriptionField( smallFont, 
                             -50,  -260, 18,
                             false,
                             "Description", NULL, NULL ),
          mBiomeField( smallFont, -55, -220, 8, false, "Biomes",
                       "0123456789,", NULL ),
          mMapChanceField( smallFont, 
                           -250,  64, 6,
                           false,
                           "MapP", "0123456789.", NULL ),
          mHeatValueField( smallFont, 
                           -250,  32, 4,
                           false,
                           "Heat", "-0123456789", NULL ),
          mRValueField( smallFont, 
                        -250,  0, 4,
                        false,
                        "R", "0123456789.", NULL ),
          mFoodValueField( smallFont, 
                           -250,  -32, 4,
                           false,
                           "Food", "0123456789", NULL ),
          mSpeedMultField( smallFont, 
                           -250,  -64, 4,
                           false,
                           "Speed", "0123456789.", NULL ),
          mContainSizeField( smallFont, 
                             250,  -120, 4,
                             false,
                             "Contain Size", "0123456789", NULL ),
          mSlotSizeField( smallFont, 
                          -280,  -230, 4,
                          false,
                          "Slot Size", "0123456789", NULL ),
          mSlotTimeStretchField( smallFont, 
                                 -155,  -110, 4,
                                 false,
                                 "Tm Strch", "0123456789.", NULL ),
          mDeadlyDistanceField( smallFont, 
                                150,  -220, 4,
                                false,
                                "Deadly Distance", "0123456789", NULL ),
          mUseDistanceField( smallFont, 
                             150,  -190, 4,
                             false,
                             "Use Dist", "0123456789", NULL ),
          mMinPickupAgeField( smallFont, 
                              300,  -220, 4,
                              false,
                              "Pickup Age", "0123456789.", NULL ),
          mRaceField( smallFont, 
                      150, -120, 2,
                      true,
                      "Race", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL ),
          mSaveObjectButton( smallFont, 210, -260, "Save New" ),
          mReplaceObjectButton( smallFont, 310, -260, "Replace" ),
          mClearObjectButton( smallFont, -260, 200, "Blank" ),
          mClearRotButton( smallFont, -260, 120, "0 Rot" ),
          mRot45ForwardButton( smallFont, -215, 120, ">" ),
          mRot45BackwardButton( smallFont, -305, 120, "<" ),
          mFlipHButton( smallFont, -260, 160, "H Flip" ),
          mBakeButton( smallFont, -260, 160, "< Bake" ),
          mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mTransEditorButton( mainFont, 210, 260, "Trans" ),
          mAnimEditorButton( mainFont, 330, 260, "Anim" ),
          mMoreSlotsButton( smallFont, -280, -110, "More" ),
          mLessSlotsButton( smallFont, -280, -166, "Less" ),
          mAgingLayerCheckbox( 290, -22, 2 ),
          mHeadLayerCheckbox( 290, 104, 2 ),
          mBodyLayerCheckbox( 290, 84, 2 ),
          mBackFootLayerCheckbox( 290, 64, 2 ),
          mFrontFootLayerCheckbox( 290, 44, 2 ),
          mInvisibleWhenHoldingCheckbox( 290, 15, 2 ),
          mInvisibleWhenWornCheckbox( 290, 0, 2 ),
          mInvisibleWhenUnwornCheckbox( 290, 0, 2 ),
          mBehindSlotsCheckbox( -190, 0, 2 ),
          mAgeInField( smallFont, 
                       260,  -52, 6,
                       false,
                       "In", "0123456789.", NULL ),
          mAgeOutField( smallFont, 
                        260,  -90, 6,
                        false,
                        "Out", "0123456789.", NULL ),
          mAgePunchInButton( smallFont, 310, -52, "S" ),
          mAgePunchOutButton( smallFont, 310, -90, "S" ),
          mPersonNoSpawnCheckbox( 290, -150, 2 ),
          // these are in same spot because they're never shown at same time
          mMaleCheckbox( 290, -190, 2 ),
          mDeathMarkerCheckbox( 290, -190, 2 ),
          mHomeMarkerCheckbox( 100, -120, 2 ),
          mFloorCheckbox( 290, -150, 2 ),
          mHeldInHandCheckbox( 290, 36, 2 ),
          mRideableCheckbox( 290, 16, 2 ),
          mBlocksWalkingCheckbox( 290, -4, 2 ),
          mDrawBehindPlayerCheckbox( 290, -90, 2 ),
          mFloorHuggingCheckbox( 290, -110, 2 ),
          mLeftBlockingRadiusField( smallFont, 
                                    290,  -30, 2,
                                    false,
                                    "L Radius", "0123456789", NULL ),
          mRightBlockingRadiusField( smallFont, 
                                     290,  -60, 2,
                                     false,
                                     "R Radius", "0123456789", NULL ),
          mNumUsesField( smallFont, 
                         300,  110, 2,
                         false,
                         "Num Uses", "0123456789", NULL ),
          mUseVanishCheckbox( 248, 86, 2 ),
          mUseAppearCheckbox( 272, 86, 2 ),
          mSimUseCheckbox( 248, 64, 2 ),
          mSimUseSlider( smallFont, 220, 64, 2, 50, 20, 0, 10, "" ),
          mDemoClothesButton( smallFont, 300, 200, "Pos" ),
          mEndClothesDemoButton( smallFont, 300, 160, "XPos" ),
          mDemoSlotsButton( smallFont, -200, -166, "Demo Slots" ),
          mClearSlotsDemoButton( smallFont, -200, -212, "End Demo" ),
          mSetHeldPosButton( smallFont, 250, -32, "Held Pos" ),
          mEndSetHeldPosButton( smallFont, 240, -76, "End Held" ),
          mNextHeldDemoButton( smallFont, 312, -76, ">" ),
          mPrevHeldDemoButton( smallFont, 290, -76, "<" ),
          mCopyHeldPosButton( smallFont, 290, -106, "c" ),
          mPasteHeldPosButton( smallFont, 312, -106, "p" ),
          mDemoVertRotButton( smallFont, 90, -150, "Vert" ),
          mResetVertRotButton( smallFont, 130, -150, "D" ),
          mDemoVertRot( false ),
          mSpritePicker( &spritePickable, -410, 90 ),
          mObjectPicker( &objectPickable, +410, 90 ),
          mPersonAgeSlider( smallFont, -55, -220, 2,
                            100, 20,
                            0, 100, "Age" ),
          mHueSlider( smallFont, -90, -125, 2,
                      75, 20,
                      0, 1, "H" ),
          mSaturationSlider( smallFont, -90, -157, 2,
                             75, 20,
                             0, 1, "S" ),
          mValueSlider( smallFont, -90, -189, 2,
                        75, 20,
                        0, 1, "V" ),
          mSlotVertCheckbox( -150, -138, 2 ),
          mCreationSoundWidget( smallFont, -300, -310 ),
          mUsingSoundWidget( smallFont, -50, -310 ),
          mEatingSoundWidget( smallFont, +200, -310 ),
          mDecaySoundWidget( smallFont, +450, -310 ),
          mCreationSoundInitialOnlyCheckbox( -185, -285, 2 ),
          mSlotPlaceholderSprite( loadSprite( "slotPlaceholder.tga" ) ) {


    mDragging = false;
    
    mObjectCenterOnScreen.x = 0;
    mObjectCenterOnScreen.y = -32;
    

    mDemoSlots = false;
    mSlotsDemoObject = -1;

    mSetHeldPos = false;
    mDemoPersonObject = -1;

    mSetClothesPos = false;
    

    addComponent( &mDescriptionField );
    addComponent( &mMapChanceField );
    addComponent( &mBiomeField );
    addComponent( &mHeatValueField );
    addComponent( &mRValueField );
    addComponent( &mFoodValueField );
    addComponent( &mSpeedMultField );

    addComponent( &mContainSizeField );
    addComponent( &mSlotSizeField );
    addComponent( &mSlotTimeStretchField );

    addComponent( &mCreationSoundWidget );
    addComponent( &mUsingSoundWidget );
    addComponent( &mEatingSoundWidget );
    addComponent( &mDecaySoundWidget );

    addComponent( &mCreationSoundInitialOnlyCheckbox );
    mCreationSoundInitialOnlyCheckbox.setVisible( false );
    mCreationSoundInitialOnlyCheckbox.addActionListener( this );


    mContainSizeField.setVisible( false );
    mSlotSizeField.setVisible( false );
    mSlotTimeStretchField.setVisible( false );

    addComponent( &mDeadlyDistanceField );
    addComponent( &mUseDistanceField );
    addComponent( &mMinPickupAgeField );
    
    addComponent( &mRaceField );

    addComponent( &mSaveObjectButton );
    addComponent( &mReplaceObjectButton );
    addComponent( &mImportEditorButton );
    addComponent( &mTransEditorButton );
    addComponent( &mAnimEditorButton );


    addComponent( &mMoreSlotsButton );
    addComponent( &mLessSlotsButton );
    
    addComponent( &mDemoSlotsButton );
    addComponent( &mClearSlotsDemoButton );

    addComponent( &mDemoClothesButton );
    addComponent( &mEndClothesDemoButton );

    mDemoClothesButton.setVisible( false );
    mEndClothesDemoButton.setVisible( false );

    mDemoSlotsButton.setVisible( false );
    mClearSlotsDemoButton.setVisible( false );    

    addComponent( &mSetHeldPosButton );
    addComponent( &mEndSetHeldPosButton );

    mSetHeldPosButton.setVisible( true );
    mMinPickupAgeField.setVisible( true );
    
    mEndSetHeldPosButton.setVisible( false );    
    
    addComponent( &mNextHeldDemoButton );
    addComponent( &mPrevHeldDemoButton );
    
    mNextHeldDemoButton.setVisible( false );
    mPrevHeldDemoButton.setVisible( false );

    addComponent( &mCopyHeldPosButton );
    addComponent( &mPasteHeldPosButton );
    
    mCopyHeldPosButton.setVisible( false );
    mPasteHeldPosButton.setVisible( false );
    
    
    addComponent( &mDemoVertRotButton );
    addComponent( &mResetVertRotButton );
    
    mDemoVertRotButton.addActionListener( this );
    mResetVertRotButton.addActionListener( this );
    
    mDemoVertRotButton.setVisible( false );
    mResetVertRotButton.setVisible( false );
    
    

    addComponent( &mClearObjectButton );
    addComponent( &mClearRotButton );
    addComponent( &mRot45ForwardButton );
    addComponent( &mRot45BackwardButton );

    addComponent( &mFlipHButton );
    addComponent( &mBakeButton );

    addComponent( &mSpritePicker );
    addComponent( &mObjectPicker );

    addComponent( &mPersonAgeSlider );
    mPersonAgeSlider.setValue( defaultAge );
    
    addComponent( &mHueSlider );
    addComponent( &mSaturationSlider );
    addComponent( &mValueSlider );
    
    addComponent( &mSlotVertCheckbox );
    mSlotVertCheckbox.setVisible( false );
    mSlotVertCheckbox.addActionListener( this );

    addComponent( &mAgingLayerCheckbox );
    addComponent( &mHeadLayerCheckbox );
    addComponent( &mBodyLayerCheckbox );
    addComponent( &mBackFootLayerCheckbox );
    addComponent( &mFrontFootLayerCheckbox );
    addComponent( &mInvisibleWhenHoldingCheckbox );
    addComponent( &mInvisibleWhenWornCheckbox );
    addComponent( &mInvisibleWhenUnwornCheckbox );
    addComponent( &mBehindSlotsCheckbox );
    
    mInvisibleWhenWornCheckbox.setVisible( false );
    mInvisibleWhenUnwornCheckbox.setVisible( false );

    mBehindSlotsCheckbox.setVisible( false );
    
    addComponent( &mAgeInField );
    addComponent( &mAgeOutField );
    
    addComponent( &mAgePunchInButton );
    addComponent( &mAgePunchOutButton );
    
    mAgingLayerCheckbox.addActionListener( this );
    mAgePunchInButton.addActionListener( this );
    mAgePunchOutButton.addActionListener( this );

    mHeadLayerCheckbox.addActionListener( this );
    mBodyLayerCheckbox.addActionListener( this );
    mBackFootLayerCheckbox.addActionListener( this );
    mFrontFootLayerCheckbox.addActionListener( this );
    mInvisibleWhenHoldingCheckbox.addActionListener( this );
    mInvisibleWhenWornCheckbox.addActionListener( this );
    mInvisibleWhenUnwornCheckbox.addActionListener( this );
    mBehindSlotsCheckbox.addActionListener( this );
    

    mAgeInField.addActionListener( this );
    mAgeOutField.addActionListener( this );

    mAgeInField.setFireOnLoseFocus( true );
    mAgeOutField.setFireOnLoseFocus( true );


    mAgingLayerCheckbox.setVisible( false );
    mAgeInField.setVisible( false );
    mAgeOutField.setVisible( false );
    mAgePunchInButton.setVisible( false );
    mAgePunchOutButton.setVisible( false );

    mHeadLayerCheckbox.setVisible( false );
    mBodyLayerCheckbox.setVisible( false );
    mBackFootLayerCheckbox.setVisible( false );
    mFrontFootLayerCheckbox.setVisible( false );
    mInvisibleWhenHoldingCheckbox.setVisible( false );

    mPersonAgeSlider.setVisible( false );
    
    mHueSlider.setVisible( false );
    mSaturationSlider.setVisible( false );
    mValueSlider.setVisible( false );
    
    mHueSlider.addActionListener( this );
    mSaturationSlider.addActionListener( this );
    mValueSlider.addActionListener( this );
    


    mDescriptionField.setFireOnAnyTextChange( true );
    mDescriptionField.addActionListener( this );
    

    mSaveObjectButton.addActionListener( this );
    mReplaceObjectButton.addActionListener( this );
    mImportEditorButton.addActionListener( this );
    mTransEditorButton.addActionListener( this );
    mAnimEditorButton.addActionListener( this );

    mClearObjectButton.addActionListener( this );

    mClearRotButton.addActionListener( this );
    mClearRotButton.setVisible( false );

    mRot45ForwardButton.addActionListener( this );
    mRot45BackwardButton.addActionListener( this );
    mRot45ForwardButton.setVisible( false );
    mRot45BackwardButton.setVisible( false );

    mFlipHButton.addActionListener( this );
    mFlipHButton.setVisible( false );

    mBakeButton.addActionListener( this );
    mBakeButton.setVisible( false );

    mMoreSlotsButton.addActionListener( this );
    mLessSlotsButton.addActionListener( this );
    
    mDemoSlotsButton.addActionListener( this );
    mClearSlotsDemoButton.addActionListener( this );

    mSetHeldPosButton.addActionListener( this );
    mEndSetHeldPosButton.addActionListener( this );

    mNextHeldDemoButton.addActionListener( this );
    mPrevHeldDemoButton.addActionListener( this );

    mCopyHeldPosButton.addActionListener( this );
    mPasteHeldPosButton.addActionListener( this );

    mDemoClothesButton.addActionListener( this );
    mEndClothesDemoButton.addActionListener( this );
    

    mSpritePicker.addActionListener( this );
    mObjectPicker.addActionListener( this );

    mSaveObjectButton.setVisible( false );
    mReplaceObjectButton.setVisible( false );
    mClearObjectButton.setVisible( false );
    
    mPrintRequested = false;
    mSavePrintOnly = false;
    
    mCurrentObject.id = -1;
    mCurrentObject.description = mDescriptionField.getText();
    mCurrentObject.containable = 0;
    mCurrentObject.vertContainRotationOffset = 0;
    
    mCurrentObject.heldOffset.x = 0;
    mCurrentObject.heldOffset.y = 0;

    mCurrentObject.clothing = 'n';
    mCurrentObject.clothingOffset.x = 0;
    mCurrentObject.clothingOffset.y = 0;
    
    mCurrentObject.numSlots = 0;
    mCurrentObject.slotPos = new doublePair[ 0 ];
    mCurrentObject.slotVert = new char[ 0 ];
    mCurrentObject.slotParent = new int[ 0 ];
    
    mCurrentObject.numSprites = 0;
    mCurrentObject.sprites = new int[ 0 ];
    mCurrentObject.spritePos = new doublePair[ 0 ];
    mCurrentObject.spriteRot = new double[ 0 ];
    mCurrentObject.spriteHFlip = new char[ 0 ];
    mCurrentObject.spriteColor = new FloatRGB[ 0 ];

    mCurrentObject.spriteAgeStart = new double[ 0 ];
    mCurrentObject.spriteAgeEnd = new double[ 0 ];
    mCurrentObject.spriteParent = new int[ 0 ];

    mCurrentObject.spriteInvisibleWhenHolding = new char[ 0 ];
    mCurrentObject.spriteInvisibleWhenWorn = new int[ 0 ];
    mCurrentObject.spriteBehindSlots = new char[ 0 ];

    mCurrentObject.spriteIsHead = new char[ 0 ];
    mCurrentObject.spriteIsBody = new char[ 0 ];
    mCurrentObject.spriteIsBackFoot = new char[ 0 ];
    mCurrentObject.spriteIsFrontFoot = new char[ 0 ];
        
    mCurrentObject.spriteUseVanish = new char[ 0 ];
    mCurrentObject.spriteUseAppear = new char[ 0 ];
    
    mCurrentObject.spriteSkipDrawing = NULL;

    mPickedObjectLayer = -1;
    mPickedSlot = -1;
    
    mHoverObjectLayer = -1;
    mHoverSlot = -1;
    mHoverStrength = 0;
    mHoverFrameCount = 0;
    

    mRotAdjustMode = false;
    mRotStartMouseX = 0;
    
    mMapChanceField.setText( "0.00" );
    mBiomeField.setText( "0" );
    
    mBiomeField.setFireOnLoseFocus( true );
    mBiomeField.addActionListener( this );

    mHeatValueField.setText( "0" );
    mRValueField.setText( "0.00" );
    
    mFoodValueField.setText( "0" );
    mSpeedMultField.setText( "1.00" );

    mContainSizeField.setInt( 1 );
    mSlotSizeField.setInt( 1 );
    mSlotTimeStretchField.setText( "1.0" );

    mDeadlyDistanceField.setInt( 0 );
    mUseDistanceField.setInt( 1 );

    mMinPickupAgeField.setInt( 3 );
    
    mRaceField.setText( "A" );
    mRaceField.setMaxLength( 1 );
    mRaceField.setVisible( false );
    
    double boxY = -150;
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 250, boxY, 2 );
        addComponent( mCheckboxes[i] );

        boxY -= 20;
        }
    mCheckboxNames[0] = "Containable";
    mCheckboxNames[1] = "Permanent";
    mCheckboxNames[2] = "Person";
    
    mCheckboxes[0]->addActionListener( this );
    mCheckboxes[1]->addActionListener( this );
    mCheckboxes[2]->addActionListener( this );


    addComponent( &mPersonNoSpawnCheckbox );
    mPersonNoSpawnCheckbox.setVisible( false );

    addComponent( &mMaleCheckbox );
    mMaleCheckbox.setVisible( false );

    addComponent( &mDeathMarkerCheckbox );
    mDeathMarkerCheckbox.setVisible( true );
    mDeathMarkerCheckbox.addActionListener( this );

    addComponent( &mHomeMarkerCheckbox );
    mHomeMarkerCheckbox.setVisible( true );
    mHomeMarkerCheckbox.addActionListener( this );


    addComponent( &mFloorCheckbox );
    mFloorCheckbox.setVisible( true );
    mFloorCheckbox.addActionListener( this );
    

    addComponent( &mHeldInHandCheckbox );
    mHeldInHandCheckbox.setVisible( true );

    addComponent( &mRideableCheckbox );
    mRideableCheckbox.setVisible( true );

    mHeldInHandCheckbox.addActionListener( this );
    mRideableCheckbox.addActionListener( this );
    

    addComponent( &mBlocksWalkingCheckbox );
    mBlocksWalkingCheckbox.setVisible( true );
    mBlocksWalkingCheckbox.addActionListener( this );

    addComponent( &mDrawBehindPlayerCheckbox );
    mDrawBehindPlayerCheckbox.setVisible( false );
    mDrawBehindPlayerCheckbox.addActionListener( this );

    addComponent( &mFloorHuggingCheckbox );
    mFloorHuggingCheckbox.setVisible( false );
    mFloorHuggingCheckbox.addActionListener( this );

    addComponent( &mLeftBlockingRadiusField );
    mLeftBlockingRadiusField.setVisible( false );
    
    addComponent( &mRightBlockingRadiusField );
    mRightBlockingRadiusField.setVisible( false );
    
    addComponent( &mNumUsesField );
    mNumUsesField.setVisible( false );
    
    mNumUsesField.addActionListener( this );
    mNumUsesField.setFireOnAnyTextChange( true );
    
    addComponent( &mUseVanishCheckbox );
    
    mUseVanishCheckbox.addActionListener( this );
    
    mUseVanishCheckbox.setVisible( false );

    addComponent( &mUseAppearCheckbox );
    
    mUseAppearCheckbox.addActionListener( this );
    
    mUseAppearCheckbox.setVisible( false );
    

    addComponent( &mSimUseCheckbox );
    mSimUseCheckbox.addActionListener( this );
    mSimUseCheckbox.setVisible( false );
    
    addComponent( &mSimUseSlider );
    
    mSimUseSlider.setVisible( false );
    mSimUseSlider.toggleField( false );

    boxY = 217;

    for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
        mClothingCheckboxes[i] = new CheckboxButton( 261, boxY, 2 );
        addComponent( mClothingCheckboxes[i] );
        
        mClothingCheckboxes[i]->addActionListener( this );
    
        boxY -= 20;
        }
    mClothingCheckboxNames[0] = "Bottom";
    mClothingCheckboxNames[1] = "Backpack";
    mClothingCheckboxNames[2] = "Shoe";
    mClothingCheckboxNames[3] = "Tunic";
    mClothingCheckboxNames[4] = "Hat";
    
    mInvisibleWhenWornCheckbox.setPosition( 168, 217 );
    mInvisibleWhenUnwornCheckbox.setPosition( 168, 197 );
    mBehindSlotsCheckbox.setPosition( -118, 217 );


    addKeyClassDescription( &mKeyLegend, "n/m", "Switch layers" );
    addKeyClassDescription( &mKeyLegend, "arrows", "Move layer" );
    addKeyClassDescription( &mKeyLegend, "Pg Up/Down", "Layer order" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyDescription( &mKeyLegend, 'r', "Rotate layer" );
    addKeyDescription( &mKeyLegend, 'p', "Ignore parent links" );
    addKeyClassDescription( &mKeyLegend, "c/v", "Copy/paste color" );
    addKeyClassDescription( &mKeyLegend, "C/V", "Copy/paste delta from saved" );


    addKeyClassDescription( &mKeyLegendB, "R-Click", "Layer parent" );
    addKeyClassDescription( &mKeyLegendB, "Bkspce", "Remv layer" );
    addKeyDescription( &mKeyLegendB, 'd', "Dupe Layer" );

    addKeyClassDescription( &mKeyLegendC, "R-Click", "Replace layer" );

    addKeyClassDescription( &mKeyLegendD, "R-Click", "Insert object" );
    

    mColorClipboard.r = 1;
    mColorClipboard.g = 1;
    mColorClipboard.b = 1;
    
    mSaveDeltaPosClipboard.x = 0;
    mSaveDeltaPosClipboard.y = 0;
    mSaveDeltaRotClipboard = 0;
    }



EditorObjectPage::~EditorObjectPage() {

    if( isSpellCheckReady() ) {
        freeSpellCheck();
        }

    delete [] mCurrentObject.description;
    delete [] mCurrentObject.slotPos;
    delete [] mCurrentObject.slotVert;
    delete [] mCurrentObject.slotParent;
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;
    delete [] mCurrentObject.spriteColor;

    delete [] mCurrentObject.spriteAgeStart;
    delete [] mCurrentObject.spriteAgeEnd;
    delete [] mCurrentObject.spriteParent;

    delete [] mCurrentObject.spriteInvisibleWhenHolding;
    delete [] mCurrentObject.spriteInvisibleWhenWorn;
    delete [] mCurrentObject.spriteBehindSlots;

    delete [] mCurrentObject.spriteIsHead;
    delete [] mCurrentObject.spriteIsBody;
    delete [] mCurrentObject.spriteIsBackFoot;
    delete [] mCurrentObject.spriteIsFrontFoot;
    
    delete [] mCurrentObject.spriteUseVanish;
    delete [] mCurrentObject.spriteUseAppear;
    

    freeSprite( mSlotPlaceholderSprite );
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
        }

     for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
         delete mClothingCheckboxes[i];
         }
    }



static void fixCommaIntList( TextField *inField ) {
    char *text = inField->getText();
    
    if( strcmp( text, "" ) == 0 ) {
        inField->setText( "0" );
        }
    else {
        int numParts;
        char **parts = split( text, ",", &numParts );
    

        SimpleVector<int> seenInts;
        
        SimpleVector<char> goodParsedInts;

        char firstAdded = false;
    
        for( int i=0; i<numParts; i++ ) {
            int readInt;
            
            int numRead = sscanf( parts[i], "%d", &readInt );
            
            if( numRead == 1 ) {

                if( seenInts.getElementIndex( readInt ) == -1 ) {
                    
                    if( firstAdded ) {
                        goodParsedInts.push_back( ',' );
                        }

                    // reprint int to condense its formatting
                    char *newIntString = autoSprintf( "%d", readInt );
                
                    goodParsedInts.appendElementString( newIntString );
                
                    seenInts.push_back( readInt );
                    
                    delete [] newIntString;
                    
                    firstAdded = true;
                    }
                }
            delete [] parts[i];
            }
        delete [] parts;

        char *newText = goodParsedInts.getElementString();
        
        inField->setText( newText );
        delete [] newText;
        }


    delete [] text;
    }






void EditorObjectPage::updateSliderColors() {
    Color *pureHue = Color::makeColorFromHSV(
        mHueSlider.getValue(), 1, 1 );
    
    mHueSlider.setFillColor( *pureHue );
    delete pureHue;
    
    Color *pureHueSat = Color::makeColorFromHSV(
        mHueSlider.getValue(), mSaturationSlider.getValue(), 1 );
    
    mSaturationSlider.setFillColor( *pureHueSat );

    mValueSlider.setFillColor( *pureHueSat );
    delete pureHueSat;

    
    Color *totalColor = Color::makeColorFromHSV(
        mHueSlider.getValue(), mSaturationSlider.getValue(),
        mValueSlider.getValue() );

    mValueSlider.setBackFillColor( *totalColor );
    delete totalColor;
    }



void EditorObjectPage::updateAgingPanel() {
    
    char agingPanelVisible = true;
    char person = true;

    if( ! mCheckboxes[2]->getToggled() ) {
        person = false;
        agingPanelVisible = false;
        mAgingLayerCheckbox.setVisible( false );
            
        mHeadLayerCheckbox.setVisible( false );
        mBodyLayerCheckbox.setVisible( false );
        mBackFootLayerCheckbox.setVisible( false );
        mFrontFootLayerCheckbox.setVisible( false );
        
        mInvisibleWhenHoldingCheckbox.setVisible( false );

        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
            mCurrentObject.spriteAgeStart[i] = -1;
            mCurrentObject.spriteAgeEnd[i] = -1;
        
            mCurrentObject.spriteIsHead[i] = false;
            mCurrentObject.spriteIsBody[i] = false;
            mCurrentObject.spriteIsBackFoot[i] = false;
            mCurrentObject.spriteIsFrontFoot[i] = false;
            }
        
        if( mPickedObjectLayer != -1 ) {
            mUseVanishCheckbox.setToggled( 
                mCurrentObject.spriteUseVanish[ mPickedObjectLayer ] );
            mUseAppearCheckbox.setToggled( 
                mCurrentObject.spriteUseAppear[ mPickedObjectLayer ] );
            }

        mPersonNoSpawnCheckbox.setToggled( false );
        mPersonNoSpawnCheckbox.setVisible( false );

        mMaleCheckbox.setToggled( false );
        mMaleCheckbox.setVisible( false );

        mDeathMarkerCheckbox.setVisible( true );
        mHomeMarkerCheckbox.setVisible( true );
        
        if( ! mContainSizeField.isVisible() ) {
            mFloorCheckbox.setVisible( true );
            }
        
        mHeldInHandCheckbox.setVisible( true );
        mRideableCheckbox.setVisible( true );
        mBlocksWalkingCheckbox.setVisible( true );

        mNumUsesField.setVisible( true );
        if( mNumUsesField.getInt() > 1 && mPickedObjectLayer != -1 ) {
            mUseVanishCheckbox.setVisible( true );
            
            mUseVanishCheckbox.setToggled( 
                mCurrentObject.spriteUseVanish[ mPickedObjectLayer ] );
            
            mUseAppearCheckbox.setVisible( true );
            
            mUseAppearCheckbox.setToggled( 
                mCurrentObject.spriteUseAppear[ mPickedObjectLayer ] );
            }
        else {
            mUseVanishCheckbox.setVisible( false );
            mUseAppearCheckbox.setVisible( false );
            }
        }
    else {
        mPersonNoSpawnCheckbox.setVisible( true );
        mMaleCheckbox.setVisible( true );

        mDeathMarkerCheckbox.setToggled( false );
        mDeathMarkerCheckbox.setVisible( false );
        
        mHomeMarkerCheckbox.setToggled( false );
        mHomeMarkerCheckbox.setVisible( false );

        mFloorCheckbox.setToggled( false );
        mFloorCheckbox.setVisible( false );
        
        mNumUsesField.setInt( 1 );
        mNumUsesField.setVisible( false );
        mUseVanishCheckbox.setVisible( false );
        mUseAppearCheckbox.setVisible( false );
        
        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
            mCurrentObject.spriteUseVanish[i] = false;
            mCurrentObject.spriteUseAppear[i] = false;
            }


        mHeldInHandCheckbox.setVisible( false );
        mRideableCheckbox.setVisible( false );
        mBlocksWalkingCheckbox.setVisible( false );

        mHeldInHandCheckbox.setToggled( false );
        mRideableCheckbox.setToggled( false );
        mBlocksWalkingCheckbox.setToggled( false );
        
        if( mPickedObjectLayer != -1 ) {
            mAgingLayerCheckbox.setVisible( true );
            
            mHeadLayerCheckbox.setVisible( true );
            mBodyLayerCheckbox.setVisible( true );
            mBackFootLayerCheckbox.setVisible( true );
            mFrontFootLayerCheckbox.setVisible( true );

            mInvisibleWhenHoldingCheckbox.setVisible( true );

            if( mCurrentObject.spriteAgeStart[mPickedObjectLayer] != -1 &&
                mCurrentObject.spriteAgeEnd[mPickedObjectLayer] != -1 ) {
                
                mAgingLayerCheckbox.setToggled( true );
                agingPanelVisible = true;
                
                mAgeInField.setFloat(
                    mCurrentObject.spriteAgeStart[mPickedObjectLayer], 2 );
                mAgeOutField.setFloat(
                    mCurrentObject.spriteAgeEnd[mPickedObjectLayer], 2 );
                }
            else {
                mAgingLayerCheckbox.setToggled( false );
                agingPanelVisible = false;
                }

            mInvisibleWhenHoldingCheckbox.setToggled(
                mCurrentObject.spriteInvisibleWhenHolding[
                    mPickedObjectLayer] );

            mHeadLayerCheckbox.setToggled( 
                mCurrentObject.spriteIsHead[ mPickedObjectLayer ] );

            mBodyLayerCheckbox.setToggled( 
                mCurrentObject.spriteIsBody[ mPickedObjectLayer ] );

            mBackFootLayerCheckbox.setToggled( 
                mCurrentObject.spriteIsBackFoot[ mPickedObjectLayer ] );

            mFrontFootLayerCheckbox.setToggled( 
                mCurrentObject.spriteIsFrontFoot[ mPickedObjectLayer ] );
            }
        else {
            mAgingLayerCheckbox.setVisible( false );
                
            agingPanelVisible = false;

            mHeadLayerCheckbox.setVisible( false );
            mBodyLayerCheckbox.setVisible( false );
            mBackFootLayerCheckbox.setVisible( false );
            mFrontFootLayerCheckbox.setVisible( false );
            
            mInvisibleWhenHoldingCheckbox.setVisible( false );
            }
        
        }

    mAgeInField.setVisible( agingPanelVisible );
    mAgeOutField.setVisible( agingPanelVisible );
    mAgePunchInButton.setVisible( agingPanelVisible );
    mAgePunchOutButton.setVisible( agingPanelVisible );

    mBiomeField.setVisible( !person );
    mDeadlyDistanceField.setVisible( !person );
    mUseDistanceField.setVisible( !person );
    
    if( mPickedObjectLayer == -1 || ! anyClothingToggled() ) {
        mInvisibleWhenWornCheckbox.setVisible( false );
        mInvisibleWhenUnwornCheckbox.setVisible( false );
        }
    else {
        mInvisibleWhenWornCheckbox.setVisible( true );
        mInvisibleWhenUnwornCheckbox.setVisible( true );
        
        switch( mCurrentObject.spriteInvisibleWhenWorn[ mPickedObjectLayer ] ) {
            case 0:
                mInvisibleWhenWornCheckbox.setToggled( false );
                mInvisibleWhenUnwornCheckbox.setToggled( false );
                break;
            case 1:
                mInvisibleWhenWornCheckbox.setToggled( true );
                mInvisibleWhenUnwornCheckbox.setToggled( false );
                break;
            case 2:
                mInvisibleWhenWornCheckbox.setToggled( false );
                mInvisibleWhenUnwornCheckbox.setToggled( true );
                break;
            }
        }
    if( mPickedObjectLayer != -1 && mCurrentObject.numSlots > 0 ) {
        mBehindSlotsCheckbox.setVisible( true );
        mBehindSlotsCheckbox.setToggled( 
            mCurrentObject.spriteBehindSlots[ mPickedObjectLayer ] );
        }
    else {
        mBehindSlotsCheckbox.setVisible( false );
        }
    }


char EditorObjectPage::anyClothingToggled() {
    for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
        if( mClothingCheckboxes[i]->getToggled() ) {
            return true;
            }
        }
    return false;
    }



void EditorObjectPage::recursiveRotate( ObjectRecord *inObject,
                                        int inRotatingParentIndex,
                                        doublePair inRotationCenter,
                                        double inRotationDelta ) {
    if( mIgnoreParentLinkMode ) {
        return;
        }
    
    // adjust children recursively
    for( int i=0; i<inObject->numSprites; i++ ) {
        if( inObject->spriteParent[i] == inRotatingParentIndex ) {
            
            inObject->spriteRot[i] += inRotationDelta;
            
            inObject->spritePos[i] = sub( inObject->spritePos[i],
                                          inRotationCenter );
            
            inObject->spritePos[i] = rotate( inObject->spritePos[i],
                                             -2 * M_PI * inRotationDelta );
            
            inObject->spritePos[i] = add( inObject->spritePos[i],
                                          inRotationCenter );

            recursiveRotate( inObject, i, 
                             inRotationCenter, inRotationDelta );
            }
        }

    for( int i=0; i<inObject->numSlots; i++ ) {
        if( inObject->slotParent[i] == inRotatingParentIndex ) {
            
            inObject->slotPos[i] = sub( inObject->slotPos[i],
                                        inRotationCenter );
            
            inObject->slotPos[i] = rotate( inObject->slotPos[i],
                                           -2 * M_PI * inRotationDelta );
            
            inObject->slotPos[i] = add( inObject->slotPos[i],
                                        inRotationCenter );
            }
        }
    }



void EditorObjectPage::addNewSprite( int inSpriteID ) {
    mSimUseSlider.setVisible( false );
    mSimUseCheckbox.setToggled( false );
    
    int newNumSprites = mCurrentObject.numSprites + 1;
            
    int *newSprites = new int[ newNumSprites ];
    memcpy( newSprites, mCurrentObject.sprites, 
            mCurrentObject.numSprites * sizeof( int ) );
            
    doublePair *newSpritePos = new doublePair[ newNumSprites ];
    memcpy( newSpritePos, mCurrentObject.spritePos, 
            mCurrentObject.numSprites * sizeof( doublePair ) );

    double *newSpriteRot = new double[ newNumSprites ];
    memcpy( newSpriteRot, mCurrentObject.spriteRot, 
            mCurrentObject.numSprites * sizeof( double ) );

    char *newSpriteHFlip = new char[ newNumSprites ];
    memcpy( newSpriteHFlip, mCurrentObject.spriteHFlip, 
            mCurrentObject.numSprites * sizeof( char ) );

    FloatRGB *newSpriteColor = new FloatRGB[ newNumSprites ];
    memcpy( newSpriteColor, mCurrentObject.spriteColor, 
            mCurrentObject.numSprites * sizeof( FloatRGB ) );


    double *newSpriteAgeStart = new double[ newNumSprites ];
    memcpy( newSpriteAgeStart, mCurrentObject.spriteAgeStart, 
            mCurrentObject.numSprites * sizeof( double ) );

    double *newSpriteAgeEnd = new double[ newNumSprites ];
    memcpy( newSpriteAgeEnd, mCurrentObject.spriteAgeEnd, 
            mCurrentObject.numSprites * sizeof( double ) );

    int *newSpriteParent = new int[ newNumSprites ];
    memcpy( newSpriteParent, mCurrentObject.spriteParent, 
            mCurrentObject.numSprites * sizeof( int ) );

    char *newSpriteInvisibleWhenHolding = new char[ newNumSprites ];
    memcpy( newSpriteInvisibleWhenHolding, 
            mCurrentObject.spriteInvisibleWhenHolding, 
            mCurrentObject.numSprites * sizeof( char ) );

    int *newSpriteInvisibleWhenWorn = new int[ newNumSprites ];
    memcpy( newSpriteInvisibleWhenWorn, 
            mCurrentObject.spriteInvisibleWhenWorn, 
            mCurrentObject.numSprites * sizeof( int ) );

    char *newSpriteBehindSlots = new char[ newNumSprites ];
    memcpy( newSpriteBehindSlots, 
            mCurrentObject.spriteBehindSlots, 
            mCurrentObject.numSprites * sizeof( char ) );


    char *newSpriteIsHead = new char[ newNumSprites ];
    memcpy( newSpriteIsHead, 
            mCurrentObject.spriteIsHead, 
            mCurrentObject.numSprites * sizeof( char ) );

    char *newSpriteIsBody = new char[ newNumSprites ];
    memcpy( newSpriteIsBody, 
            mCurrentObject.spriteIsBody, 
            mCurrentObject.numSprites * sizeof( char ) );

    char *newSpriteIsBackFoot = new char[ newNumSprites ];
    memcpy( newSpriteIsBackFoot, 
            mCurrentObject.spriteIsBackFoot, 
            mCurrentObject.numSprites * sizeof( char ) );

    char *newSpriteIsFrontFoot = new char[ newNumSprites ];
    memcpy( newSpriteIsFrontFoot, 
            mCurrentObject.spriteIsFrontFoot, 
            mCurrentObject.numSprites * sizeof( char ) );

    
    char *newSpriteUseVanish = new char[ newNumSprites ];
    memcpy( newSpriteUseVanish, 
            mCurrentObject.spriteUseVanish, 
            mCurrentObject.numSprites * sizeof( char ) );

    char *newSpriteUseAppear = new char[ newNumSprites ];
    memcpy( newSpriteUseAppear, 
            mCurrentObject.spriteUseAppear, 
            mCurrentObject.numSprites * sizeof( char ) );


    newSprites[ mCurrentObject.numSprites ] = inSpriteID;
            
    doublePair pos = {0,0};
            
    newSpritePos[ mCurrentObject.numSprites ] = pos;

    newSpriteRot[ mCurrentObject.numSprites ] = 0;

    newSpriteHFlip[ mCurrentObject.numSprites ] = 0;

    FloatRGB white = { 1, 1, 1 };
            
    newSpriteColor[ mCurrentObject.numSprites ] = white;

    newSpriteAgeStart[ mCurrentObject.numSprites ] = -1;
    newSpriteAgeEnd[ mCurrentObject.numSprites ] = -1;
            
    newSpriteParent[ mCurrentObject.numSprites ] = -1;

    newSpriteInvisibleWhenHolding[ mCurrentObject.numSprites ] = 0;
    newSpriteInvisibleWhenWorn[ mCurrentObject.numSprites ] = 0;
    newSpriteBehindSlots[ mCurrentObject.numSprites ] = false;
            
    newSpriteIsHead[ mCurrentObject.numSprites ] = false;
    newSpriteIsBody[ mCurrentObject.numSprites ] = false;
    newSpriteIsBackFoot[ mCurrentObject.numSprites ] = false;
    newSpriteIsFrontFoot[ mCurrentObject.numSprites ] = false;

    newSpriteUseVanish[ mCurrentObject.numSprites ] = false;
    newSpriteUseAppear[ mCurrentObject.numSprites ] = false;
    

    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;
    delete [] mCurrentObject.spriteColor;

    delete [] mCurrentObject.spriteAgeStart;
    delete [] mCurrentObject.spriteAgeEnd;
    delete [] mCurrentObject.spriteParent;
            
    delete [] mCurrentObject.spriteInvisibleWhenHolding;
    delete [] mCurrentObject.spriteInvisibleWhenWorn;
    delete [] mCurrentObject.spriteBehindSlots;

    delete [] mCurrentObject.spriteIsHead;
    delete [] mCurrentObject.spriteIsBody;
    delete [] mCurrentObject.spriteIsBackFoot;
    delete [] mCurrentObject.spriteIsFrontFoot;
    
    delete [] mCurrentObject.spriteUseVanish;
    delete [] mCurrentObject.spriteUseAppear;
    

    mCurrentObject.sprites = newSprites;
    mCurrentObject.spritePos = newSpritePos;
    mCurrentObject.spriteRot = newSpriteRot;
    mCurrentObject.spriteHFlip = newSpriteHFlip;
    mCurrentObject.spriteColor = newSpriteColor;

    mCurrentObject.spriteAgeStart = newSpriteAgeStart;
    mCurrentObject.spriteAgeEnd = newSpriteAgeEnd;
    mCurrentObject.spriteParent = newSpriteParent;

    mCurrentObject.spriteInvisibleWhenHolding = 
        newSpriteInvisibleWhenHolding;
    mCurrentObject.spriteInvisibleWhenWorn = 
        newSpriteInvisibleWhenWorn;
    mCurrentObject.spriteBehindSlots = 
        newSpriteBehindSlots;


    mCurrentObject.spriteIsHead = newSpriteIsHead;
    mCurrentObject.spriteIsBody = newSpriteIsBody;
    mCurrentObject.spriteIsBackFoot = newSpriteIsBackFoot;
    mCurrentObject.spriteIsFrontFoot = newSpriteIsFrontFoot;


    mCurrentObject.spriteUseVanish = newSpriteUseVanish;
    mCurrentObject.spriteUseAppear = newSpriteUseAppear;
    

    mCurrentObject.numSprites = newNumSprites;
            
    }



void EditorObjectPage::endVertRotDemo() {
    mDemoVertRot = false;
    mDemoVertRotButton.setFillColor( 0.25, 0.25, 0.25, 1 );
    }



void EditorObjectPage::hideVertRotButtons() {
    endVertRotDemo();
    mDemoVertRotButton.setVisible( false );
    mResetVertRotButton.setVisible( false );
    }


void EditorObjectPage::showVertRotButtons() {
    mDemoVertRotButton.setVisible( true );
    mResetVertRotButton.setVisible( 
                    mCurrentObject.vertContainRotationOffset != 0 );
    
    // containable
    // so we can't have slots
    
    // good place to take care of making this invisible
    mSlotVertCheckbox.setVisible( false );
    }



void EditorObjectPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mDescriptionField ) {
        
        mStepsSinceDescriptionChange = 0;
        
        char *text = mDescriptionField.getText();
        

        mSaveObjectButton.setVisible(
            ( mCurrentObject.numSprites > 0 )
            &&
            ( strcmp( text, "" ) != 0 ) );
        
        mReplaceObjectButton.setVisible(
            mSaveObjectButton.isVisible() 
            &&
            mCurrentObject.id != -1 );

        delete [] text;
        }
    else if( inTarget == &mBiomeField ) {
        fixCommaIntList( &mBiomeField );
        }
    else if( inTarget == &mSaveObjectButton ) {
        char *text = mDescriptionField.getText();
        
        fixCommaIntList( &mBiomeField );
        char *biomes = mBiomeField.getText();

        int race = 0;
        
        if( mCheckboxes[2]->getToggled() ) {
            race = 1;
            char *raceText = mRaceField.getText();
            
            if( strlen( raceText ) > 0 ) {
                race = ( raceText[0] - 'A' ) + 1;
                }
            delete [] raceText;
            }
        
        char creationSoundInitialOnly = false;
        
        if( mCreationSoundWidget.getSoundUsage().numSubSounds > 0 ) {
            creationSoundInitialOnly = 
                mCreationSoundInitialOnlyCheckbox.getToggled();
            }

        int newID =
        addObject( text,
                   mCheckboxes[0]->getToggled(),
                   mContainSizeField.getInt(),
                   mCurrentObject.vertContainRotationOffset,
                   mCheckboxes[1]->getToggled(),
                   mMinPickupAgeField.getFloat(),
                   mHeldInHandCheckbox.getToggled(),
                   mRideableCheckbox.getToggled(),
                   mBlocksWalkingCheckbox.getToggled(),
                   mLeftBlockingRadiusField.getInt(),
                   mRightBlockingRadiusField.getInt(),
                   mDrawBehindPlayerCheckbox.getToggled(),
                   biomes,
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mPersonNoSpawnCheckbox.getToggled(),
                   mMaleCheckbox.getToggled(),
                   race,
                   mDeathMarkerCheckbox.getToggled(),
                   mHomeMarkerCheckbox.getToggled(),
                   mFloorCheckbox.getToggled(),
                   mFloorHuggingCheckbox.getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.clothing,
                   mCurrentObject.clothingOffset,
                   mDeadlyDistanceField.getInt(),
                   mUseDistanceField.getInt(),
                   mCreationSoundWidget.getSoundUsage(),
                   mUsingSoundWidget.getSoundUsage(),
                   mEatingSoundWidget.getSoundUsage(),
                   mDecaySoundWidget.getSoundUsage(),
                   creationSoundInitialOnly,
                   mCurrentObject.numSlots,
                   mSlotSizeField.getInt(),
                   mCurrentObject.slotPos,
                   mCurrentObject.slotVert,
                   mCurrentObject.slotParent,
                   mSlotTimeStretchField.getFloat(),
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip,
                   mCurrentObject.spriteColor,
                   mCurrentObject.spriteAgeStart,
                   mCurrentObject.spriteAgeEnd,
                   mCurrentObject.spriteParent,
                   mCurrentObject.spriteInvisibleWhenHolding,
                   mCurrentObject.spriteInvisibleWhenWorn,
                   mCurrentObject.spriteBehindSlots,
                   mCurrentObject.spriteIsHead,
                   mCurrentObject.spriteIsBody,
                   mCurrentObject.spriteIsBackFoot,
                   mCurrentObject.spriteIsFrontFoot,
                   mNumUsesField.getInt(),
                   mCurrentObject.spriteUseVanish,
                   mCurrentObject.spriteUseAppear );
        
        objectPickable.usePickable( newID );
        
        delete [] text;
        delete [] biomes;
        
        
        mSpritePicker.unselectObject();

        mObjectPicker.redoSearch( false );
        actionPerformed( &mClearObjectButton );
        }
    else if( inTarget == &mReplaceObjectButton ) {
        char *text = mDescriptionField.getText();

        fixCommaIntList( &mBiomeField );
        
        char *biomes = mBiomeField.getText();

        ObjectRecord *oldObject = getObject( mCurrentObject.id );
                
        int oldNumSprites = 0;
        int oldNumSlots = 0;
        
        double oldSpeedMult = 1.0;

        if( oldObject != NULL ) {
            oldNumSprites = oldObject->numSprites;
            oldNumSlots = oldObject->numSlots;
            oldSpeedMult = oldObject->speedMult;
            }
        
        
        char layersSwapped = false;
        

        if( mObjectLayerSwaps.size() > 0 ) {
            performLayerSwaps( mCurrentObject.id, &mObjectLayerSwaps,
                               mCurrentObject.numSprites );
            
            layersSwapped = true;
            }
        
        int race = 0;
        
        if( mCheckboxes[2]->getToggled() ) {
            race = 1;
            char *raceText = mRaceField.getText();
            
            if( strlen( raceText ) > 0 ) {
                race = ( raceText[0] - 'A' ) + 1;
                }
            delete [] raceText;
            }
        
        char creationSoundInitialOnly = false;
        
        if( mCreationSoundWidget.getSoundUsage().numSubSounds > 0 ) {
            creationSoundInitialOnly = 
                mCreationSoundInitialOnlyCheckbox.getToggled();
            }


        addObject( text,
                   mCheckboxes[0]->getToggled(),
                   mContainSizeField.getInt(),
                   mCurrentObject.vertContainRotationOffset,
                   mCheckboxes[1]->getToggled(),
                   mMinPickupAgeField.getFloat(),
                   mHeldInHandCheckbox.getToggled(),
                   mRideableCheckbox.getToggled(),
                   mBlocksWalkingCheckbox.getToggled(),
                   mLeftBlockingRadiusField.getInt(),
                   mRightBlockingRadiusField.getInt(),
                   mDrawBehindPlayerCheckbox.getToggled(),
                   biomes,
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mPersonNoSpawnCheckbox.getToggled(),
                   mMaleCheckbox.getToggled(),
                   race,
                   mDeathMarkerCheckbox.getToggled(),
                   mHomeMarkerCheckbox.getToggled(),
                   mFloorCheckbox.getToggled(),
                   mFloorHuggingCheckbox.getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.clothing,
                   mCurrentObject.clothingOffset,
                   mDeadlyDistanceField.getInt(),
                   mUseDistanceField.getInt(),
                   mCreationSoundWidget.getSoundUsage(),
                   mUsingSoundWidget.getSoundUsage(),
                   mEatingSoundWidget.getSoundUsage(),
                   mDecaySoundWidget.getSoundUsage(),
                   creationSoundInitialOnly,
                   mCurrentObject.numSlots,
                   mSlotSizeField.getInt(), 
                   mCurrentObject.slotPos,
                   mCurrentObject.slotVert,
                   mCurrentObject.slotParent,
                   mSlotTimeStretchField.getFloat(), 
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip,
                   mCurrentObject.spriteColor,
                   mCurrentObject.spriteAgeStart,
                   mCurrentObject.spriteAgeEnd,
                   mCurrentObject.spriteParent,
                   mCurrentObject.spriteInvisibleWhenHolding,
                   mCurrentObject.spriteInvisibleWhenWorn,
                   mCurrentObject.spriteBehindSlots,
                   mCurrentObject.spriteIsHead,
                   mCurrentObject.spriteIsBody,
                   mCurrentObject.spriteIsBackFoot,
                   mCurrentObject.spriteIsFrontFoot,
                   mNumUsesField.getInt(),
                   mCurrentObject.spriteUseVanish,
                   mCurrentObject.spriteUseAppear,
                   false,
                   mCurrentObject.id );
        
        objectPickable.usePickable( mCurrentObject.id );
        
        delete [] text;
        delete [] biomes;
        
        mSpritePicker.unselectObject();
        
        mObjectPicker.redoSearch( false );
        
        
        if( mCurrentObject.numSprites != oldNumSprites ||
            mCurrentObject.numSlots != oldNumSlots ||
            mSpeedMultField.getFloat() != oldSpeedMult || 
            layersSwapped ) {
                    
            animPage->objectLayersChanged( mCurrentObject.id );
            }    


        actionPerformed( &mClearObjectButton );
        }
    else if( inTarget == &mClearObjectButton ) {
        mCurrentObject.id = -1;
        
        mDescriptionField.setText( "" );

        delete [] mCurrentObject.description;
        mCurrentObject.description = mDescriptionField.getText();
        
        
        mPersonAgeSlider.setVisible( false );
        
        mRaceField.setVisible( false );
        
        for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
            mCheckboxes[i]->setToggled( false );
            }

        mMapChanceField.setText( "0.00" );
        mBiomeField.setText( "0" );

        mHeatValueField.setText( "0" );
        mRValueField.setText( "0.00" );

        mFoodValueField.setText( "0" );
        mSpeedMultField.setText( "1.00" );

        mContainSizeField.setInt( 1 );
        mSlotSizeField.setInt( 1 );
        mSlotTimeStretchField.setText( "1.0" );
        
        mContainSizeField.setVisible( false );
        mSlotSizeField.setVisible( false );
        mSlotTimeStretchField.setVisible( false );
        
        mFloorCheckbox.setToggled( false );
        mFloorCheckbox.setVisible( true );
        

        mDeadlyDistanceField.setInt( 0 );
        mUseDistanceField.setInt( 1 );
        mMinPickupAgeField.setInt( 3 );
        
        mCurrentObject.numSlots = 0;
        
        mCurrentObject.heldOffset.x = 0;
        mCurrentObject.heldOffset.y = 0;
        
        for( int c=0; c<NUM_CLOTHING_CHECKBOXES; c++ ) {
            mClothingCheckboxes[c]->setToggled( false );
            }
        mInvisibleWhenWornCheckbox.setToggled( false );
        mInvisibleWhenWornCheckbox.setVisible( false );

        mInvisibleWhenUnwornCheckbox.setToggled( false );
        mInvisibleWhenUnwornCheckbox.setVisible( false );

        mBehindSlotsCheckbox.setToggled( false );
        mBehindSlotsCheckbox.setVisible( false );
        

        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = new doublePair[ 0 ];

        delete [] mCurrentObject.slotVert;
        mCurrentObject.slotVert = new char[ 0 ];

        delete [] mCurrentObject.slotParent;
        mCurrentObject.slotParent = new int[ 0 ];

        
        mCurrentObject.numSprites = 0;
        
        delete [] mCurrentObject.sprites;
        mCurrentObject.sprites = new int[ 0 ];

        delete [] mCurrentObject.spritePos;
        mCurrentObject.spritePos = new doublePair[ 0 ];
        
        delete [] mCurrentObject.spriteRot;
        mCurrentObject.spriteRot = new double[ 0 ];

        delete [] mCurrentObject.spriteHFlip;
        mCurrentObject.spriteHFlip = new char[ 0 ];

        delete [] mCurrentObject.spriteColor;
        mCurrentObject.spriteColor = new FloatRGB[ 0 ];

        delete [] mCurrentObject.spriteAgeStart;
        mCurrentObject.spriteAgeStart = new double[ 0 ];

        delete [] mCurrentObject.spriteAgeEnd;
        mCurrentObject.spriteAgeEnd = new double[ 0 ];


        delete [] mCurrentObject.spriteParent;
        mCurrentObject.spriteParent = new int[ 0 ];

        delete [] mCurrentObject.spriteInvisibleWhenHolding;
        mCurrentObject.spriteInvisibleWhenHolding = new char[ 0 ];


        delete [] mCurrentObject.spriteIsHead;
        mCurrentObject.spriteIsHead = new char[ 0 ];

        delete [] mCurrentObject.spriteIsBody;
        mCurrentObject.spriteIsBody = new char[ 0 ];

        delete [] mCurrentObject.spriteIsBackFoot;
        mCurrentObject.spriteIsBackFoot = new char[ 0 ];

        delete [] mCurrentObject.spriteIsFrontFoot;
        mCurrentObject.spriteIsFrontFoot = new char[ 0 ];

        delete [] mCurrentObject.spriteUseVanish;
        mCurrentObject.spriteUseVanish = new char[ 0 ];

        delete [] mCurrentObject.spriteUseAppear;
        mCurrentObject.spriteUseAppear = new char[ 0 ];
        
        mNumUsesField.setInt( 1 );
        mNumUsesField.setVisible( true );


        mSaveObjectButton.setVisible( false );
        mReplaceObjectButton.setVisible( false );
        mClearObjectButton.setVisible( false );
        

        mDemoSlotsButton.setVisible( false );
        mClearSlotsDemoButton.setVisible( false );
        mSetHeldPosButton.setVisible( false );
        mMinPickupAgeField.setVisible( false );
        
        mEndSetHeldPosButton.setVisible( false );
        mNextHeldDemoButton.setVisible( false );
        mPrevHeldDemoButton.setVisible( false );
        
        mCopyHeldPosButton.setVisible( false );
        mPasteHeldPosButton.setVisible( false );
        
        mDemoSlots = false;
        mSlotsDemoObject = -1;
        
        mSetHeldPos = false;
        mDemoPersonObject = -1;

        mSetClothesPos = false;
        
        
        mPickedObjectLayer = -1;
        mPickedSlot = -1;
        
        pickedLayerChanged();

        
        mObjectPicker.unselectObject();
        }
    else if( inTarget == &mClearRotButton ) {
        if( mDemoVertRot ) {
            mCurrentObject.vertContainRotationOffset = 0;
            }
        else if( mPickedObjectLayer != -1 ) {
            
            double oldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];
            
            mCurrentObject.spriteRot[mPickedObjectLayer] = 0;

            recursiveRotate( &mCurrentObject,
                             mPickedObjectLayer,
                             mCurrentObject.spritePos[ mPickedObjectLayer ],
                             mCurrentObject.spriteRot[ mPickedObjectLayer ] - 
                             oldRot );
            }
        mClearRotButton.setVisible( false );
        }
    else if( inTarget == &mRot45ForwardButton ) {
        if( mDemoVertRot ) {
            mCurrentObject.vertContainRotationOffset += 0.125;
            }
        else if( mPickedObjectLayer != -1 ) {
            double oldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];

            mCurrentObject.spriteRot[mPickedObjectLayer] = 
                0.125 + floor( oldRot * 8 ) / 8;
            
            recursiveRotate( &mCurrentObject,
                             mPickedObjectLayer,
                             mCurrentObject.spritePos[ mPickedObjectLayer ],
                             mCurrentObject.spriteRot[ mPickedObjectLayer ] - 
                             oldRot );
            }
        }
    else if( inTarget == &mRot45BackwardButton ) {
        if( mDemoVertRot ) {
            mCurrentObject.vertContainRotationOffset -= 0.125;
            }
        else if( mPickedObjectLayer != -1 ) {
            double oldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];

            mCurrentObject.spriteRot[mPickedObjectLayer] = 
                -0.125 + ceil( oldRot * 8 ) / 8;
            
            recursiveRotate( &mCurrentObject,
                             mPickedObjectLayer,
                             mCurrentObject.spritePos[ mPickedObjectLayer ],
                             mCurrentObject.spriteRot[ mPickedObjectLayer ] - 
                             oldRot );
            }
        }
    else if( inTarget == &mFlipHButton ) {
        if( mPickedObjectLayer != -1 ) {
            mCurrentObject.spriteHFlip[mPickedObjectLayer] =
                ! mCurrentObject.spriteHFlip[mPickedObjectLayer];
            }
        else {
            mFlipHButton.setVisible( false );
            }
        }
    else if( inTarget == &mBakeButton ) {
        char *des = mDescriptionField.getText();

        char found;
        char *tag = replaceAll( des, " ", "", &found );

        int newID = bakeSprite( tag,
                                mCurrentObject.numSprites,
                                mCurrentObject.sprites,
                                mCurrentObject.spritePos,
                                mCurrentObject.spriteHFlip );
        
        spritePickable.usePickable( newID );

        mSpritePicker.redoSearch( false );
        
        delete [] des;
        delete [] tag;
        }
    else if( inTarget == &mImportEditorButton ) {
        setSignal( "importEditor" );
        }
    else if( inTarget == &mTransEditorButton ) {
        setSignal( "transEditor" );
        }
    else if( inTarget == &mAnimEditorButton ) {
        setSignal( "animEditor" );
        }
    else if( inTarget == &mMoreSlotsButton ) {
        
        int numSlots = mCurrentObject.numSlots;
        
        doublePair *slots = new doublePair[ numSlots + 1 ];
        
        memcpy( slots, mCurrentObject.slotPos, 
                sizeof( doublePair ) * numSlots );

        char *slotsVert = new char[ numSlots + 1 ];
        
        memcpy( slotsVert, mCurrentObject.slotVert, 
                sizeof( char ) * numSlots );


        int *slotsParent = new int[ numSlots + 1 ];
        
        memcpy( slotsParent, mCurrentObject.slotParent, 
                sizeof( int ) * numSlots );
        

        if( mCurrentObject.numSlots == 0 ) {
            slotsVert[numSlots] = false;
            slotsParent[numSlots] = -1;
            slots[numSlots].x = 0;
            slots[numSlots].y = 0;
            mDemoSlotsButton.setVisible( true );
            mClearSlotsDemoButton.setVisible( false );
            }
        else {
            slotsVert[numSlots] = slotsVert[ numSlots - 1 ];
            slotsParent[numSlots] = slotsParent[ numSlots - 1 ];
            slots[numSlots].x = 
                slots[numSlots - 1].x + 16;
            slots[numSlots].y = 
                slots[numSlots - 1].y;
            }
        
        mPickedSlot = numSlots;
        mPickedObjectLayer = -1;

        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = slots;

        delete [] mCurrentObject.slotVert;
        mCurrentObject.slotVert = slotsVert;

        delete [] mCurrentObject.slotParent;
        mCurrentObject.slotParent = slotsParent;


        mCurrentObject.numSlots = numSlots + 1;
        
        pickedLayerChanged();
        
        mSlotSizeField.setVisible( true );
        mSlotTimeStretchField.setVisible( true );
        
        
        mPersonAgeSlider.setVisible( false );

        mRaceField.setVisible( false );
        
        mCheckboxes[2]->setToggled( false );
        updateAgingPanel();
        
        mSetHeldPosButton.setVisible( true );
        mMinPickupAgeField.setVisible( true );
        }
    else if( inTarget == &mLessSlotsButton ) {
        if( mCurrentObject.numSlots > 0 ) {
            
            mCurrentObject.numSlots --;
            
            if( mCurrentObject.numSlots == 0 ) {
                mDemoSlots = false;
                mSlotsDemoObject = -1;
                mDemoSlotsButton.setVisible( false );
                mClearSlotsDemoButton.setVisible( false );
                
                mSlotSizeField.setInt( 1 );
                mSlotSizeField.setVisible( false );
                
                mSlotTimeStretchField.setText( "1.0" );
                mSlotTimeStretchField.setVisible( false );
                
                mBehindSlotsCheckbox.setVisible( false );
                }
            }
        }
    else if( inTarget == &mDemoSlotsButton ) {
        mDemoSlots = true;
        mDemoSlotsButton.setVisible( false );
        mClearSlotsDemoButton.setVisible( true );
        }
    else if( inTarget == &mClearSlotsDemoButton ) {
        mDemoSlots = false;
        mSlotsDemoObject = -1;
        mDemoSlotsButton.setVisible( true );
        mClearSlotsDemoButton.setVisible( false );
        }
    else if( inTarget == &mSetHeldPosButton ) {
        mDemoPersonObject = getRandomPersonObject();
        
        if( mDemoPersonObject != -1 ) {
            
            mSetHeldPos = true;
            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( false );
            mEndSetHeldPosButton.setVisible( true );
            
            mNextHeldDemoButton.setVisible( true );
            mPrevHeldDemoButton.setVisible( true );

            mCopyHeldPosButton.setVisible( true );
            mPasteHeldPosButton.setVisible( true );

            mPersonAgeSlider.setValue( defaultAge );
            mPersonAgeSlider.setVisible( true );
            
            mBiomeField.setVisible( false );
            mDeadlyDistanceField.setVisible( false );
            mUseDistanceField.setVisible( false );
            }
        }
    else if( inTarget == &mEndSetHeldPosButton ) {
        mSetHeldPos = false;
        mDemoPersonObject = -1;

        mSetHeldPosButton.setVisible( true );
        mMinPickupAgeField.setVisible( true );
        mEndSetHeldPosButton.setVisible( false );

        mNextHeldDemoButton.setVisible( false );
        mPrevHeldDemoButton.setVisible( false );

        mCopyHeldPosButton.setVisible( false );
        mPasteHeldPosButton.setVisible( false );

        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( defaultAge );
            mPersonAgeSlider.setVisible( true );
            mRaceField.setVisible( true );
            
            mBiomeField.setVisible( false );
            mDeadlyDistanceField.setVisible( false );
            mUseDistanceField.setVisible( false );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            mRaceField.setVisible( false );
            
            mBiomeField.setVisible( true );
            mDeadlyDistanceField.setVisible( true );
            mUseDistanceField.setVisible( true );
            }
        
        if( anyClothingToggled() ) {
            mDemoClothesButton.setVisible( true );
            mEndClothesDemoButton.setVisible( false );
            }
        }
    else if( inTarget == &mNextHeldDemoButton ) {
        mDemoPersonObject = getNextPersonObject( mDemoPersonObject );
        }
    else if( inTarget == &mPrevHeldDemoButton ) {
        mDemoPersonObject = getPrevPersonObject( mDemoPersonObject );
        }
    else if( inTarget == &mCopyHeldPosButton ) {
        mHeldOffsetClipboard = mCurrentObject.heldOffset;
        }
    else if( inTarget == &mPasteHeldPosButton ) {
        mCurrentObject.heldOffset = mHeldOffsetClipboard;
        }
    else if( inTarget == &mDemoClothesButton ) {
        mDemoPersonObject = getRandomPersonObject();
        
        if( mDemoPersonObject != -1 ) {
            
            mSetClothesPos = true;
            mDemoClothesButton.setVisible( false );
            mEndClothesDemoButton.setVisible( true );
            
            mSetHeldPos = false;
            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( true );
            mEndSetHeldPosButton.setVisible( false );

            mNextHeldDemoButton.setVisible( true );
            mPrevHeldDemoButton.setVisible( true );

            mCopyHeldPosButton.setVisible( true );
            mPasteHeldPosButton.setVisible( true );

            mPersonAgeSlider.setValue( defaultAge );
            mPersonAgeSlider.setVisible( true );

            mBiomeField.setVisible( false );
            mDeadlyDistanceField.setVisible( false );
            mUseDistanceField.setVisible( false );
            }
        }
    else if( inTarget == &mEndClothesDemoButton ) {
        mSetClothesPos = false;
        mDemoPersonObject = -1;

        mDemoClothesButton.setVisible( true );
        mEndClothesDemoButton.setVisible( false );

        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( defaultAge );
            mPersonAgeSlider.setVisible( true );
            mRaceField.setVisible( true );
            
            mBiomeField.setVisible( false );
            mDeadlyDistanceField.setVisible( false );
            mUseDistanceField.setVisible( false );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            mRaceField.setVisible( false );

            mBiomeField.setVisible( true );
            mDeadlyDistanceField.setVisible( true );
            mUseDistanceField.setVisible( true );
            }

        mSetHeldPosButton.setVisible( true );
        mMinPickupAgeField.setVisible( true );
        mEndSetHeldPosButton.setVisible( false );
        
        mNextHeldDemoButton.setVisible( false );
        mPrevHeldDemoButton.setVisible( false );
        
        mCopyHeldPosButton.setVisible( false );
        mPasteHeldPosButton.setVisible( false );
        }
    else if( inTarget == &mResetVertRotButton ) {
        mCurrentObject.vertContainRotationOffset = 0;
        mResetVertRotButton.setVisible( false );
        }
    else if( inTarget == &mDemoVertRotButton ) {
        if( mDemoVertRot ) {
            endVertRotDemo();
            pickedLayerChanged();
            }
        else {
            mPickedSlot = -1;
            mPickedObjectLayer = -1;
            pickedLayerChanged();
            mDemoVertRot = true;
            mDemoVertRotButton.setFillColor( 0.5, 0.0, 0.0, 1 );
            mRot45ForwardButton.setVisible( true );
            mRot45BackwardButton.setVisible( true );
            }
        }
    else if( inTarget == &mHeldInHandCheckbox ) {
        if( mHeldInHandCheckbox.getToggled() ) {
            mRideableCheckbox.setToggled( false );
            }
        }
    else if( inTarget == &mRideableCheckbox ) {
        if( mRideableCheckbox.getToggled() ) {
            mHeldInHandCheckbox.setToggled( false );
            }
        }
    else if( inTarget == &mDeathMarkerCheckbox ) {
        if( mDeathMarkerCheckbox.getToggled() ) {
            mBlocksWalkingCheckbox.setToggled( false );
            
            mLeftBlockingRadiusField.setVisible( false );
            mRightBlockingRadiusField.setVisible( false );
            mLeftBlockingRadiusField.setInt( 0 );
            mRightBlockingRadiusField.setInt( 0 );
            mDrawBehindPlayerCheckbox.setToggled( false );
            mFloorHuggingCheckbox.setToggled( false );
            } 
        }
    else if( inTarget == &mBlocksWalkingCheckbox ) {
        mLeftBlockingRadiusField.setVisible( false );
        mRightBlockingRadiusField.setVisible( false );

        if( mBlocksWalkingCheckbox.getToggled() ) {
            mDeathMarkerCheckbox.setToggled( false );
            
            if( mCheckboxes[1]->getToggled() ) {
                mLeftBlockingRadiusField.setVisible( true );
                mRightBlockingRadiusField.setVisible( true );
                }
            }
        if( ! mLeftBlockingRadiusField.isVisible() ) {
            mLeftBlockingRadiusField.setInt( 0 );
            mRightBlockingRadiusField.setInt( 0 );
            }
        }
    else if( inTarget == mCheckboxes[1] ) {
        mLeftBlockingRadiusField.setVisible( false );
        mRightBlockingRadiusField.setVisible( false );

        mDrawBehindPlayerCheckbox.setToggled( false );
        mDrawBehindPlayerCheckbox.setVisible( false );

        mFloorHuggingCheckbox.setToggled( false );
        mFloorHuggingCheckbox.setVisible( false );

        if( ! mCheckboxes[1]->getToggled() ) {
            if( !mSetHeldPos ) {
                mSetHeldPosButton.setVisible( true );
                mMinPickupAgeField.setVisible( true );
                }
         
            mLeftBlockingRadiusField.setVisible( false );
            mRightBlockingRadiusField.setVisible( false );
            
            mLeftBlockingRadiusField.setInt( 0 );
            mRightBlockingRadiusField.setInt( 0 );
            
            mFloorHuggingCheckbox.setVisible( false );
            mFloorHuggingCheckbox.setToggled( false );
            }
        else {
            if( mSetHeldPos ) {
                actionPerformed( &mEndSetHeldPosButton );
                }
            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( false );
            mDrawBehindPlayerCheckbox.setVisible( true );
            mFloorHuggingCheckbox.setVisible( true );
            }
        
        
        if( mBlocksWalkingCheckbox.getToggled() ) {

            if( mCheckboxes[1]->getToggled() ) {
                mLeftBlockingRadiusField.setVisible( true );
                mRightBlockingRadiusField.setVisible( true );
     
                mDrawBehindPlayerCheckbox.setVisible( true );
                
                if( mSetHeldPos ) {
                    actionPerformed( &mEndSetHeldPosButton );
                    }
                mSetHeldPosButton.setVisible( false );
                mMinPickupAgeField.setVisible( false );
                }
            }
        if( ! mLeftBlockingRadiusField.isVisible() ) {
            mLeftBlockingRadiusField.setInt( 0 );
            mRightBlockingRadiusField.setInt( 0 );
            }

        if( mCheckboxes[1]->getToggled() ) {
            if( mCheckboxes[0]->getToggled() ) {
                mCheckboxes[0]->setToggled( false );
                actionPerformed( mCheckboxes[0] );
                }
            }
        }
    else if( inTarget == &mAgingLayerCheckbox ) {
        if( mAgingLayerCheckbox.getToggled() ) {
            mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] = 0;
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] = 999;
            }
        else {
            mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] = -1;
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] = -1;
            }
        updateAgingPanel();
        }
    else if( inTarget == &mInvisibleWhenHoldingCheckbox ) {
        mCurrentObject.spriteInvisibleWhenHolding[ mPickedObjectLayer]
            = mInvisibleWhenHoldingCheckbox.getToggled();
        } 
    else if( inTarget == &mInvisibleWhenWornCheckbox ) {
        if( mInvisibleWhenWornCheckbox.getToggled() ) {
            mInvisibleWhenUnwornCheckbox.setToggled( false );
            mCurrentObject.spriteInvisibleWhenWorn[ mPickedObjectLayer ] = 1;
            }
        else {
            mCurrentObject.spriteInvisibleWhenWorn[ mPickedObjectLayer ] = 0;
            }
        }
    else if( inTarget == &mInvisibleWhenUnwornCheckbox ) {
        if( mInvisibleWhenUnwornCheckbox.getToggled() ) {
            mInvisibleWhenWornCheckbox.setToggled( false );
            mCurrentObject.spriteInvisibleWhenWorn[ mPickedObjectLayer ] = 2;
            }
        else {
            mCurrentObject.spriteInvisibleWhenWorn[ mPickedObjectLayer ] = 0;
            }
        }
    else if( inTarget == &mBehindSlotsCheckbox ) {
        mCurrentObject.spriteBehindSlots[ mPickedObjectLayer ]
            = mBehindSlotsCheckbox.getToggled();
        }
    else if( inTarget == &mHeadLayerCheckbox ) {
        mCurrentObject.spriteIsHead[ mPickedObjectLayer ] =
            mHeadLayerCheckbox.getToggled();
        }
    else if( inTarget == &mBodyLayerCheckbox ) {
        mCurrentObject.spriteIsBody[ mPickedObjectLayer ] =
            mBodyLayerCheckbox.getToggled();
        }
    else if( inTarget == &mBackFootLayerCheckbox ) {
        mCurrentObject.spriteIsBackFoot[ mPickedObjectLayer ] =
            mBackFootLayerCheckbox.getToggled();
        }
    else if( inTarget == &mFrontFootLayerCheckbox ) {
        mCurrentObject.spriteIsFrontFoot[ mPickedObjectLayer ] =
            mFrontFootLayerCheckbox.getToggled();
        }
    else if( inTarget == &mUseVanishCheckbox ) {
        char on = mUseVanishCheckbox.getToggled();
 
        mCurrentObject.spriteUseVanish[ mPickedObjectLayer ] = on;
        
        if( on ) {
            mUseAppearCheckbox.setToggled( false );
            mCurrentObject.spriteUseAppear[ mPickedObjectLayer ] = false;
            }
        }
    else if( inTarget == &mUseAppearCheckbox ) {
        char on = mUseAppearCheckbox.getToggled();
 
        mCurrentObject.spriteUseAppear[ mPickedObjectLayer ] = on;
        
        if( on ) {
            mUseVanishCheckbox.setToggled( false );
            mCurrentObject.spriteUseVanish[ mPickedObjectLayer ] = false;
            }
        }
    else if( inTarget == &mSimUseCheckbox ) {
        char on = mSimUseCheckbox.getToggled();
 
        mSimUseSlider.setVisible( on );
        mSimUseSlider.setHighValue( mCurrentObject.numSprites );
        mSimUseSlider.setValue( mCurrentObject.numSprites );
        }
    else if( inTarget == &mNumUsesField ) {
        int val = mNumUsesField.getInt();
        
        if( val > 1 ) {
            if( mPickedObjectLayer != -1 ) {
                mUseVanishCheckbox.setVisible( true );
                mUseVanishCheckbox.setToggled( 
                    mCurrentObject.spriteUseVanish[ mPickedObjectLayer ] );
                
                mUseAppearCheckbox.setVisible( true );
                mUseAppearCheckbox.setToggled( 
                    mCurrentObject.spriteUseAppear[ mPickedObjectLayer ] );
                }
            
            mSimUseCheckbox.setVisible( true );
            }
        else {
            mUseVanishCheckbox.setVisible( false );
            mUseAppearCheckbox.setVisible( false );
            mSimUseCheckbox.setToggled( false );
            mSimUseCheckbox.setVisible( false );
            mSimUseSlider.setVisible( false );
            }
        }
    else if( inTarget == &mAgePunchInButton ) {
        mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] =
            mPersonAgeSlider.getValue();
        
        if( mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] >
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] ) {
            
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] = 
                mCurrentObject.spriteAgeStart[ mPickedObjectLayer ];
            }
        
        updateAgingPanel();
        }
    else if( inTarget == &mAgePunchOutButton ) {
        mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] =
            mPersonAgeSlider.getValue();

        if( mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] >
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] ) {
            
            mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] =
                mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ];
            }

        updateAgingPanel();
        }
    else if( inTarget == &mAgeInField ) {
        
        // sometimes we lose field focus when a new layer is clicked
        // make sure this is still an aging layer.
        if( mPickedObjectLayer != -1 && mAgingLayerCheckbox.getToggled() ) {    

            float value = mAgeInField.getFloat();
            if( value < 0 ) {
                value = 0;
                }
            mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] = value;
            
            if( mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] >
                mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] ) {
                
                mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] =
                    mCurrentObject.spriteAgeStart[ mPickedObjectLayer ];
                }
            updateAgingPanel();
            }
        }
    else if( inTarget == &mAgeOutField ) {

        // sometimes we lose field focus when a new layer is clicked
        // make sure this is still an aging layer.
        if( mPickedObjectLayer != -1  && mAgingLayerCheckbox.getToggled() ) {
            float value = mAgeOutField.getFloat();
            if( value < 0 ) {
                value = 0;
                }
            mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] = value;
            
            if( mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] >
                mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ] ) {
                
                mCurrentObject.spriteAgeStart[ mPickedObjectLayer ] =
                    mCurrentObject.spriteAgeEnd[ mPickedObjectLayer ];
                }
            updateAgingPanel();
            }
        }
    else if( inTarget == &mSpritePicker ) {
        
        if( mDemoSlots ) {
            mDemoSlots = false;
            mSlotsDemoObject = -1;
            mDemoSlotsButton.setVisible( true );
            mClearSlotsDemoButton.setVisible( false );
            }
        else {
            mSetHeldPos = false;
            mDemoPersonObject = -1;
            
            if( ! mCheckboxes[2]->getToggled() ) {
                
                if( mPickedObjectLayer != -1 ) {    
                    mSetHeldPosButton.setVisible( true );
                    mMinPickupAgeField.setVisible( true );
                    }
                
                if( anyClothingToggled() ) {
                    mDemoClothesButton.setVisible( true );
                    mEndClothesDemoButton.setVisible( false );
                    }
                }
            mEndSetHeldPosButton.setVisible( false );
            
            mNextHeldDemoButton.setVisible( false );
            mPrevHeldDemoButton.setVisible( false );

            mCopyHeldPosButton.setVisible( false );
            mPasteHeldPosButton.setVisible( false );

            mSetClothesPos = false;
            mDemoPersonObject = -1;
            
            mEndClothesDemoButton.setVisible( false );
            }
        

        char rightClick = false;
        
        int spriteID = mSpritePicker.getSelectedObject( &rightClick );
        

        if( spriteID != -1 && mPickedObjectLayer != -1 && rightClick ) {
            // replace the current layer
            mCurrentObject.sprites[ mPickedObjectLayer ] = spriteID;
            }
        else if( spriteID != -1 ) {
            
            addNewSprite( spriteID );
            

            

            mClearObjectButton.setVisible( true );
            
            char *text = mDescriptionField.getText();
            
            if( strlen( text ) > 0 ) {
                mSaveObjectButton.setVisible( true );
                }
            delete [] text;

            mPickedObjectLayer = mCurrentObject.numSprites - 1;
            mPickedSlot = -1;
            
            pickedLayerChanged();
            }
        }
    
    else if( inTarget == &mObjectPicker ) {
        
        mSimUseSlider.setVisible( false );
        mSimUseCheckbox.setToggled( false );
        mSimUseCheckbox.setVisible( false );
        
        
        char rightClick;
        
        int objectID = mObjectPicker.getSelectedObject( &rightClick );

        
        // auto-end the held-pos setting if a new object is picked
        // (also, potentially enable the setting button for the first time) 
        if( objectID != -1 && ! mDemoSlots ) {
            mSetHeldPos = false;
            mSetClothesPos = false;
            mDemoPersonObject = -1;
            mSetHeldPosButton.setVisible( true );
            mMinPickupAgeField.setVisible( true );
            mEndSetHeldPosButton.setVisible( false );
            
            mNextHeldDemoButton.setVisible( false );
            mPrevHeldDemoButton.setVisible( false );
            
            mCopyHeldPosButton.setVisible( false );
            mPasteHeldPosButton.setVisible( false );
            }
        

        if( objectID != -1 && mDemoSlots ) {
            mSlotsDemoObject = objectID;
            }
        else if( objectID != -1 && rightClick ) {
            ObjectRecord *pickedRecord = getObject( objectID );

            int oldNumSprites = mCurrentObject.numSprites;
            
            for( int i=0; i<pickedRecord->numSprites; i++ ) {
                
                addNewSprite( pickedRecord->sprites[i] );
                
                mCurrentObject.spritePos[i + oldNumSprites] = 
                    pickedRecord->spritePos[i];
                mCurrentObject.spriteRot[i + oldNumSprites] = 
                    pickedRecord->spriteRot[i];
                mCurrentObject.spriteHFlip[i + oldNumSprites] = 
                    pickedRecord->spriteHFlip[i];
                mCurrentObject.spriteColor[i + oldNumSprites] = 
                    pickedRecord->spriteColor[i];
                
                mCurrentObject.spriteInvisibleWhenHolding[i + oldNumSprites] = 
                    pickedRecord->spriteInvisibleWhenHolding[i];
                mCurrentObject.spriteInvisibleWhenWorn[i + oldNumSprites] = 
                    pickedRecord->spriteInvisibleWhenWorn[i];
                
                mCurrentObject.spriteBehindSlots[i + oldNumSprites] = 
                    pickedRecord->spriteBehindSlots[i];
                
                if( pickedRecord->spriteParent[i] != -1 ) {
                    mCurrentObject.spriteParent[i + oldNumSprites] = 
                        pickedRecord->spriteParent[i] + oldNumSprites;
                    }
                }
            }
        else if( objectID != -1 ) {
            ObjectRecord *pickedRecord = getObject( objectID );
                
            delete [] mCurrentObject.slotPos;
            delete [] mCurrentObject.slotVert;
            delete [] mCurrentObject.slotParent;
            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            delete [] mCurrentObject.spriteRot;
            delete [] mCurrentObject.spriteHFlip;
            delete [] mCurrentObject.spriteColor;
            delete [] mCurrentObject.spriteAgeStart;
            delete [] mCurrentObject.spriteAgeEnd;
            delete [] mCurrentObject.spriteParent;
            delete [] mCurrentObject.spriteInvisibleWhenHolding;
            delete [] mCurrentObject.spriteInvisibleWhenWorn;
            delete [] mCurrentObject.spriteBehindSlots;

            delete [] mCurrentObject.spriteIsHead;
            delete [] mCurrentObject.spriteIsBody;
            delete [] mCurrentObject.spriteIsBackFoot;
            delete [] mCurrentObject.spriteIsFrontFoot;

            delete [] mCurrentObject.spriteUseVanish;
            delete [] mCurrentObject.spriteUseAppear;
            
            
            mObjectLayerSwaps.deleteAll();

            mCurrentObject.id = objectID;
                
            mDescriptionField.setText( pickedRecord->description );

            mMapChanceField.setFloat( pickedRecord->mapChance, 4 );
            
            char *biomeText = getBiomesString( pickedRecord );
            
            mBiomeField.setText( biomeText );
            
            delete [] biomeText;
            

            mHeatValueField.setInt( pickedRecord->heatValue );
            mRValueField.setFloat( pickedRecord->rValue, 2 );
            
            mFoodValueField.setInt( pickedRecord->foodValue );

            mSpeedMultField.setFloat( pickedRecord->speedMult, 2 );

            mContainSizeField.setInt( pickedRecord->containSize );
            mSlotSizeField.setInt( pickedRecord->slotSize );
            mSlotTimeStretchField.setFloat( pickedRecord->slotTimeStretch,
                                            -1, true );
            
            mDeadlyDistanceField.setInt( pickedRecord->deadlyDistance );
            mUseDistanceField.setInt( pickedRecord->useDistance );
            
            mMinPickupAgeField.setInt( pickedRecord->minPickupAge );

            mCurrentObject.containable = pickedRecord->containable;
            mCurrentObject.vertContainRotationOffset = 
                pickedRecord->vertContainRotationOffset;
            endVertRotDemo();
            mRotAdjustMode = false;
            
            mCurrentObject.heldOffset = pickedRecord->heldOffset;

            mCurrentObject.clothing = pickedRecord->clothing;
            mCurrentObject.clothingOffset = pickedRecord->clothingOffset;

            mCurrentObject.numSlots = pickedRecord->numSlots;

            mCurrentObject.slotPos = 
                new doublePair[ pickedRecord->numSlots ];
                
            memcpy( mCurrentObject.slotPos, pickedRecord->slotPos,
                    sizeof( doublePair ) * pickedRecord->numSlots );

            mCurrentObject.slotVert = 
                new char[ pickedRecord->numSlots ];
                
            memcpy( mCurrentObject.slotVert, pickedRecord->slotVert,
                    sizeof( char ) * pickedRecord->numSlots );

            mCurrentObject.slotParent = 
                new int[ pickedRecord->numSlots ];
                
            memcpy( mCurrentObject.slotParent, pickedRecord->slotParent,
                    sizeof( int ) * pickedRecord->numSlots );


            mCurrentObject.numSprites = pickedRecord->numSprites;
                
            mCurrentObject.sprites = new int[ pickedRecord->numSprites ];
            mCurrentObject.spritePos = 
                new doublePair[ pickedRecord->numSprites ];
            mCurrentObject.spriteRot = 
                new double[ pickedRecord->numSprites ];
            mCurrentObject.spriteHFlip = 
                new char[ pickedRecord->numSprites ];
            mCurrentObject.spriteColor = 
                new FloatRGB[ pickedRecord->numSprites ];
            
            mCurrentObject.spriteAgeStart = 
                new double[ pickedRecord->numSprites ];
            mCurrentObject.spriteAgeEnd = 
                new double[ pickedRecord->numSprites ];
            mCurrentObject.spriteParent = 
                new int[ pickedRecord->numSprites ];

            mCurrentObject.spriteInvisibleWhenHolding = 
                new char[ pickedRecord->numSprites ];

            mCurrentObject.spriteInvisibleWhenWorn = 
                new int[ pickedRecord->numSprites ];

            mCurrentObject.spriteBehindSlots = 
                new char[ pickedRecord->numSprites ];


            mCurrentObject.spriteIsHead = 
                new char[ pickedRecord->numSprites ];

            mCurrentObject.spriteIsBody = 
                new char[ pickedRecord->numSprites ];

            mCurrentObject.spriteIsBackFoot = 
                new char[ pickedRecord->numSprites ];

            mCurrentObject.spriteIsFrontFoot = 
                new char[ pickedRecord->numSprites ];
            
            mCurrentObject.spriteUseVanish =
                new char[ pickedRecord->numSprites ];

            mCurrentObject.spriteUseAppear =
                new char[ pickedRecord->numSprites ];
            
            
            mNumUsesField.setInt( pickedRecord->numUses );
            
            mSimUseCheckbox.setVisible( pickedRecord->numUses > 1 );
            

            memcpy( mCurrentObject.sprites, pickedRecord->sprites,
                    sizeof( int ) * pickedRecord->numSprites );
                
            memcpy( mCurrentObject.spritePos, pickedRecord->spritePos,
                    sizeof( doublePair ) * pickedRecord->numSprites );

            memcpy( mCurrentObject.spriteRot, pickedRecord->spriteRot,
                    sizeof( double ) * pickedRecord->numSprites );
            
            memcpy( mCurrentObject.spriteHFlip, pickedRecord->spriteHFlip,
                    sizeof( char ) * pickedRecord->numSprites );
                
            memcpy( mCurrentObject.spriteColor, pickedRecord->spriteColor,
                    sizeof( FloatRGB ) * pickedRecord->numSprites );


            memcpy( mCurrentObject.spriteAgeStart, 
                    pickedRecord->spriteAgeStart,
                    sizeof( double ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteAgeEnd, 
                    pickedRecord->spriteAgeEnd,
                    sizeof( double ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteParent, 
                    pickedRecord->spriteParent,
                    sizeof( int ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteInvisibleWhenHolding, 
                    pickedRecord->spriteInvisibleWhenHolding,
                    sizeof( char ) * pickedRecord->numSprites );
            
            memcpy( mCurrentObject.spriteInvisibleWhenWorn, 
                    pickedRecord->spriteInvisibleWhenWorn,
                    sizeof( int ) * pickedRecord->numSprites );

            memcpy( mCurrentObject.spriteBehindSlots, 
                    pickedRecord->spriteBehindSlots,
                    sizeof( char ) * pickedRecord->numSprites );
            
            memcpy( mCurrentObject.spriteIsHead, 
                    pickedRecord->spriteIsHead,
                    sizeof( char ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteIsBody, 
                    pickedRecord->spriteIsBody,
                    sizeof( char ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteIsBackFoot, 
                    pickedRecord->spriteIsBackFoot,
                    sizeof( char ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteIsFrontFoot, 
                    pickedRecord->spriteIsFrontFoot,
                    sizeof( char ) * pickedRecord->numSprites );
            
            
            memcpy( mCurrentObject.spriteUseVanish, 
                    pickedRecord->spriteUseVanish,
                    sizeof( char ) * pickedRecord->numSprites );
            memcpy( mCurrentObject.spriteUseAppear, 
                    pickedRecord->spriteUseAppear,
                    sizeof( char ) * pickedRecord->numSprites );
            
            
            mSaveObjectButton.setVisible( true );
            mReplaceObjectButton.setVisible( true );
            mClearObjectButton.setVisible( true );

            mPickedObjectLayer = -1;
            mPickedSlot = -1;

            if( mCurrentObject.numSlots > 0 ) {
                mDemoSlots = false;
                mSlotsDemoObject = -1;
                mDemoSlotsButton.setVisible( true );
                mClearSlotsDemoButton.setVisible( false );
                }
            else {
                mDemoSlots = false;
                mSlotsDemoObject = -1;
                mDemoSlotsButton.setVisible( false );
                mClearSlotsDemoButton.setVisible( false );
                }

            for( int c=0; c<NUM_CLOTHING_CHECKBOXES; c++ ) {
                mClothingCheckboxes[c]->setToggled( false );
                }
            
            switch( mCurrentObject.clothing ) {
                case 'b':
                    mClothingCheckboxes[0]->setToggled( true );
                    actionPerformed( mClothingCheckboxes[0] );
                    break;
                case 'p':
                    mClothingCheckboxes[1]->setToggled( true );
                    actionPerformed( mClothingCheckboxes[1] );
                    break;
                case 's':
                    mClothingCheckboxes[2]->setToggled( true );
                    actionPerformed( mClothingCheckboxes[2] );
                    break;
                case 't':
                    mClothingCheckboxes[3]->setToggled( true );
                    actionPerformed( mClothingCheckboxes[3] );
                    break;
                case 'h':
                    mClothingCheckboxes[4]->setToggled( true );
                    actionPerformed( mClothingCheckboxes[4] );
                    break;
                }   

            mInvisibleWhenWornCheckbox.setToggled( false );
            mInvisibleWhenWornCheckbox.setVisible( false );

            mInvisibleWhenUnwornCheckbox.setToggled( false );
            mInvisibleWhenUnwornCheckbox.setVisible( false );

            mBehindSlotsCheckbox.setToggled( false );
            mBehindSlotsCheckbox.setVisible( false );
            
            mCheckboxes[0]->setToggled( pickedRecord->containable );
            mCheckboxes[1]->setToggled( pickedRecord->permanent );
            mCheckboxes[2]->setToggled( pickedRecord->person );

            if( mCheckboxes[0]->getToggled() ) {
                showVertRotButtons();
                }
            else {
                hideVertRotButtons();
                }

            mPersonNoSpawnCheckbox.setToggled( pickedRecord->personNoSpawn );

            mMaleCheckbox.setToggled( pickedRecord->male );
            mDeathMarkerCheckbox.setToggled( pickedRecord->deathMarker );
            mHomeMarkerCheckbox.setToggled( pickedRecord->homeMarker );
            mFloorCheckbox.setToggled( pickedRecord->floor );
            
            mCreationSoundWidget.setSoundUsage( pickedRecord->creationSound );
            mUsingSoundWidget.setSoundUsage( pickedRecord->usingSound );
            mEatingSoundWidget.setSoundUsage( pickedRecord->eatingSound );
            mDecaySoundWidget.setSoundUsage( pickedRecord->decaySound );

            mCreationSoundInitialOnlyCheckbox.setToggled( 
                pickedRecord->creationSoundInitialOnly );

            mCreationSoundInitialOnlyCheckbox.setVisible( 
                mCreationSoundWidget.getSoundUsage().numSubSounds > 0 );
            

            mRaceField.setText( "A" );
            
            if( pickedRecord->person ) {
                char raceChar = 'A';
                raceChar += pickedRecord->race - 1;
                
                char *raceText = autoSprintf( "%c", raceChar );
                mRaceField.setText( raceText );
                delete [] raceText;
                }

            mHeldInHandCheckbox.setToggled( pickedRecord->heldInHand );
            mRideableCheckbox.setToggled( pickedRecord->rideable );
            mBlocksWalkingCheckbox.setToggled( pickedRecord->blocksWalking );
            
            mLeftBlockingRadiusField.setInt( pickedRecord->leftBlockingRadius );
            
            mRightBlockingRadiusField.setInt( 
                pickedRecord->rightBlockingRadius );
            
            if( mBlocksWalkingCheckbox.getToggled() &&
                mCheckboxes[1]->getToggled() ) {
                mLeftBlockingRadiusField.setVisible( true );
                mRightBlockingRadiusField.setVisible( true );
                mDrawBehindPlayerCheckbox.setVisible( true );
                mDrawBehindPlayerCheckbox.setToggled( 
                    pickedRecord->drawBehindPlayer );
                mSetHeldPosButton.setVisible( false );
                mMinPickupAgeField.setVisible( false );
                } 
            else {
                mLeftBlockingRadiusField.setVisible( false );
                mRightBlockingRadiusField.setVisible( false );
                mDrawBehindPlayerCheckbox.setToggled( false );
                mDrawBehindPlayerCheckbox.setVisible( false );
                mSetHeldPosButton.setVisible( true );
                mMinPickupAgeField.setVisible( true );
                }
            
            if( mCheckboxes[1]->getToggled() ) {
                mSetHeldPosButton.setVisible( false );
                mDrawBehindPlayerCheckbox.setVisible( true );
                mDrawBehindPlayerCheckbox.setToggled( 
                    pickedRecord->drawBehindPlayer );
                                
                mFloorHuggingCheckbox.setVisible( true );
                mFloorHuggingCheckbox.setToggled( pickedRecord->floorHugging );
                }
            else {
                mFloorHuggingCheckbox.setToggled( false );
                mFloorHuggingCheckbox.setVisible( false );
                }
            
            if( mRideableCheckbox.getToggled() ) {
                mHeldInHandCheckbox.setToggled( false );
                }
            else if( mHeldInHandCheckbox.getToggled() ) {
                mRideableCheckbox.setToggled( false );
                }


            if( mCheckboxes[2]->getToggled() ) {
                mPersonAgeSlider.setVisible( true );
                mRaceField.setVisible( true );
                }
            else {
                mPersonAgeSlider.setVisible( false );
                mRaceField.setVisible( false );
                }
            pickedLayerChanged();

            mFloorCheckbox.setVisible( false );
            
            if( ! pickedRecord->containable ) {
                mContainSizeField.setInt( 1 );
                mContainSizeField.setVisible( false );
                
                if( ! mCheckboxes[2]->getToggled() ) {
                    mFloorCheckbox.setVisible( true );
                    }
                }
            else {
                mContainSizeField.setVisible( true );    
                }
            
            if( pickedRecord->numSlots == 0 ) {
                mSlotSizeField.setInt( 1 );
                mSlotSizeField.setVisible( false );
                
                mSlotTimeStretchField.setText( "1.0" );
                mSlotTimeStretchField.setVisible( false );
                }
            else {
                mSlotSizeField.setVisible( true );
                mSlotTimeStretchField.setVisible( true );
                }
            }
        }
    else if( inTarget == mCheckboxes[0] ) {
        if( mCheckboxes[0]->getToggled() ) {
            
            showVertRotButtons();
                
            mContainSizeField.setVisible( true );
            mFloorCheckbox.setToggled( false );
            mFloorCheckbox.setVisible( false );
            
            mPersonAgeSlider.setVisible( false );
            mCheckboxes[2]->setToggled( false );
            
            if( ! mSetHeldPos ) {
                mSetHeldPosButton.setVisible( true );
                mMinPickupAgeField.setVisible( true );
                }
            
            mRaceField.setVisible( false );
            
            if( mCheckboxes[1]->getToggled() ) {
                mCheckboxes[1]->setToggled( false );
                actionPerformed( mCheckboxes[1] );
                }
            }
        else {
            mContainSizeField.setInt( 1 );
            mContainSizeField.setVisible( false );
            mFloorCheckbox.setVisible( true );
            mCurrentObject.vertContainRotationOffset = 0;
            hideVertRotButtons();
            }
                    
        updateAgingPanel();
        }
    else if( inTarget == mCheckboxes[2] ) {
        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( defaultAge );
            mPersonAgeSlider.setVisible( true );
            mRaceField.setVisible( true );

            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( false );
            
            mContainSizeField.setInt( 1 );
            mContainSizeField.setVisible( false );
            mCheckboxes[0]->setToggled( false );
            hideVertRotButtons();
            
            mFloorCheckbox.setVisible( true );

            mCurrentObject.vertContainRotationOffset = 0;
            
            mSlotSizeField.setInt( 1 );
            mSlotSizeField.setVisible( false );
            mSlotTimeStretchField.setText( "1.0" );
            mSlotTimeStretchField.setVisible( false );
            
            mDemoSlotsButton.setVisible( false );
            mCurrentObject.numSlots = 0;

            mDemoClothesButton.setVisible( false );
            mCurrentObject.clothing = 'n';
            for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
                mClothingCheckboxes[i]->setToggled( false );
                }
            }
        else {
            mPersonAgeSlider.setVisible( false );
            mSetHeldPosButton.setVisible( true );
            mMinPickupAgeField.setVisible( true );
            mRaceField.setVisible( false );
            }
                    
        updateAgingPanel();    
        }
    else if( inTarget == &mHueSlider ||
             inTarget == &mSaturationSlider ||
             inTarget == &mValueSlider ) {
        
        if( mPickedObjectLayer != -1 &&
            ! getUsesMultiplicativeBlending( 
                mCurrentObject.sprites[mPickedObjectLayer] ) ) {
            
            Color *c = Color::makeColorFromHSV(
                mHueSlider.getValue(), mSaturationSlider.getValue(),
                mValueSlider.getValue() );

            mCurrentObject.spriteColor[mPickedObjectLayer].r = c->r;
            mCurrentObject.spriteColor[mPickedObjectLayer].g = c->g;
            mCurrentObject.spriteColor[mPickedObjectLayer].b = c->b;
            delete c;
            
            updateSliderColors();
            }
        
        }
    else if( inTarget == &mSlotVertCheckbox ) {
        if( mPickedSlot != -1 ) {
            mCurrentObject.slotVert[ mPickedSlot ] = 
                mSlotVertCheckbox.getToggled();
            }
        }
    else {
        // check clothing checkboxes
                
        for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
            if( inTarget == mClothingCheckboxes[i] ) {
                char c = 'n';

                if( mClothingCheckboxes[i]->getToggled() ) {
                    for( int j=0; j<NUM_CLOTHING_CHECKBOXES; j++ ) {
                        if( i != j ) {
                            mClothingCheckboxes[j]->setToggled( false );
                            }
                        }

                    switch( i ) {
                        case 0:
                            c = 'b';
                            break;
                        case 1:
                            c = 'p';
                            break;                            
                        case 2:
                            c = 's';
                            break;
                        case 3:
                            c = 't';
                            break;
                        case 4:
                            c = 'h';
                            break;
                        }
                    }
                
                
                
                
                mCurrentObject.clothing = c;

                mEndClothesDemoButton.setVisible( false );
                mSetClothesPos = false;

                if( c != 'n' ) {
                    mDemoClothesButton.setVisible( true );
                    
                    mPersonAgeSlider.setVisible( false );
                    mCheckboxes[2]->setToggled( false );
                    mSetHeldPosButton.setVisible( true );
                    mMinPickupAgeField.setVisible( true );
                    pickedLayerChanged();
                    }
                else {
                    mInvisibleWhenWornCheckbox.setVisible( false );
                    mInvisibleWhenUnwornCheckbox.setVisible( false );
                    mDemoClothesButton.setVisible( false );
                    }
                
                break;
                }
            }
        }

    }



void EditorObjectPage::drawSpriteLayers( doublePair inDrawOffset, 
                                         char inBehindSlots ) {
    doublePair drawOffset = inDrawOffset;

    doublePair headPos = {0,0};
    int headIndex = 0;

    doublePair frontFootPos = {0,0};
    int frontFootIndex = 0;

    doublePair bodyPos = {0,0};
    int bodyIndex = 0;

    if( mPersonAgeSlider.isVisible() &&
        mCheckboxes[2]->getToggled() ) {
        
        double age = mPersonAgeSlider.getValue();

        if( mSetHeldPos ) {
            // we're setting the held pos of a person by another person
            // hold the held person's aget constant at 0 (baby)
            // and only apply slider to holding adult
            age = 0;
            }

        headIndex = getHeadIndex( &mCurrentObject, age );

        headPos = mCurrentObject.spritePos[ headIndex ];

        
        frontFootIndex = getFrontFootIndex( &mCurrentObject, age );

        frontFootPos = mCurrentObject.spritePos[ frontFootIndex ];


        bodyIndex = getBodyIndex( &mCurrentObject, age );

        bodyPos = mCurrentObject.spritePos[ bodyIndex ];
        }
    
    
    doublePair layerZeroPos = { 0, 0 };
    
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        doublePair spritePos = mCurrentObject.spritePos[i];
        
        float blue = 1;
        float red = 1;
        float green = 1;
        float alpha = 1;


        char multiplicative = 
            getUsesMultiplicativeBlending( mCurrentObject.sprites[i] );

        
        if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
            blue = 1 - mHoverStrength;
            red = 1 - mHoverStrength;
            alpha *= mHoverFlash;
            
            if( multiplicative ) {
                red = 1 - red;
                green = 1 - green;
                blue = 1 - blue;
                }
            }
        else {
            FloatRGB color = mCurrentObject.spriteColor[i];
            
            red = color.r;
            green = color.g;
            blue = color.b;

            if( ( mHoverObjectLayer != -1 || mHoverSlot != -1 )
                && mHoverStrength > 0 ) {
                
                alpha = 0.25;
                }
            }
        
        setDrawColor( red, green, blue, alpha );

        
                        
        if( mPersonAgeSlider.isVisible() &&
            mCheckboxes[2]->getToggled() ) {
        
            double age = mPersonAgeSlider.getValue();
            
            if( mSetHeldPos ) {
                // we're setting the held pos of a person by another person
                // hold the held person's aget constant at 0 (baby)
                // and only apply slider to holding adult
                age = 0;
                }

            if( mCurrentObject.spriteAgeStart[i] != -1 &&
                mCurrentObject.spriteAgeEnd[i] != -1 ) {
                
                if( age < mCurrentObject.spriteAgeStart[i] || 
                    age >= mCurrentObject.spriteAgeEnd[i] ) {
                    
                    if( i != mPickedObjectLayer ) {
                        // skip drawing
                        continue;
                        }
                    }
                }

            if( i == headIndex ||
                checkSpriteAncestor( &mCurrentObject, i,
                                     headIndex ) ) {
            
                spritePos = add( spritePos, getAgeHeadOffset( age, headPos,
                                                              bodyPos,
                                                              frontFootPos ) );
                }
            if( i == bodyIndex ||
                checkSpriteAncestor( &mCurrentObject, i,
                                     bodyIndex ) ) {
            
                spritePos = add( spritePos, getAgeBodyOffset( age, bodyPos ) );
                }
            }
                
        spritePos = add( spritePos, drawOffset );
        
        if( i == 0 ) {
            layerZeroPos = spritePos;
            }
        
        
        if( multiplicative ) {
            toggleMultiplicativeBlend( true );
            if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
                toggleAdditiveTextureColoring( true );
                }
            }
        
        if( ( inBehindSlots && mCurrentObject.spriteBehindSlots[i] )
            ||
            ( ! inBehindSlots && ! mCurrentObject.spriteBehindSlots[i] ) ) {
            
            double spriteRot = mCurrentObject.spriteRot[i];

            if( mDemoVertRot ) {
                double rot = 0.25 + mCurrentObject.vertContainRotationOffset;
            
                double rotRadians = rot * 2 * M_PI;
                
                // rotate relative to layer 0
                doublePair posOffset = sub( spritePos, layerZeroPos );

                posOffset = rotate( posOffset, -rotRadians );
                
                spritePos = add( layerZeroPos, posOffset );

                spriteRot += rot;
                }
            
            if( mSimUseSlider.isVisible() &&
                mSimUseSlider.getValue() <= i &&
                mCurrentObject.spriteUseVanish[i]  ) {
                // skip this sprite, simulating vanish order
                }
            else if( mSimUseSlider.isVisible() &&
                     mSimUseSlider.getHighValue() - 
                     mSimUseSlider.getValue() <= i &&
                mCurrentObject.spriteUseAppear[i]  ) {
                // skip this sprite, simulating appear order
                }
            else {
                drawSprite( getSprite( mCurrentObject.sprites[i] ), spritePos,
                            1.0, spriteRot,
                            mCurrentObject.spriteHFlip[i] );
                }
            }
        
        if( multiplicative ) {
            toggleMultiplicativeBlend( false );
            if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
                toggleAdditiveTextureColoring( false );
                }
            }
        }    
    }


     
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    


    
    setDrawColor( 1, 1, 1, 1 );
    for( int y=0; y<3; y++ ) {
        for( int x=0; x<6; x++ ) {
            
            doublePair pos = { (double)( x * 64 - 160 ), 
                               (double)( y * 64 - 64 ) };
            
            drawSquare( pos, 32 );
            }
        }

    doublePair basePos = { -128, 0 };
    

    for( int c =-1; c<2; c++ ) {
        float f = 1.0;
        
        if( c != 0 ) {
            f = 0.75f;
            }
        
        int i = 0;
        for( int y=0; y<2; y++ ) {
            for( int x=0; x<2; x++ ) {
                
                doublePair pos = { (double)( x * 64 - 32), 
                                   (double)( y * 64 - 64 ) };
                pos = add( basePos, pos );
                if( i%2 == 0 ) {
                    setDrawColor( .85, .85 * f, .85 * f, 1 );
                    }
                else {
                    setDrawColor( 0.75, 0.75 * f, 0.75 * f, 1 );
                    }
                
                drawSquare( pos, 32 );
                i++;
                }
            i++;
            }
        basePos.x += 128;
        }

    // draw overlay to show foot-cross-over point

    doublePair footRecPos = { 0, -96 };
    
    setDrawColor( 0, 0, 0, 0.1 );
    
    drawRect( footRecPos, 192, 32 );
    

    
    doublePair barPos = { 0, 128 };
    
    setDrawColor( .85, .85, .85, 1 );

    drawRect( barPos, 192, 32 );

    barPos.y += 64 - 16;
    setDrawColor( 1, 1, 1, 1 );

    drawRect( barPos, 192, 16 );


    if( mPrintRequested ) {
        doublePair pos = { 0, 0 };
                           
        drawSquare( pos, 600 );
        }
    

    doublePair drawOffset = mObjectCenterOnScreen;
    

    if( mSetHeldPos ) {
        
        // draw sample person behind
        setDrawColor( 1, 1, 1, 1 );


        double age = mPersonAgeSlider.getValue();
        
        int hideClosestArm = 0;
        
        char hideAllLimbs = false;

        ObjectRecord *personObject = getObject( mDemoPersonObject );

        doublePair personPos = drawOffset;

        if( mHeldInHandCheckbox.getToggled() ) {
            
            hideClosestArm = 0;
            }
        else if( mRideableCheckbox.getToggled() ) {
            hideAllLimbs = true;
            hideClosestArm = 0;

            personPos = 
                sub( personPos, 
                     getAgeBodyOffset( 
                         age,
                         personObject->spritePos[ 
                             getBodyIndex( personObject, age ) ] ) );
            }
        else {
            // no longer hiding arm for non-handheld objects
            
            // but pass -2 so that body position is returned
            // instead of hand position
            hideClosestArm = -2;
            hideAllLimbs = false;
            }
        
        
        
        HoldingPos holdingPos =
            drawObject( personObject, 2, personPos, 0, false, false, 
                        age, hideClosestArm, hideAllLimbs, false,
                        getEmptyClothingSet() );

        if( holdingPos.valid ) {
            doublePair rotatedOffset = mCurrentObject.heldOffset;
            
            if( hideClosestArm ) {
                // only rotate held pos for non-handheld objects
                rotatedOffset = rotate( mCurrentObject.heldOffset, 
                                        -2 * M_PI * holdingPos.rot );
                }
                    
            drawOffset = add( rotatedOffset, holdingPos.pos );
            }
        else {
            drawOffset = add( mCurrentObject.heldOffset, drawOffset );
            }
        
        if( ! mCheckboxes[2]->getToggled() ) {
            // don't use center offset of a person when calculating
            // baby held pos
            drawOffset = sub( drawOffset, 
                              getObjectCenterOffset( &mCurrentObject ) );
            }
        }

    char skipDrawing = false;
    
    if( mSetClothesPos ) {
        
        // draw sample person behind
        setDrawColor( 1, 1, 1, 1 );


        double age = mPersonAgeSlider.getValue();
          

        ClothingSet s = getEmptyClothingSet();
        
        switch( mCurrentObject.clothing ) {
            case 'b':
                s.bottom = &mCurrentObject;
                break;
            case 'p':
                s.backpack = &mCurrentObject;
                break;
            case 's':
                s.backShoe = &mCurrentObject;
                s.frontShoe = &mCurrentObject;
                break;
            case 't':
                s.tunic = &mCurrentObject;
                break;
            case 'h':
                s.hat = &mCurrentObject;
                break;
            }
        
                
  
        drawObject( getObject( mDemoPersonObject ), 2, drawOffset, 0, 
                    false, false, 
                    age, 0, false, false, s );

        // offset from body part
        //switch( mCurrentObject.clothing ) {
        //    case 's':
        
        skipDrawing = true;
        }
    


    if( !skipDrawing ) {
        drawSpriteLayers( drawOffset, true );
        }
    
    if( ! skipDrawing && mCurrentObject.numSlots > 0 ) {

        if( mSlotsDemoObject == -1 ) {
            
            for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                
                float blue = 1;
                float red = 1;
                
                float alpha = 0.5;

                if( mHoverSlot == i && mHoverStrength > 0 ) {
                    blue = 1 - mHoverStrength;
                    red = 1 - mHoverStrength;
                    
                    // use changing hover strength to guide timing
                    alpha *= mHoverFlash;
                    }
                
                float green = 1;
                
                if( mCurrentObject.slotVert[i] ) {
                    green = 0;
                    }

                setDrawColor( red, green, blue, alpha );
                drawSprite( mSlotPlaceholderSprite, 
                            add( mCurrentObject.slotPos[i],
                                 drawOffset ) );
                
                
                setDrawColor( 0, 1, 1, 0.5 );
                
                if( mCurrentObject.slotVert[i] ) {
                    setDrawColor( 1, 1, 1, 0.5 );
                    }
                
                char *numberString = autoSprintf( "%d", i + 1 );
                
                mainFont->drawString( numberString, 
                                      add( mCurrentObject.slotPos[i], 
                                           drawOffset ),
                                      alignCenter );
                
                delete [] numberString;
                }
            }
        else {
            ObjectRecord *demoObject = getObject( mSlotsDemoObject );
            
            for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                float blue = 1;
                float red = 1;
                
                float alpha = 1;

                if( mHoverSlot == i && mHoverStrength > 0 ) {
                    blue = 1 - mHoverStrength;
                    red = 1 - mHoverStrength;
                    
                    // use changing hover strength to guide timing
                    alpha *= mHoverFlash;
                    }

                setDrawColor( red, 1, blue, alpha );
                
                double rot = 0;
                doublePair centerOffset = getObjectCenterOffset( demoObject );

                if( mCurrentObject.slotVert[i] ) {
                    rot = 0.25 + demoObject->vertContainRotationOffset;

                    centerOffset = rotate( centerOffset, -rot * 2 * M_PI );
                    }
                

                drawObject( demoObject, 2, 
                            sub( add( mCurrentObject.slotPos[i], drawOffset ),
                                 centerOffset ),
                            rot, false, false, -1, 0, false, false, 
                            getEmptyClothingSet() );
                }
            }
        
        }
    

    if( !skipDrawing ) {
        drawSpriteLayers( drawOffset, false );
        }

    
    
    if( mPrintRequested ) {
        
        Image *im = getScreenRegion( -250, -200,
                                    500, 560 );
        if( ! mSavePrintOnly ) {
            printImage( im );
            }
        else {
            writeTGAFile( "savedObjectImage.tga", im );
            }
        
        delete im;
        
        mPrintRequested = false;
        }
            
    


    doublePair pos = { 0, 0 };
    


    setDrawColor( 1, 1, 1, 1 );
    pos.x = mMoreSlotsButton.getPosition().x - 40;
    
    pos.y = 0.5 * ( mMoreSlotsButton.getPosition().y + 
                    mLessSlotsButton.getPosition().y );

    char *numSlotString = autoSprintf( "Slots: %d", mCurrentObject.numSlots );
    
    smallFont->drawString( numSlotString, pos, alignLeft );

    delete [] numSlotString;


    
    if( mPickedObjectLayer != -1 ) {
        pos.x = -310;
        pos.y = 90;

        char *rotString = 
            autoSprintf( "Rot: %f", 
                         mCurrentObject.spriteRot[mPickedObjectLayer] );
    
        smallFont->drawString( rotString, pos, alignLeft );
        
        delete [] rotString;

        if( mCurrentObject.spriteHFlip[ mPickedObjectLayer ] ) {
            pos = mFlipHButton.getPosition();
            
            pos.x += 32;
            
            smallFont->drawString( "F", pos, alignLeft );
            }
        }
    


    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        pos = mCheckboxes[i]->getPosition();
    
        pos.x -= checkboxSep;
        
        smallFont->drawString( mCheckboxNames[i], pos, alignRight );
        }

    for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
        pos = mClothingCheckboxes[i]->getPosition();
    
        pos.x -= checkboxSep;
        
        smallFont->drawString( mClothingCheckboxNames[i], pos, alignRight );
        }
    
    if( mInvisibleWhenWornCheckbox.isVisible() ) {
        pos = mInvisibleWhenWornCheckbox.getPosition();
    
        pos.x -= checkboxSep;
        
        smallFont->drawString( "Worn X", pos, alignRight );
        }
    if( mInvisibleWhenUnwornCheckbox.isVisible() ) {
        pos = mInvisibleWhenUnwornCheckbox.getPosition();
    
        pos.x -= checkboxSep;
        
        smallFont->drawString( "Unworn X", pos, alignRight );
        }

    if( mBehindSlotsCheckbox.isVisible() ) {
        pos = mBehindSlotsCheckbox.getPosition();
    
        pos.x -= checkboxSep;
        
        smallFont->drawString( "Behind Slots", pos, alignRight );
        }
    

    if( mAgingLayerCheckbox.isVisible() ) {
        pos = mAgingLayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Age Layer", pos, alignRight );
        }


    if( mInvisibleWhenHoldingCheckbox.isVisible() ) {
        pos = mInvisibleWhenHoldingCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Holder", pos, alignRight );
        }
    

    if( mPersonNoSpawnCheckbox.isVisible() ) {
        pos = mPersonNoSpawnCheckbox.getPosition();
        pos.y += checkboxSep + 5;
        smallFont->drawString( "NoSpawn", pos, alignCenter );
        }

    if( mMaleCheckbox.isVisible() ) {
        pos = mMaleCheckbox.getPosition();
        pos.y += checkboxSep + 5;
        smallFont->drawString( "Male", pos, alignCenter );
        }

    if( mDeathMarkerCheckbox.isVisible() ) {
        pos = mDeathMarkerCheckbox.getPosition();
        pos.y += checkboxSep + 5;
        smallFont->drawString( "Death", pos, alignCenter );
        }

    if( mHomeMarkerCheckbox.isVisible() ) {
        pos = mHomeMarkerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Home", pos, alignRight );
        }

    if( mFloorCheckbox.isVisible() ) {
        pos = mFloorCheckbox.getPosition();
        pos.y += checkboxSep + 5;
        smallFont->drawString( "Floor", pos, alignCenter );
        }

    if( mHeldInHandCheckbox.isVisible() ) {
        pos = mHeldInHandCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Handheld", pos, alignRight );
        }
    if( mRideableCheckbox.isVisible() ) {
        pos = mRideableCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Rideable", pos, alignRight );
        }

    if( mBlocksWalkingCheckbox.isVisible() ) {
        pos = mBlocksWalkingCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Blocking", pos, alignRight );
        }

    if( mDrawBehindPlayerCheckbox.isVisible() ) {
        pos = mDrawBehindPlayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Behind", pos, alignRight );
        }

    if( mFloorHuggingCheckbox.isVisible() ) {
        pos = mFloorHuggingCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Hug Floor", pos, alignRight );
        }
    

    if( mUseVanishCheckbox.isVisible() ) {
        pos = mUseVanishCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Vnsh", pos, alignRight );
        }
    if( mUseAppearCheckbox.isVisible() ) {
        pos = mUseAppearCheckbox.getPosition();
        pos.x += checkboxSep;
        smallFont->drawString( "Appr", pos, alignLeft );
        }

    if( mSimUseCheckbox.isVisible() ) {
        pos = mSimUseCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Sim", pos, alignRight );
        }


    if( mHeadLayerCheckbox.isVisible() ) {
        pos = mHeadLayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Head", pos, alignRight );
        }

    if( mBodyLayerCheckbox.isVisible() ) {
        pos = mBodyLayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Body", pos, alignRight );
        }

    if( mBackFootLayerCheckbox.isVisible() ) {
        pos = mBackFootLayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Back Foot", pos, alignRight );
        }

    if( mFrontFootLayerCheckbox.isVisible() ) {
        pos = mFrontFootLayerCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Front Foot", pos, alignRight );
        }

    if( mSlotVertCheckbox.isVisible() ) {
        pos = mSlotVertCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Vertical Slot", pos, alignRight );
        }
    
    
    if( mCreationSoundWidget.isVisible() ) {
        pos = mCreationSoundWidget.getPosition();
        pos.x -= 100;
        pos.y += 25;
        
        smallFont->drawString( "Creation:", pos, alignLeft );
        }
    if( mUsingSoundWidget.isVisible() ) {
        pos = mUsingSoundWidget.getPosition();
        pos.x -= 100;
        pos.y += 25;
        
        smallFont->drawString( "Using:", pos, alignLeft );
        }
    if( mEatingSoundWidget.isVisible() ) {
        pos = mEatingSoundWidget.getPosition();
        pos.x -= 100;
        pos.y += 25;
        
        smallFont->drawString( "Eating:", pos, alignLeft );
        }
    if( mDecaySoundWidget.isVisible() ) {
        pos = mDecaySoundWidget.getPosition();
        pos.x -= 100;
        pos.y += 25;
        
        smallFont->drawString( "Decay:", pos, alignLeft );
        }


    if( mCreationSoundInitialOnlyCheckbox.isVisible() ) {
        pos = mCreationSoundInitialOnlyCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Initial Only", pos, alignRight );
        }

    
    if( mPickedObjectLayer != -1 ) {
        char *tag = getSpriteRecord( 
            mCurrentObject.sprites[ mPickedObjectLayer ] )->tag;
        
        pos.x = 0;
        pos.y = -106;

        doublePair spritePos = mCurrentObject.spritePos[ mPickedObjectLayer ];
        
        char *posString = autoSprintf( "  ( %.0f, %.0f )",
                                       spritePos.x, spritePos.y );
        
        smallFont->drawString( tag, pos, alignRight );
        
        smallFont->drawString( posString, pos, alignLeft );
        
        delete [] posString;
        }
    


    doublePair legendPos = mImportEditorButton.getPosition();
    
    legendPos.x = -100;
    legendPos.y += 52;
    
    drawKeyLegend( &mKeyLegend, legendPos );


    legendPos = mImportEditorButton.getPosition();
    legendPos.x = -440;
    legendPos.y += 20;
    
    drawKeyLegend( &mKeyLegendB, legendPos );


    if( mPickedObjectLayer != -1 ) {
        legendPos = mSpritePicker.getPosition();
        legendPos.y -= 255;

        drawKeyLegend( &mKeyLegendC, legendPos, alignCenter );
        }
    
    legendPos = mObjectPicker.getPosition();
    legendPos.y -= 255;

    drawKeyLegend( &mKeyLegendD, legendPos, alignCenter );
        

    setDrawColor( 0, 0, 1, 0.50 );
            
    doublePair mouseCenter = { lastMouseX + 1, lastMouseY - 1 };
    
    drawRect( mouseCenter, 1000, 0.5 );
    drawRect( mouseCenter, 0.5, 1000 );
    


    if( isSpellCheckReady() ) {
        char *desc = mDescriptionField.getText();
        
        SimpleVector<char*> *words = tokenizeString( desc );
        
        if( mDescriptionField.isFocused() ) {
            int cursorPos = mDescriptionField.getCursorPosition();
            
            int descLen = strlen( desc );

            if( cursorPos > 0 &&
                cursorPos < descLen &&
                desc[ cursorPos ] == ' ' ) {
                cursorPos--;
                }
                
            int wordFirstChar = cursorPos;
            int wordLastChar = cursorPos;
            


            // walk back to space
            while( wordFirstChar - 1 >= 0 &&
                   desc[ wordFirstChar - 1 ] != ' ' ) {
                wordFirstChar--;
                }

            // walk forward to space
            while( wordLastChar + 1  < descLen &&
                   desc[ wordLastChar + 1 ] != ' ' ) {
                wordLastChar++;
                }
            
            if( wordLastChar == descLen ) {
                wordLastChar --;
                }

            if( wordFirstChar < wordLastChar &&
                // show spelling error in cursor word if enough
                // time has passed (pause in typing)
                mStepsSinceDescriptionChange < 60 / frameRateFactor ) {

                SimpleVector<char> wordVec;
                
                for( int i=wordFirstChar; i<=wordLastChar; i++ ) {
                    wordVec.push_back( desc[i] );
                    }
                char *word = wordVec.getElementString();
                
                // remove word from list
                
                for( int i=0; i<words->size(); i++ ) {
                    
                    if( strcmp( words->getElementDirect( i ), word ) == 0 ) {
                        delete [] words->getElementDirect( i );
                        
                        words->deleteElement( i );
                        break;
                        }
                    }
                delete [] word;
                }
            }
        

        delete [] desc;
        
        SimpleVector<char*> badWords;
        
        for( int i=0; i<words->size(); i++ ) {
            if( ! checkWord( words->getElementDirect( i ) ) ) {
                badWords.push_back( 
                    stringDuplicate( words->getElementDirect( i ) ) );
                }
            }
        

        words->deallocateStringElements();
        delete words;
        
        
        

        if( badWords.size() > 0 ) {
            badWords.push_front( stringDuplicate( "Spelling: " ) );
            
            char **badArray = badWords.getElementArray();
            
            char *badString = join( badArray, badWords.size(), " " );
            
            delete [] badArray;
            
            doublePair pos = mDescriptionField.getPosition();
            
            pos.x -= 157;
            pos.y += 19;
            
            setDrawColor( 1, 0, 0, 1 );
            
            smallFont->drawString( badString, pos, alignLeft );
            delete [] badString;
            }
        

        badWords.deallocateStringElements();
        }





    doublePair zoomPos = { lastMouseX, lastMouseY };

    pos.x = -500;
    pos.y = -290;
    
    drawZoomView( zoomPos, 16, 4, pos );
    }



void EditorObjectPage::step() {
    
    mStepsSinceDescriptionChange ++;
    
    char creationSoundPresent = 
        ( mCreationSoundWidget.getSoundUsage().numSubSounds > 0 );
    
    mCreationSoundInitialOnlyCheckbox.setVisible( creationSoundPresent );
    
    if( ! creationSoundPresent ) {
        // un-toggle whenever sound cleared
        mCreationSoundInitialOnlyCheckbox.setToggled( false );
        }
    

    mHoverStrength -= 0.01 * frameRateFactor;
    
    if( mHoverStrength < 0 ) {
        mHoverStrength = 0;
        mHoverFlash = 1;
        }
    else {
        mHoverFlash = 
            0.375 * sin( 0.2 * frameRateFactor * mHoverFrameCount ) + 
            0.625;

        // flash gets less strong as hover fades
        mHoverFlash = mHoverFlash * mHoverStrength + 1 - mHoverStrength;
        
        mHoverFrameCount++;
        }
    
    if( mDemoVertRot ) {
        if( mCurrentObject.vertContainRotationOffset != 0 ) {
            mClearRotButton.setVisible( true );
            }
        else {
            mClearRotButton.setVisible( false );
            }
        }
    else if( mPickedObjectLayer != -1 ) {
        
        if( mCurrentObject.spriteRot[mPickedObjectLayer] != 0 ) {
            mClearRotButton.setVisible( true );
            }
        else {
            mClearRotButton.setVisible( false );
            }

        mFlipHButton.setVisible( true );
        }
    else {
        mClearRotButton.setVisible( false );
        mFlipHButton.setVisible( false );
        }

    if( mPersonAgeSlider.isVisible() || 
        mSlotSizeField.isVisible() ||
        anyClothingToggled() ||
        mCheckboxes[1]->getToggled() ||
        mFloorCheckbox.getToggled() ) {
        
        mUsingSoundWidget.setVisible( true );
        }
    else {
        mUsingSoundWidget.setSoundUsage( blankSoundUsage );
        mUsingSoundWidget.setVisible( false );
        }
    }




void EditorObjectPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    if( ! isSpellCheckReady() ) {
        initSpellCheck();
        }

    mStepsSinceDescriptionChange = 0;

    mSpritePicker.redoSearch( false );
    mObjectPicker.redoSearch( false );

    mRotAdjustMode = false;
    mIgnoreParentLinkMode = false;
    
    endVertRotDemo();
    }




void EditorObjectPage::clearUseOfSprite( int inSpriteID ) {
    int numUses = 0;
    
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        if( mCurrentObject.sprites[i] == inSpriteID ) {
            numUses ++;
            }
        }

    if( numUses == 0 ) {
        return;
        }
    
    if( mPickedObjectLayer != -1 ) {
        
        mPickedObjectLayer -= numUses;
        
        if( mPickedObjectLayer < 0 ) {
            mPickedObjectLayer = -1;
            }
        }
    

    int newNumSprites = mCurrentObject.numSprites - numUses;
    
    int *newSprites = new int[ newNumSprites ];


    doublePair *newSpritePos = new doublePair[ newNumSprites ];
    double *newSpriteRot = new double[ newNumSprites ];
    char *newSpriteHFlip = new char[ newNumSprites ];
    FloatRGB *newSpriteColor = new FloatRGB[ newNumSprites ];
    
    double *newSpriteAgeStart = new double[ newNumSprites ];
    double *newSpriteAgeEnd = new double[ newNumSprites ];
    int *newSpriteParent = new int[ newNumSprites ];
    char *newSpriteInvisibleWhenHolding = new char[ newNumSprites ];
    int *newSpriteInvisibleWhenWorn = new int[ newNumSprites ];
    char *newSpriteBehindSlots = new char[ newNumSprites ];

    char *newSpriteIsHead = new char[ newNumSprites ];
    char *newSpriteIsBody = new char[ newNumSprites ];
    char *newSpriteIsBackFoot = new char[ newNumSprites ];
    char *newSpriteIsFrontFoot = new char[ newNumSprites ];

    char *newSpriteUseVanish = new char[ newNumSprites ];
    char *newSpriteUseAppear = new char[ newNumSprites ];

    int j = 0;
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        
        if( mCurrentObject.sprites[i] != inSpriteID ) {
            // not one we're skipping
            newSprites[j] = mCurrentObject.sprites[i];
            newSpritePos[j] = mCurrentObject.spritePos[i];
            newSpriteRot[j] = mCurrentObject.spriteRot[i];
            newSpriteHFlip[j] = mCurrentObject.spriteHFlip[i];
            newSpriteColor[j] = mCurrentObject.spriteColor[i];
            newSpriteAgeStart[j] = mCurrentObject.spriteAgeStart[i];
            newSpriteAgeEnd[j] = mCurrentObject.spriteAgeEnd[i];
            newSpriteParent[j] = mCurrentObject.spriteParent[i];
            newSpriteInvisibleWhenHolding[j] = 
                mCurrentObject.spriteInvisibleWhenHolding[i];
            newSpriteInvisibleWhenWorn[j] = 
                mCurrentObject.spriteInvisibleWhenWorn[i];
            newSpriteBehindSlots[j] = 
                mCurrentObject.spriteBehindSlots[i];
            newSpriteIsHead[j] = mCurrentObject.spriteIsHead[i];
            newSpriteIsBody[j] = mCurrentObject.spriteIsBody[i];
            newSpriteIsBackFoot[j] = mCurrentObject.spriteIsBackFoot[i];
            newSpriteIsFrontFoot[j] = mCurrentObject.spriteIsFrontFoot[i];
            newSpriteUseVanish[j] = mCurrentObject.spriteUseVanish[i];
            newSpriteUseAppear[j] = mCurrentObject.spriteUseAppear[i];
            j++;
            }
        }
    
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;
    delete [] mCurrentObject.spriteColor;
    
    delete [] mCurrentObject.spriteAgeStart;
    delete [] mCurrentObject.spriteAgeEnd;
    delete [] mCurrentObject.spriteParent;
    delete [] mCurrentObject.spriteInvisibleWhenHolding;
    delete [] mCurrentObject.spriteInvisibleWhenWorn;
    delete [] mCurrentObject.spriteBehindSlots;

    delete [] mCurrentObject.spriteIsHead;
    delete [] mCurrentObject.spriteIsBody;
    delete [] mCurrentObject.spriteIsBackFoot;
    delete [] mCurrentObject.spriteIsFrontFoot;
    
    delete [] mCurrentObject.spriteUseVanish;
    delete [] mCurrentObject.spriteUseAppear;

    mCurrentObject.sprites = newSprites;
    mCurrentObject.spritePos = newSpritePos;
    mCurrentObject.spriteRot = newSpriteRot;
    mCurrentObject.spriteHFlip = newSpriteHFlip;
    mCurrentObject.spriteColor = newSpriteColor;
    mCurrentObject.spriteAgeStart = newSpriteAgeStart;
    mCurrentObject.spriteAgeEnd = newSpriteAgeEnd;
    mCurrentObject.spriteParent = newSpriteParent;
    mCurrentObject.spriteInvisibleWhenHolding = newSpriteInvisibleWhenHolding;
    mCurrentObject.spriteInvisibleWhenWorn = newSpriteInvisibleWhenWorn;
    mCurrentObject.spriteBehindSlots = newSpriteBehindSlots;

    mCurrentObject.spriteIsHead = newSpriteIsHead;
    mCurrentObject.spriteIsBody = newSpriteIsBody;
    mCurrentObject.spriteIsBackFoot = newSpriteIsBackFoot;
    mCurrentObject.spriteIsFrontFoot = newSpriteIsFrontFoot;

    mCurrentObject.spriteUseVanish = newSpriteUseVanish;
    mCurrentObject.spriteUseAppear = newSpriteUseAppear;
    
    
    mCurrentObject.numSprites = newNumSprites;


    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        if( mCurrentObject.spriteParent[i] >= newNumSprites ) {
            mCurrentObject.spriteParent[i] = -1;
            }
        }


    for( int i=0; i<mCurrentObject.numSlots; i++ ) {
        if( mCurrentObject.slotParent[i] >= newNumSprites ) {
            mCurrentObject.slotParent[i] = -1;
            }
        }
    }




double EditorObjectPage::getClosestSpriteOrSlot( float inX, float inY,
                                                 int *outSprite,
                                                 int *outSlot ) {

    doublePair pos = { inX, inY };
    
    pos = sub( pos, mObjectCenterOnScreen );

    int oldLayerPick = mPickedObjectLayer;
    int oldSlotPick = mPickedSlot;
    
    double age = 20;
    
    if( mPersonAgeSlider.isVisible() && mCheckboxes[2]->getToggled() ) {
        
        mCurrentObject.person = true;
        
        age = mPersonAgeSlider.getValue();
        }
    else {
        mCurrentObject.person = false;
        }
    
    int cl;
    
    double smallestDist = getClosestObjectPart( &mCurrentObject,
                                                NULL,
                                                NULL,
                                                NULL,
                                                false,
                                                age,
                                                mPickedObjectLayer,
                                                false,
                                                pos.x, pos.y,
                                                outSprite, &cl, outSlot );
        


    if( *outSprite == -1 && smallestDist > 32 ) {
        // too far to count as a new pick

        if( smallestDist < 52 ) {
            // drag old pick around
            *outSprite = oldLayerPick;
            *outSlot = oldSlotPick;
            }
        else {
            // far enough away to clear the pick completely
            // and allow whole-object dragging
            *outSprite = -1;
            *outSlot = -1;
            }
        }
    
    return smallestDist;
    }



void EditorObjectPage::pickedLayerChanged() {
    if( mPickedObjectLayer == -1 ) {
        mHueSlider.setVisible( false );
        mSaturationSlider.setVisible( false );
        mValueSlider.setVisible( false );
        
        if( mLeftBlockingRadiusField.isVisible() ) {
            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( false );
            }
        else {
            mSetHeldPosButton.setVisible( true );
            mMinPickupAgeField.setVisible( true );
            }
        
        if( mCheckboxes[1]->getToggled() ) {
            mSetHeldPosButton.setVisible( false );
            }
        
        mUseVanishCheckbox.setVisible( false );
        mUseAppearCheckbox.setVisible( false );
        
        mRot45ForwardButton.setVisible( false );
        mRot45BackwardButton.setVisible( false );

        if( mPickedSlot != -1 ) {
            mSlotVertCheckbox.setToggled( 
                mCurrentObject.slotVert[ mPickedSlot ] );
            mSlotVertCheckbox.setVisible( true );
            }
        else {
            mSlotVertCheckbox.setVisible( false );
            }

        char *des = mDescriptionField.getText();
        
        mBakeButton.setVisible( false );

        if( strcmp( des, "" ) != 0 ) {
            char allLayersOpaque = true;
            
            for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                if( getUsesMultiplicativeBlending( 
                        mCurrentObject.sprites[i] ) ) {
                    
                    allLayersOpaque = false;
                    break;
                    }
                }
            if( allLayersOpaque ) {
                mBakeButton.setVisible( true );
                }
            }
        delete [] des;
        }
    else {
        mSlotVertCheckbox.setVisible( false );
        
        if( mCheckboxes[2]->getToggled() ||
            mLeftBlockingRadiusField.isVisible() ) {
            // person, and a layer selected, disable held demo button
            // because of overlap
            // or if blocking radius visible
            mSetHeldPosButton.setVisible( false );
            mMinPickupAgeField.setVisible( false );
            }
        else {
            mSetHeldPosButton.setVisible( true );
            mMinPickupAgeField.setVisible( true );
            }
        
        if( mCheckboxes[1]->getToggled() ) {
            mSetHeldPosButton.setVisible( false );
            }

        mRot45ForwardButton.setVisible( true );
        mRot45BackwardButton.setVisible( true );
        
        mBakeButton.setVisible( false );

        if( getUsesMultiplicativeBlending( 
                mCurrentObject.sprites[ mPickedObjectLayer ] ) ) {
            
            // colors don't work for multiplicative blending sprites
            mHueSlider.setVisible( false );
            mSaturationSlider.setVisible( false );
            mValueSlider.setVisible( false );
            }
        else {
            
            mHueSlider.setVisible( true );
            mSaturationSlider.setVisible( true );
            mValueSlider.setVisible( true );
            
            
            FloatRGB rgb = mCurrentObject.spriteColor[ mPickedObjectLayer ];
            
            Color c( rgb.r, rgb.g, rgb.b );
            
            float h, s, v;
            c.makeHSV( &h, &s, &v );
            
            mHueSlider.setValue( h );
            mSaturationSlider.setValue( s );
            mValueSlider.setValue( v );
            
            updateSliderColors();
            }
        }
    updateAgingPanel();
    }





void EditorObjectPage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( mSetHeldPos ) {
        return;
        }

    if( mRotAdjustMode && mPickedObjectLayer != -1 ) {
        // mouse move adjusts rotation
        double oldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];
        
        mCurrentObject.spriteRot[ mPickedObjectLayer ] =
            ( inX - mRotStartMouseX ) / 200 + mLayerOldRot;

        recursiveRotate( &mCurrentObject,
                         mPickedObjectLayer,
                         mCurrentObject.spritePos[ mPickedObjectLayer ],
                         mCurrentObject.spriteRot[ mPickedObjectLayer ] - 
                         oldRot );
        }
    else if( mRotAdjustMode && mDemoVertRot ) {
        mCurrentObject.vertContainRotationOffset =
            ( inX - mRotStartMouseX ) / 200 + mLayerOldRot;
        
        // this will update reset button vis
        showVertRotButtons();
        }
    else {    
        mRotStartMouseX = inX;
        getClosestSpriteOrSlot( inX, inY, &mHoverObjectLayer, &mHoverSlot );
        mHoverStrength = 1;
        }
    }




void EditorObjectPage::pointerDown( float inX, float inY ) {
    mHoverStrength = 0;
    
    if( inX < -192 || inX > 192 || 
        inY < -96 || inY > 192 ) {
        return;
        }
    
    if( mDemoVertRot ) {
        endVertRotDemo();
        }
    
    if( isLastMouseButtonRight() && mPickedObjectLayer != -1 ) {
        // pick new parent for this layer

        int obj, slot;
        
        double smallestDist = 
            getClosestSpriteOrSlot( inX, inY, &obj, &slot );

        if( smallestDist < 200 && obj != -1 && obj != mPickedObjectLayer ) {
            // parent picked
            
            // make sure this isn't one of our children
            // walk up the chain, looking for us, or looking for the end
            int nextParent = obj;
            while( nextParent != -1 && nextParent != mPickedObjectLayer ) {
                
                nextParent = mCurrentObject.spriteParent[nextParent];
                }
            
            if( nextParent != mPickedObjectLayer ) {
                mCurrentObject.spriteParent[ mPickedObjectLayer ] = obj;
                }
            // else creating a loop, block it
            }
        else {
            // parent cleared
            mCurrentObject.spriteParent[ mPickedObjectLayer ] = -1;
            }
        }
    

    if( isLastMouseButtonRight() && mPickedSlot != -1 ) {
        // pick new parent for this slot

        int obj, slot;
        
        double smallestDist = 
            getClosestSpriteOrSlot( inX, inY, &obj, &slot );

        if( smallestDist < 200 && obj != -1 && slot == -1 ) {
            // parent picked
            
            mCurrentObject.slotParent[ mPickedSlot ] = obj;
            }
        else {
            // parent cleared
            mCurrentObject.slotParent[ mPickedSlot ] = -1;
            }
        }
    
    

    mDragging = true;
    
    doublePair pos = { inX, inY };


    if( mSetHeldPos ) {    
        mSetHeldMouseStart = pos;
        mSetHeldOffsetStart = mCurrentObject.heldOffset;
        return;
        }
    else if( mSetClothesPos ) {    
        mSetClothingMouseStart = pos;
        mSetClothingOffsetStart = mCurrentObject.clothingOffset;
        return;
        }
    
    
    getClosestSpriteOrSlot( inX, inY, &mPickedObjectLayer, &mPickedSlot );
    
    pickedLayerChanged();
    
    if( mPickedObjectLayer != -1 || mPickedSlot != -1 ) {
        TextField::unfocusAll();
        
        if( mPickedObjectLayer != -1 ) {
            mPickedMouseOffset = 
                sub( pos, mCurrentObject.spritePos[mPickedObjectLayer] );
            }
        else {
            mPickedMouseOffset = 
                sub( pos, mCurrentObject.slotPos[mPickedSlot] );
            }
        }
    else {
        TextField::unfocusAll();
        // dragging whole object?
        doublePair center = {0,0};
        
        mPickedMouseOffset = sub( pos, center );
        }
    }



void EditorObjectPage::recursiveMove( ObjectRecord *inObject,
                                      int inMovingParentIndex,
                                      doublePair inMoveDelta ) {
    if( mIgnoreParentLinkMode ) {
        return;
        }
    
    // adjust children recursively
    for( int i=0; i<inObject->numSprites; i++ ) {
        if( inObject->spriteParent[i] == inMovingParentIndex ) {
            
            inObject->spritePos[i] =
                add( inObject->spritePos[i], inMoveDelta );
            
            recursiveMove( inObject, i, inMoveDelta );
            }
        }

    for( int i=0; i<inObject->numSlots; i++ ) {
        if( inObject->slotParent[i] == inMovingParentIndex ) {
            
            inObject->slotPos[i] =
                add( inObject->slotPos[i], inMoveDelta );
            }
        }
    }




void EditorObjectPage::pointerDrag( float inX, float inY ) {

    lastMouseX = inX;
    lastMouseY = inY;

    mHoverStrength = 0;
    
    if( ! mDragging ) {
        return;
        }

    if( inX < -192 || inX > 192 || 
        inY < -96 || inY > 192 ) {
        return;
        }
    
    if( mSetHeldPos ) {    
        doublePair cur = { inX, inY };
        
        doublePair diff = sub( cur, mSetHeldMouseStart );
        
        mCurrentObject.heldOffset = add( mSetHeldOffsetStart, diff );
        return;
        }
    else if( mSetClothesPos ) {    
        doublePair cur = { inX, inY };
        
        doublePair diff = sub( cur, mSetClothingMouseStart );
        
        mCurrentObject.clothingOffset = add( mSetClothingOffsetStart, diff );
        return;
        }


    
    doublePair pos = {inX, inY};

    if( mPickedObjectLayer != -1 ) {
        doublePair oldPos = mCurrentObject.spritePos[mPickedObjectLayer];
        
        mCurrentObject.spritePos[mPickedObjectLayer]
            = sub( pos, mPickedMouseOffset );

        doublePair delta = sub( mCurrentObject.spritePos[mPickedObjectLayer],
                                oldPos );
        
        recursiveMove( &mCurrentObject, mPickedObjectLayer, delta );
        }
    else if( mPickedSlot != -1 ) {
        mCurrentObject.slotPos[mPickedSlot]
            = sub( pos, mPickedMouseOffset );
        }
    else {
        doublePair center = {0,0};
        
        double centerDist = distance( center, pos );
        
        if( centerDist < 300 ) {
            for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                mCurrentObject.spritePos[i] =
                    add( mCurrentObject.spritePos[i],
                         sub( pos, mPickedMouseOffset ) );
                }
            for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                mCurrentObject.slotPos[i] =
                    add( mCurrentObject.slotPos[i],
                         sub( pos, mPickedMouseOffset ) );
                }
            mPickedMouseOffset = pos;
            }
        }
    }



void EditorObjectPage::pointerUp( float inX, float inY ) {
    mDragging = false;
    }




// makes a new array with element missing
// does not delete old array
static char *deleteFromCharArray( char *inArray, int inOldLength,
                                  int inIndexToRemove ) {
    
    int newLength = inOldLength - 1;

    char *newArray = new char[ newLength ];
        
    memcpy( newArray, inArray, inIndexToRemove * sizeof( char ) );
    
    memcpy( &( newArray[inIndexToRemove] ), 
            &( inArray[inIndexToRemove + 1] ), 
            (newLength - inIndexToRemove ) * sizeof( char ) );
    
    return newArray;
    }

static int *deleteFromIntArray( int *inArray, int inOldLength,
                                int inIndexToRemove ) {
    
    int newLength = inOldLength - 1;

    int *newArray = new int[ newLength ];
        
    memcpy( newArray, inArray, inIndexToRemove * sizeof( int ) );
    
    memcpy( &( newArray[inIndexToRemove] ), 
            &( inArray[inIndexToRemove + 1] ), 
            (newLength - inIndexToRemove ) * sizeof( int ) );
    
    return newArray;
    }



void EditorObjectPage::keyDown( unsigned char inASCII ) {
    
    if( TextField::isAnyFocused() || mSetHeldPos ) {
        return;
        }
    
    if( inASCII == 'p' ) {
        mIgnoreParentLinkMode = true;
        }
    
    if( mPickedObjectLayer != -1 && inASCII == 'r' ) {
        mRotAdjustMode = true;
        mLayerOldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];
        }
    else if( mDemoVertRot && inASCII == 'r' ) {
        mRotAdjustMode = true;
        mLayerOldRot = mCurrentObject.vertContainRotationOffset;
        mRotStartMouseX = lastMouseX;
        }
    
    if( mPickedObjectLayer != -1 && inASCII == 'c' ) {
        if( ! getUsesMultiplicativeBlending( 
                mCurrentObject.sprites[ mPickedObjectLayer ] ) ) {
            
            mColorClipboard = mCurrentObject.spriteColor[ mPickedObjectLayer ];
            }
        }
    if( mPickedObjectLayer != -1 && inASCII == 'v' ) {
        if( ! getUsesMultiplicativeBlending( 
                mCurrentObject.sprites[ mPickedObjectLayer ] ) ) {
            
            mCurrentObject.spriteColor[ mPickedObjectLayer ] = mColorClipboard;
            pickedLayerChanged();
            }
        }
    if( mPickedObjectLayer != -1 && inASCII == 'C' ) {
        ObjectRecord *saved = getObject( mCurrentObject.id );
        
        if( saved != NULL && saved->numSprites > mPickedObjectLayer ) {
            mSaveDeltaPosClipboard = 
                sub( mCurrentObject.spritePos[ mPickedObjectLayer ],
                     saved->spritePos[ mPickedObjectLayer ] );
            
            mSaveDeltaRotClipboard = 
                mCurrentObject.spriteRot[ mPickedObjectLayer ] -
                saved->spriteRot[ mPickedObjectLayer ];
            }
        }
    if( mPickedObjectLayer != -1 && inASCII == 'V' ) {
        mCurrentObject.spritePos[ mPickedObjectLayer ] = 
            add( mCurrentObject.spritePos[ mPickedObjectLayer ],
                 mSaveDeltaPosClipboard );
        mCurrentObject.spriteRot[ mPickedObjectLayer ] += 
            mSaveDeltaRotClipboard;
        pickedLayerChanged();
        }
    
    if( mPickedObjectLayer != -1 && inASCII == 'd' ) {
        // duplicate layer
        
        int layerToDupe = mPickedObjectLayer;
        int idToDupe = mCurrentObject.sprites[ mPickedObjectLayer ];
        
        addNewSprite( idToDupe );
        
        mPickedObjectLayer = mCurrentObject.numSprites - 1;

        doublePair offset = { 15, 15 };
        
        mCurrentObject.spritePos[mPickedObjectLayer] =
            add( mCurrentObject.spritePos[layerToDupe], offset );

        mCurrentObject.spriteRot[mPickedObjectLayer] =
            mCurrentObject.spriteRot[layerToDupe];
        
        mCurrentObject.spriteHFlip[mPickedObjectLayer] =
            mCurrentObject.spriteHFlip[layerToDupe];

        mCurrentObject.spriteColor[mPickedObjectLayer] =
            mCurrentObject.spriteColor[layerToDupe];

        mCurrentObject.spriteAgeStart[mPickedObjectLayer] =
            mCurrentObject.spriteAgeStart[layerToDupe];

        mCurrentObject.spriteAgeEnd[mPickedObjectLayer] =
            mCurrentObject.spriteAgeEnd[layerToDupe];

        mCurrentObject.spriteParent[mPickedObjectLayer] =
            mCurrentObject.spriteParent[layerToDupe];

        
        mCurrentObject.spriteInvisibleWhenHolding[mPickedObjectLayer] =
            mCurrentObject.spriteInvisibleWhenHolding[layerToDupe];

        mCurrentObject.spriteInvisibleWhenWorn[mPickedObjectLayer] =
            mCurrentObject.spriteInvisibleWhenWorn[layerToDupe];

        mCurrentObject.spriteBehindSlots[mPickedObjectLayer] =
            mCurrentObject.spriteBehindSlots[layerToDupe];

        mCurrentObject.spriteUseVanish[mPickedObjectLayer] =
            mCurrentObject.spriteUseVanish[layerToDupe];

        mCurrentObject.spriteUseAppear[mPickedObjectLayer] =
            mCurrentObject.spriteUseAppear[layerToDupe];

        // don't dupe body part status
        }
    if( mPickedObjectLayer != -1 && inASCII == 8 ) {
        // backspace
        
        int newNumSprites = mCurrentObject.numSprites - 1;
            
        int *newSprites = new int[ newNumSprites ];
        

        if( newNumSprites != mPickedObjectLayer ) {
            
            for( int i=mPickedObjectLayer; i<=newNumSprites; i++ ) {
                
                LayerSwapRecord swap = { i,
                                         i + 1 };
                mObjectLayerSwaps.push_back( swap );
                }
            }
            

        memcpy( newSprites, mCurrentObject.sprites, 
                mPickedObjectLayer * sizeof( int ) );
        
        memcpy( &( newSprites[mPickedObjectLayer] ), 
                &( mCurrentObject.sprites[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( int ) );

        doublePair *newSpritePos = new doublePair[ newNumSprites ];
        
        memcpy( newSpritePos, mCurrentObject.spritePos, 
                mPickedObjectLayer * sizeof( doublePair ) );
        
        memcpy( &( newSpritePos[mPickedObjectLayer] ), 
                &( mCurrentObject.spritePos[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( doublePair ) );


        double *newSpriteRot = new double[ newNumSprites ];
        
        memcpy( newSpriteRot, mCurrentObject.spriteRot, 
                mPickedObjectLayer * sizeof( double ) );
        
        memcpy( &( newSpriteRot[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteRot[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( double ) );


        char *newSpriteHFlip = new char[ newNumSprites ];
        
        memcpy( newSpriteHFlip, mCurrentObject.spriteHFlip, 
                mPickedObjectLayer * sizeof( char ) );
        
        memcpy( &( newSpriteHFlip[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteHFlip[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( char ) );


        FloatRGB *newSpriteColor = new FloatRGB[ newNumSprites ];
        
        memcpy( newSpriteColor, mCurrentObject.spriteColor, 
                mPickedObjectLayer * sizeof( FloatRGB ) );
        
        memcpy( &( newSpriteColor[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteColor[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( FloatRGB ) );



        double *newSpriteAgeStart = new double[ newNumSprites ];
        
        memcpy( newSpriteAgeStart, mCurrentObject.spriteAgeStart, 
                mPickedObjectLayer * sizeof( double ) );
        
        memcpy( &( newSpriteAgeStart[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteAgeStart[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( double ) );


        double *newSpriteAgeEnd = new double[ newNumSprites ];
        
        memcpy( newSpriteAgeEnd, mCurrentObject.spriteAgeEnd, 
                mPickedObjectLayer * sizeof( double ) );
        
        memcpy( &( newSpriteAgeEnd[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteAgeEnd[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( double ) );
        

        int *newSpriteParent = new int[ newNumSprites ];
        
        memcpy( newSpriteParent, mCurrentObject.spriteParent, 
                mPickedObjectLayer * sizeof( int ) );
        
        memcpy( &( newSpriteParent[mPickedObjectLayer] ), 
                &( mCurrentObject.spriteParent[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( int ) );



        char *newSpriteInvisibleWhenHolding = 
            deleteFromCharArray( mCurrentObject.spriteInvisibleWhenHolding, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );

        int *newSpriteInvisibleWhenWorn = 
            deleteFromIntArray( mCurrentObject.spriteInvisibleWhenWorn, 
                                mCurrentObject.numSprites,
                                mPickedObjectLayer );

        char *newSpriteBehindSlots = 
            deleteFromCharArray( mCurrentObject.spriteBehindSlots, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );
        
        char *newSpriteIsHead = 
            deleteFromCharArray( mCurrentObject.spriteIsHead, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );

        char *newSpriteIsBody = 
            deleteFromCharArray( mCurrentObject.spriteIsBody, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );

        char *newSpriteIsBackFoot = 
            deleteFromCharArray( mCurrentObject.spriteIsBackFoot, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );

        char *newSpriteIsFrontFoot = 
            deleteFromCharArray( mCurrentObject.spriteIsFrontFoot, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );


        char *newSpriteUseVanish = 
            deleteFromCharArray( mCurrentObject.spriteUseVanish, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );
        char *newSpriteUseAppear = 
            deleteFromCharArray( mCurrentObject.spriteUseAppear, 
                                 mCurrentObject.numSprites,
                                 mPickedObjectLayer );


        delete [] mCurrentObject.sprites;
        delete [] mCurrentObject.spritePos;
        delete [] mCurrentObject.spriteRot;
        delete [] mCurrentObject.spriteHFlip;
        delete [] mCurrentObject.spriteColor;
        delete [] mCurrentObject.spriteAgeStart;
        delete [] mCurrentObject.spriteAgeEnd;
        delete [] mCurrentObject.spriteParent;
        delete [] mCurrentObject.spriteInvisibleWhenHolding;
        delete [] mCurrentObject.spriteInvisibleWhenWorn;
        delete [] mCurrentObject.spriteBehindSlots;
        delete [] mCurrentObject.spriteIsHead;
        delete [] mCurrentObject.spriteIsBody;
        delete [] mCurrentObject.spriteIsBackFoot;
        delete [] mCurrentObject.spriteIsFrontFoot;

        delete [] mCurrentObject.spriteUseVanish;
        delete [] mCurrentObject.spriteUseAppear;
            
        mCurrentObject.sprites = newSprites;
        mCurrentObject.spritePos = newSpritePos;
        mCurrentObject.spriteRot = newSpriteRot;
        mCurrentObject.spriteHFlip = newSpriteHFlip;
        mCurrentObject.spriteColor = newSpriteColor;
        mCurrentObject.spriteAgeStart = newSpriteAgeStart;
        mCurrentObject.spriteAgeEnd = newSpriteAgeEnd;
        mCurrentObject.spriteParent = newSpriteParent;
        mCurrentObject.spriteInvisibleWhenHolding = 
            newSpriteInvisibleWhenHolding;
        mCurrentObject.spriteInvisibleWhenWorn = 
            newSpriteInvisibleWhenWorn;
        mCurrentObject.spriteBehindSlots = 
            newSpriteBehindSlots;
        
        mCurrentObject.spriteIsHead = newSpriteIsHead;
        mCurrentObject.spriteIsBody = newSpriteIsBody;
        mCurrentObject.spriteIsBackFoot = newSpriteIsBackFoot;
        mCurrentObject.spriteIsFrontFoot = newSpriteIsFrontFoot;

        mCurrentObject.spriteUseVanish = newSpriteUseVanish;
        mCurrentObject.spriteUseAppear = newSpriteUseAppear;


        mCurrentObject.numSprites = newNumSprites;
        

        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
            if( mCurrentObject.spriteParent[i] == mPickedObjectLayer ) {
                // parent is gone
                mCurrentObject.spriteParent[i] = -1;
                }
            else if( mCurrentObject.spriteParent[i] > mPickedObjectLayer ) {
                // all parents above move down one index
                mCurrentObject.spriteParent[i] --;
                }
            }
        
        for( int i=0; i<mCurrentObject.numSlots; i++ ) {
            if( mCurrentObject.slotParent[i] >= newNumSprites ) {
                mCurrentObject.slotParent[i] = -1;
                }
            }


        mPickedObjectLayer = -1;
        pickedLayerChanged();

        if( newNumSprites == 0 ) {
            mSaveObjectButton.setVisible( false );
            mReplaceObjectButton.setVisible( false );
            }
        }
    else if( inASCII == 'm' ) {
        if( mPickedObjectLayer == -1 && mPickedSlot == -1 ) {
            mPickedObjectLayer = 0; 
            }
        else if( mPickedObjectLayer != -1 ) {
            mPickedObjectLayer ++;
            
            if( mPickedObjectLayer >= mCurrentObject.numSprites ) {
                mPickedObjectLayer = -1;

                if( mCurrentObject.numSlots > 0 ) {
                    
                    mPickedSlot = 0;
                    }
                }
            }
        else if( mPickedSlot != -1 ) {
            mPickedSlot++;
            
            if( mPickedSlot >= mCurrentObject.numSlots ) {
                mPickedSlot = -1;
                }
            }        
        
        pickedLayerChanged();

        mHoverObjectLayer = mPickedObjectLayer;
        mHoverSlot = mPickedSlot;
        mHoverStrength = 1;
        }
    else if( inASCII == 'n' ) {
        if( mPickedObjectLayer == -1 && mPickedSlot == -1 ) {
            if( mCurrentObject.numSlots > 0 ) {
                mPickedSlot = mCurrentObject.numSlots - 1;
                }
            else if( mCurrentObject.numSprites > 0 ) {
                mPickedObjectLayer = mCurrentObject.numSprites - 1;
                }
            }
        else if( mPickedObjectLayer != -1 ) {
            mPickedObjectLayer --;
            
            if( mPickedObjectLayer < 0 ) {
                mPickedObjectLayer = -1;
                }
            }
        else if( mPickedSlot != -1 ) {
            mPickedSlot--;
            
            if( mPickedSlot < 0 ) {
                mPickedSlot = -1;
                if( mCurrentObject.numSprites > 0 ) {
                    mPickedObjectLayer = mCurrentObject.numSprites - 1;
                    }
                }
            }        

        pickedLayerChanged();
        
        mHoverObjectLayer = mPickedObjectLayer;
        mHoverSlot = mPickedSlot;
        mHoverStrength = 1;
        }
    else if( inASCII == 0x10 ||
             ( inASCII == 'p' && isCommandKeyDown() ) ) {
        printf( "Print requested\n" );
        
        if( isPrintingSupported() ) {
            mPrintRequested = true;
            mSavePrintOnly = false;
            }
        else {
            printf( "Printing not supported\n" );
            }
        }
    else if( inASCII == 0x13 ||
             ( inASCII == 's' && isCommandKeyDown() ) ) {
        printf( "Save image requested\n" );
        mPrintRequested = true;
        mSavePrintOnly = true;
        }
    }


void EditorObjectPage::keyUp( unsigned char inASCII ) {
    
    if( mRotAdjustMode && inASCII == 'r' ) {
        mRotAdjustMode = false;
        }

    if( mIgnoreParentLinkMode && inASCII == 'p' ) {
        mIgnoreParentLinkMode = false;
        }
    
    }



void EditorObjectPage::specialKeyDown( int inKeyCode ) {
    
    if( mDescriptionField.isAnyFocused() ) {
        return;
        }


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
    

    if( mSetHeldPos ) {
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.heldOffset.x -= offset;
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.heldOffset.x += offset;
                break;
            case MG_KEY_DOWN:
                mCurrentObject.heldOffset.y -= offset;
                break;
            case MG_KEY_UP:
                mCurrentObject.heldOffset.y += offset;
                break;
            }
        return;
        }
    else if( mSetClothesPos ) {    
        
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.clothingOffset.x -= offset;
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.clothingOffset.x += offset;
                break;
            case MG_KEY_DOWN:
                mCurrentObject.clothingOffset.y -= offset;
                break;
            case MG_KEY_UP:
                mCurrentObject.clothingOffset.y += offset;
                break;
            }
        return;
        }


    if( mPickedObjectLayer == -1 && mPickedSlot == -1 ) {

        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
            switch( inKeyCode ) {
                case MG_KEY_LEFT:
                    mCurrentObject.spritePos[i].x -= offset;
                    break;
                case MG_KEY_RIGHT:
                    mCurrentObject.spritePos[i].x += offset;
                    break;
                case MG_KEY_DOWN:
                    mCurrentObject.spritePos[i].y -= offset;
                    break;
                case MG_KEY_UP:
                    mCurrentObject.spritePos[i].y += offset;
                    break;
                }
            }
        for( int i=0; i<mCurrentObject.numSlots; i++ ) {
            switch( inKeyCode ) {
                case MG_KEY_LEFT:
                    mCurrentObject.slotPos[i].x -= offset;
                    break;
                case MG_KEY_RIGHT:
                    mCurrentObject.slotPos[i].x += offset;
                    break;
                case MG_KEY_DOWN:
                    mCurrentObject.slotPos[i].y -= offset;
                    break;
                case MG_KEY_UP:
                    mCurrentObject.slotPos[i].y += offset;
                    break;
                }
            }
        return;
        }
    
    if( mPickedObjectLayer != -1 ) {
        
        doublePair delta = { 0, 0 };
        
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.spritePos[mPickedObjectLayer].x -= offset;
                delta.x = -offset;
                recursiveMove( &mCurrentObject, mPickedObjectLayer, delta );
                if( isCommandKeyDown() ) {
                    // only show highlighting for big jumps
                    mHoverStrength = 1;
                    mHoverObjectLayer = mPickedObjectLayer;
                    }
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.spritePos[mPickedObjectLayer].x += offset;
                delta.x = +offset;
                recursiveMove( &mCurrentObject, mPickedObjectLayer, delta );
                if( isCommandKeyDown() ) {
                    // only show highlighting for big jumps
                    mHoverStrength = 1;
                    mHoverObjectLayer = mPickedObjectLayer;
                    }
                break;
            case MG_KEY_DOWN:
                mCurrentObject.spritePos[mPickedObjectLayer].y -= offset;
                delta.y = -offset;
                recursiveMove( &mCurrentObject, mPickedObjectLayer, delta );
                if( isCommandKeyDown() ) {
                    // only show highlighting for big jumps
                    mHoverStrength = 1;
                    mHoverObjectLayer = mPickedObjectLayer;
                    }
                break;
            case MG_KEY_UP:
                mCurrentObject.spritePos[mPickedObjectLayer].y += offset;
                delta.y = +offset;
                recursiveMove( &mCurrentObject, mPickedObjectLayer, delta );
                if( isCommandKeyDown() ) {
                    // only show highlighting for big jumps
                    mHoverStrength = 1;
                    mHoverObjectLayer = mPickedObjectLayer;
                    }
                break;
            case MG_KEY_PAGE_UP:  {
                for( int o=0; o<offset; o++ ) {
                    
                    int layerOffset = 1;
                    if( mPickedObjectLayer + 1 
                        >= mCurrentObject.numSprites ) {
                    
                        layerOffset = 
                            mCurrentObject.numSprites - 1 - mPickedObjectLayer;
                        }
                    if( mPickedObjectLayer < 
                        mCurrentObject.numSprites - layerOffset ) {
                

                        // two indices being swapped
                        int indexA = mPickedObjectLayer;
                        int indexB = mPickedObjectLayer + layerOffset;
                        
                        LayerSwapRecord swap = { indexA, indexB };
                        
                        mObjectLayerSwaps.push_back( swap );
                        

                        int tempSprite = 
                            mCurrentObject.sprites[mPickedObjectLayer + 
                                                   layerOffset];
                        doublePair tempPos = 
                            mCurrentObject.spritePos[mPickedObjectLayer + 
                                                     layerOffset];
                        double tempRot = 
                            mCurrentObject.spriteRot[mPickedObjectLayer + 
                                                     layerOffset];
                        char tempHFlip = 
                            mCurrentObject.spriteHFlip[mPickedObjectLayer + 
                                                       layerOffset];
                
                        FloatRGB tempColor = 
                            mCurrentObject.spriteColor[mPickedObjectLayer + 
                                                       layerOffset];

                        double tempAgeStart = 
                            mCurrentObject.spriteAgeStart[mPickedObjectLayer + 
                                                          layerOffset];
                        double tempAgeEnd = 
                            mCurrentObject.spriteAgeEnd[mPickedObjectLayer + 
                                                        layerOffset];
                        int tempParent = 
                            mCurrentObject.spriteParent[mPickedObjectLayer + 
                                                        layerOffset];
                    
                        char tempInvisibleWhenHolding = 
                            mCurrentObject.spriteInvisibleWhenHolding[
                                mPickedObjectLayer + 
                                layerOffset];

                        int tempInvisibleWhenWorn = 
                            mCurrentObject.spriteInvisibleWhenWorn[
                                mPickedObjectLayer + 
                                layerOffset];
                        char tempBehindSlots = 
                            mCurrentObject.spriteBehindSlots[
                                mPickedObjectLayer + 
                                layerOffset];

                        char tempIsHead = 
                            mCurrentObject.spriteIsHead[
                                mPickedObjectLayer + 
                                layerOffset];
                        char tempIsBody = 
                            mCurrentObject.spriteIsBody[
                                mPickedObjectLayer + 
                                layerOffset];
                        char tempIsBackFoot = 
                            mCurrentObject.spriteIsBackFoot[
                                mPickedObjectLayer + 
                                layerOffset];
                        char tempIsFrontFoot = 
                            mCurrentObject.spriteIsFrontFoot[
                                mPickedObjectLayer + 
                                layerOffset];

                        char tempUseVanish = 
                            mCurrentObject.spriteUseVanish[
                                mPickedObjectLayer + 
                                layerOffset];
                        char tempUseAppear = 
                            mCurrentObject.spriteUseAppear[
                                mPickedObjectLayer + 
                                layerOffset];


                        mCurrentObject.sprites[mPickedObjectLayer + 
                                               layerOffset]
                            = mCurrentObject.sprites[mPickedObjectLayer];

                        mCurrentObject.sprites[mPickedObjectLayer] = 
                            tempSprite;
                
                        mCurrentObject.spritePos[mPickedObjectLayer + 
                                                 layerOffset]
                            = mCurrentObject.spritePos[mPickedObjectLayer];

                        mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;

                        mCurrentObject.spriteRot[mPickedObjectLayer + 
                                                 layerOffset]
                            = mCurrentObject.spriteRot[mPickedObjectLayer];

                        mCurrentObject.spriteRot[mPickedObjectLayer] = tempRot;

                        mCurrentObject.spriteHFlip[mPickedObjectLayer 
                                                   + layerOffset]
                            = mCurrentObject.spriteHFlip[mPickedObjectLayer];
                        
                        mCurrentObject.spriteHFlip[mPickedObjectLayer] = 
                            tempHFlip;

                        mCurrentObject.spriteColor[mPickedObjectLayer 
                                                   + layerOffset]
                            = mCurrentObject.spriteColor[mPickedObjectLayer];
                        
                        mCurrentObject.spriteColor[mPickedObjectLayer] = 
                            tempColor;


                        mCurrentObject.spriteAgeStart[mPickedObjectLayer 
                                                      + layerOffset]
                            = mCurrentObject.spriteAgeStart[
                                mPickedObjectLayer];
                        
                        mCurrentObject.spriteAgeStart[mPickedObjectLayer] = 
                            tempAgeStart;

                        mCurrentObject.spriteAgeEnd[mPickedObjectLayer 
                                                    + layerOffset]
                            = mCurrentObject.spriteAgeEnd[mPickedObjectLayer];
                        mCurrentObject.spriteAgeEnd[mPickedObjectLayer] = 
                            tempAgeEnd;
                    
                        mCurrentObject.spriteParent[mPickedObjectLayer 
                                                    + layerOffset]
                            = mCurrentObject.spriteParent[
                                mPickedObjectLayer];
                        mCurrentObject.spriteParent[mPickedObjectLayer] = 
                            tempParent;


                        mCurrentObject.spriteInvisibleWhenHolding[
                            mPickedObjectLayer 
                            + layerOffset]
                            = mCurrentObject.spriteInvisibleWhenHolding[
                                mPickedObjectLayer];
                        mCurrentObject.spriteInvisibleWhenHolding[
                            mPickedObjectLayer] = tempInvisibleWhenHolding;

                        mCurrentObject.spriteInvisibleWhenWorn[
                            mPickedObjectLayer 
                            + layerOffset]
                            = mCurrentObject.spriteInvisibleWhenWorn[
                                mPickedObjectLayer];
                        mCurrentObject.spriteInvisibleWhenWorn[
                            mPickedObjectLayer] = tempInvisibleWhenWorn;

                        mCurrentObject.spriteBehindSlots[
                            mPickedObjectLayer 
                            + layerOffset]
                            = mCurrentObject.spriteBehindSlots[
                                mPickedObjectLayer];
                        mCurrentObject.spriteBehindSlots[
                            mPickedObjectLayer] = tempBehindSlots;


                        mCurrentObject.spriteIsHead[ mPickedObjectLayer 
                                                 + layerOffset ] =
                            mCurrentObject.spriteIsHead[mPickedObjectLayer];
                        mCurrentObject.spriteIsHead[mPickedObjectLayer] = 
                            tempIsHead;
                
                        mCurrentObject.spriteIsBody[ mPickedObjectLayer 
                                                 + layerOffset ] =
                            mCurrentObject.spriteIsBody[mPickedObjectLayer];
                        mCurrentObject.spriteIsBody[mPickedObjectLayer] = 
                            tempIsBody;
                
                        mCurrentObject.spriteIsBackFoot[ mPickedObjectLayer 
                                                 + layerOffset ] =
                            mCurrentObject.spriteIsBackFoot[
                                mPickedObjectLayer];
                        mCurrentObject.spriteIsBackFoot[mPickedObjectLayer] = 
                            tempIsBackFoot;
                
                        mCurrentObject.spriteIsFrontFoot[ mPickedObjectLayer 
                                                 + layerOffset ] =
                            mCurrentObject.spriteIsFrontFoot[
                                mPickedObjectLayer];
                        mCurrentObject.spriteIsFrontFoot[mPickedObjectLayer] = 
                            tempIsFrontFoot;
                


                        mCurrentObject.spriteUseVanish[ mPickedObjectLayer 
                                                        + layerOffset ] =
                            mCurrentObject.spriteUseVanish[mPickedObjectLayer];
                        mCurrentObject.spriteUseVanish[mPickedObjectLayer] = 
                            tempUseVanish;

                        mCurrentObject.spriteUseAppear[ mPickedObjectLayer 
                                                        + layerOffset ] =
                            mCurrentObject.spriteUseAppear[mPickedObjectLayer];
                        mCurrentObject.spriteUseAppear[mPickedObjectLayer] = 
                            tempUseAppear;

                    
                        mPickedObjectLayer += layerOffset;
                    
                        // any children pointing to index A must now
                        // point to B and vice-versa
                        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                            if( mCurrentObject.spriteParent[i] == indexA ) {
                                mCurrentObject.spriteParent[i] = indexB;
                                }
                            else if( mCurrentObject.spriteParent[i] == 
                                     indexB ) {
                                
                                mCurrentObject.spriteParent[i] = indexA;
                                }
                            }
                        for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                            if( mCurrentObject.slotParent[i] == indexA ) {
                                mCurrentObject.slotParent[i] = indexB;
                                }
                            else if( mCurrentObject.slotParent[i] == indexB ) {
                                mCurrentObject.slotParent[i] = indexA;
                                }
                            }
                        }
                    }
                break;
                }
            case MG_KEY_PAGE_DOWN: {
                for( int o=0; o<offset; o++ ) {
                    
                    int layerOffset = 1;
                    if( mPickedObjectLayer - 1 < 0 ) {
                        layerOffset = mPickedObjectLayer;
                        }

                    if( mPickedObjectLayer >= layerOffset ) {

                        // two indices being swapped
                        int indexA = mPickedObjectLayer;
                        int indexB = mPickedObjectLayer - layerOffset;


                        LayerSwapRecord swap = { indexA, indexB };
                        
                        mObjectLayerSwaps.push_back( swap );


                        int tempSprite = 
                            mCurrentObject.sprites[mPickedObjectLayer - 
                                                   layerOffset];
                        doublePair tempPos = 
                            mCurrentObject.spritePos[mPickedObjectLayer - 
                                                     layerOffset];

                        double tempRot = 
                            mCurrentObject.spriteRot[mPickedObjectLayer - 
                                                     layerOffset];

                        char tempHFlip = 
                            mCurrentObject.spriteHFlip[mPickedObjectLayer - 
                                                       layerOffset];

                        FloatRGB tempColor = 
                            mCurrentObject.spriteColor[mPickedObjectLayer - 
                                                       layerOffset];
                
                        double tempAgeStart = 
                            mCurrentObject.spriteAgeStart[mPickedObjectLayer - 
                                                          layerOffset];
                        double tempAgeEnd = 
                            mCurrentObject.spriteAgeEnd[mPickedObjectLayer - 
                                                        layerOffset];
                        int tempParent = 
                            mCurrentObject.spriteParent[mPickedObjectLayer - 
                                                        layerOffset];
                    
                        char tempInvisibleWhenHolding = 
                            mCurrentObject.spriteInvisibleWhenHolding[
                                mPickedObjectLayer - 
                                layerOffset];

                        int tempInvisibleWhenWorn = 
                            mCurrentObject.spriteInvisibleWhenWorn[
                                mPickedObjectLayer - 
                                layerOffset];

                        char tempBehindSlots = 
                            mCurrentObject.spriteBehindSlots[
                                mPickedObjectLayer - 
                                layerOffset];


                        char tempIsHead = 
                            mCurrentObject.spriteIsHead[
                                mPickedObjectLayer - 
                                layerOffset];
                        char tempIsBody = 
                            mCurrentObject.spriteIsBody[
                                mPickedObjectLayer - 
                                layerOffset];
                        char tempIsBackFoot = 
                            mCurrentObject.spriteIsBackFoot[
                                mPickedObjectLayer - 
                                layerOffset];
                        char tempIsFrontFoot = 
                            mCurrentObject.spriteIsFrontFoot[
                                mPickedObjectLayer - 
                                layerOffset];

                        char tempUseVanish = 
                            mCurrentObject.spriteUseVanish[
                                mPickedObjectLayer - 
                                layerOffset];
                        char tempUseAppear = 
                            mCurrentObject.spriteUseAppear[
                                mPickedObjectLayer - 
                                layerOffset];

                        mCurrentObject.sprites[mPickedObjectLayer - 
                                               layerOffset]
                            = mCurrentObject.sprites[mPickedObjectLayer];
                        
                        mCurrentObject.sprites[mPickedObjectLayer] = 
                            tempSprite;
                
                        mCurrentObject.spritePos[mPickedObjectLayer - 
                                                 layerOffset]
                            = mCurrentObject.spritePos[mPickedObjectLayer];
                        
                        mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;

                        mCurrentObject.spriteRot[mPickedObjectLayer - 
                                                 layerOffset]
                            = mCurrentObject.spriteRot[mPickedObjectLayer];
                        
                        mCurrentObject.spriteRot[mPickedObjectLayer] = tempRot;

                        mCurrentObject.spriteHFlip[mPickedObjectLayer - 
                                                   layerOffset]
                            = mCurrentObject.spriteHFlip[mPickedObjectLayer];
                        
                        mCurrentObject.spriteHFlip[mPickedObjectLayer] = 
                            tempHFlip;


                        mCurrentObject.spriteColor[mPickedObjectLayer - 
                                                   layerOffset]
                            = mCurrentObject.spriteColor[mPickedObjectLayer];
                        
                        mCurrentObject.spriteColor[mPickedObjectLayer] = 
                            tempColor;

                
                        mCurrentObject.spriteAgeStart[mPickedObjectLayer 
                                                      - layerOffset]
                            = mCurrentObject.spriteAgeStart[
                                mPickedObjectLayer];
                        
                        mCurrentObject.spriteAgeStart[mPickedObjectLayer] = 
                            tempAgeStart;

                        mCurrentObject.spriteAgeEnd[mPickedObjectLayer 
                                                    - layerOffset]
                            = mCurrentObject.spriteAgeEnd[mPickedObjectLayer];
                        mCurrentObject.spriteAgeEnd[mPickedObjectLayer] = 
                            tempAgeEnd;

                        mCurrentObject.spriteParent[mPickedObjectLayer 
                                                    - layerOffset]
                            = mCurrentObject.spriteParent[
                                mPickedObjectLayer];
                        mCurrentObject.spriteParent[mPickedObjectLayer] = 
                            tempParent;

                    
                        mCurrentObject.spriteInvisibleWhenHolding[
                            mPickedObjectLayer 
                            - layerOffset]
                            = mCurrentObject.spriteInvisibleWhenHolding[
                                mPickedObjectLayer];
                        mCurrentObject.spriteInvisibleWhenHolding[
                            mPickedObjectLayer] = tempInvisibleWhenHolding;

                        mCurrentObject.spriteInvisibleWhenWorn[
                            mPickedObjectLayer 
                            - layerOffset]
                            = mCurrentObject.spriteInvisibleWhenWorn[
                                mPickedObjectLayer];
                        mCurrentObject.spriteInvisibleWhenWorn[
                            mPickedObjectLayer] = tempInvisibleWhenWorn;

                        mCurrentObject.spriteBehindSlots[
                            mPickedObjectLayer 
                            - layerOffset]
                            = mCurrentObject.spriteBehindSlots[
                                mPickedObjectLayer];
                        mCurrentObject.spriteBehindSlots[
                            mPickedObjectLayer] = tempBehindSlots;



                        mCurrentObject.spriteIsHead[ mPickedObjectLayer 
                                                 - layerOffset ] =
                            mCurrentObject.spriteIsHead[mPickedObjectLayer];
                        mCurrentObject.spriteIsHead[mPickedObjectLayer] = 
                            tempIsHead;
                
                        mCurrentObject.spriteIsBody[ mPickedObjectLayer 
                                                 - layerOffset ] =
                            mCurrentObject.spriteIsBody[mPickedObjectLayer];
                        mCurrentObject.spriteIsBody[mPickedObjectLayer] = 
                            tempIsBody;
                
                        mCurrentObject.spriteIsBackFoot[ mPickedObjectLayer 
                                                 - layerOffset ] =
                            mCurrentObject.spriteIsBackFoot[
                                mPickedObjectLayer];
                        mCurrentObject.spriteIsBackFoot[mPickedObjectLayer] = 
                            tempIsBackFoot;
                
                        mCurrentObject.spriteIsFrontFoot[ mPickedObjectLayer 
                                                 - layerOffset ] =
                            mCurrentObject.spriteIsFrontFoot[
                                mPickedObjectLayer];
                        mCurrentObject.spriteIsFrontFoot[mPickedObjectLayer] = 
                            tempIsFrontFoot;


                        mCurrentObject.spriteUseVanish[ mPickedObjectLayer 
                                                     - layerOffset ] =
                            mCurrentObject.spriteUseVanish[mPickedObjectLayer];
                        mCurrentObject.spriteUseVanish[mPickedObjectLayer] = 
                            tempUseVanish;

                        mCurrentObject.spriteUseAppear[ mPickedObjectLayer 
                                                     - layerOffset ] =
                            mCurrentObject.spriteUseAppear[mPickedObjectLayer];
                        mCurrentObject.spriteUseAppear[mPickedObjectLayer] = 
                            tempUseAppear;
                        

                        mPickedObjectLayer -= layerOffset;

                        // any children pointing to index A must now
                        // point to B and vice-versa
                        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                            if( mCurrentObject.spriteParent[i] == indexA ) {
                                mCurrentObject.spriteParent[i] = indexB;
                                }
                            else if( mCurrentObject.spriteParent[i] == 
                                     indexB ) {
                                mCurrentObject.spriteParent[i] = indexA;
                                }
                            }
                        for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                            if( mCurrentObject.slotParent[i] == indexA ) {
                                mCurrentObject.slotParent[i] = indexB;
                                }
                            else if( mCurrentObject.slotParent[i] == indexB ) {
                                mCurrentObject.slotParent[i] = indexA;
                                }
                            }

                        }
                    }
                break;   
                }
            }
        }
    else if( mPickedSlot != -1 ) {
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.slotPos[mPickedSlot].x -= offset;
                mHoverSlot = mPickedSlot;
                mHoverStrength = 1;
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.slotPos[mPickedSlot].x += offset;
                mHoverSlot = mPickedSlot;
                mHoverStrength = 1;
                break;
            case MG_KEY_DOWN:
                mCurrentObject.slotPos[mPickedSlot].y -= offset;
                mHoverSlot = mPickedSlot;
                mHoverStrength = 1;
                break;
            case MG_KEY_UP:
                mCurrentObject.slotPos[mPickedSlot].y += offset;
                mHoverSlot = mPickedSlot;
                mHoverStrength = 1;
                break;
            }
        
        }
    
    
            
    }
