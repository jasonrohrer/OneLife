#include "EditorSpriteTrimPage.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



extern Font *mainFont;
extern Font *smallFont;


#include "SpritePickable.h"

static SpritePickable spritePickable;

static double lastMouseX = 0;
static double lastMouseY = 0;




EditorSpriteTrimPage::EditorSpriteTrimPage()
        : mImportEditorButton( mainFont, -210, 260, "Sprites" ),
          mSaveButton( mainFont, 400, 64, "Save" ),
          mClearRectButton( mainFont, 400, -65, "X Rect" ),
          mSpritePicker( &spritePickable, -410, 90 ),
          mPickedSprite( -1 ),
          mPickingRect( false ) {
    
    addComponent( &mImportEditorButton );
    mImportEditorButton.addActionListener( this );

    addComponent( &mSpritePicker );
    mSpritePicker.addActionListener( this );


    addComponent( &mSaveButton );
    mSaveButton.addActionListener( this );
    
    addComponent( &mClearRectButton );
    mClearRectButton.addActionListener( this );

    mSaveButton.setVisible( false );
    mClearRectButton.setVisible( false );
    }



EditorSpriteTrimPage::~EditorSpriteTrimPage() {
    }



static Image *expandToPowersOfTwo( Image *inImage ) {
    
    int w = 1;
    int h = 1;
                    
    while( w < inImage->getWidth() ) {
        w *= 2;
        }
    while( h < inImage->getHeight() ) {
        h *= 2;
        }
    
    return inImage->expandImage( w, h );
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

            mSaveButton.setVisible( false );
            mClearRectButton.setVisible( false );
            }
        }
    else if( inTarget == &mClearRectButton ) {
        mRects.deleteElement( mRects.size() - 1 );
        
        if( mRects.size() == 0 ) {
            mSaveButton.setVisible( false );
            mClearRectButton.setVisible( false );
            }
        }
    else if( inTarget == &mSaveButton ) {
        
        File spritesDir( NULL, "sprites" );

        char *fileName = autoSprintf( "%d.tga", mPickedSprite );
        
        File *spriteFile = spritesDir.getChildFile( fileName );
            
        delete [] fileName;
            

        char *fullName = spriteFile->getFullFileName();
        
        delete spriteFile;

        Image *im = readTGAFileBase( fullName );

        delete [] fullName;

        if( im != NULL ) {
            
            SpriteRecord *oldSprite = getSpriteRecord( mPickedSprite );
            
            
            int numSprites = mRects.size();

            int *spriteIDs = new int[ numSprites ];
            doublePair *spritePos = new doublePair[ numSprites ];
            
            double *spriteRot = new double[ numSprites ];
            char *spriteHFlip = new char[ numSprites ];
            FloatRGB *spriteColor = new FloatRGB[ numSprites ];
            double *spriteAgeStart = new double[ numSprites ];
            double *spriteAgeEnd = new double[ numSprites ];
            int *spriteParent = new int[ numSprites ];
            char *spriteInvisibleWhenHolding = new char[ numSprites ];
            int *spriteInvisibleWhenWorn = new int[ numSprites ];
            char *spriteBehindSlots = new char[ numSprites ];
            char *spriteIsHead = new char[ numSprites ];
            char *spriteIsBody = new char[ numSprites ];
            char *spriteIsBackFoot = new char[ numSprites ];
            char *spriteIsFrontFoot = new char[ numSprites ];
            
            FloatRGB whiteColor = { 1, 1, 1 };
            

            for( int i=0; i<numSprites; i++ ) {
                
                PickedRect r = mRects.getElementDirect( i );

                Image *subIm =im->getSubImage( oldSprite->w/2 + r.xStart, 
                                               oldSprite->h/2 - r.yStart, 
                                               r.xEnd - r.xStart,
                                               r.yStart - r.yEnd );
                
                Image *subExpanded = expandToPowersOfTwo( subIm );
                
                delete subIm;

                char *newTag = autoSprintf( "%s_%d", oldSprite->tag, i+1 );
                

                spriteIDs[i] = addSprite( newTag, 
                                          fillSprite( subExpanded, false ), 
                                          subExpanded,
                                          oldSprite->multiplicativeBlend );

                // rectangles enforced to be even sizes, so this works
                spritePos[i].x = ( r.xEnd + r.xStart ) / 2.0;
                spritePos[i].y = ( r.yStart + r.yEnd ) / 2.0;
                

                spriteParent[i] = -1;

                if( i > 0 ) {
                    spriteParent[i] = i-1;
                    }

                spriteRot[i] = 0;
                spriteHFlip[i] = false;
                spriteColor[i] = whiteColor;
                spriteAgeStart[i] = -1;
                spriteAgeEnd[i] = -1;
                spriteInvisibleWhenHolding[i] = false;
                spriteInvisibleWhenWorn[i] = false;
                spriteBehindSlots[i] = false;
                spriteIsHead[i] = false;
                spriteIsBody[i] = false;
                spriteIsBackFoot[i] = false;
                spriteIsFrontFoot[i] = false;

                delete subExpanded;
                delete [] newTag;
                }
            
            delete im;

            mSpritePicker.redoSearch();            
            
            char *objName = autoSprintf( "%s_all", oldSprite->tag );
            
            doublePair zeroOffset = { 0, 0 };
            
            doublePair *slotPos = new doublePair[ 0 ];
            char *slotVert = new char[ 0 ];

            addObject( objName,
                       false,
                       1,
                       0,
                       false,
                       3,
                       false,
                       false,
                       false,
                       0, 0,
                       false,
                       (char*)"0",
                       0,
                       0,
                       0,
                       false,
                       false,
                       0,
                       false,
                       0,
                       1,
                       zeroOffset,
                       false,
                       zeroOffset,
                       0,
                       blankSoundUsage,
                       blankSoundUsage,
                       blankSoundUsage,
                       0,
                       0, 0, slotPos,
                       slotVert,
                       1,
                       numSprites, spriteIDs, 
                       spritePos,
                       spriteRot,
                       spriteHFlip,
                       spriteColor,
                       spriteAgeStart,
                       spriteAgeEnd,
                       spriteParent,
                       spriteInvisibleWhenHolding,
                       spriteInvisibleWhenWorn,
                       spriteBehindSlots,
                       spriteIsHead,
                       spriteIsBody,
                       spriteIsBackFoot,
                       spriteIsFrontFoot );
            

            delete [] spriteIDs;
            delete [] spritePos;
            delete [] spriteRot;
            delete [] spriteHFlip;
            delete [] spriteColor;
            delete [] spriteAgeStart;
            delete [] spriteAgeEnd;
            delete [] spriteParent;
            delete [] spriteInvisibleWhenHolding;
            delete [] spriteInvisibleWhenWorn;
            delete [] spriteBehindSlots;
            delete [] spriteIsHead;
            delete [] spriteIsBody;
            delete [] spriteIsBackFoot;
            delete [] spriteIsFrontFoot;


            delete [] objName;
            delete [] slotPos;
            delete [] slotVert;
            }
        }
    }


char EditorSpriteTrimPage::trimRectByExisting( PickedRect *inRect ) {

    PickedRect r = *inRect;
    
    
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
                
            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                
                // top edge intersects
                r.yStart = otherR.yEnd;
                }


            if( otherR.xStart > r.xStart &&
                otherR.xStart < r.xEnd &&
                otherR.xEnd > r.xStart &&
                otherR.xEnd < r.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top edge intersects (new rect swallows)
                r.yStart = otherR.yEnd;
                }


            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom edge intersects
                r.yEnd = otherR.yStart;
                }

            if( otherR.xStart > r.xStart &&
                otherR.xStart < r.xEnd &&
                otherR.xEnd > r.xStart &&
                otherR.xEnd < r.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom edge intersects (new rect swallows)
                r.yEnd = otherR.yStart;
                }

                
            if( otherR.yStart < r.yStart &&
                otherR.yStart > r.yEnd &&
                otherR.yEnd < r.yStart &&
                otherR.yEnd > r.yEnd &&
                r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd ) {
                    
                // left edge intersects (new rect swallows)
                r.xStart = otherR.xEnd;
                }

            if( otherR.yStart < r.yStart &&
                otherR.yStart > r.yEnd &&
                otherR.yEnd < r.yStart &&
                otherR.yEnd > r.yEnd &&
                r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd ) {
                    
                // right edge intersects (new rect swallows)
                r.xEnd = otherR.xStart;
                }


                
            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top left corner intersects
                r.xStart = otherR.xEnd;
                }
                
            if( r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top right corner intersects
                r.xEnd = otherR.xStart;
                }

            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom left corner intersects
                r.xStart = otherR.xEnd;
                }
                
            if( r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom right corner intersects
                r.xEnd = otherR.xStart;
                }
            }
        }



    if( ( r.xStart + r.xEnd ) % 2 != 0 ) {
        // not even (can't be centered at an integer position)
        // expand it if we can
        
        if( r.xEnd == inRect->xEnd ) {
            // end not touched, expand here
            r.xEnd += 1;
            }
        else if( r.xStart == inRect->xStart ) {
            // start not touched, expand here
            r.xStart -= 1;
            }
        }

    

    if( ( r.yStart + r.yEnd ) % 2 != 0 ) {
        // not even (can't be centered at an integer position)
        // expand it if we can
        
        if( r.yEnd == inRect->yEnd ) {
            // end not touched, expand here
            r.yEnd -= 1;
            }
        else if( r.yStart == inRect->yStart ) {
            // start not touched, expand here
            r.yStart += 1;
            }
        }
        

    *inRect = r;
    
    return skip;
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
                
                PickedRect rect = { mPickStartX, mPickStartY,
                                    mPickEndX, mPickEndY };
                
                trimRectByExisting( &rect );

                drawRect( rect.xStart, rect.yStart,
                          rect.xEnd, rect.yEnd );
                }
            else {
                setDrawColor( 0, 0, 1, 0.50 );
            
                doublePair mouseCenter = { lastMouseX + 1, lastMouseY - 1 };
            
                drawRect( mouseCenter, 1000, 0.5 );
                drawRect( mouseCenter, 0.5, 1000 );
                }
            }
        
        }
    }



void EditorSpriteTrimPage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }

    mSpritePicker.redoSearch();
    
    lastMouseX = 0;
    lastMouseY = 0;
    }



char EditorSpriteTrimPage::isPointInSprite( int inX, int inY ) {
    if( mPickedSprite != -1 ) {
        
        SpriteHandle h = getSprite( mPickedSprite );
        
        if( h != NULL ) {
            
            doublePair boxPos = { 0, 0 };
            
            SpriteRecord *r = getSpriteRecord( mPickedSprite );
            
            boxPos.x += r->centerAnchorXOffset;
            boxPos.y -= r->centerAnchorYOffset;

            if( inX >= - r->w / 2 && 
                inX <= r->w / 2 &&
                inY >= - r->h / 2 &&
                inY <= r->h / 2 ) {
                
                return true;
                }
            }
        }

    return false;
    }



void EditorSpriteTrimPage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    }



void EditorSpriteTrimPage::pointerDown( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    
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
    lastMouseX = inX;
    lastMouseY = inY;
    
    int x = lrint( inX );
    int y = lrint( inY );
    
    if( isPointInSprite( x, y ) ) {
        mPickEndX = x;
        mPickEndY = y;
        }
    }


void EditorSpriteTrimPage::pointerUp( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    
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
        

        char skip = trimRectByExisting( &r );

        if( !skip ) {
            mRects.push_back( r );

            mSaveButton.setVisible( true );
            mClearRectButton.setVisible( true );
            }
        
        }
    
    
    mPickingRect = false;
    }

