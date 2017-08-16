#include "EditorScenePage.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



extern Font *mainFont;
extern Font *smallFont;


#include "GroundPickable.h"

static GroundPickable groundPickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;


static double multAmount = 0.15;
static double addAmount = 0.25;



EditorScenePage::EditorScenePage()
        : mAnimEditorButton( mainFont, -210, 260, "Anim" ),
          mSaveNewButton( mainFont, 400, 260, "Save" ),
          mGroundPicker( &groundPickable, -410, 90 ),
          mObjectPicker( &objectPickable, 410, 90 ),
          mCurX( 0 ),
          mCurY( 0 ) {
    
    addComponent( &mAnimEditorButton );
    mAnimEditorButton.addActionListener( this );

    addComponent( &mGroundPicker );
    mGroundPicker.addActionListener( this );

    addComponent( &mObjectPicker );
    mObjectPicker.addActionListener( this );


    addComponent( &mSaveNewButton );
    mSaveNewButton.addActionListener( this );


    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }

    SceneCell emptyCell;
    
    emptyCell.biome = -1;
    
    for( int y=0; y<SCENE_H; y++ ) {
        for( int x=0; x<SCENE_W; x++ ) {
            mCells[y][x] = emptyCell;
            }
        }
    

    }



EditorScenePage::~EditorScenePage() {
    for( int i=0; i<4; i++ ) {
        freeSprite( mGroundOverlaySprite[i] );
        }
    }



void EditorScenePage::floodFill( int inX, int inY, 
                                 int inOldBiome, int inNewBiome ) {
    
    if( inX < 0 || inX >= SCENE_W ||
        inY < 0 || inY >= SCENE_H ) {
        return;
        }
    

    if( mCells[inY][inX].biome == inOldBiome &&
        mCells[inY][inX].biome != inNewBiome ) {
        
        mCells[inY][inX].biome = inNewBiome;
        
        floodFill( inX - 1, inY, inOldBiome, inNewBiome );
        floodFill( inX + 1, inY, inOldBiome, inNewBiome );
        
        floodFill( inX, inY - 1, inOldBiome, inNewBiome );
        floodFill( inX, inY + 1, inOldBiome, inNewBiome );
        }
    }



void EditorScenePage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mAnimEditorButton ) {
        setSignal( "animEditor" );
        }
    else if( inTarget == &mGroundPicker ) {
        
        char wasRightClick = false;
        
        int biome = mGroundPicker.getSelectedObject( &wasRightClick );
        
        if( biome >= 0 ) {
            
            if( wasRightClick ) {
                
                floodFill( mCurX, mCurY,
                           mCells[ mCurY ][ mCurX ].biome,
                           biome );
                }
            else {
                // single cell
                mCells[ mCurY ][ mCurX ].biome = biome;
                }
            }
        }
    else if( inTarget == &mObjectPicker ) {
        
        }
    else if( inTarget == &mSaveNewButton ) {
        }
    }


void EditorScenePage::drawGroundOverlaySprites() {
    doublePair cornerPos = { - 640 + 512, 360 - 512 };
    for( int y=0; y<1; y++ ) {
        for( int x=0; x<2; x++ ) {
            doublePair pos = cornerPos;
            pos.x += x * 1024;
            pos.y -= y * 1024;
            
            int tileY = y;
            int tileX = x % 2;
            
            int tileI = tileY * 2 + tileX;
            
            drawSprite( mGroundOverlaySprite[tileI], pos );
            }
        }
    }




void EditorScenePage::drawUnderComponents( doublePair inViewCenter, 
                                           double inViewSize ) {
    
    doublePair cornerPos = { - 640, 360 };
    

    for( int y=0; y<SCENE_H; y++ ) {
        for( int x=0; x<SCENE_W; x++ ) {
            doublePair pos = cornerPos;
                
            pos.x += x * 128;
            pos.y -= y * 128;
                
            SceneCell *c = &( mCells[y][x] );
            
            if( c->biome != -1 ) {
                
                
                GroundSpriteSet *s = groundSprites[ c->biome ];
                
                if( s  != NULL ) {

                    int tileX = x % s->numTilesWide;
                    int tileY = y % s->numTilesWide;
                    
                    setDrawColor( 1, 1, 1, 1 );
                    drawSprite( s->tiles[tileY][tileX], pos );
                    }
                }

            }
        }



    toggleMultiplicativeBlend( true );
    
    // use this to lighten ground overlay
    toggleAdditiveTextureColoring( true );
    setDrawColor( multAmount, multAmount, multAmount, 1 );
    
    drawGroundOverlaySprites();

    toggleAdditiveTextureColoring( false );
    toggleMultiplicativeBlend( false );


    toggleAdditiveBlend( true );
    
    // use this to lighten ground overlay
    //toggleAdditiveTextureColoring( true );
    setDrawColor( 1, 1, 1, addAmount );

    drawGroundOverlaySprites();

    
    toggleAdditiveBlend( false );


    
    doublePair curPos = cornerPos;
                
    curPos.x += mCurX * 128;
    curPos.y -= mCurY * 128;

    // draw a frame around the current cell
    startAddingToStencil( false, true );
    
    setDrawColor( 1, 1, 1, 1 );
    
    drawSquare( curPos, 62 );

    startDrawingThroughStencil( true );
    
    setDrawColor( 1, 1, 1, 0.75 );
    drawSquare( curPos, 64 );

    stopStencil();
 
    }



void EditorScenePage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }
    
    TextField::unfocusAll();
    
    mGroundPicker.redoSearch( true );
    mObjectPicker.redoSearch( true );
    }




void EditorScenePage::specialKeyDown( int inKeyCode ) {
    
    if( TextField::isAnyFocused() ) {
        return;
        }


    int offset = 1;
    
    if( isCommandKeyDown() ) {
        offset = 2;
        }
    if( isShiftKeyDown() ) {
        offset = 4;
        }
    if( isCommandKeyDown() && isShiftKeyDown() ) {
        offset = 8;
        }
    

    switch( inKeyCode ) {
        case MG_KEY_LEFT:
            mCurX -= offset;
            while( mCurX < 0 ) {
                mCurX += SCENE_W;
                }
            break;
        case MG_KEY_RIGHT:
            mCurX += offset;
            mCurX = mCurX % SCENE_W;
            break;
        case MG_KEY_DOWN:
            mCurY += offset;
            mCurY = mCurY % SCENE_H;
            break;
        case MG_KEY_UP:
            mCurY -= offset;
            while( mCurY < 0 ) {
                mCurY += SCENE_H;
                }
            break;
        }
    }
