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
          mPersonAgeSlider( smallFont, -55, -220, 2,
                            100, 20,
                            0, 100, "Age" ),
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


    addComponent( &mPersonAgeSlider );
    mPersonAgeSlider.setValue( 20 );
    mPersonAgeSlider.setVisible( false );
    mPersonAgeSlider.addActionListener( this );
    

    for( int i=0; i<4; i++ ) {
        char *name = autoSprintf( "ground_t%d.tga", i );    
        mGroundOverlaySprite[i] = loadSprite( name, false );
        delete [] name;
        }

    
    mEmptyCell.biome = -1;
    clearCell( &mEmptyCell );
    
    mCopyBuffer = mEmptyCell;
    
    for( int y=0; y<SCENE_H; y++ ) {
        for( int x=0; x<SCENE_W; x++ ) {
            mCells[y][x] = mEmptyCell;
            mPersonCells[y][x] = mEmptyCell;
            }
        }
    

    addKeyClassDescription( &mKeyLegend, "Arrows", "Change selected cell" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger cell jumps" );
    addKeyClassDescription( &mKeyLegend, "f/F", "Flip obj/person" );
    addKeyClassDescription( &mKeyLegend, "c/C", "Copy obj/person" );
    addKeyClassDescription( &mKeyLegend, "x/X", "Cut obj/person" );
    addKeyClassDescription( &mKeyLegend, "v", "Paste" );
    addKeyClassDescription( &mKeyLegend, "i", "Insert contained" );
    addKeyClassDescription( &mKeyLegend, "Bkspc", "Clear cell" );

    addKeyClassDescription( &mKeyLegendG, "R-Click", "Flood fill" );

    addKeyClassDescription( &mKeyLegendC, "R-Click", "Add to Container" );
    addKeyClassDescription( &mKeyLegendP, "R-Click", "Add Clothing" );
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
    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();
    
    if( inTarget == &mAnimEditorButton ) {
        setSignal( "animEditor" );
        }
    else if( inTarget == &mGroundPicker ) {
        
        char wasRightClick = false;
        
        int biome = mGroundPicker.getSelectedObject( &wasRightClick );
        
        if( biome >= 0 ) {
            
            if( wasRightClick ) {
                
                floodFill( mCurX, mCurY,
                           c->biome,
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
       
        if( id > 0 ) {
            char placed = false;
            ObjectRecord *o = getObject( id );
            
            if( wasRightClick && c->oID > 0 ) {
                
                
                if( getObject( c->oID )->numSlots > c->contained.size() ) {
                    c->contained.push_back( id );
                    SimpleVector<int> sub;
                    c->subContained.push_back( sub );
                    placed = true;
                    }
                }
            if( !placed && wasRightClick && p->oID > 0 ) {
                if( o->clothing != 'n' ) {
                    
                    switch( o->clothing ) {
                        case 's': {
                            if( p->clothing.backShoe == NULL ) {
                                p->clothing.backShoe = o;
                                }
                            else if( p->clothing.frontShoe == NULL ) {
                                p->clothing.frontShoe = o;
                                }                            
                            break;
                            }
                        case 'h':
                            p->clothing.hat = o;
                            break;
                        case 't':
                            p->clothing.tunic = o;
                            break;
                        case 'b':
                            p->clothing.bottom = o;
                            break;
                        case 'p':
                            p->clothing.backpack = o;
                            break;
                        }
                    placed = true;
                    }
                }
            
            if( !placed ) {
                if( getObject( id )->person ) {
                    p->oID = id;
                    p->age = 20;
                    }
                else {
                    c->oID = id;
                    c->contained.deleteAll();
                    c->subContained.deleteAll();
                    }
                }
            }
        checkVisible();
        }
    else if( inTarget == &mSaveNewButton ) {
        }
    else if( inTarget == &mPersonAgeSlider ) {
        getCurrentPersonCell()->age = mPersonAgeSlider.getValue();
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



SceneCell *EditorScenePage::getCurrentCell() {
    return &( mCells[ mCurY ][ mCurX ] );
    }


SceneCell *EditorScenePage::getCurrentPersonCell() {
    return &( mPersonCells[ mCurY ][ mCurX ] );
    }



void EditorScenePage::checkVisible() {
    SceneCell *p = getCurrentPersonCell();

    if( p->oID > 0 ) {
        mPersonAgeSlider.setVisible( true );
        mPersonAgeSlider.setValue( p->age );
        }
    else {
        mPersonAgeSlider.setVisible( false );
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

    
    double frameTime = frameRateFactor * mFrameCount / 60.0;

    for( int y=0; y<SCENE_H; y++ ) {
        
        // draw behind stuff first
        for( int b=0; b<2; b++ ) {
            

            if( b == 1 ) {
                // draw players behind objects in this row

                for( int x=0; x<SCENE_W; x++ ) {
                    doublePair pos = cornerPos;
                
                    pos.x += x * 128;
                    pos.y -= y * 128;
                    
                    SceneCell *p = &( mPersonCells[y][x] );

                    if( p->oID > 0 ) {
                        
                        char used;
                    
                        drawObjectAnim( p->oID, 2, ground, 
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
                                        p->flipH,
                                        p->age,
                                        0,
                                        false,
                                        false,
                                        p->clothing,
                                        NULL );
                        }
                    }
                }
            
            // now objects in row
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
                                    -1,
                                    0,
                                    false,
                                    false,
                                    c->clothing,
                                    NULL,
                                    c->contained.size(), contained,
                                    subContained );
                    delete [] contained;
                    delete [] subContained;
                    }
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
 


    doublePair legendPos = mAnimEditorButton.getPosition();
            
    legendPos.x = -150;
    legendPos.y += 20;
            
    drawKeyLegend( &mKeyLegend, legendPos );


    legendPos = mGroundPicker.getPosition();
    legendPos.y -= 255;
    drawKeyLegend( &mKeyLegendG, legendPos, alignCenter );


    legendPos = mObjectPicker.getPosition();
    legendPos.y -= 255;
    
    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();
    
    if( c->oID > 0 &&
        getObject( c->oID )->numSlots > c->contained.size() ) {
        
        drawKeyLegend( &mKeyLegendC, legendPos, alignCenter );
        }
    else if( p->oID > 0 ) {
        drawKeyLegend( &mKeyLegendP, legendPos, alignCenter );
        }
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
    
    SceneCell *c = getCurrentCell();
    SceneCell *p = getCurrentPersonCell();
    

    if( inASCII == 'f' ) {
        c->flipH = ! c->flipH;
        }
    else if( inASCII == 'F' ) {
        p->flipH = ! p->flipH;
        }
    else if( inASCII == 'c' ) {
        // copy
        mCopyBuffer = *c;
        }
    else if( inASCII == 'C' ) {
        // copy
        mCopyBuffer = *p;
        }
    else if( inASCII == 'x' ) {
        // cut
        // delete all but biome
        int oldBiome = c->biome;
        mCopyBuffer = *c;
        *c = mEmptyCell;
        c->biome = oldBiome;
        }
    else if( inASCII == 'X' ) {
        // cut person
        mCopyBuffer = *p;
        *p = mEmptyCell;
        }
    else if( inASCII == 'v' ) {
        // paste
        if( mCopyBuffer.oID > 0 &&
            getObject( mCopyBuffer.oID )->person ) {
            *p = mCopyBuffer;
            }
        else {
            *c = mCopyBuffer;
            }
        }
    else if( inASCII == 'i' ) {
        // insert into container
        
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
        clearCell( c );
        clearCell( p );
        }
    }




void EditorScenePage::clearCell( SceneCell *inCell ) {
    inCell->oID = -1;
    inCell->flipH = false;
    inCell->age = -1;
    inCell->heldID = -1;
    
    inCell->clothing = getEmptyClothingSet();
    
    inCell->contained.deleteAll();
    inCell->subContained.deleteAll();    
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

    checkVisible();
    }
