#include "EditorSpriteTrimPage.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"



extern Font *mainFont;
extern Font *smallFont;


#include "SpritePickable.h"

static SpritePickable spritePickable;


EditorSpriteTrimPage::EditorSpriteTrimPage()
        : mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mSaveButton( mainFont, 0, -260, "Save" ),
          mSpritePicker( &spritePickable, -410, 90 ),
          mPickedSprite( -1 ) {
    
    addComponent( &mImportEditorButton );
    mImportEditorButton.addActionListener( this );

    addComponent( &mSpritePicker );
    mSpritePicker.addActionListener( this );
    }

EditorSpriteTrimPage::~EditorSpriteTrimPage() {
    }



void EditorSpriteTrimPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mImportEditorButton ) {
        setSignal( "importEditor" );
        }
    else if( inTarget == &mSpritePicker ) {
        
        int spriteID = mSpritePicker.getSelectedObject();

        if( spriteID != -1 ) {
            mPickedSprite = spriteID;
            }
        }
    }



void EditorSpriteTrimPage::drawUnderComponents( doublePair inViewCenter, 
                                                double inViewSize ) {
    if( mPickedSprite != -1 ) {
        
        SpriteHandle h = getSprite( mPickedSprite );
        
        if( h != NULL ) {
            
            setDrawColor( 1, 1, 1, 1 );
            doublePair center = { 0, 0 };
            
            SpriteRecord *r = getSpriteRecord( mPickedSprite );
            
            doublePair boxPos = center;
            
            boxPos.x += r->centerAnchorXOffset;
            boxPos.y -= r->centerAnchorYOffset;

            drawRect( center,
                      r->w / 2,
                      r->h / 2 );
            
            drawSprite( h, boxPos );
            }
        
        }
    }



void EditorSpriteTrimPage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }

    mSpritePicker.redoSearch();
    }
