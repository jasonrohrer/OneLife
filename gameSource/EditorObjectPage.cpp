#include "EditorObjectPage.h"

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
          mHeatValueField( smallFont, 
                           -150,  32, 4,
                           false,
                           "Heat", "0123456789", NULL ),
          mRValueField( smallFont, 
                        -150,  0, 4,
                        false,
                        "R", "0123456789.", NULL ),
          mSaveObjectButton( smallFont, 210, -260, "Save New" ),
          mReplaceObjectButton( smallFont, 310, -260, "Replace" ),
          mClearObjectButton( mainFont, 0, 160, "Blank" ),
          mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mTransEditorButton( mainFont, 210, 260, "Trans" ),
          mAnimEditorButton( mainFont, 330, 260, "Anim" ),
          mMoreSlotsButton( smallFont, -120, -110, "More" ),
          mLessSlotsButton( smallFont, -120, -190, "Less" ),
          mDemoSlotsButton( smallFont, 150, 32, "Demo Slots" ),
          mClearSlotsDemoButton( smallFont, 150, -32, "End Demo" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mSlotPlaceholderSprite( loadSprite( "slotPlaceholder.tga" ) ) {

    mDemoSlots = false;
    mSlotsDemoObject = -1;

    addComponent( &mDescriptionField );
    addComponent( &mHeatValueField );
    addComponent( &mRValueField );

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
    

    addComponent( &mClearObjectButton );

    addComponent( &mSpritePicker );
    addComponent( &mObjectPicker );

    mDescriptionField.setFireOnAnyTextChange( true );
    mDescriptionField.addActionListener( this );
    

    mSaveObjectButton.addActionListener( this );
    mReplaceObjectButton.addActionListener( this );
    mImportEditorButton.addActionListener( this );
    mTransEditorButton.addActionListener( this );
    mAnimEditorButton.addActionListener( this );

    mClearObjectButton.addActionListener( this );

    mMoreSlotsButton.addActionListener( this );
    mLessSlotsButton.addActionListener( this );
    
    mDemoSlotsButton.addActionListener( this );
    mClearSlotsDemoButton.addActionListener( this );
    

    mSpritePicker.addActionListener( this );
    mObjectPicker.addActionListener( this );

    mSaveObjectButton.setVisible( false );
    mReplaceObjectButton.setVisible( false );
    mClearObjectButton.setVisible( false );

    mCurrentObject.id = -1;
    mCurrentObject.description = mDescriptionField.getText();
    mCurrentObject.containable = 0;
    mCurrentObject.numSlots = 0;
    mCurrentObject.slotPos = new doublePair[ 0 ];
    
    mCurrentObject.numSprites = 0;
    mCurrentObject.sprites = new int[ 0 ];
    mCurrentObject.spritePos = new doublePair[ 0 ];

    mPickedObjectLayer = -1;
    mPickedSlot = -1;
    
    mHoverObjectLayer = -1;
    mHoverSlot = -1;
    mHoverStrength = 0;
    mHoverFrameCount = 0;
    

    mHeatValueField.setText( "0" );
    mRValueField.setText( "0.0" );


    double boxY = -150;
    
    for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 150, boxY, 2 );
        addComponent( mCheckboxes[i] );

        boxY -= 20;
        }
    mCheckboxNames[0] = "Containable";
    mCheckboxNames[1] = "Permanent";
    
    }



EditorObjectPage::~EditorObjectPage() {

    delete [] mCurrentObject.description;
    delete [] mCurrentObject.slotPos;
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;

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
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCurrentObject.numSlots, mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos );
        
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
                   mHeatValueField.getInt(),
                   mRValueField.getFloat(),
                   mCurrentObject.numSlots, mCurrentObject.slotPos,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos,
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
        
        
        for( int i=0; i<NUM_OBJECT_CHECKBOXES; i++ ) {
            mCheckboxes[i]->setToggled( false );
            }

        mHeatValueField.setText( "0" );
        mRValueField.setText( "0.0" );

        mCurrentObject.numSlots = 0;
        
        delete [] mCurrentObject.slotPos;
        mCurrentObject.slotPos = new doublePair[ 0 ];

        
        mCurrentObject.numSprites = 0;
        
        delete [] mCurrentObject.sprites;
        mCurrentObject.sprites = new int[ 0 ];

        delete [] mCurrentObject.spritePos;
        mCurrentObject.spritePos = new doublePair[ 0 ];
        
        mSaveObjectButton.setVisible( false );
        mReplaceObjectButton.setVisible( false );
        mClearObjectButton.setVisible( false );
        
        
        mPickedObjectLayer = -1;
        mPickedSlot = -1;
        
        mObjectPicker.unselectObject();
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
    else if( inTarget == &mSpritePicker ) {
        
        if( mDemoSlots ) {
            mDemoSlots = false;
            mSlotsDemoObject = -1;
            mDemoSlotsButton.setVisible( true );
            mClearSlotsDemoButton.setVisible( false );
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
        
            newSprites[ mCurrentObject.numSprites ] = spriteID;
            
            doublePair pos = {0,0};
            
            newSpritePos[ mCurrentObject.numSprites ] = pos;

            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            
            mCurrentObject.sprites = newSprites;
            mCurrentObject.spritePos = newSpritePos;
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

        if( objectID != -1 && mDemoSlots ) {
            mSlotsDemoObject = objectID;
            }
        else if( objectID != -1 ) {
            ObjectRecord *pickedRecord = getObject( objectID );

                
            delete [] mCurrentObject.slotPos;
            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;

            mCurrentObject.id = objectID;
                
            mDescriptionField.setText( pickedRecord->description );

            mHeatValueField.setInt( pickedRecord->heatValue );
            mRValueField.setFloat( pickedRecord->rValue, 2 );
            

            mCurrentObject.containable = pickedRecord->containable;
            
            mCurrentObject.numSlots = pickedRecord->numSlots;

            mCurrentObject.slotPos = 
                new doublePair[ pickedRecord->numSlots ];
                
            memcpy( mCurrentObject.slotPos, pickedRecord->slotPos,
                    sizeof( doublePair ) * pickedRecord->numSlots );

            mCurrentObject.numSprites = pickedRecord->numSprites;
                
            mCurrentObject.sprites = new int[ pickedRecord->numSprites ];
            mCurrentObject.spritePos = 
                new doublePair[ pickedRecord->numSprites ];
                
            memcpy( mCurrentObject.sprites, pickedRecord->sprites,
                    sizeof( int ) * pickedRecord->numSprites );
                
            memcpy( mCurrentObject.spritePos, pickedRecord->spritePos,
                    sizeof( doublePair ) * pickedRecord->numSprites );
                

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
            
            }
        }
    
    
    }

   

     
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    


    
    int i = 0;
    for( int y=0; y<5; y++ ) {
        for( int x=0; x<5; x++ ) {
            
	  doublePair pos = { (double)( y * 32 - 2 * 32 ), 
			     (double)( x * 32 - 2 * 32 ) };
            
            if( i%2 == 0 ) {
                setDrawColor( 1, 1, 1, 1 );
                }
            else {
                setDrawColor( 0.75, 0.75, 0.75, 1 );
                }
            
            drawSquare( pos, 16 );
            i++;
            }
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
                            mCurrentObject.slotPos[i] );
                
                
                setDrawColor( 0, 1, 1, 0.5 );
                
                char *numberString = autoSprintf( "%d", i + 1 );
                
                mainFont->drawString( numberString, mCurrentObject.slotPos[i],
                                      alignCenter );
                
                delete [] numberString;
                }
            }
        else {
            ObjectRecord *demoObject = getObject( mSlotsDemoObject );
            
            setDrawColor( 1, 1, 1, 1 );
            
            for( int i=0; i<mCurrentObject.numSlots; i++ ) {
                drawObject( demoObject, mCurrentObject.slotPos[i] );
                }
            }
        
        }
    


    

    doublePair pos = { 0, 0 };
    

    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        doublePair spritePos = add( mCurrentObject.spritePos[i], pos );
        
        float blue = 1;
        float red = 1;
        float alpha = 1;
        
        if( mHoverObjectLayer == i && mHoverStrength > 0 ) {
            blue = 1 - mHoverStrength;
            red = 1 - mHoverStrength;
            alpha *= mHoverFlash;
            }
                
        setDrawColor( red, 1, blue, alpha );
        drawSprite( getSprite( mCurrentObject.sprites[i] ), spritePos );
        }


    setDrawColor( 1, 1, 1, 1 );
    pos.y -= 150;
    
    pos.x -= 200;

    char *numSlotString = autoSprintf( "Slots: %d", mCurrentObject.numSlots );
    
    mainFont->drawString( numSlotString, pos, alignLeft );

    delete [] numSlotString;
    

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
    
    
    }




void EditorObjectPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mSpritePicker.redoSearch();
    mObjectPicker.redoSearch();

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
    
    int j = 0;
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        
        if( mCurrentObject.sprites[i] != inSpriteID ) {
            // not one we're skipping
            newSprites[j] = mCurrentObject.sprites[i];
            newSpritePos[j] = mCurrentObject.spritePos[i];
            j++;
            }
        }
    
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
            
    mCurrentObject.sprites = newSprites;
    mCurrentObject.spritePos = newSpritePos;
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
    getClosestSpriteOrSlot( inX, inY, &mHoverObjectLayer, &mHoverSlot );
    mHoverStrength = 1;
    }




void EditorObjectPage::pointerDown( float inX, float inY ) {
    mHoverStrength = 0;
    
    if( inX < -80 || inX > 80 || 
        inY < -80 || inY > 80 ) {
        return;
        }
    
    doublePair pos = { inX, inY };
    
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
    
    if( mPickedObjectLayer == -1 && inASCII == 8 ) {
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

        delete [] mCurrentObject.sprites;
        delete [] mCurrentObject.spritePos;
            
        mCurrentObject.sprites = newSprites;
        mCurrentObject.spritePos = newSpritePos;
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
                
                    mCurrentObject.sprites[mPickedObjectLayer + layerOffset]
                        = mCurrentObject.sprites[mPickedObjectLayer];
                    mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                    mCurrentObject.spritePos[mPickedObjectLayer + layerOffset]
                        = mCurrentObject.spritePos[mPickedObjectLayer];
                    mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;
                
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
                
                    mCurrentObject.sprites[mPickedObjectLayer - layerOffset]
                        = mCurrentObject.sprites[mPickedObjectLayer];
                    mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                    mCurrentObject.spritePos[mPickedObjectLayer - layerOffset]
                        = mCurrentObject.spritePos[mPickedObjectLayer];
                    mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;
                
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
