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
          mPickedSprite( -1 ),
          mPickingRect( false ) {
    
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
            
            mPickingRect = false;
            mRects.deleteAll();
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


            for( int i=0; i<mRects.size(); i++ ) {
            
                PickedRect r = mRects.getElementDirect( i );

                
                setDrawColor( 1, 0, 0, 0.5 );
                
                drawRect( r.xStart, r.yStart,
                          r.xEnd, r.yEnd );
                }
            

            if( mPickingRect ) {
                
                setDrawColor( 0, 0, 1, 0.5 );
                
                drawRect( mPickStartX, mPickStartY,
                          mPickEndX, mPickEndY );
                }

            }
        
        }
    }



void EditorSpriteTrimPage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }

    mSpritePicker.redoSearch();
    }



char EditorSpriteTrimPage::isPointInSprite( int inX, int inY ) {
    if( mPickedSprite != -1 ) {
        
        SpriteHandle h = getSprite( mPickedSprite );
        
        if( h != NULL ) {
            
            doublePair boxPos = { 0, 0 };
            
            SpriteRecord *r = getSpriteRecord( mPickedSprite );
            
            boxPos.x += r->centerAnchorXOffset;
            boxPos.y -= r->centerAnchorYOffset;

            if( inX >= - r->w && 
                inX <= r->w &&
                inY >= - r->h &&
                inY <= r->h ) {
                
                return true;
                }
            }
        }

    return false;
    }




void EditorSpriteTrimPage::pointerDown( float inX, float inY ) {
    int x = lrint( inX );
    int y = lrint( inY );
    
    mPickingRect = false;
    
    if( isPointInSprite( x, y ) ) {
        mPickingRect = true;
        mPickStartX = x;
        mPickStartY = y;
        mPickEndX = x;
        mPickEndY = y;
        }
    }


void EditorSpriteTrimPage::pointerDrag( float inX, float inY ) {
    int x = lrint( inX );
    int y = lrint( inY );
    
    if( isPointInSprite( x, y ) ) {
        mPickEndX = x;
        mPickEndY = y;
        }
    }


void EditorSpriteTrimPage::pointerUp( float inX, float inY ) {
    if( mPickingRect ) {
    
        int x = lrint( inX );
        int y = lrint( inY );
        
        if( isPointInSprite( x, y ) ) {
            mPickEndX = x;
            mPickEndY = y;
            }


        PickedRect r = { mPickStartX, mPickStartY, mPickEndX, mPickEndY };
        
        printf( "Rect start = (%d,%d), end = (%d,%d)\n",
                mPickStartX, mPickStartY, mPickEndX, mPickEndY );
        

        char skip = false;
        
        for( int i=0; i<mRects.size(); i++ ) {
            PickedRect otherR = mRects.getElementDirect( i );
            
            if( otherR.xStart >= r.xStart &&
                otherR.xEnd <= r.xEnd &&
                otherR.yStart <= r.yStart &&
                otherR.yEnd >= r.yEnd ) {
                
                skip = true;
                break;
                }
            else {

                if( r.xStart >= otherR.xStart &&
                    r.xStart <= otherR.xEnd &&
                    r.xEnd >= otherR.xStart &&
                    r.xEnd <= otherR.xEnd &&
                    r.yStart <= otherR.yStart &&
                    r.yStart >= otherR.yEnd ) {
                    
                    // top edge intersects
                    r.yStart = otherR.yEnd - 1;
                    }


                if( otherR.xStart >= r.xStart &&
                    otherR.xStart <= r.xEnd &&
                    otherR.xEnd >= r.xStart &&
                    otherR.xEnd <= r.xEnd &&
                    r.yStart <= otherR.yStart &&
                    r.yStart >= otherR.yEnd ) {
                    
                    // top edge intersects (new rect swallows)
                    r.yStart = otherR.yEnd - 1;
                    }


                if( r.xStart >= otherR.xStart &&
                    r.xStart <= otherR.xEnd &&
                    r.xEnd >= otherR.xStart &&
                    r.xEnd <= otherR.xEnd &&
                    r.yEnd <= otherR.yStart &&
                    r.yEnd >= otherR.yEnd ) {
                    
                    // bottom edge intersects
                    r.yEnd = otherR.yStart + 1;
                    }

                if( otherR.xStart >= r.xStart &&
                    otherR.xStart <= r.xEnd &&
                    otherR.xEnd >= r.xStart &&
                    otherR.xEnd <= r.xEnd &&
                    r.yEnd <= otherR.yStart &&
                    r.yEnd >= otherR.yEnd ) {
                    
                    // bottom edge intersects (new rect swallows)
                    r.yEnd = otherR.yStart + 1;
                    }

                
                if( otherR.yStart <= r.yStart &&
                    otherR.yStart >= r.yEnd &&
                    otherR.yEnd <= r.yStart &&
                    otherR.yEnd >= r.yEnd &&
                    r.xStart >= otherR.xStart &&
                    r.xStart <= otherR.xEnd ) {
                    
                    // left edge intersects (new rect swallows)
                    r.xStart = otherR.xEnd + 1;
                    }

                if( otherR.yStart <= r.yStart &&
                    otherR.yStart >= r.yEnd &&
                    otherR.yEnd <= r.yStart &&
                    otherR.yEnd >= r.yEnd &&
                    r.xEnd >= otherR.xStart &&
                    r.xEnd <= otherR.xEnd ) {
                    
                    // right edge intersects (new rect swallows)
                    r.xEnd = otherR.xStart - 1;
                    }


                
                if( r.xStart >= otherR.xStart &&
                    r.xStart <= otherR.xEnd &&
                    r.yStart <= otherR.yStart &&
                    r.yStart >= otherR.yEnd ) {
                    
                    // top left corner intersects
                    r.xStart = otherR.xEnd + 1;
                    }
                
                if( r.xEnd >= otherR.xStart &&
                    r.xEnd <= otherR.xEnd &&
                    r.yStart <= otherR.yStart &&
                    r.yStart >= otherR.yEnd ) {
                    
                    // top right corner intersects
                    r.xEnd = otherR.xStart - 1;
                    }

                if( r.xStart >= otherR.xStart &&
                    r.xStart <= otherR.xEnd &&
                    r.yEnd <= otherR.yStart &&
                    r.yEnd >= otherR.yEnd ) {
                    
                    // bottom left corner intersects
                    r.xStart = otherR.xEnd + 1;
                    }
                
                if( r.xEnd >= otherR.xStart &&
                    r.xEnd <= otherR.xEnd &&
                    r.yEnd <= otherR.yStart &&
                    r.yEnd >= otherR.yEnd ) {
                    
                    // bottom right corner intersects
                    r.xEnd = otherR.xStart - 1;
                    }
                }
                
            }
        
        if( !skip ) {
            mRects.push_back( r );
            }
        
        }
    
    
    mPickingRect = false;
    }

