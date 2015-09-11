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

static ObjectPickable objectPickable;



EditorAnimationPage::EditorAnimationPage()
        : mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mCurrentObjectID( -1 ),
          mCurrentSlotDemoID( -1 ),
          mCurrentAnim( NULL ),
          mCurrentType( ground ),
          mCurrentSpriteOrSlot( 0 ),
          mPickSlotDemoButton( smallFont, 180, 0, "Fill Slots" ),
          mPickingSlotDemo( false ),
          mClearSlotDemoButton( smallFont, 180, -60, "Clear Slots" ),
          mNextSpriteOrSlotButton( smallFont, -180, -210, "Next Layer" ),
          mPrevSpriteOrSlotButton( smallFont, -180, -270, "Prev Layer" ),
          mFrameCount( 0 ) {
    
    
    addComponent( &mObjectEditorButton );
    addComponent( &mObjectPicker );

    addComponent( &mPickSlotDemoButton );
    addComponent( &mClearSlotDemoButton );

    addComponent( &mNextSpriteOrSlotButton );
    addComponent( &mPrevSpriteOrSlotButton );
    

    mObjectEditorButton.addActionListener( this );
    mObjectPicker.addActionListener( this );

    mPickSlotDemoButton.addActionListener( this );
    mClearSlotDemoButton.addActionListener( this );

    mNextSpriteOrSlotButton.addActionListener( this );
    mPrevSpriteOrSlotButton.addActionListener( this );

    mClearSlotDemoButton.setVisible( false );

    checkNextPrevVisible();
    
    
    double boxY = -150;
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        mCheckboxes[i] = new CheckboxButton( 150, boxY, 2 );
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



    boxY = 100;
    
    double space = 20;
    double x = -140;
    
    mSliders[0] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 3, "X Osc" );
    mSliders[1] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 64, "X Amp" );
    mSliders[2] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "X Phase" );

    mSliders[3] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 3, "Y Osc" );
    mSliders[4] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 64, "Y Amp" );
    mSliders[5] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Y Phase" );
    
    mSliders[6] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 3, "Rot" );
    mSliders[7] = new ValueSlider( smallFont, x, boxY -= space, 2,
                                   100, 20,
                                   0, 1, "Rot Phase" );
        

    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        addComponent( mSliders[i] );

        mSliders[i]->addActionListener( this );
        mSliders[i]->setVisible( false );
        }
    }



EditorAnimationPage::~EditorAnimationPage() {
    freeCurrentAnim();
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
        }
    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        delete mSliders[i];
        }
    }



void EditorAnimationPage::freeCurrentAnim() {
    if( mCurrentAnim != NULL ) {
        delete [] mCurrentAnim->spriteAnim;
        delete [] mCurrentAnim->slotAnim;
        delete mCurrentAnim;
        mCurrentAnim = NULL;        
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
    
    AnimationRecord *oldRecord =
        getAnimation( mCurrentObjectID, mCurrentType );
    
    
    ObjectRecord *obj = getObject( mCurrentObjectID );
 
    int sprites = obj->numSprites;
    int slots = obj->numSlots;
        
    if( oldRecord == NULL ) {
        // no anim exists
        mCurrentAnim = new AnimationRecord;
        
        mCurrentAnim->objectID = mCurrentObjectID;
        mCurrentAnim->type = mCurrentType;
        
        
        mCurrentAnim->numSprites = sprites;
        mCurrentAnim->numSlots = slots;
        
        mCurrentAnim->spriteAnim = 
            new SpriteAnimationRecord[ sprites ];
        
        mCurrentAnim->slotAnim = 
            new SpriteAnimationRecord[ slots ];
        
        for( int i=0; i<sprites; i++ ) {
            zeroRecord( &( mCurrentAnim->spriteAnim[i] ) );
            }
        for( int i=0; i<slots; i++ ) {
            zeroRecord( &( mCurrentAnim->slotAnim[i] ) );
            }
        }
    else {
        mCurrentAnim = copyRecord( oldRecord );
        
        adjustRecordList( &( mCurrentAnim->spriteAnim ),
                          mCurrentAnim->numSprites,
                          sprites );
        
        mCurrentAnim->numSprites = sprites;
        
        adjustRecordList( &( mCurrentAnim->slotAnim ),
                          mCurrentAnim->numSlots,
                          slots );
        
        mCurrentAnim->numSlots = slots;
        }        
    
    updateSlidersFromAnim();
    }



void EditorAnimationPage::checkNextPrevVisible() {
    if( mCurrentObjectID == -1 ) {
        mNextSpriteOrSlotButton.setVisible( false );
        mPrevSpriteOrSlotButton.setVisible( false );
        return;
        }
    
    ObjectRecord *r = getObject( mCurrentObjectID );

    int num = r->numSprites + r->numSlots;
    
    mNextSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot < num - 1 );
    mPrevSpriteOrSlotButton.setVisible( mCurrentSpriteOrSlot > 0 );
    }



void EditorAnimationPage::updateAnimFromSliders() {
    SpriteAnimationRecord *r;
    
    if( mCurrentSpriteOrSlot > mCurrentAnim->numSprites - 1 ) {
        r = &( mCurrentAnim->slotAnim[ mCurrentSpriteOrSlot -
                                       mCurrentAnim->numSprites ] );
        }
    else {
        r = &( mCurrentAnim->spriteAnim[ mCurrentSpriteOrSlot ] );
        }
    
    
    r->xOscPerSec = mSliders[0]->getValue();
    r->xAmp = mSliders[1]->getValue();
    r->xPhase = mSliders[2]->getValue();
    r->yOscPerSec = mSliders[3]->getValue();
    r->yAmp = mSliders[4]->getValue();
    r->yPhase = mSliders[5]->getValue();
    r->rotPerSec = mSliders[6]->getValue();
    r->rotPhase = mSliders[7]->getValue();
    }



void EditorAnimationPage::updateSlidersFromAnim() {
    SpriteAnimationRecord *r;
    
    if( mCurrentSpriteOrSlot > mCurrentAnim->numSprites - 1 ) {
        r = &( mCurrentAnim->slotAnim[ mCurrentSpriteOrSlot -
                                       mCurrentAnim->numSprites ] );
        }
    else {
        r = &( mCurrentAnim->spriteAnim[ mCurrentSpriteOrSlot ] );
        }
    
    
    mSliders[0]->setValue( r->xOscPerSec );
    mSliders[1]->setValue( r->xAmp );
    mSliders[2]->setValue( r->xPhase );
    mSliders[3]->setValue( r->yOscPerSec );
    mSliders[4]->setValue( r->yAmp );
    mSliders[5]->setValue( r->yPhase );
    mSliders[6]->setValue( r->rotPerSec );
    mSliders[7]->setValue( r->rotPhase );

    for( int i=0; i<NUM_ANIM_SLIDERS; i++ ) {
        mSliders[i]->setVisible( true );
        }
    }

    
    



void EditorAnimationPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
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
            else {
                mCurrentObjectID = newPickID;
            
                mCurrentSpriteOrSlot = 0;
                
                checkNextPrevVisible();
                
                populateCurrentAnim();
                }
            }
        }
    else if( inTarget == &mPickSlotDemoButton ) {
        mPickingSlotDemo = true;
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
    else if( inTarget == &mNextSpriteOrSlotButton ) {
        mCurrentSpriteOrSlot ++;
        checkNextPrevVisible();
        updateSlidersFromAnim();
        }
    else if( inTarget == &mPrevSpriteOrSlotButton ) {
        mCurrentSpriteOrSlot --;
        checkNextPrevVisible();
        updateSlidersFromAnim();
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
            
            mCurrentSpriteOrSlot = 0;
            
            checkNextPrevVisible();
            populateCurrentAnim();
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




void EditorAnimationPage::draw( doublePair inViewCenter, 
                   double inViewSize ) {

    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };
    
    drawSquare( pos, 128 );

    if( mCurrentObjectID != -1 ) {
        
        ObjectRecord *obj = getObject( mCurrentObjectID );

        int *demoSlots = NULL;
        if( mCurrentSlotDemoID != -1 && obj->numSlots > 0 ) {
            demoSlots = new int[ obj->numSlots ];
            for( int i=0; i<obj->numSlots; i++ ) {
                demoSlots[i] = mCurrentSlotDemoID;
                }
            }
        

        if( mCurrentAnim != NULL ) {

            double frameTime = ( mFrameCount / 60.0 ) * frameRateFactor;
            
            
            if( demoSlots != NULL ) {
                drawObjectAnim( mCurrentObjectID, 
                                mCurrentAnim, frameTime, pos,
                                obj->numSlots, demoSlots );
                }
            else {
                drawObjectAnim( mCurrentObjectID, 
                                mCurrentAnim, frameTime, pos );
                }
            }
        else {
            if( mCurrentSlotDemoID != -1 && obj->numSlots > 0 ) {
                drawObject( obj, pos,
                            obj->numSlots, demoSlots );
                }
            else {
                drawObject( obj, pos );
                }
            }
        
        if( demoSlots != NULL ) {
            delete [] demoSlots;
            }
        }
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        pos = mCheckboxes[i]->getPosition();
    
        pos.x -= 20;
        
        smallFont->drawString( mCheckboxNames[i], pos, alignRight );
        }

    if( mCurrentObjectID != -1 ) {
        
        setDrawColor( 1, 1, 1, 1 );
        
        pos = mNextSpriteOrSlotButton.getPosition();
        
        pos.y -= 30;
        
        const char *tag;
        int num;
        
        
        if( mCurrentSpriteOrSlot > mCurrentAnim->numSprites - 1 ) {
            tag = "Slot";
            num = mCurrentSpriteOrSlot - mCurrentAnim->numSprites;
            }
        else {
            tag = "Sprite";
            num = mCurrentSpriteOrSlot;
            }

        char *string = autoSprintf( "%s %d", tag, num );
        smallFont->drawString( string, pos, alignLeft );
        delete [] string;
        }
    
    
    }




void EditorAnimationPage::step() {
    mFrameCount++;
    }




void EditorAnimationPage::makeActive( char inFresh ) {
    }





void EditorAnimationPage::pointerMove( float inX, float inY ) {
    }



void EditorAnimationPage::pointerDown( float inX, float inY ) {
    }



void EditorAnimationPage::pointerDrag( float inX, float inY ) {
    }



void EditorAnimationPage::pointerUp( float inX, float inY ) {
    }




void EditorAnimationPage::keyDown( unsigned char inASCII ) {
    /*
    if( inASCII == 'o' ) {
        if( mCurrentObjectID != -1 && mCurrentAnim != NULL ) {
            mCurrentAnim->spriteAnim[0].yOscPerSec = 1;
            mCurrentAnim->spriteAnim[0].yAmp += 2;
            
            mCurrentAnim->spriteAnim[1].rotPerSec += 0.25;
            }
        }
    else if( inASCII == 'm' ) {
        toggleLinearMagFilter( false );
        }
    else if( inASCII == 'M' ) {
        toggleLinearMagFilter( true );
        }
    */
    }



void EditorAnimationPage::specialKeyDown( int inKeyCode ) {
    }




