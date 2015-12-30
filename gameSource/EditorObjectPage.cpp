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
          mSaveObjectButton( smallFont, 210, -260, "Save New" ),
          mReplaceObjectButton( smallFont, 310, -260, "Replace" ),
          mClearObjectButton( mainFont, 0, 160, "Blank" ),
          mClearRotButton( smallFont, 0, -160, "0 Rot" ),
          mFlipHButton( smallFont, -160, 160, "H Flip" ),
          mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mTransEditorButton( mainFont, 210, 260, "Trans" ),
          mAnimEditorButton( mainFont, 330, 260, "Anim" ),
          mMoreSlotsButton( smallFont, -120, -110, "More" ),
          mLessSlotsButton( smallFont, -120, -190, "Less" ),
          mDemoSlotsButton( smallFont, 150, 32, "Demo Slots" ),
          mClearSlotsDemoButton( smallFont, 150, -32, "End Demo" ),
          mSetHeldPosButton( smallFont, 150, 76, "Held Pos" ),
          mEndSetHeldPosButton( smallFont, 150, -76, "End Held" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mPersonAgeSlider( smallFont, 0, -110, 2,
                            100, 20,
                            0, 100, "Age" ),
          mSlotPlaceholderSprite( loadSprite( "slotPlaceholder.tga" ) ) {

    mDemoSlots = false;
    mSlotsDemoObject = -1;

    mSetHeldPos = false;
    mHeldPosPersonObject = -1;

    addComponent( &mDescriptionField );
    addComponent( &mMapChanceField );
    addComponent( &mHeatValueField );
    addComponent( &mRValueField );
    addComponent( &mFoodValueField );
    addComponent( &mSpeedMultField );

    addComponent( &mSaveObjectButton );
    addComponent( &mReplaceObjectButton );
    addComponent( &mImportEditorButton );
    addComponent( &mTransEditorButton );
    addComponent( &mAnimEditorButton );

    addComponent( &mMoreSlotsButton );
    addComponent( &mLessSlotsButton );
    
    addComponent( &mDemoSlotsButton );
    addComponent( &mClearSlotsDemoButton );

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
    
    mPersonAgeSlider.setVisible( false );
    

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
    
    mCurrentObject.numSlots = 0;
    mCurrentObject.slotPos = new doublePair[ 0 ];
    
    mCurrentObject.numSprites = 0;
    mCurrentObject.sprites = new int[ 0 ];
    mCurrentObject.spritePos = new doublePair[ 0 ];
    mCurrentObject.spriteRot = new double[ 0 ];
    mCurrentObject.spriteHFlip = new char[ 0 ];

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


    double boxY = -150;
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 150, boxY, 2 );
        addComponent( mCheckboxes[i] );

        boxY -= 20;
        }
    mCheckboxNames[0] = "Containable";
    mCheckboxNames[1] = "Permanent";
    mCheckboxNames[2] = "Person";
    
    mCheckboxes[2]->addActionListener( this );
    }



EditorObjectPage::~EditorObjectPage() {

    delete [] mCurrentObject.description;
    delete [] mCurrentObject.slotPos;
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;

    freeSprite( mSlotPlaceholderSprite );
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
        }
    }




float getFloat( TextField *inField ) {
    char *text = inField->getText();
    
    float f = 0;
    
    sscanf( text, "%f", &f );

    delete [] text;
    
    return f;
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
                   mCheckboxes[1]->getToggled(),
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.numSlots, mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip );
        
        delete [] text;
        
        mSpritePicker.unselectObject();

        mObjectPicker.redoSearch();
        actionPerformed( &mClearObjectButton );
        }
    else if( inTarget == &mReplaceObjectButton ) {
        char *text = mDescriptionField.getText();

        addObject( text,
                   mCheckboxes[0]->getToggled(),
                   mCheckboxes[1]->getToggled(),
                   mMapChanceField.getFloat(),
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCheckboxes[2]->getToggled(),
                   mFoodValueField.getInt(),
                   mSpeedMultField.getFloat(),
                   mCurrentObject.heldOffset,
                   mCurrentObject.numSlots, mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
                   mCurrentObject.spriteRot,
                   mCurrentObject.spriteHFlip,
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

        mCurrentObject.numSlots = 0;
        
        mCurrentObject.heldOffset.x = 0;
        mCurrentObject.heldOffset.y = 0;

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
        mHeldPosPersonObject = -1;

        
        mPickedObjectLayer = -1;
        mPickedSlot = -1;
        
        mObjectPicker.unselectObject();
        }
    else if( inTarget == &mClearRotButton ) {
        if( mPickedObjectLayer != -1 ) {
            mCurrentObject.spriteRot[mPickedObjectLayer] = 0;
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
        
        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = slots;

        mCurrentObject.numSlots = numSlots + 1;
        }
    else if( inTarget == &mLessSlotsButton ) {
        if( mCurrentObject.numSlots > 0 ) {
            
            mCurrentObject.numSlots --;
            
            if( mCurrentObject.numSlots == 0 ) {
                mDemoSlots = false;
                mSlotsDemoObject = -1;
                mDemoSlotsButton.setVisible( false );
                mClearSlotsDemoButton.setVisible( false );
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
        mHeldPosPersonObject = getRandomPersonObject();
        
        if( mHeldPosPersonObject != -1 ) {
            
            mSetHeldPos = true;
            mSetHeldPosButton.setVisible( false );
            mEndSetHeldPosButton.setVisible( true );
            }
        }
    else if( inTarget == &mEndSetHeldPosButton ) {
        mSetHeldPos = false;
        mHeldPosPersonObject = -1;

        mSetHeldPosButton.setVisible( true );
        mEndSetHeldPosButton.setVisible( false );
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
            mHeldPosPersonObject = -1;
            mSetHeldPosButton.setVisible( true );
            mEndSetHeldPosButton.setVisible( false );
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
        
            newSprites[ mCurrentObject.numSprites ] = spriteID;
            
            doublePair pos = {0,0};
            
            newSpritePos[ mCurrentObject.numSprites ] = pos;

            newSpriteRot[ mCurrentObject.numSprites ] = 0;

            newSpriteHFlip[ mCurrentObject.numSprites ] = 0;

            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            delete [] mCurrentObject.spriteRot;
            delete [] mCurrentObject.spriteHFlip;
            
            mCurrentObject.sprites = newSprites;
            mCurrentObject.spritePos = newSpritePos;
            mCurrentObject.spriteRot = newSpriteRot;
            mCurrentObject.spriteHFlip = newSpriteHFlip;
            mCurrentObject.numSprites = newNumSprites;

            mClearObjectButton.setVisible( true );
            
            char *text = mDescriptionField.getText();
            
            if( strlen( text ) > 0 ) {
                mSaveObjectButton.setVisible( true );
                }
            delete [] text;

            mPickedObjectLayer = mCurrentObject.numSprites - 1;
            mPickedSlot = -1;
            }
        }
    
    else if( inTarget == &mObjectPicker ) {
        int objectID = mObjectPicker.getSelectedObject();

        
        // auto-end the held-pos setting if a new object is picked
        // (also, potentially enable the setting button for the first time) 
        if( objectID != -1 && ! mDemoSlots ) {
            mSetHeldPos = false;
            mHeldPosPersonObject = -1;
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

            mCurrentObject.id = objectID;
                
            mDescriptionField.setText( pickedRecord->description );

            mMapChanceField.setFloat( pickedRecord->mapChance, 2 );
            
            mHeatValueField.setInt( pickedRecord->heatValue );
            mRValueField.setFloat( pickedRecord->rValue, 2 );
            
            mFoodValueField.setInt( pickedRecord->foodValue );

            mSpeedMultField.setFloat( pickedRecord->speedMult, 2 );


            mCurrentObject.containable = pickedRecord->containable;
            
            mCurrentObject.heldOffset = pickedRecord->heldOffset;


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
                
            memcpy( mCurrentObject.sprites, pickedRecord->sprites,
                    sizeof( int ) * pickedRecord->numSprites );
                
            memcpy( mCurrentObject.spritePos, pickedRecord->spritePos,
                    sizeof( doublePair ) * pickedRecord->numSprites );

            memcpy( mCurrentObject.spriteRot, pickedRecord->spriteRot,
                    sizeof( double ) * pickedRecord->numSprites );
            
            memcpy( mCurrentObject.spriteHFlip, pickedRecord->spriteHFlip,
                    sizeof( char ) * pickedRecord->numSprites );
                

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
            
            if( mCheckboxes[2]->getToggled() ) {
                mPersonAgeSlider.setValue( 20 );
                mPersonAgeSlider.setVisible( true );
                }
            else {
                mPersonAgeSlider.setVisible( false );
                }
            }
        }
    else if( inTarget == mCheckboxes[2] ) {
        if( mCheckboxes[2]->getToggled() ) {
            mPersonAgeSlider.setValue( 20 );
            mPersonAgeSlider.setVisible( true );
            }
        else {
            mPersonAgeSlider.setVisible( false );
            }
        }
    
    
    }

   

     
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    


    
    int i = 0;
    for( int y=0; y<3; y++ ) {
        for( int x=0; x<3; x++ ) {
            
            doublePair pos = { (double)( y * 64 - 64 ), 
                               (double)( x * 64 - 64 ) };
            
            if( i%2 == 0 ) {
                setDrawColor( 1, 1, 1, 1 );
                }
            else {
                setDrawColor( 0.75, 0.75, 0.75, 1 );
                }
            
            drawSquare( pos, 32 );
            i++;
            }
        }
    

    doublePair drawOffset = { 0, 0 };
    

    if( mSetHeldPos ) {
        
        // draw sample person behind
        setDrawColor( 1, 1, 1, 1 );
        
        drawObject( getObject( mHeldPosPersonObject ), drawOffset, false, -1 );

        drawOffset = mCurrentObject.heldOffset;
        }
    


    
    if( mCurrentObject.numSlots > 0 ) {

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
                            false, -1 );
                }
            }
        
        }
    


    

    doublePair pos = { 0, 0 };
    

    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        doublePair spritePos = add( mCurrentObject.spritePos[i], drawOffset );
        
        float blue = 1;
        float red = 1;
        float alpha = 1;
        
        if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
            blue = 1 - mHoverStrength;
            red = 1 - mHoverStrength;
            alpha *= mHoverFlash;
            }
                
        setDrawColor( red, 1, blue, alpha );

        
                        
        if( mPersonAgeSlider.isVisible() &&
            i == mCurrentObject.numSprites - 1 ) {
            
            double age = mPersonAgeSlider.getValue();
            
            spritePos = add( spritePos, getAgeHeadOffset( age, spritePos ) );
            }
                

        drawSprite( getSprite( mCurrentObject.sprites[i] ), spritePos,
                    1.0, mCurrentObject.spriteRot[i],
                    mCurrentObject.spriteHFlip[i] );
        }


    setDrawColor( 1, 1, 1, 1 );
    pos.y -= 150;
    
    pos.x -= 200;

    char *numSlotString = autoSprintf( "Slots: %d", mCurrentObject.numSlots );
    
    mainFont->drawString( numSlotString, pos, alignLeft );

    delete [] numSlotString;


    
    if( mPickedObjectLayer != -1 ) {
        pos.x += 150;
        pos.y -= 50;

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
    
    int j = 0;
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        
        if( mCurrentObject.sprites[i] != inSpriteID ) {
            // not one we're skipping
            newSprites[j] = mCurrentObject.sprites[i];
            newSpritePos[j] = mCurrentObject.spritePos[i];
            newSpriteRot[j] = mCurrentObject.spriteRot[i];
            newSpriteHFlip[j] = mCurrentObject.spriteHFlip[i];
            j++;
            }
        }
    
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    delete [] mCurrentObject.spriteRot;
    delete [] mCurrentObject.spriteHFlip;
            
    mCurrentObject.sprites = newSprites;
    mCurrentObject.spritePos = newSpritePos;
    mCurrentObject.spriteRot = newSpriteRot;
    mCurrentObject.spriteHFlip = newSpriteHFlip;
    mCurrentObject.numSprites = newNumSprites;
    }




double EditorObjectPage::getClosestSpriteOrSlot( float inX, float inY,
                                               int *outSprite,
                                               int *outSlot ) {

    doublePair pos = { inX, inY };
    
    int oldLayerPick = mPickedObjectLayer;
    int oldSlotPick = mPickedSlot;
    

    *outSprite = -1;
    double smallestDist = 9999999;
    
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        
        double dist = distance( pos, mCurrentObject.spritePos[i] );
        
        if( dist < smallestDist ) {
            *outSprite = i;
            
            smallestDist = dist;
            }
        }
    
    *outSlot = -1;
    for( int i=0; i<mCurrentObject.numSlots; i++ ) {
        
        double dist = distance( pos, mCurrentObject.slotPos[i] );
        
        if( dist < smallestDist ) {
            *outSprite = -1;
            *outSlot = i;
            
            smallestDist = dist;
            }
        }


    if( smallestDist > 32 ) {
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





void EditorObjectPage::pointerMove( float inX, float inY ) {
    if( mRotAdjustMode && mPickedObjectLayer != -1 ) {
        // mouse move adjusts rotation
        mCurrentObject.spriteRot[ mPickedObjectLayer ] =
            ( inX - mRotStartMouseX ) / 200;
        }
    else {    
        mRotStartMouseX = inX;
        getClosestSpriteOrSlot( inX, inY, &mHoverObjectLayer, &mHoverSlot );
        mHoverStrength = 1;
        }
    }




void EditorObjectPage::pointerDown( float inX, float inY ) {
    mHoverStrength = 0;
    
    if( inX < -80 || inX > 80 || 
        inY < -80 || inY > 80 ) {
        return;
        }
    
    doublePair pos = { inX, inY };


    if( mSetHeldPos ) {    
        mSetHeldMouseStart = pos;
        mSetHeldOffsetStart = mCurrentObject.heldOffset;
        return;
        }
    
    
    double smallestDist = 
        getClosestSpriteOrSlot( inX, inY, &mPickedObjectLayer, &mPickedSlot );
    
    
    if( mPickedObjectLayer != -1 || mPickedSlot != -1 ) {
        mDescriptionField.unfocusAll();
        
        if( mPickedObjectLayer != -1 ) {
            mPickedMouseOffset = 
                sub( pos, mCurrentObject.spritePos[mPickedObjectLayer] );
            }
        else {
            mPickedMouseOffset = 
                sub( pos, mCurrentObject.slotPos[mPickedSlot] );
            }
        }
    else if( smallestDist < 200 ) {
        mDescriptionField.unfocusAll();
        // dragging whole object?
        doublePair center = {0,0};
        
        mPickedMouseOffset = sub( pos, center );
        }
    }




void EditorObjectPage::pointerDrag( float inX, float inY ) {
    mHoverStrength = 0;
    if( inX < -80 || inX > 80 || 
        inY < -80 || inY > 80 ) {
        return;
        }
    
    if( mSetHeldPos ) {    
        doublePair cur = { inX, inY };
        
        doublePair diff = sub( cur, mSetHeldMouseStart );
        
        mCurrentObject.heldOffset = add( mSetHeldOffsetStart, diff );
        return;
        }


    
    doublePair pos = {inX, inY};

    if( mPickedObjectLayer != -1 ) {
        mCurrentObject.spritePos[mPickedObjectLayer]
            = sub( pos, mPickedMouseOffset );
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
    }



void EditorObjectPage::keyDown( unsigned char inASCII ) {
    
    if( mDescriptionField.isAnyFocused() ) {
        return;
        }
    
    if( mPickedObjectLayer != -1 && inASCII == 'r' ) {
        mRotAdjustMode = true;
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


        delete [] mCurrentObject.sprites;
        delete [] mCurrentObject.spritePos;
        delete [] mCurrentObject.spriteRot;
        delete [] mCurrentObject.spriteHFlip;
            
        mCurrentObject.sprites = newSprites;
        mCurrentObject.spritePos = newSpritePos;
        mCurrentObject.spriteRot = newSpriteRot;
        mCurrentObject.spriteHFlip = newSpriteHFlip;
        mCurrentObject.numSprites = newNumSprites;
        
        mPickedObjectLayer = -1;
        

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
                
                    
                    mPickedObjectLayer += layerOffset;
                    }
                }
                break;
            case MG_KEY_PAGE_DOWN:
                int layerOffset = offset;
                if( mPickedObjectLayer - offset < 0 ) {
                    layerOffset = mPickedObjectLayer;
                    }

                if( mPickedObjectLayer >= layerOffset ) {
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

                
                    mPickedObjectLayer -= layerOffset;
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
