#include "EditorObjectPage.h"

#include "ageControl.h"


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



EditorObjectPage::EditorObjectPage()
        : mDescriptionField( mainFont, 
                             0,  -260, 6,
                             false,
                             "Description", NULL, NULL ),
          mMapChanceField( smallFont, 
                           -150,  64, 4,
                           false,
                           "MapP", "0123456789.", NULL ),
          mHeatValueField( smallFont, 
                           -150,  32, 4,
                           false,
                           "Heat", "0123456789", NULL ),
          mRValueField( smallFont, 
                        -150,  0, 4,
                        false,
                        "R", "0123456789.", NULL ),
          mFoodValueField( smallFont, 
                           -150,  -32, 4,
                           false,
                           "Food", "0123456789", NULL ),
          mSpeedMultField( smallFont, 
                           -150,  -64, 4,
                           false,
                           "Speed", "0123456789.", NULL ),
          mContainSizeField( smallFont, 
                             150,  -120, 4,
                             false,
                             "Contain Size", "0123456789.", NULL ),
          mSlotSizeField( smallFont, 
                          -160,  -230, 4,
                          false,
                          "Slot Size", "0123456789.", NULL ),
          mDeadlyDistanceField( smallFont, 
                                150,  -220, 4,
                                false,
                                "Deadly Distance", "0123456789.", NULL ),
          mSaveObjectButton( smallFont, 210, -260, "Save New" ),
          mReplaceObjectButton( smallFont, 310, -260, "Replace" ),
          mClearObjectButton( smallFont, -160, 200, "Blank" ),
          mClearRotButton( smallFont, -160, 120, "0 Rot" ),
          mFlipHButton( smallFont, -160, 160, "H Flip" ),
          mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mTransEditorButton( mainFont, 210, 260, "Trans" ),
          mAnimEditorButton( mainFont, 330, 260, "Anim" ),
          mMoreSlotsButton( smallFont, -160, -110, "More" ),
          mLessSlotsButton( smallFont, -160, -190, "Less" ),

          mAgingLayerCheckbox( 190, -22, 2 ),
          mHeadLayerCheckbox( 190, 104, 2 ),
          mBodyLayerCheckbox( 190, 84, 2 ),
          mBackFootLayerCheckbox( 190, 64, 2 ),
          mFrontFootLayerCheckbox( 190, 44, 2 ),
          mAgeInField( smallFont, 
                       160,  -52, 6,
                       false,
                       "In", "0123456789.", NULL ),
          mAgeOutField( smallFont, 
                        160,  -90, 6,
                        false,
                        "Out", "0123456789.", NULL ),
          mAgePunchInButton( smallFont, 210, -52, "S" ),
          mAgePunchOutButton( smallFont, 210, -90, "S" ),

          // these are in same spot because they're never shown at same time
          mMaleCheckbox( 190, -190, 2 ),
          mDeathMarkerCheckbox( 190, -190, 2 ),

          mDemoClothesButton( smallFont, 200, 200, "Pos" ),
          mEndClothesDemoButton( smallFont, 200, 160, "XPos" ),
          mDemoSlotsButton( smallFont, 150, 32, "Demo Slots" ),
          mClearSlotsDemoButton( smallFont, 150, -32, "End Demo" ),
          mSetHeldPosButton( smallFont, 150, 76, "Held Pos" ),
          mEndSetHeldPosButton( smallFont, 150, -76, "End Held" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mPersonAgeSlider( smallFont, -70, 175, 2,
                            100, 20,
                            0, 100, "Age" ),
          mHueSlider( smallFont, -90, -130, 2,
                      75, 20,
                      0, 1, "H" ),
          mSaturationSlider( smallFont, -90, -162, 2,
                             75, 20,
                             0, 1, "S" ),
          mValueSlider( smallFont, -90, -194, 2,
                        75, 20,
                        0, 1, "V" ),
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
    addComponent( &mHeatValueField );
    addComponent( &mRValueField );
    addComponent( &mFoodValueField );
    addComponent( &mSpeedMultField );

    addComponent( &mContainSizeField );
    addComponent( &mSlotSizeField );

    mContainSizeField.setVisible( false );
    mSlotSizeField.setVisible( false );

    addComponent( &mDeadlyDistanceField );

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

    mSetHeldPosButton.setVisible( false );
    mEndSetHeldPosButton.setVisible( false );    
    

    addComponent( &mClearObjectButton );
    addComponent( &mClearRotButton );
    
    addComponent( &mFlipHButton );

    addComponent( &mSpritePicker );
    addComponent( &mObjectPicker );

    addComponent( &mPersonAgeSlider );

    addComponent( &mHueSlider );
    addComponent( &mSaturationSlider );
    addComponent( &mValueSlider );
    

    addComponent( &mAgingLayerCheckbox );
    addComponent( &mHeadLayerCheckbox );
    addComponent( &mBodyLayerCheckbox );
    addComponent( &mBackFootLayerCheckbox );
    addComponent( &mFrontFootLayerCheckbox );
    
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

    mFlipHButton.addActionListener( this );
    mFlipHButton.setVisible( false );

    mMoreSlotsButton.addActionListener( this );
    mLessSlotsButton.addActionListener( this );
    
    mDemoSlotsButton.addActionListener( this );
    mClearSlotsDemoButton.addActionListener( this );

    mSetHeldPosButton.addActionListener( this );
    mEndSetHeldPosButton.addActionListener( this );

    mDemoClothesButton.addActionListener( this );
    mEndClothesDemoButton.addActionListener( this );
    

    mSpritePicker.addActionListener( this );
    mObjectPicker.addActionListener( this );

    mSaveObjectButton.setVisible( false );
    mReplaceObjectButton.setVisible( false );
    mClearObjectButton.setVisible( false );

    mCurrentObject.id = -1;
    mCurrentObject.description = mDescriptionField.getText();
    mCurrentObject.containable = 0;

    mCurrentObject.heldOffset.x = 0;
    mCurrentObject.heldOffset.y = 0;

    mCurrentObject.clothing = 'n';
    mCurrentObject.clothingOffset.x = 0;
    mCurrentObject.clothingOffset.y = 0;
    
    mCurrentObject.numSlots = 0;
    mCurrentObject.slotPos = new doublePair[ 0 ];
    
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


    mCurrentObject.headIndex = 0;
    mCurrentObject.bodyIndex = 0;
    mCurrentObject.backFootIndex = 0;
    mCurrentObject.frontFootIndex = 0;
    

    mCurrentObject.headIndex = 0;
    

    mPickedObjectLayer = -1;
    mPickedSlot = -1;
    
    mHoverObjectLayer = -1;
    mHoverSlot = -1;
    mHoverStrength = 0;
    mHoverFrameCount = 0;
    

    mRotAdjustMode = false;
    mRotStartMouseX = 0;
    
    mMapChanceField.setText( "0.00" );
    
    mHeatValueField.setText( "0" );
    mRValueField.setText( "0.00" );
    
    mFoodValueField.setText( "0" );
    mSpeedMultField.setText( "1.00" );

    mContainSizeField.setInt( 1 );
    mSlotSizeField.setInt( 1 );
    
    mDeadlyDistanceField.setInt( 0 );
    

    double boxY = -150;
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 150, boxY, 2 );
        addComponent( mCheckboxes[i] );

        boxY -= 20;
        }
    mCheckboxNames[0] = "Containable";
    mCheckboxNames[1] = "Permanent";
    mCheckboxNames[2] = "Person";
    
    mCheckboxes[0]->addActionListener( this );
    mCheckboxes[2]->addActionListener( this );


    addComponent( &mMaleCheckbox );
    mMaleCheckbox.setVisible( false );

    addComponent( &mDeathMarkerCheckbox );
    mDeathMarkerCheckbox.setVisible( true );
    
    boxY = 200;

    for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
        mClothingCheckboxes[i] = new CheckboxButton( 160, boxY, 2 );
        addComponent( mClothingCheckboxes[i] );
        
        mClothingCheckboxes[i]->addActionListener( this );
    
        boxY -= 20;
        }
    mClothingCheckboxNames[0] = "No wear";
    mClothingCheckboxNames[1] = "Shoe";
    mClothingCheckboxNames[2] = "Tunic";
    mClothingCheckboxNames[3] = "Hat";
    mClothingCheckboxes[0]->setToggled( true );



    addKeyClassDescription( &mKeyLegend, "n/m", "Switch layers" );
    addKeyClassDescription( &mKeyLegend, "arrows", "Move layer" );
    addKeyClassDescription( &mKeyLegend, "Pg Up/Down", "Layer order" );
    addKeyClassDescription( &mKeyLegend, "Ctrl", "Bigger jumps" );
    addKeyDescription( &mKeyLegend, 'r', "Rotate layer" );
    addKeyClassDescription( &mKeyLegend, "c/v", "Copy/paste color" );


    addKeyClassDescription( &mKeyLegendB, "R-Click", "Layer parent" );
    addKeyClassDescription( &mKeyLegendB, "Bkspce", "Remv layer" );


    mColorClipboard.r = 1;
    mColorClipboard.g = 1;
    mColorClipboard.b = 1;
    }



EditorObjectPage::~EditorObjectPage() {

    delete [] mCurrentObject.description;
    delete [] mCurrentObject.slotPos;
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;
    delete [] mCurrentObject.spriteColor;

    delete [] mCurrentObject.spriteAgeStart;
    delete [] mCurrentObject.spriteAgeEnd;
    delete [] mCurrentObject.spriteParent;

    delete [] mCurrentObject.spriteInvisibleWhenHolding;


    freeSprite( mSlotPlaceholderSprite );
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
        }

     for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
         delete mClothingCheckboxes[i];
         }
    }




float getFloat( TextField *inField ) {
    char *text = inField->getText();
    
    float f = 0;
    
    sscanf( text, "%f", &f );

    delete [] text;
    
    return f;
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
    

    if( ! mCheckboxes[2]->getToggled() ) {
        agingPanelVisible = false;
        mAgingLayerCheckbox.setVisible( false );
            
        mHeadLayerCheckbox.setVisible( false );
        mBodyLayerCheckbox.setVisible( false );
        mBackFootLayerCheckbox.setVisible( false );
        mFrontFootLayerCheckbox.setVisible( false );

        for( int i=0; i<mCurrentObject.numSprites; i++ ) {
            mCurrentObject.spriteAgeStart[i] = -1;
            mCurrentObject.spriteAgeEnd[i] = -1;
            }
        
        mCurrentObject.headIndex = 0;
        mCurrentObject.bodyIndex = 0;
        mCurrentObject.backFootIndex = 0;
        mCurrentObject.frontFootIndex = 0;


        mMaleCheckbox.setToggled( false );
        mMaleCheckbox.setVisible( false );
        
        mDeathMarkerCheckbox.setVisible( true );
        }
    else {
        mMaleCheckbox.setVisible( true );

        mDeathMarkerCheckbox.setToggled( false );
        mDeathMarkerCheckbox.setVisible( false );
        
        if( mPickedObjectLayer != -1 ) {
            mAgingLayerCheckbox.setVisible( true );
            
            mHeadLayerCheckbox.setVisible( true );
            mBodyLayerCheckbox.setVisible( true );
            mBackFootLayerCheckbox.setVisible( true );
            mFrontFootLayerCheckbox.setVisible( true );


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

            if( mCurrentObject.headIndex == mPickedObjectLayer ) {
                mHeadLayerCheckbox.setToggled( true );
                }
            else {
                mHeadLayerCheckbox.setToggled( false );
                }

            if( mCurrentObject.bodyIndex == mPickedObjectLayer ) {
                mBodyLayerCheckbox.setToggled( true );
                }
            else {
                mBodyLayerCheckbox.setToggled( false );
                }

            if( mCurrentObject.backFootIndex == mPickedObjectLayer ) {
                mBackFootLayerCheckbox.setToggled( true );
                }
            else {
                mBackFootLayerCheckbox.setToggled( false );
                }

            if( mCurrentObject.frontFootIndex == mPickedObjectLayer ) {
                mFrontFootLayerCheckbox.setToggled( true );
                }
            else {
                mFrontFootLayerCheckbox.setToggled( false );
                }
            
            }
        else {
            mAgingLayerCheckbox.setVisible( false );
                
            agingPanelVisible = false;

            mHeadLayerCheckbox.setVisible( false );
            mBodyLayerCheckbox.setVisible( false );
            mBackFootLayerCheckbox.setVisible( false );
            mFrontFootLayerCheckbox.setVisible( false );
            }
        
        }

    mAgeInField.setVisible( agingPanelVisible );
    mAgeOutField.setVisible( agingPanelVisible );
    mAgePunchInButton.setVisible( agingPanelVisible );
    mAgePunchOutButton.setVisible( agingPanelVisible );
    }




static void recursiveRotate( ObjectRecord *inObject,
                             int inRotatingParentIndex,
                             doublePair inRotationCenter,
                             double inRotationDelta ) {

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
    
    }





void EditorObjectPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mDescriptionField ) {
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
    else if( inTarget == &mSaveObjectButton ) {
        char *text = mDescriptionField.getText();

        addObject( text,
                   mCheckboxes[0]->getToggled(),
                   mContainSizeField.getInt(),
                   mCheckboxes[1]->getToggled(),
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mMaleCheckbox.getToggled(),
                   mDeathMarkerCheckbox.getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.clothing,
                   mCurrentObject.clothingOffset,
                   mDeadlyDistanceField.getInt(),
                   mCurrentObject.numSlots,
                   mSlotSizeField.getInt(),
                   mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip,
                   mCurrentObject.spriteColor,
                   mCurrentObject.spriteAgeStart,
                   mCurrentObject.spriteAgeEnd,
                   mCurrentObject.spriteParent,
                   mCurrentObject.spriteInvisibleWhenHolding,
                   mCurrentObject.headIndex,
                   mCurrentObject.bodyIndex,
                   mCurrentObject.backFootIndex,
                   mCurrentObject.frontFootIndex );
        
        delete [] text;
        
        mSpritePicker.unselectObject();

        mObjectPicker.redoSearch();
        actionPerformed( &mClearObjectButton );
        }
    else if( inTarget == &mReplaceObjectButton ) {
        char *text = mDescriptionField.getText();

        addObject( text,
                   mCheckboxes[0]->getToggled(),
                   mContainSizeField.getInt(),
                   mCheckboxes[1]->getToggled(),
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mMaleCheckbox.getToggled(),
                   mDeathMarkerCheckbox.getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.clothing,
                   mCurrentObject.clothingOffset,
                   mDeadlyDistanceField.getInt(),
                   mCurrentObject.numSlots,
                   mSlotSizeField.getInt(), 
                   mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip,
                   mCurrentObject.spriteColor,
                   mCurrentObject.spriteAgeStart,
                   mCurrentObject.spriteAgeEnd,
                   mCurrentObject.spriteParent,
                   mCurrentObject.spriteInvisibleWhenHolding,
                   mCurrentObject.headIndex,
                   mCurrentObject.bodyIndex,
                   mCurrentObject.backFootIndex,
                   mCurrentObject.frontFootIndex,
                   mCurrentObject.id );
        
        delete [] text;
        
        mSpritePicker.unselectObject();
        
        mObjectPicker.redoSearch();
        actionPerformed( &mClearObjectButton );
        }
    else if( inTarget == &mClearObjectButton ) {
        mCurrentObject.id = -1;
        
        mDescriptionField.setText( "" );

        delete [] mCurrentObject.description;
        mCurrentObject.description = mDescriptionField.getText();
        
        
        mPersonAgeSlider.setVisible( false );

        for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
            mCheckboxes[i]->setToggled( false );
            }

        mMapChanceField.setText( "0.00" );

        mHeatValueField.setText( "0" );
        mRValueField.setText( "0.00" );

        mFoodValueField.setText( "0" );
        mSpeedMultField.setText( "1.00" );

        mContainSizeField.setInt( 1 );
        mSlotSizeField.setInt( 1 );
        mContainSizeField.setVisible( false );
        mSlotSizeField.setVisible( false );

        mDeadlyDistanceField.setInt( 0 );

        mCurrentObject.numSlots = 0;
        
        mCurrentObject.heldOffset.x = 0;
        mCurrentObject.heldOffset.y = 0;

        actionPerformed( mClothingCheckboxes[0] );
        

        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = new doublePair[ 0 ];

        
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


        mCurrentObject.headIndex = 0;
        mCurrentObject.bodyIndex = 0;
        mCurrentObject.backFootIndex = 0;
        mCurrentObject.frontFootIndex = 0;

        

        mSaveObjectButton.setVisible( false );
        mReplaceObjectButton.setVisible( false );
        mClearObjectButton.setVisible( false );
        

        mDemoSlotsButton.setVisible( false );
        mClearSlotsDemoButton.setVisible( false );
        mSetHeldPosButton.setVisible( false );
        mEndSetHeldPosButton.setVisible( false );
        
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
        if( mPickedObjectLayer != -1 ) {
            
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
    else if( inTarget == &mFlipHButton ) {
        if( mPickedObjectLayer != -1 ) {
            mCurrentObject.spriteHFlip[mPickedObjectLayer] =
                ! mCurrentObject.spriteHFlip[mPickedObjectLayer];
            }
        else {
            mFlipHButton.setVisible( false );
            }
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
        mCurrentObject.containable = 0;
        
        int numSlots = mCurrentObject.numSlots;
        
        doublePair *slots = new doublePair[ numSlots + 1 ];
        
        memcpy( slots, mCurrentObject.slotPos, 
                sizeof( doublePair ) * numSlots );
        
        if( mCurrentObject.numSlots == 0 ) {
            
            slots[numSlots].x = 0;
            slots[numSlots].y = 0;
            
            mDemoSlotsButton.setVisible( true );
            mClearSlotsDemoButton.setVisible( false );
            }
        else {
            slots[numSlots].x = 
                slots[numSlots - 1].x + 16;
            slots[numSlots].y = 
                slots[numSlots - 1].y;
            }
        
        mPickedSlot = numSlots;
        mPickedObjectLayer = -1;
        pickedLayerChanged();

        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = slots;

        mCurrentObject.numSlots = numSlots + 1;
        
        mSlotSizeField.setVisible( true );
        
        mContainSizeField.setInt( 1 );
        mContainSizeField.setVisible( false );
        mCheckboxes[0]->setToggled( false );

        mPersonAgeSlider.setVisible( false );
        mCheckboxes[2]->setToggled( false );
        updateAgingPanel();
        
        mSetHeldPosButton.setVisible( true );

        mDemoClothesButton.setVisible( false );
        mCurrentObject.clothing = 'n';
        for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
            mClothingCheckboxes[i]->setToggled( false );
            }
        mClothingCheckboxes[0]->setToggled( true );
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
            mEndSetHeldPosButton.setVisible( true );
            
            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            }
        }
    else if( inTarget == &mEndSetHeldPosButton ) {
        mSetHeldPos = false;
        mDemoPersonObject = -1;

        mSetHeldPosButton.setVisible( true );
        mEndSetHeldPosButton.setVisible( false );

        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            }
        
        if( ! mClothingCheckboxes[0]->getToggled() ) {
            mDemoClothesButton.setVisible( true );
            mEndClothesDemoButton.setVisible( false );
            }
        }
    else if( inTarget == &mDemoClothesButton ) {
        mDemoPersonObject = getRandomPersonObject();
        
        if( mDemoPersonObject != -1 ) {
            
            mSetClothesPos = true;
            mDemoClothesButton.setVisible( false );
            mEndClothesDemoButton.setVisible( true );
            
            mSetHeldPos = false;
            mSetHeldPosButton.setVisible( false );
            mEndSetHeldPosButton.setVisible( false );

            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            }
        }
    else if( inTarget == &mEndClothesDemoButton ) {
        mSetClothesPos = false;
        mDemoPersonObject = -1;

        mDemoClothesButton.setVisible( true );
        mEndClothesDemoButton.setVisible( false );

        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            }

        mSetHeldPosButton.setVisible( true );
        mEndSetHeldPosButton.setVisible( false );
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
    else if( inTarget == &mHeadLayerCheckbox ) {
        if( mHeadLayerCheckbox.getToggled() ) {
            mCurrentObject.headIndex = mPickedObjectLayer;
            }
        else {
            mCurrentObject.headIndex = 0;
            }
        }
    else if( inTarget == &mBodyLayerCheckbox ) {
        if( mBodyLayerCheckbox.getToggled() ) {
            mCurrentObject.bodyIndex = mPickedObjectLayer;
            }
        else {
            mCurrentObject.bodyIndex = 0;
            }
        }
    else if( inTarget == &mBackFootLayerCheckbox ) {
        if( mBackFootLayerCheckbox.getToggled() ) {
            mCurrentObject.backFootIndex = mPickedObjectLayer;
            }
        else {
            mCurrentObject.backFootIndex = 0;
            }
        }
    else if( inTarget == &mFrontFootLayerCheckbox ) {
        if( mFrontFootLayerCheckbox.getToggled() ) {
            mCurrentObject.frontFootIndex = mPickedObjectLayer;
            }
        else {
            mCurrentObject.frontFootIndex = 0;
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
        if( mPickedObjectLayer != -1 ) {
            
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
        if( mPickedObjectLayer != -1 ) {
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
                mSetHeldPosButton.setVisible( true );

                if( ! mClothingCheckboxes[0]->getToggled() ) {
                    mDemoClothesButton.setVisible( true );
                    mEndClothesDemoButton.setVisible( false );
                    }
                }
            mEndSetHeldPosButton.setVisible( false );

            mSetClothesPos = false;
            mDemoPersonObject = -1;
            
            mEndClothesDemoButton.setVisible( false );
            }
        

        int spriteID = mSpritePicker.getSelectedObject();
        
        if( spriteID != -1 ) {            

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
        

            newSprites[ mCurrentObject.numSprites ] = spriteID;
            
            doublePair pos = {0,0};
            
            newSpritePos[ mCurrentObject.numSprites ] = pos;

            newSpriteRot[ mCurrentObject.numSprites ] = 0;

            newSpriteHFlip[ mCurrentObject.numSprites ] = 0;

            FloatRGB white = { 1, 1, 1 };
            
            newSpriteColor[ mCurrentObject.numSprites ] = white;

            newSpriteAgeStart[ mCurrentObject.numSprites ] = -1;
            newSpriteAgeEnd[ mCurrentObject.numSprites ] = -1;
            
            newSpriteParent[ mCurrentObject.numSprites ] = -1;

            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            delete [] mCurrentObject.spriteRot;
            delete [] mCurrentObject.spriteHFlip;
            delete [] mCurrentObject.spriteColor;

            delete [] mCurrentObject.spriteAgeStart;
            delete [] mCurrentObject.spriteAgeEnd;
            delete [] mCurrentObject.spriteParent;
            
            delete [] mCurrentObject.spriteInvisibleWhenHolding;
                        

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

            mCurrentObject.numSprites = newNumSprites;
            

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
        int objectID = mObjectPicker.getSelectedObject();

        
        // auto-end the held-pos setting if a new object is picked
        // (also, potentially enable the setting button for the first time) 
        if( objectID != -1 && ! mDemoSlots ) {
            mSetHeldPos = false;
            mDemoPersonObject = -1;
            mSetHeldPosButton.setVisible( true );
            mEndSetHeldPosButton.setVisible( false );
            }
        

        if( objectID != -1 && mDemoSlots ) {
            mSlotsDemoObject = objectID;
            }
        else if( objectID != -1 ) {
            ObjectRecord *pickedRecord = getObject( objectID );

                
            delete [] mCurrentObject.slotPos;
            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            delete [] mCurrentObject.spriteRot;
            delete [] mCurrentObject.spriteHFlip;
            delete [] mCurrentObject.spriteColor;
            delete [] mCurrentObject.spriteAgeStart;
            delete [] mCurrentObject.spriteAgeEnd;
            delete [] mCurrentObject.spriteParent;
            delete [] mCurrentObject.spriteInvisibleWhenHolding;


            mCurrentObject.id = objectID;
                
            mDescriptionField.setText( pickedRecord->description );

            mMapChanceField.setFloat( pickedRecord->mapChance, 2 );
            
            mHeatValueField.setInt( pickedRecord->heatValue );
            mRValueField.setFloat( pickedRecord->rValue, 2 );
            
            mFoodValueField.setInt( pickedRecord->foodValue );

            mSpeedMultField.setFloat( pickedRecord->speedMult, 2 );

            mContainSizeField.setInt( pickedRecord->containSize );
            mSlotSizeField.setInt( pickedRecord->slotSize );
            
            mDeadlyDistanceField.setInt( pickedRecord->deadlyDistance );


            mCurrentObject.containable = pickedRecord->containable;
            
            mCurrentObject.heldOffset = pickedRecord->heldOffset;

            mCurrentObject.clothing = pickedRecord->clothing;
            mCurrentObject.clothingOffset = pickedRecord->clothingOffset;


            switch( mCurrentObject.clothing ) {
                case 'n':
                    actionPerformed( mClothingCheckboxes[0] );
                    break;
                case 's':
                    actionPerformed( mClothingCheckboxes[1] );
                    break;
                case 't':
                    actionPerformed( mClothingCheckboxes[2] );
                    break;
                case 'h':
                    actionPerformed( mClothingCheckboxes[3] );
                    break;
                }   


            mCurrentObject.numSlots = pickedRecord->numSlots;

            mCurrentObject.slotPos = 
                new doublePair[ pickedRecord->numSlots ];
                
            memcpy( mCurrentObject.slotPos, pickedRecord->slotPos,
                    sizeof( doublePair ) * pickedRecord->numSlots );

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
            

            mCurrentObject.headIndex = pickedRecord->headIndex;
            mCurrentObject.bodyIndex = pickedRecord->bodyIndex;
            mCurrentObject.backFootIndex = pickedRecord->backFootIndex;
            mCurrentObject.frontFootIndex = pickedRecord->frontFootIndex;

            
            mSaveObjectButton.setVisible( true );
            mReplaceObjectButton.setVisible( true );
            mClearObjectButton.setVisible( true );

            mPickedObjectLayer = -1;
            mPickedSlot = -1;

            if( !mCurrentObject.containable && mCurrentObject.numSlots > 0 ) {
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

            mCheckboxes[0]->setToggled( pickedRecord->containable );
            mCheckboxes[1]->setToggled( pickedRecord->permanent );
            mCheckboxes[2]->setToggled( pickedRecord->person );

            mMaleCheckbox.setToggled( pickedRecord->male );
            mDeathMarkerCheckbox.setToggled( pickedRecord->deathMarker );
            
            if( mCheckboxes[2]->getToggled() ) {
                mPersonAgeSlider.setValue( 20 );
                mPersonAgeSlider.setVisible( true );
                mSetHeldPosButton.setVisible( false );
                }
            else {
                mPersonAgeSlider.setVisible( false );
                }
            pickedLayerChanged();

            if( ! pickedRecord->containable ) {
                mContainSizeField.setInt( 1 );
                mContainSizeField.setVisible( false );
                }
            else {
                mContainSizeField.setVisible( true );
                }
            
            if( pickedRecord->numSlots == 0 ) {
                mSlotSizeField.setInt( 1 );
                mSlotSizeField.setVisible( false );
                }
            else {
                mSlotSizeField.setVisible( true );
                }
            }
        }
    else if( inTarget == mCheckboxes[0] ) {
        printf( "Toggle\n" );
        if( mCheckboxes[0]->getToggled() ) {
            mContainSizeField.setVisible( true );
            
            mSlotSizeField.setInt( 1 );
            mSlotSizeField.setVisible( false );
            mDemoSlotsButton.setVisible( false );
            mCurrentObject.numSlots = 0;

            mPersonAgeSlider.setVisible( false );
            mCheckboxes[2]->setToggled( false );
            mSetHeldPosButton.setVisible( true );
            }
        else {
            mContainSizeField.setInt( 1 );
            mContainSizeField.setVisible( false );
            }
                    
        updateAgingPanel();
        }
    else if( inTarget == mCheckboxes[2] ) {
        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            mSetHeldPosButton.setVisible( false );

            
            mContainSizeField.setInt( 1 );
            mContainSizeField.setVisible( false );
            mCheckboxes[0]->setToggled( false );

            mSlotSizeField.setInt( 1 );
            mSlotSizeField.setVisible( false );
            mDemoSlotsButton.setVisible( false );
            mCurrentObject.numSlots = 0;

            mDemoClothesButton.setVisible( false );
            mCurrentObject.clothing = 'n';
            for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
                mClothingCheckboxes[i]->setToggled( false );
                }
            mClothingCheckboxes[0]->setToggled( true );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            mSetHeldPosButton.setVisible( true );
            }
                    
        updateAgingPanel();    
        }
    else if( inTarget == &mHueSlider ||
             inTarget == &mSaturationSlider ||
             inTarget == &mValueSlider ) {
        
        if( mPickedObjectLayer != -1 ) {
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
    else {
        // check clothing checkboxes
                
        for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
            if( inTarget == mClothingCheckboxes[i] ) {
                        
                mClothingCheckboxes[i]->setToggled( true );
                
                for( int j=0; j<NUM_CLOTHING_CHECKBOXES; j++ ) {
                    if( i != j ) {
                        mClothingCheckboxes[j]->setToggled( false );
                        }
                    }
                
                char c = 'n';
                
                switch( i ) {
                    case 1:
                        c = 's';
                        break;
                    case 2:
                        c = 't';
                        break;
                    case 3:
                        c = 'h';
                        break;   
                    }
                mCurrentObject.clothing = c;

                mEndClothesDemoButton.setVisible( false );
                mSetClothesPos = false;

                if( i != 0 ) {
                    mDemoClothesButton.setVisible( true );
                    
                    mSlotSizeField.setInt( 1 );
                    mSlotSizeField.setVisible( false );
                    mCurrentObject.numSlots = 0;
                    mDemoSlotsButton.setVisible( false );

                    mPersonAgeSlider.setVisible( false );
                    mCheckboxes[2]->setToggled( false );
                    mSetHeldPosButton.setVisible( true );

                    updateAgingPanel();
                    }
                else {
                    mDemoClothesButton.setVisible( false );
                    }
                
                break;
                }
            }
        }

    }

   

     
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    


    
    setDrawColor( 1, 1, 1, 1 );
    for( int y=0; y<3; y++ ) {
        for( int x=0; x<3; x++ ) {
            
            doublePair pos = { (double)( x * 64 - 64 ), 
                               (double)( y * 64 - 64 ) };
            
            drawSquare( pos, 32 );
            }
        }

    int i = 0;
    for( int y=0; y<2; y++ ) {
        for( int x=0; x<2; x++ ) {
            
            doublePair pos = { (double)( x * 64 - 32), 
                               (double)( y * 64 - 64 ) };
            
            if( i%2 == 0 ) {
                setDrawColor( .85, .85, .85, 1 );
                }
            else {
                setDrawColor( 0.75, 0.75, 0.75, 1 );
                }
            
            drawSquare( pos, 32 );
            i++;
            }
        i++;
        }

    
    doublePair barPos = { 0, 128 };
    
    setDrawColor( .85, .85, .85, 1 );

    drawRect( barPos, 96, 32 );

    

    doublePair drawOffset = mObjectCenterOnScreen;
    

    if( mSetHeldPos ) {
        
        // draw sample person behind
        setDrawColor( 1, 1, 1, 1 );


        double age = mPersonAgeSlider.getValue();
            
        drawObject( getObject( mDemoPersonObject ), drawOffset, 0, false, 
                    age, getEmptyClothingSet() );

        drawOffset = add( mCurrentObject.heldOffset, drawOffset );
        }

    char skipDrawing = false;
    
    if( mSetClothesPos ) {
        
        // draw sample person behind
        setDrawColor( 1, 1, 1, 1 );


        double age = mPersonAgeSlider.getValue();
          

        ClothingSet s = getEmptyClothingSet();
        
        switch( mCurrentObject.clothing ) {
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
        
                
  
        drawObject( getObject( mDemoPersonObject ), drawOffset, 0, false, 
                    age, s );

        // offset from body part
        //switch( mCurrentObject.clothing ) {
        //    case 's':
        
        skipDrawing = true;
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
                
                setDrawColor( red, 1, blue, alpha );
                drawSprite( mSlotPlaceholderSprite, 
                            add( mCurrentObject.slotPos[i],
                                 drawOffset ) );
                
                
                setDrawColor( 0, 1, 1, 0.5 );
                
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
                
                drawObject( demoObject, 
                            add( mCurrentObject.slotPos[i], drawOffset ),
                            0, false, -1, getEmptyClothingSet() );
                }
            }
        
        }
    


    

    doublePair pos = { 0, 0 };
    

    int bodyIndex = 0;
    
    doublePair headPos = {0,0};

    if( mCurrentObject.headIndex <= mCurrentObject.numSprites ) {
        headPos = mCurrentObject.spritePos[ mCurrentObject.headIndex ];
        }


    doublePair frontFootPos = {0,0};

    if( mCurrentObject.frontFootIndex <= mCurrentObject.numSprites ) {
        frontFootPos = 
            mCurrentObject.spritePos[ mCurrentObject.frontFootIndex ];
        }


    doublePair bodyPos = {0,0};

    if( mCurrentObject.bodyIndex <= mCurrentObject.numSprites ) {
        bodyPos = mCurrentObject.spritePos[ mCurrentObject.bodyIndex ];
        }

    if( !skipDrawing )
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        doublePair spritePos = mCurrentObject.spritePos[i];
        
        float blue = 1;
        float red = 1;
        float green = 1;
        float alpha = 1;
        
        if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
            blue = 1 - mHoverStrength;
            red = 1 - mHoverStrength;
            alpha *= mHoverFlash;
            }
        else {
            FloatRGB color = mCurrentObject.spriteColor[i];
            
            red = color.r;
            green = color.g;
            blue = color.b;
            }
        
        setDrawColor( red, green, blue, alpha );

        
                        
        if( mPersonAgeSlider.isVisible() &&
            mCheckboxes[2]->getToggled() ) {
        
            double age = mPersonAgeSlider.getValue();
            
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
            if( i == mCurrentObject.headIndex ||
                checkSpriteAncestor( &mCurrentObject, i,
                                     mCurrentObject.headIndex ) ) {
            
                spritePos = add( spritePos, getAgeHeadOffset( age, headPos,
                                                              bodyPos,
                                                              frontFootPos ) );
                }
            if( i == mCurrentObject.bodyIndex ||
                checkSpriteAncestor( &mCurrentObject, i,
                                     mCurrentObject.bodyIndex ) ) {
            
                spritePos = add( spritePos, getAgeBodyOffset( age, bodyPos ) );
                }
            }
                
        spritePos = add( spritePos, drawOffset );

        
        drawSprite( getSprite( mCurrentObject.sprites[i] ), spritePos,
                    1.0, mCurrentObject.spriteRot[i],
                    mCurrentObject.spriteHFlip[i] );

        if( mCurrentObject.spriteAgeStart[i] == -1 &&
            mCurrentObject.spriteAgeEnd[i] == -1 ) {
            
            bodyIndex++;
            }
        }


    setDrawColor( 1, 1, 1, 1 );
    pos.y -= 150;
    
    pos.x -= 200;

    char *numSlotString = autoSprintf( "Slots: %d", mCurrentObject.numSlots );
    
    smallFont->drawString( numSlotString, pos, alignLeft );

    delete [] numSlotString;


    
    if( mPickedObjectLayer != -1 ) {
        pos.x = -210;
        pos.y = 90;

        char *rotString = 
            autoSprintf( "Rot: %f", 
                         mCurrentObject.spriteRot[mPickedObjectLayer] );
    
        smallFont->drawString( rotString, pos, alignLeft );
        
        delete [] rotString;
        }
    


    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        pos = mCheckboxes[i]->getPosition();
    
        pos.x -= 20;
        
        smallFont->drawString( mCheckboxNames[i], pos, alignRight );
        }

    for( int i=0; i<NUM_CLOTHING_CHECKBOXES; i++ ) {
        pos = mClothingCheckboxes[i]->getPosition();
    
        pos.x -= 20;
        
        smallFont->drawString( mClothingCheckboxNames[i], pos, alignRight );
        }

    if( mAgingLayerCheckbox.isVisible() ) {
        pos = mAgingLayerCheckbox.getPosition();
        pos.x -= 20;
        smallFont->drawString( "Age Layer", pos, alignRight );
        }
    

    if( mMaleCheckbox.isVisible() ) {
        pos = mMaleCheckbox.getPosition();
        pos.y += 20;
        smallFont->drawString( "Male", pos, alignCenter );
        }

    if( mDeathMarkerCheckbox.isVisible() ) {
        pos = mDeathMarkerCheckbox.getPosition();
        pos.y += 20;
        smallFont->drawString( "Death", pos, alignCenter );
        }
    


    if( mHeadLayerCheckbox.isVisible() ) {
        pos = mHeadLayerCheckbox.getPosition();
        pos.x -= 20;
        smallFont->drawString( "Head", pos, alignRight );
        }

    if( mBodyLayerCheckbox.isVisible() ) {
        pos = mBodyLayerCheckbox.getPosition();
        pos.x -= 20;
        smallFont->drawString( "Body", pos, alignRight );
        }

    if( mBackFootLayerCheckbox.isVisible() ) {
        pos = mBackFootLayerCheckbox.getPosition();
        pos.x -= 20;
        smallFont->drawString( "Back Foot", pos, alignRight );
        }

    if( mFrontFootLayerCheckbox.isVisible() ) {
        pos = mFrontFootLayerCheckbox.getPosition();
        pos.x -= 20;
        smallFont->drawString( "Front Foot", pos, alignRight );
        }



    doublePair legendPos = mImportEditorButton.getPosition();
    
    legendPos.x = -100;
    legendPos.y += 20;
    
    drawKeyLegend( &mKeyLegend, legendPos );


    legendPos = mImportEditorButton.getPosition();
    legendPos.x = -440;
    legendPos.y += 20;
    
    drawKeyLegend( &mKeyLegendB, legendPos );
    }



void EditorObjectPage::step() {
    
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
    
    if( mPickedObjectLayer != -1 ) {
        
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
    }




void EditorObjectPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mSpritePicker.redoSearch();
    mObjectPicker.redoSearch();

    mRotAdjustMode = false;
    }




void EditorObjectPage::clearUseOfSprite( int inSpriteID ) {
    int numUses = 0;
    
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        if( mCurrentObject.sprites[i] == inSpriteID ) {
            numUses ++;
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
            
    mCurrentObject.sprites = newSprites;
    mCurrentObject.spritePos = newSpritePos;
    mCurrentObject.spriteRot = newSpriteRot;
    mCurrentObject.spriteHFlip = newSpriteHFlip;
    mCurrentObject.spriteColor = newSpriteColor;
    mCurrentObject.spriteAgeStart = newSpriteAgeStart;
    mCurrentObject.spriteAgeEnd = newSpriteAgeEnd;
    mCurrentObject.spriteParent = newSpriteParent;
    mCurrentObject.spriteInvisibleWhenHolding = newSpriteInvisibleWhenHolding;
    
    mCurrentObject.numSprites = newNumSprites;
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
    
    double smallestDist = getClosestObjectPart( &mCurrentObject, 
                                                age,
                                                mPickedObjectLayer,
                                                pos.x, pos.y,
                                                outSprite, outSlot );
        


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
    updateAgingPanel();
    }





void EditorObjectPage::pointerMove( float inX, float inY ) {
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
    else {    
        mRotStartMouseX = inX;
        getClosestSpriteOrSlot( inX, inY, &mHoverObjectLayer, &mHoverSlot );
        mHoverStrength = 1;
        }
    }




void EditorObjectPage::pointerDown( float inX, float inY ) {
    mHoverStrength = 0;
    
    if( inX < -96 || inX > 96 || 
        inY < -96 || inY > 160 ) {
        return;
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



static void recursiveMove( ObjectRecord *inObject,
                           int inMovingParentIndex,
                           doublePair inMoveDelta ) {

    // adjust children recursively
    for( int i=0; i<inObject->numSprites; i++ ) {
        if( inObject->spriteParent[i] == inMovingParentIndex ) {
            
            inObject->spritePos[i] =
                add( inObject->spritePos[i], inMoveDelta );
            
            recursiveMove( inObject, i, inMoveDelta );
            }
        }
    
    }




void EditorObjectPage::pointerDrag( float inX, float inY ) {
    mHoverStrength = 0;
    
    if( ! mDragging ) {
        return;
        }

    if( inX < -96 || inX > 96 || 
        inY < -96 || inY > 160 ) {
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
        
        if( centerDist < 200 ) {
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



void EditorObjectPage::keyDown( unsigned char inASCII ) {
    
    if( TextField::isAnyFocused() ) {
        return;
        }
    
    if( mPickedObjectLayer != -1 && inASCII == 'r' ) {
        mRotAdjustMode = true;
        mLayerOldRot = mCurrentObject.spriteRot[ mPickedObjectLayer ];
        }
    if( mPickedObjectLayer != -1 && inASCII == 'c' ) {
        mColorClipboard = mCurrentObject.spriteColor[ mPickedObjectLayer ];
        }
    if( mPickedObjectLayer != -1 && inASCII == 'v' ) {
        mCurrentObject.spriteColor[ mPickedObjectLayer ] = mColorClipboard;
        pickedLayerChanged();
        }
    if( mPickedObjectLayer != -1 && inASCII == 8 ) {
        // backspace
        
        int newNumSprites = mCurrentObject.numSprites - 1;
            
        int *newSprites = new int[ newNumSprites ];
        
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



        char *newSpriteInvisibleWhenHolding = new char[ newNumSprites ];
        
        memcpy( newSpriteInvisibleWhenHolding, 
                mCurrentObject.spriteInvisibleWhenHolding, 
                mPickedObjectLayer * sizeof( char ) );
        
        memcpy( &( newSpriteInvisibleWhenHolding[mPickedObjectLayer] ), 
                &( mCurrentObject.
                     spriteInvisibleWhenHolding[mPickedObjectLayer+1] ), 
                (newNumSprites - mPickedObjectLayer ) * sizeof( char ) );




        delete [] mCurrentObject.sprites;
        delete [] mCurrentObject.spritePos;
        delete [] mCurrentObject.spriteRot;
        delete [] mCurrentObject.spriteHFlip;
        delete [] mCurrentObject.spriteColor;
        delete [] mCurrentObject.spriteAgeStart;
        delete [] mCurrentObject.spriteAgeEnd;
        delete [] mCurrentObject.spriteParent;
        delete [] mCurrentObject.spriteInvisibleWhenHolding;
            
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
            pickedLayerChanged();
            }
        else if( mPickedObjectLayer != -1 ) {
            mPickedObjectLayer ++;
            
            pickedLayerChanged();
                
            if( mPickedObjectLayer >= mCurrentObject.numSprites ) {
                mPickedObjectLayer = -1;
                pickedLayerChanged();

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
                pickedLayerChanged();
                }
            }
        else if( mPickedObjectLayer != -1 ) {
            mPickedObjectLayer --;
            pickedLayerChanged();
            if( mPickedObjectLayer < 0 ) {
                mPickedObjectLayer = -1;
                pickedLayerChanged();
                }
            }
        else if( mPickedSlot != -1 ) {
            mPickedSlot--;
            
            if( mPickedSlot < 0 ) {
                mPickedSlot = -1;
                if( mCurrentObject.numSprites > 0 ) {
                    mPickedObjectLayer = mCurrentObject.numSprites - 1;
                    pickedLayerChanged();
                    }
                }
            }        
        
        mHoverObjectLayer = mPickedObjectLayer;
        mHoverSlot = mPickedSlot;
        mHoverStrength = 1;
        }
    
        
    }


void EditorObjectPage::keyUp( unsigned char inASCII ) {
    
    if( mPickedObjectLayer != -1 && inASCII == 'r' ) {
        mRotAdjustMode = false;
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
        
        
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.spritePos[mPickedObjectLayer].x -= offset;
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.spritePos[mPickedObjectLayer].x += offset;
                break;
            case MG_KEY_DOWN:
                mCurrentObject.spritePos[mPickedObjectLayer].y -= offset;
                break;
            case MG_KEY_UP:
                mCurrentObject.spritePos[mPickedObjectLayer].y += offset;
                break;
            case MG_KEY_PAGE_UP:  {
                int layerOffset = offset;
                if( mPickedObjectLayer + offset 
                    >= mCurrentObject.numSprites ) {
                    
                    layerOffset = 
                        mCurrentObject.numSprites - 1 - mPickedObjectLayer;
                    }
                if( mPickedObjectLayer < 
                    mCurrentObject.numSprites - layerOffset ) {
                

                    // two indices being swapped
                    int indexA = mPickedObjectLayer;
                    int indexB = mPickedObjectLayer + layerOffset;
                    

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

                    mCurrentObject.sprites[mPickedObjectLayer + layerOffset]
                        = mCurrentObject.sprites[mPickedObjectLayer];
                    mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                    mCurrentObject.spritePos[mPickedObjectLayer + layerOffset]
                        = mCurrentObject.spritePos[mPickedObjectLayer];
                    mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;

                    mCurrentObject.spriteRot[mPickedObjectLayer + layerOffset]
                        = mCurrentObject.spriteRot[mPickedObjectLayer];
                    mCurrentObject.spriteRot[mPickedObjectLayer] = tempRot;

                    mCurrentObject.spriteHFlip[mPickedObjectLayer 
                                               + layerOffset]
                        = mCurrentObject.spriteHFlip[mPickedObjectLayer];
                    mCurrentObject.spriteHFlip[mPickedObjectLayer] = tempHFlip;

                    mCurrentObject.spriteColor[mPickedObjectLayer 
                                               + layerOffset]
                        = mCurrentObject.spriteColor[mPickedObjectLayer];
                    mCurrentObject.spriteColor[mPickedObjectLayer] = tempColor;


                    mCurrentObject.spriteAgeStart[mPickedObjectLayer 
                                                  + layerOffset]
                        = mCurrentObject.spriteAgeStart[mPickedObjectLayer];
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
                
                    
                    mPickedObjectLayer += layerOffset;
                    
                    // any children pointing to index A must now
                    // point to B and vice-versa
                    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                        if( mCurrentObject.spriteParent[i] == indexA ) {
                            mCurrentObject.spriteParent[i] = indexB;
                            }
                        else if( mCurrentObject.spriteParent[i] == indexB ) {
                            mCurrentObject.spriteParent[i] = indexA;
                            }
                        }

                    if( mCurrentObject.headIndex == indexA ) {
                        mCurrentObject.headIndex = indexB;
                        }
                    else if( mCurrentObject.headIndex == indexB ) {
                        mCurrentObject.headIndex = indexA;
                        }
                    if( mCurrentObject.bodyIndex == indexA ) {
                        mCurrentObject.bodyIndex = indexB;
                        }
                    else if( mCurrentObject.bodyIndex == indexB ) {
                        mCurrentObject.bodyIndex = indexA;
                        }
                    if( mCurrentObject.backFootIndex == indexA ) {
                        mCurrentObject.backFootIndex = indexB;
                        }
                    else if( mCurrentObject.backFootIndex == indexB ) {
                        mCurrentObject.backFootIndex = indexA;
                        }
                    if( mCurrentObject.frontFootIndex == indexA ) {
                        mCurrentObject.frontFootIndex = indexB;
                        }
                    else if( mCurrentObject.frontFootIndex == indexB ) {
                        mCurrentObject.frontFootIndex = indexA;
                        }                    
                    }
                }
                break;
            case MG_KEY_PAGE_DOWN:
                int layerOffset = offset;
                if( mPickedObjectLayer - offset < 0 ) {
                    layerOffset = mPickedObjectLayer;
                    }

                if( mPickedObjectLayer >= layerOffset ) {

                    // two indices being swapped
                    int indexA = mPickedObjectLayer;
                    int indexB = mPickedObjectLayer - layerOffset;

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

                    mCurrentObject.sprites[mPickedObjectLayer - layerOffset]
                        = mCurrentObject.sprites[mPickedObjectLayer];
                    mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                    mCurrentObject.spritePos[mPickedObjectLayer - layerOffset]
                        = mCurrentObject.spritePos[mPickedObjectLayer];
                    mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;

                    mCurrentObject.spriteRot[mPickedObjectLayer - layerOffset]
                        = mCurrentObject.spriteRot[mPickedObjectLayer];
                    mCurrentObject.spriteRot[mPickedObjectLayer] = tempRot;

                    mCurrentObject.spriteHFlip[mPickedObjectLayer - 
                                               layerOffset]
                        = mCurrentObject.spriteHFlip[mPickedObjectLayer];
                    mCurrentObject.spriteHFlip[mPickedObjectLayer] = tempHFlip;


                    mCurrentObject.spriteColor[mPickedObjectLayer - 
                                               layerOffset]
                        = mCurrentObject.spriteColor[mPickedObjectLayer];
                    mCurrentObject.spriteColor[mPickedObjectLayer] = tempColor;

                
                    mCurrentObject.spriteAgeStart[mPickedObjectLayer 
                                                  - layerOffset]
                        = mCurrentObject.spriteAgeStart[mPickedObjectLayer];
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


                    mPickedObjectLayer -= layerOffset;

                    // any children pointing to index A must now
                    // point to B and vice-versa
                    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
                        if( mCurrentObject.spriteParent[i] == indexA ) {
                            mCurrentObject.spriteParent[i] = indexB;
                            }
                        else if( mCurrentObject.spriteParent[i] == indexB ) {
                            mCurrentObject.spriteParent[i] = indexA;
                            }
                        }
                    
                    if( mCurrentObject.headIndex == indexA ) {
                        mCurrentObject.headIndex = indexB;
                        }
                    else if( mCurrentObject.headIndex == indexB ) {
                        mCurrentObject.headIndex = indexA;
                        }
                    if( mCurrentObject.bodyIndex == indexA ) {
                        mCurrentObject.bodyIndex = indexB;
                        }
                    else if( mCurrentObject.bodyIndex == indexB ) {
                        mCurrentObject.bodyIndex = indexA;
                        }
                    if( mCurrentObject.backFootIndex == indexA ) {
                        mCurrentObject.backFootIndex = indexB;
                        }
                    else if( mCurrentObject.backFootIndex == indexB ) {
                        mCurrentObject.backFootIndex = indexA;
                        }
                    if( mCurrentObject.frontFootIndex == indexA ) {
                        mCurrentObject.frontFootIndex = indexB;
                        }
                    else if( mCurrentObject.frontFootIndex == indexB ) {
                        mCurrentObject.frontFootIndex = indexA;
                        }
                    }
                break;
            
            }
        }
    else if( mPickedSlot != -1 ) {
        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mCurrentObject.slotPos[mPickedSlot].x -= offset;
                break;
            case MG_KEY_RIGHT:
                mCurrentObject.slotPos[mPickedSlot].x += offset;
                break;
            case MG_KEY_DOWN:
                mCurrentObject.slotPos[mPickedSlot].y -= offset;
                break;
            case MG_KEY_UP:
                mCurrentObject.slotPos[mPickedSlot].y += offset;
                break;
            }
        
        }
    
    
            
    }
