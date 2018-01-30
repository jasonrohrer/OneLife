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
            
            char *spriteUseVanish = new char[ numSprites ];
            char *spriteUseAppear = new char[ numSprites ];
            

            FloatRGB whiteColor = { 1, 1, 1 };
            

            for( int i=0; i<numSprites; i++ ) {
                
                PickedRect r = mRects.getElementDirect( i );

                // add 16 overlap to edge where we touch another tile
                // thus, we can mip-map safely down to a scale of 1/16
                // without seams showing
                int over = 16;

                double overFades[16];
                
                // cross-fading alphas in overlap area
                for( int f=0; f<16; f++ ) {
                    overFades[f] = ( 16.0 - f ) / 16.0;
                    }
                
                // non-trans areas of overlap stay solid for first 8 pixels
                // then crossfade
                double solidFades[16];
                
                for( int f=0; f<8; f++ ) {
                    solidFades[f] = 1.0;
                    }
                for( int f=8; f<16; f++ ) {
                    solidFades[f] = ( 8.0 - (f - 8.0) ) / 8.0;
                    }
                
                PickedRect rE = r;
                
                if( r.intersectSides[0] ) {
                    rE.yStart += over;
                    }
                if( r.intersectSides[1] ) {
                    rE.xEnd += over;
                    }
                if( r.intersectSides[2] ) {
                    rE.yEnd -= over;
                    }
                if( r.intersectSides[3] ) {
                    rE.xStart -= over;
                    }
                
                Image *subIm =im->getSubImage( oldSprite->w/2 + rE.xStart, 
                                               oldSprite->h/2 - rE.yStart, 
                                               rE.xEnd - rE.xStart,
                                               rE.yStart - rE.yEnd );
                
                double *subA = subIm->getChannel( 3 );
                int subW = subIm->getWidth();
                int subH = subIm->getHeight();
                
                // reduce alpha of semi-transparent areas
                // we're going to reduce other alpha with an f multiplier
                
                // so this alpha becomes (A - F * A ) / ( 1 - F * A )
                
                // for opaque areas, do straight cross-fade as well

                if( r.intersectSides[0] ) {
                    for( int y=0; y<over; y++ ) {
                        double F = overFades[y];
                        for( int x=0; x<subW; x++ ) {
                            int p = y * subW + x;
                            
                            if( subA[p] < 1 ) {
                                subA[p] = 
                                    ( subA[p] - F * subA[p] ) / 
                                    ( 1 - F *subA[p] );
                                }
                            else {
                                // opaque
                                subA[p] = solidFades[ over- y - 1 ];
                                }
                            }
                        }
                    }
                if( r.intersectSides[2] ) {
                    for( int y=subH-1; y>=subH-over; y-- ) {
                        double F = overFades[(subH-1) - y];
                        for( int x=0; x<subW; x++ ) {
                            int p = y * subW + x;
                            
                            if( subA[p] < 1 ) {
                                subA[p] = 
                                    ( subA[p] - F * subA[p] ) / 
                                    ( 1 - F *subA[p] );
                                }
                            else {
                                // opaque
                                subA[p] = solidFades[ y - (subH - over) ];
                                }
                            }
                        }
                    }
                if( r.intersectSides[3] ) {
                    for( int x=0; x<over; x++ ) {
                        double F = overFades[x];
                        for( int y=0; y<subH; y++ ) {
                            int p = y * subW + x;
                            if( subA[p] < 1 ) {
                                subA[p] = 
                                    ( subA[p] - F * subA[p] ) / 
                                    ( 1 - F *subA[p] );
                                }
                            else {
                                // opaque
                                subA[p] = solidFades[ over- x - 1 ];
                                }
                            }
                        }
                    }
                if( r.intersectSides[1] ) {
                    for( int x=subW-1; x>=subW-over; x-- ) {
                        double F = overFades[(subW - 1) - x];
                        for( int y=0; y<subH; y++ ) {
                            int p = y * subW + x;
                            if( subA[p] < 1 ) {
                                subA[p] = 
                                    ( subA[p] - F * subA[p] ) / 
                                    ( 1 - F *subA[p] );
                                }
                            else {
                                // opaque
                                subA[p] = solidFades[ x - (subW - over) ];
                                }
                            }
                        }

                    }
                
                // now handle case where we're being overlapped

                // our alpha becomes 0.5 * A
                if( r.coveredSides[0] ) {
                    for( int y=0; y<over; y++ ) {
                        double F = overFades[ over - y - 1 ];
                        for( int x=0; x<subW; x++ ) {
                            int p = y * subW + x;
                            
                            if( subA[p] < 1 ) {
                                subA[p] = F * subA[p];
                                }
                            }
                        }
                    }
                if( r.coveredSides[2] ) {
                    for( int y=subH-1; y>=subH-over; y-- ) {
                        double F = overFades[ y - (subH - over)];
                        for( int x=0; x<subW; x++ ) {
                            int p = y * subW + x;
                            
                            if( subA[p] < 1 ) {
                                subA[p] = F * subA[p];
                                }
                            }
                        }
                    }
                if( r.coveredSides[3] ) {
                    for( int x=0; x<over; x++ ) {
                        double F = overFades[ over - x - 1 ];
                        for( int y=0; y<subH; y++ ) {
                            int p = y * subW + x;
                            if( subA[p] < 1 ) {
                                subA[p] = F * subA[p];
                                }
                            }
                        }
                    }
                if( r.coveredSides[1] ) {
                    for( int x=subW-1; x>=subW-over; x-- ) {
                        double F = overFades[ x - (subW - over)];
                        for( int y=0; y<subH; y++ ) {
                            int p = y * subW + x;
                            if( subA[p] < 1 ) {
                                subA[p] = F * subA[p];
                                }
                            }
                        }
                    }
                

                Image *subExpanded = expandToPowersOfTwo( subIm );
                
                delete subIm;

                char *newTag = autoSprintf( "%s_%d", oldSprite->tag, i+1 );
                

                spriteIDs[i] = addSprite( newTag, 
                                          fillSprite( subExpanded, false ), 
                                          subExpanded,
                                          oldSprite->multiplicativeBlend );

                // rectangles enforced to be even sizes, so this works
                spritePos[i].x = ( rE.xEnd + rE.xStart ) / 2.0;
                spritePos[i].y = ( rE.yStart + rE.yEnd ) / 2.0;
                

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
                
                spriteUseVanish[i] = false;
                spriteUseAppear[i] = false;
                
                delete subExpanded;
                delete [] newTag;
                }
            
            delete im;

            mSpritePicker.redoSearch( false );            
            
            char *objName = autoSprintf( "%s_all", oldSprite->tag );
            
            doublePair zeroOffset = { 0, 0 };
            
            doublePair *slotPos = new doublePair[ 0 ];
            char *slotVert = new char[ 0 ];
            int *slotParent = new int[ 0 ];

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
                       false,
                       0,
                       false,
                       false,
                       false,
                       false,
                       0,
                       1,
                       zeroOffset,
                       'n',
                       zeroOffset,
                       0,  // deadly distance
                       1,  // use distance
                       blankSoundUsage,
                       blankSoundUsage,
                       blankSoundUsage,
                       blankSoundUsage,
                       0,
                       0, 0, slotPos,
                       slotVert,
                       slotParent,
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
                       spriteIsFrontFoot,
                       1,
                       spriteUseVanish,
                       spriteUseAppear );
            

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
            
            delete [] spriteUseVanish;
            delete [] spriteUseAppear;

            delete [] objName;
            delete [] slotPos;
            delete [] slotVert;
            delete [] slotParent;
            }
        }
    }


char EditorSpriteTrimPage::trimRectByExisting( PickedRect *inRect,
                                               char inUpdateCovered ) {

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
                r.intersectSides[0] = true;

                otherR.coveredSides[2] = true;
                }


            if( otherR.xStart > r.xStart &&
                otherR.xStart < r.xEnd &&
                otherR.xEnd > r.xStart &&
                otherR.xEnd < r.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top edge intersects (new rect swallows)
                r.yStart = otherR.yEnd;
                r.intersectSides[0] = true;
                
                otherR.coveredSides[2] = true;
                }


            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom edge intersects
                r.yEnd = otherR.yStart;
                r.intersectSides[2] = true;

                otherR.coveredSides[0] = true;
                }

            if( otherR.xStart > r.xStart &&
                otherR.xStart < r.xEnd &&
                otherR.xEnd > r.xStart &&
                otherR.xEnd < r.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom edge intersects (new rect swallows)
                r.yEnd = otherR.yStart;
                r.intersectSides[2] = true;

                otherR.coveredSides[0] = true;
                }

                
            if( otherR.yStart < r.yStart &&
                otherR.yStart > r.yEnd &&
                otherR.yEnd < r.yStart &&
                otherR.yEnd > r.yEnd &&
                r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd ) {
                    
                // left edge intersects (new rect swallows)
                r.xStart = otherR.xEnd;
                r.intersectSides[3] = true;

                otherR.coveredSides[1] = true;
                }

            if( otherR.yStart < r.yStart &&
                otherR.yStart > r.yEnd &&
                otherR.yEnd < r.yStart &&
                otherR.yEnd > r.yEnd &&
                r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd ) {
                    
                // right edge intersects (new rect swallows)
                r.xEnd = otherR.xStart;
                r.intersectSides[1] = true;

                otherR.coveredSides[3] = true;
                }


                
            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top left corner intersects
                r.xStart = otherR.xEnd;
                r.intersectSides[0] = true;
                r.intersectSides[3] = true;

                otherR.coveredSides[1] = true;
                }
                
            if( r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yStart < otherR.yStart &&
                r.yStart > otherR.yEnd ) {
                    
                // top right corner intersects
                r.xEnd = otherR.xStart;
                r.intersectSides[0] = true;
                r.intersectSides[1] = true;
                
                otherR.coveredSides[3] = true;
                }

            if( r.xStart > otherR.xStart &&
                r.xStart < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom left corner intersects
                r.xStart = otherR.xEnd;
                r.intersectSides[2] = true;
                r.intersectSides[3] = true;

                otherR.coveredSides[1] = true;
                }
                
            if( r.xEnd > otherR.xStart &&
                r.xEnd < otherR.xEnd &&
                r.yEnd < otherR.yStart &&
                r.yEnd > otherR.yEnd ) {
                    
                // bottom right corner intersects
                r.xEnd = otherR.xStart;
                r.intersectSides[2] = true;
                r.intersectSides[1] = true;

                otherR.coveredSides[3] = true;
                }
            
            if( inUpdateCovered ) {
                // save back into vector
                *( mRects.getElement( i ) ) = otherR;
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
                                    mPickEndX, mPickEndY,
                                    { false, false, false, false },
                                    { false, false, false, false } };
                
                // don't update covered flags until mouse released
                trimRectByExisting( &rect, false );

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

    mSpritePicker.redoSearch( false );
    
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


        PickedRect r = { mPickStartX, mPickStartY, mPickEndX, mPickEndY,
                         { false, false, false, false },
                         { false, false, false, false } };
        
        printf( "Rect start = (%d,%d), end = (%d,%d)\n",
                mPickStartX, mPickStartY, mPickEndX, mPickEndY );
        

        // update covered flags now that we're releasing mouse
        char skip = trimRectByExisting( &r, true );

        if( !skip ) {
            mRects.push_back( r );

            mSaveButton.setVisible( true );
            mClearRectButton.setVisible( true );
            }
        
        }
    
    
    mPickingRect = false;
    }

