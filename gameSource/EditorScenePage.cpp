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

extern double frameRateFactor;


EditorScenePage::EditorScenePage()
        : mAnimEditorButton( mainFont, 210, 260, "Anim" ),
          mSaveNewButton( mainFont, 400, 260, "Save" ),
          mGroundPicker( &groundPickable, -410, 90 ),
          mObjectPicker( &objectPickable, 410, 90 ),
          mCurX( SCENE_W / 2 ),
          mCurY( SCENE_H / 2 ),
          mFrameCount( 0 ) {
    
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

    
    mEmptyCell.biome = -1;
    mEmptyCell.oID = -1;
    mEmptyCell.flipH = false;
    mEmptyCell.age = -1;
    
    mCopyBuffer = mEmptyCell;
    
    for( int y=0; y<SCENE_H; y++ ) {
        for( int x=0; x<SCENE_W; x++ ) {
            mCells[y][x] = mEmptyCell;
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
        char wasRightClick = false;
        
        int id = mObjectPicker.getSelectedObject( &wasRightClick);
        SceneCell *c = &( mCells[ mCurY ][ mCurX ] );
        
        if( id > 0 ) {
            if( wasRightClick && c->oID > 0 ) {
                if( getObject( c->oID )->numSlots > c->contained.size() ) {
                    c->contained.push_back( id );
                    SimpleVector<int> sub;
                    c->subContained.push_back( sub );
                    }
                }
            else {
                c->oID = id;
                c->contained.deleteAll();
                c->subContained.deleteAll();
                }
            }
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
    
    mFrameCount ++;
    
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

    

    for( int y=0; y<SCENE_H; y++ ) {
        
        // draw behind stuff first
        for( int b=0; b<2; b++ )
        for( int x=0; x<SCENE_W; x++ ) {
            doublePair pos = cornerPos;
                
            pos.x += x * 128;
            pos.y -= y * 128;
                
            SceneCell *c = &( mCells[y][x] );
            
            if( c->oID > 0 ) {
                
                ObjectRecord *o = getObject( c->oID );
                
                if( ( b == 0 && ! o->drawBehindPlayer ) 
                    ||
                    ( b == 1 && o->drawBehindPlayer ) ) {
                    continue;
                    }
                

                double frameTime = frameRateFactor * mFrameCount / 60.0;
                
                char used;
                int *contained = c->contained.getElementArray();
                SimpleVector<int> *subContained = 
                    c->subContained.getElementArray();
                
                drawObjectAnim( c->oID, ground, 
                                frameTime, 
                                0,
                                ground,
                                frameTime,
                                frameTime,
                                &used,
                                ground,
                                ground,
                                pos,
                                0,
                                false,
                                c->flipH,
                                c->age,
                                0,
                                false,
                                false,
                                getEmptyClothingSet(),
                                NULL,
                                c->contained.size(), contained,
                                subContained );
                delete [] contained;
                delete [] subContained;
                }
            }
        }
    
    
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



void EditorScenePage::keyDown( unsigned char inASCII ) {
    
    if( TextField::isAnyFocused() ) {
        return;
        }
    
    if( inASCII == 'f' ) {
        mCells[ mCurY ][ mCurX ].flipH = ! mCells[ mCurY ][ mCurX ].flipH;
        }
    else if( inASCII == 'c' ) {
        // copy
        mCopyBuffer = mCells[ mCurY ][ mCurX ];
        }
    else if( inASCII == 'x' ) {
        // cut
        // delete all but biome
        int oldBiome = mCells[ mCurY ][ mCurX ].biome;
        mCopyBuffer = mCells[ mCurY ][ mCurX ];
        mCells[ mCurY ][ mCurX ] = mEmptyCell;
        mCells[ mCurY ][ mCurX ].biome = oldBiome;
        }
    else if( inASCII == 'v' ) {
        // paste
        mCells[ mCurY ][ mCurX ] = mCopyBuffer;
        }
    else if( inASCII == 'i' ) {
        // insert into container
        SceneCell *c = &( mCells[ mCurY ][ mCurX ] );
        
        if( mCopyBuffer.oID > 0 &&
            c->oID > 0 &&
            getObject( c->oID )->numSlots > c->contained.size() ) {
            // room
            
            c->contained.push_back( mCopyBuffer.oID );
            SimpleVector<int> sub;            
            
            if( mCopyBuffer.contained.size() > 0 ) {
                
                int *pasteContained = mCopyBuffer.contained.getElementArray();
                
                sub.appendArray( pasteContained, mCopyBuffer.contained.size() );
                delete [] pasteContained;
                }
            
            c->subContained.push_back( sub );
            }
        }
    else if( inASCII == 8 ) {
        // backspace
        mCells[ mCurY ][ mCurX ].oID = -1;
        mCells[ mCurY ][ mCurX ].flipH = false;
        mCells[ mCurY ][ mCurX ].age = -1;

        mCells[ mCurY ][ mCurX ].contained.deleteAll();
        mCells[ mCurY ][ mCurX ].subContained.deleteAll();
        }
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
