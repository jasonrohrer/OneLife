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
          mCurrentAnim( NULL ),
          mCurrentType( ground ),
          mFrameCount( 0 ) {
    
    addComponent( &mObjectEditorButton );
    addComponent( &mObjectPicker );

    mObjectEditorButton.addActionListener( this );
    mObjectPicker.addActionListener( this );


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
    }



EditorAnimationPage::~EditorAnimationPage() {
    freeCurrentAnim();
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        delete mCheckboxes[i];
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
    
    }

    



void EditorAnimationPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mObjectPicker ) {
        int newPickID = mObjectPicker.getSelectedObject();

        if( newPickID != -1 ) {
            mCurrentObjectID = newPickID;
            
            populateCurrentAnim();
            }
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
            
            populateCurrentAnim();
            }
        
        }
    
    }




void EditorAnimationPage::draw( doublePair inViewCenter, 
                   double inViewSize ) {

    setDrawColor( 1, 1, 1, 1 );
    
    doublePair pos = { 0, 0 };
    
    drawSquare( pos, 128 );

    if( mCurrentObjectID != -1 ) {
        
        if( mCurrentAnim != NULL ) {

            double frameTime = ( mFrameCount / 60.0 ) * frameRateFactor;
            
            drawObjectAnim( mCurrentObjectID, 
                            mCurrentAnim, frameTime, pos );
            }
        else {
            drawObject( getObject( mCurrentObjectID ), pos );
            }
        }
    
    for( int i=0; i<NUM_ANIM_CHECKBOXES; i++ ) {
        pos = mCheckboxes[i]->getPosition();
    
        pos.x -= 20;
        
        smallFont->drawString( mCheckboxNames[i], pos, alignRight );
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




