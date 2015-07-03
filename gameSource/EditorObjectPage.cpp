#include "EditorObjectPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;


#include "SpritePickable.h"

static SpritePickable spritePickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;



EditorObjectPage::EditorObjectPage()
        : mDescriptionField( mainFont, 
                             0,  -260, 6,
                             false,
                             "Description", NULL, NULL ),
          mSaveObjectButton( mainFont, 210, -260, "Save" ),
          mImportEditorButton( mainFont, 210, 260, "Sprites" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mObjectPicker( &objectPickable, +310, 100 ) {

    addComponent( &mDescriptionField );
    addComponent( &mSaveObjectButton );
    addComponent( &mImportEditorButton );
    addComponent( &mSpritePicker );
    addComponent( &mObjectPicker );

    mDescriptionField.setFireOnAnyTextChange( true );
    mDescriptionField.addActionListener( this );
    

    mSaveObjectButton.addActionListener( this );
    mImportEditorButton.addActionListener( this );

    mSpritePicker.addActionListener( this );
    mObjectPicker.addActionListener( this );

    mSaveObjectButton.setVisible( false );


    mCurrentObject.id = -1;
    mCurrentObject.description = mDescriptionField.getText();
    mCurrentObject.numSprites = 0;
    mCurrentObject.sprites = new int[ 0 ];
    mCurrentObject.spritePos = new doublePair[ 0 ];

    mPickedObjectLayer = -1;
    }



EditorObjectPage::~EditorObjectPage() {

    delete [] mCurrentObject.description;
    delete [] mCurrentObject.sprites;
    delete [] mCurrentObject.spritePos;
    }








void EditorObjectPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mDescriptionField ) {
        char *text = mDescriptionField.getText();
        

        mSaveObjectButton.setVisible(
            ( mCurrentObject.numSprites > 0 )
            &&
            ( strcmp( text, "" ) != 0 ) );
        
        delete [] text;

        mObjectPicker.redoSearch();
        }
    else if( inTarget == &mSaveObjectButton ) {
        char *text = mDescriptionField.getText();

        addObject( text,
                   mCurrentObject.numSprites, mCurrentObject.sprites, 
                   mCurrentObject.spritePos );
        
        delete [] text;
        
        mObjectPicker.redoSearch();
        }
    else if( inTarget == &mImportEditorButton ) {
        setSignal( "importEditor" );
        }
    else if( inTarget == &mSpritePicker ) {
        
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
            
            newSpritePos[ mCurrentObject.numSprites] = pos;

            delete [] mCurrentObject.sprites;
            delete [] mCurrentObject.spritePos;
            
            mCurrentObject.sprites = newSprites;
            mCurrentObject.spritePos = newSpritePos;
            mCurrentObject.numSprites = newNumSprites;
            
            mSpritePicker.unselectObject();            
            }
        
        }
    
    
    }

   

     
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    


    
    int i = 0;
    for( int y=0; y<5; y++ ) {
        for( int x=0; x<5; x++ ) {
            
            doublePair pos = { y * 32 - 2 * 32, 
                               x * 32 - 2 * 32};
            
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


    setDrawColor( 1, 1, 1, 1 );

    doublePair pos = { 0, 0 };
    
    drawObject( &mCurrentObject, pos );
    }



void EditorObjectPage::step() {
    }




void EditorObjectPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mSpritePicker.redoSearch();
    mObjectPicker.redoSearch();

    }



void EditorObjectPage::pointerMove( float inX, float inY ) {
    }


void EditorObjectPage::pointerDown( float inX, float inY ) {

    doublePair pos = { inX, inY };
    
    mPickedObjectLayer = -1;
    double smallestDist = 9999999;
    
    for( int i=0; i<mCurrentObject.numSprites; i++ ) {
        
        double dist = distance( pos, mCurrentObject.spritePos[i] );
        
        if( dist < smallestDist ) {
            mPickedObjectLayer = i;
            
            smallestDist = dist;
            }
        }
    

    if( smallestDist > 100 ) {
        // too far to count as a pick
        mPickedObjectLayer = -1;
        }
    
    
    if( mPickedObjectLayer != -1 ) {
        mDescriptionField.unfocus();
        
        mPickedMouseOffset = 
            sub( pos, mCurrentObject.spritePos[mPickedObjectLayer] );
        }
    
    }



void EditorObjectPage::pointerDrag( float inX, float inY ) {
    
    if( mPickedObjectLayer != -1 ) {
        doublePair pos = {inX, inY};
        

        mCurrentObject.spritePos[mPickedObjectLayer]
            = sub( pos, mPickedMouseOffset );
        }
    }



void EditorObjectPage::pointerUp( float inX, float inY ) {
    }



void EditorObjectPage::keyDown( unsigned char inASCII ) {
    if( mPickedObjectLayer == -1 ) {
        return;
        }
    
    if( inASCII == 8 ) {
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
            }
        }
    
        
    }



void EditorObjectPage::specialKeyDown( int inKeyCode ) {
    if( mPickedObjectLayer == -1 ) {
        return;
        }
    
    switch( inKeyCode ) {
        case MG_KEY_LEFT:
            mCurrentObject.spritePos[mPickedObjectLayer].x -= 1;
            break;
        case MG_KEY_RIGHT:
            mCurrentObject.spritePos[mPickedObjectLayer].x += 1;
            break;
        case MG_KEY_DOWN:
            mCurrentObject.spritePos[mPickedObjectLayer].y -= 1;
            break;
        case MG_KEY_UP:
            mCurrentObject.spritePos[mPickedObjectLayer].y += 1;
            break;
        case MG_KEY_PAGE_UP:
            if( mPickedObjectLayer < mCurrentObject.numSprites - 1 ) {
                int tempSprite = mCurrentObject.sprites[mPickedObjectLayer+1];
                doublePair tempPos = 
                    mCurrentObject.spritePos[mPickedObjectLayer+1];
                
                mCurrentObject.sprites[mPickedObjectLayer+1]
                    = mCurrentObject.sprites[mPickedObjectLayer];
                mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                mCurrentObject.spritePos[mPickedObjectLayer+1]
                    = mCurrentObject.spritePos[mPickedObjectLayer];
                mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;
                
                mPickedObjectLayer++;
                }
            break;
        case MG_KEY_PAGE_DOWN:
            if( mPickedObjectLayer > 0 ) {
                int tempSprite = mCurrentObject.sprites[mPickedObjectLayer-1];
                doublePair tempPos = 
                    mCurrentObject.spritePos[mPickedObjectLayer-1];
                
                mCurrentObject.sprites[mPickedObjectLayer-1]
                    = mCurrentObject.sprites[mPickedObjectLayer];
                mCurrentObject.sprites[mPickedObjectLayer] = tempSprite;
                
                mCurrentObject.spritePos[mPickedObjectLayer-1]
                    = mCurrentObject.spritePos[mPickedObjectLayer];
                mCurrentObject.spritePos[mPickedObjectLayer] = tempPos;
                
                mPickedObjectLayer--;
                }
            break;
            
        }
    
            
    }
