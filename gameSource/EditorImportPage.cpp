#include "EditorImportPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;
extern Font *smallFont;


#include "SpritePickable.h"
#include "OverlayPickable.h"

static SpritePickable spritePickable;

static OverlayPickable overlayPickable;



EditorImportPage::EditorImportPage()
        : mImportButton( smallFont, +180, 260, "Sprite Import" ),
          mImportOverlayButton( smallFont, +310, 260, "Overlay Import" ),
          mSelect( false ),
          mImportedSheet( NULL ),
          mImportedSheetSprite( NULL ),
          mProcessedSelection( NULL ),
          mProcessedSelectionSprite( NULL ),
          mSpriteTagField( mainFont, 
                           0,  -260, 6,
                           false,
                           "Tag", NULL, " " ),
          mSaveSpriteButton( mainFont, 210, -260, "Save" ),
          mSaveOverlayButton( smallFont, 310, -260, "Save Overlay" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mOverlayPicker( &overlayPickable, 310, 100 ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mCenterMarkSprite( loadSprite( "centerMark.tga" ) ),
          mCenterSet( false ),
          mCurrentOverlay( NULL ),
          mClearRotButton( smallFont, -300, -280, "0 Rot" ),
          mClearScaleButton( smallFont, -300, -240, "1 Scale" ),
          mFlipOverlayButton( smallFont, -230, -280, "Flip H" ),
          mClearOverlayButton( smallFont, -230, -240, "X Ovly" ) {

    addComponent( &mImportButton );
    addComponent( &mImportOverlayButton );
    addComponent( &mSpriteTagField );
    addComponent( &mSaveSpriteButton );
    addComponent( &mSaveOverlayButton );
    addComponent( &mSpritePicker );
    addComponent( &mOverlayPicker );
    addComponent( &mObjectEditorButton );

    addComponent( &mClearRotButton );
    addComponent( &mClearScaleButton );
    addComponent( &mFlipOverlayButton );
    addComponent( &mClearOverlayButton );
    

    mImportButton.addActionListener( this );
    mImportOverlayButton.addActionListener( this );
    
    mSaveSpriteButton.addActionListener( this );
    mSaveOverlayButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    
    mOverlayPicker.addActionListener( this );


    mClearRotButton.addActionListener( this );
    mClearScaleButton.addActionListener( this );
    mFlipOverlayButton.addActionListener( this );
    mClearOverlayButton.addActionListener( this );


    mSaveSpriteButton.setVisible( false );
    mSaveOverlayButton.setVisible( false );

    mClearRotButton.setVisible( false );
    mClearScaleButton.setVisible( false );
    mFlipOverlayButton.setVisible( false );
    mClearOverlayButton.setVisible( false );

    mOverlayScale = 1;
    mOverlayRotation = 0;


    mMovingSheet = false;
    doublePair offset = { 0, 0 };
    mSheetOffset = offset;
    mMovingSheetPointerStart = offset;

    mOverlayFlip = 0;


    addKeyClassDescription( &mSheetKeyLegend, "r-mouse", "Mv sheet" );
    addKeyDescription( &mSheetKeyLegend, 'c', "Mv sprite center" );
    addKeyDescription( &mOverlayKeyLegend, 't', "Mv overlay" );
    addKeyDescription( &mOverlayKeyLegend, 's', "Scale overlay" );
    addKeyDescription( &mOverlayKeyLegend, 'r', "Rot overlay" );
    
    }




EditorImportPage::~EditorImportPage() {
    if( mImportedSheet != NULL ) {
        delete mImportedSheet;
        }

    if( mImportedSheetSprite != NULL ) {
        freeSprite( mImportedSheetSprite );
        mImportedSheetSprite = NULL;
        }

    if( mProcessedSelection != NULL ) {
        delete mProcessedSelection;
        }
    if( mProcessedSelectionSprite != NULL ) {
        freeSprite( mProcessedSelectionSprite );
        }

    freeSprite( mCenterMarkSprite );
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



void EditorImportPage::clearUseOfOverlay( int inOverlayID ) {
    
    if( mCurrentOverlay->id == inOverlayID ) {
        mCurrentOverlay = NULL;
        }
    }





void EditorImportPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mImportButton ||
        inTarget == &mImportOverlayButton ) {
        
        if( mProcessedSelectionSprite != NULL ) {
            freeSprite( mProcessedSelectionSprite );
            mProcessedSelectionSprite = NULL;
            }
        
        mSettingSpriteCenter = false;

        mSaveSpriteButton.setVisible( false );
        mSaveOverlayButton.setVisible( false );

        File *importFile = NULL;
        
        if( inTarget == &mImportButton ) {
            
            char *importPath = 
                SettingsManager::getStringSetting( "editorImportPath" );
            
            if( importPath != NULL ) {
                
                importFile = new File( NULL, importPath );
                
                delete [] importPath;
                }
            mSheetOffset.x = 0;
            mSheetOffset.y = 0;
            mMovingSheet = false;
            }
        else if( inTarget == &mImportOverlayButton ) {
            // used first PNG file in overlayImport dir
            
            File importDir( NULL, "overlayImport" );
            if( importDir.exists() && importDir.isDirectory() ) {
                int numPNGFiles;
                File **pngFiles = importDir.getChildFiles( &numPNGFiles );

                char found = false;
                for( int t=0; t<numPNGFiles; t++ ) {
                    if( !found ) {
                        
                        if( !pngFiles[t]->isDirectory() ) {
                            char *pngFileName = pngFiles[t]->getFileName();
                        
                            if( strstr( pngFileName, ".png" ) != NULL ) {
                        
                                importFile = pngFiles[t];
                                found = true;
                                }
                            delete [] pngFileName;
                            }
                        if( !found ) {
                            // this file was not a match
                            delete pngFiles[t];
                            }
                        }
                    else {
                        delete pngFiles[t];
                        }
                    }
                delete [] pngFiles;
                }
            }
        

        if( importFile != NULL ) {
    
            char imported = false;
            
            if( importFile->exists() ) {
                PNGImageConverter converter;
                
                FileInputStream stream( importFile );
                
                Image *image = converter.deformatImage( &stream );
                
                if( image != NULL ) {
                    
                    // expand to powers of 2

                    if( mImportedSheet != NULL ) {
                        delete mImportedSheet;
                        mImportedSheet = NULL;
                        }

                    mImportedSheet = expandToPowersOfTwo( image );
                    
                    mSheetW = mImportedSheet->getWidth();
                    mSheetH = mImportedSheet->getHeight();
                    
                    
                    if( mImportedSheetSprite != NULL ) {
                        freeSprite( mImportedSheetSprite );
                        mImportedSheetSprite = NULL;
                        }
                    mImportedSheetSprite = fillSprite( mImportedSheet, false );
                    
                    imported = true;
                    delete image;
                    }
                }
            
            if( !imported ) {
                char *fullFileName = importFile->getFullFileName();
                
                char *message = autoSprintf( "Failed to import PNG from:##"
                                             "%s", fullFileName );
                
                delete [] fullFileName;
                
                setStatusDirect( message, true );
                delete message;
                }
            else if( inTarget == &mImportOverlayButton ) {
                // can save right away without making a selection
                mSaveOverlayButton.setVisible( true );
                }
            
            delete importFile;
            }
        else {
            if( inTarget == &mImportButton ) {
                setStatus( "Import path not set in settings folder", true );
                }
            else {
                setStatus( "No PNG file found in overlayImport folder", true );
                }
            }
        
        
        }
    else if( inTarget == &mSaveSpriteButton ) {
        char *tag = mSpriteTagField.getText();
        
        if( strcmp( tag, "" ) != 0 ) {
                                
            addSprite( tag, 
                       mProcessedSelectionSprite,
                       mProcessedSelection );

            mSpritePicker.redoSearch();
            
            // don't let it get freed now
            mProcessedSelectionSprite = NULL;
            mSaveSpriteButton.setVisible( false );
            }
        

        delete [] tag;
        }
    else if( inTarget == &mSaveOverlayButton ) {
        char *tag = mSpriteTagField.getText();
        
        if( strcmp( tag, "" ) != 0 ) {
                                
            addOverlay( tag, mImportedSheet );

            // don't let it get freed now
            mImportedSheet = NULL;
            
            mOverlayPicker.redoSearch();
            
            mSaveOverlayButton.setVisible( false );
            }
        

        delete [] tag;
        }
    else if( inTarget == &mClearRotButton ) {
        mClearRotButton.setVisible( false );
        mOverlayRotation = 0;
        }
    else if( inTarget == &mClearScaleButton ) {
        mClearScaleButton.setVisible( false );
        mOverlayScale = 1;
        }    
    else if( inTarget == &mFlipOverlayButton ) {
        mOverlayFlip = !mOverlayFlip;
        }    
    else if( inTarget == &mClearOverlayButton ) {
        mCurrentOverlay = NULL;
        mClearScaleButton.setVisible( false );
        mClearRotButton.setVisible( false );
        mFlipOverlayButton.setVisible( false );
        mClearOverlayButton.setVisible( false );
        }    
    else if( inTarget == &mOverlayPicker ) {
        int overlayID = mOverlayPicker.getSelectedObject();
    
        if( overlayID != -1 ) {
            mCurrentOverlay = getOverlay( overlayID );
            mOverlayOffset.x = 0;
            mOverlayOffset.y = 0;
            
            mMovingOverlay = false;
            mScalingOverlay = false;
            mRotatingOverlay = false;
            
            mOverlayFlip = 0;
            
            mFlipOverlayButton.setVisible( true );
            mClearOverlayButton.setVisible( true );

            mOverlayScale = 1.0;
            mOverlayRotation = 0;
            }
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    

    }


float lastMouseX, lastMouseY;

        
void EditorImportPage::drawUnderComponents( doublePair inViewCenter, 
                                            double inViewSize ) {
    
    if( mImportedSheetSprite != NULL ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mImportedSheetSprite, mSheetOffset );


        if( mSelect ) {
            setDrawColor( 0, 0, 1, 0.25 );
            
            drawRect( mSelectStart.x, mSelectStart.y,
                      mSelectEnd.x, mSelectEnd.y );
            }

            
        if( mCenterSet ) {
            setDrawColor( 1, 1, 1, 0.75 );
            drawSprite( mCenterMarkSprite, mCenterPoint );
            }
        }
    if( mCurrentOverlay != NULL ) {
        setDrawColor( 1, 1, 1, 1 );
        toggleMultiplicativeBlend( true );
        drawSprite( mCurrentOverlay->thumbnailSprite, mOverlayOffset,
                    mOverlayScale, mOverlayRotation, mOverlayFlip );
        toggleMultiplicativeBlend( false );
        }
    }



void EditorImportPage::draw( doublePair inViewCenter, 
                             double inViewSize ) {
    
    if( mProcessedSelectionSprite != NULL ) {
        doublePair pos;
        
        pos.x = lastMouseX;
        pos.y = lastMouseY;
        
        setDrawColor( 1, 1, 1, 1 );
        
        //drawSquare( pos, 100 );

        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mProcessedSelectionSprite, pos );
        }
    
    setDrawColor( 1, 1, 1, 1 );

    if( mOverlayScale != 1.0 ) {
        doublePair pos = { 300, -240 };
        char *string = 
            autoSprintf( "Scale: %.3f", 
                         mOverlayScale );
    
        smallFont->drawString( string, pos, alignLeft );
        
        delete [] string;
        }
    if( mOverlayRotation != 0 ) {
        doublePair pos = { 300, -260 };
        char *string = 
            autoSprintf( "Rot: %.3f", 
                         mOverlayRotation );
    
        smallFont->drawString( string, pos, alignLeft );
        
        delete [] string;
        }

    if( mCurrentOverlay != NULL ) {
        doublePair pos = mObjectEditorButton.getPosition();

        pos.y += 20;
        pos.x -= 380;
        
        drawKeyLegend( &mOverlayKeyLegend, pos );
        }
    if( mImportedSheetSprite != NULL ) {
        doublePair pos = mObjectEditorButton.getPosition();
        
        pos.y += 20;
        pos.x -= 240;
        
        drawKeyLegend( &mSheetKeyLegend, pos );
        }
    }



void EditorImportPage::step() {
    mClearRotButton.setVisible( mOverlayRotation != 0 );
    mClearScaleButton.setVisible( mOverlayScale != 1 );
    }




void EditorImportPage::makeActive( char inFresh ) {
    mMovingOverlay = false;
    mScalingOverlay = false;
    mRotatingOverlay = false;
    
    if( !inFresh ) {
        return;
        }

    mSpritePicker.redoSearch();
    mOverlayPicker.redoSearch();
    }


void EditorImportPage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( mCurrentOverlay != NULL  ) {
        
        if( mMovingOverlay ) {
            doublePair pos = { inX, inY };
            mOverlayOffset = sub( pos, mMovingOverlayPointerStart );
            }
        if( mScalingOverlay ) {
            doublePair pos = { inX, inY };
            mOverlayScale = mMovingOverlayScaleStart + 
                0.75 * mMovingOverlayScaleStart * 
                ( pos.y - mMovingOverlayPointerStart.y ) / 100;
        
            if( mOverlayScale < 0 ) {
                mOverlayScale = 0;
                }
            }
        if( mRotatingOverlay ) {
            doublePair pos = { inX, inY };
            mOverlayRotation = mMovingOverlayRotationStart + 
                ( pos.x - mMovingOverlayPointerStart.x ) / 400;
            }
        }

    }


void EditorImportPage::pointerDown( float inX, float inY ) {
    if( mImportedSheetSprite == NULL ) {
        return;
        }


    if( isLastMouseButtonRight() ) {
        mMovingSheet = true;
        mMovingSheetPointerStart.x = inX - mSheetOffset.x;
        mMovingSheetPointerStart.y = inY - mSheetOffset.y;
        return;
        }
    
    if( mSettingSpriteCenter ) {
    
        if( !mCenterSet ) {
            mCenterPoint.x = inX;
            mCenterPoint.y = inY;
            
            mCenterSet = true;
            }
        else {
            mCenterSet = false;
            }
        return;
        }
    

    if( inX - mSheetOffset.x > - mSheetW / 2
        &&
        inX - mSheetOffset.x < mSheetW / 2 
        &&
        inY - mSheetOffset.y > - mSheetH / 2
        &&
        inY - mSheetOffset.y < mSheetH / 2 ) {
        
        mSelectStart.x = inX;
        mSelectStart.y = inY;
        mSelectEnd.x = inX + 1;
        mSelectEnd.y = inY - 1;
        mSelect = true;
        }
    }



void EditorImportPage::pointerDrag( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    
    if( mMovingSheet ) {
        doublePair pos = { inX, inY };
        mSheetOffset = sub( pos, mMovingSheetPointerStart );
        }
    

    if( mSettingSpriteCenter ) {
        if( mCenterSet ) {
            mCenterPoint.x = inX;
            mCenterPoint.y = inY;
            }
        return;
        }


    if( mSelect ) {
        if( inX > mSelectStart.x ) {
            mSelectEnd.x = inX;
            }
        if( inY < mSelectStart.y ) {    
            mSelectEnd.y = inY;
            }
        
        }
    }



void EditorImportPage::pointerUp( float inX, float inY ) {
    pointerDrag( inX, inY );
    
    if( mSelect ) {
        processSelection();
        mSelect = false;
        }
    
    mMovingSheet = false;
    }



#include "minorGems/graphics/filters/FastBlurFilter.h"



static void addShadow( Image *inImage ) {
    
    int w = inImage->getWidth();
    int h = inImage->getHeight();
    
    int numPixels = w * h;

    Image shadowImage( w, h, 1, true );
    
    shadowImage.pasteChannel( inImage->getChannel( 3 ), 0 );
    
    // now we have black, clear shadow
    FastBlurFilter filter;
    
    for( int i=0; i<20; i++ ) {
        shadowImage.filter( &filter, 0 );
        }
    
    double *shadowAlpha = shadowImage.getChannel( 0 );
    double *imageAlpha = inImage->getChannel( 3 );

    // fade shadow out near bottom visible edge of sprite

    char visibleRowSeen = false;
    int numVisibleRows = 0;
    for( int y = h-1; y>=0; y-- ) {
        
        if( !visibleRowSeen ) {
            // check for visible pixels in this row
            for( int x=0; x<w; x++ ) {
                if( imageAlpha[y*w+x] > 0 ) {
                    visibleRowSeen = true;
                    }
                // clear shadow below visible pixels
                shadowAlpha[y*w+x] = 0;
                }
            }
        else {
            double scaleFactor = numVisibleRows / 15.0;
            if( scaleFactor < 1 ) {
                
                for( int x=0; x<w; x++ ) {
                    shadowAlpha[y*w+x] *= scaleFactor;
                    }
                }
            numVisibleRows++;
            }
        
        }
    
    
    for( int i=0; i<numPixels; i++ ) {
        imageAlpha[i] += shadowAlpha[i];
        if( imageAlpha[i] > 1.0 ) {
            imageAlpha[i] = 1.0;
            }
        }
    
    }



void EditorImportPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 't' ) {
        mMovingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX - mOverlayOffset.x;
        mMovingOverlayPointerStart.y = lastMouseY - mOverlayOffset.y;
        }
    else if( inASCII == 's' ) {
        mScalingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX;
        mMovingOverlayPointerStart.y = lastMouseY;
        mMovingOverlayScaleStart = mOverlayScale;
        }
    else if( inASCII == 'r' ) {
        mRotatingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX;
        mMovingOverlayPointerStart.y = lastMouseY;
        mMovingOverlayRotationStart = mOverlayRotation;
        }
    else if( inASCII == 'c' ) {
        mSettingSpriteCenter = true;
        }
    }



void EditorImportPage::keyUp( unsigned char inASCII ) {
    if( inASCII == 't' ) {
        mMovingOverlay = false;
        }
    else if( inASCII == 's' ) {
        mScalingOverlay = false;
        }
    else if( inASCII == 'r' ) {
        mRotatingOverlay = false;
        }
    else if( inASCII == 'c' ) {
        mSettingSpriteCenter = false;
        }
    }




void EditorImportPage::processSelection() {
    
    if( mProcessedSelection != NULL ) {
        delete mProcessedSelection;
        mProcessedSelection = NULL;
        }
    
    
    char usingCenter = mCenterSet;

    // relative to cut selection
    int centerX = (int)( mCenterPoint.x - mSelectStart.x );
    int centerY = (int)( mSelectStart.y - mCenterPoint.y );

    if( mCenterPoint.x >= mSelectStart.x &&
        mCenterPoint.x < mSelectEnd.x &&
        mCenterPoint.y <= mSelectStart.y &&
        mCenterPoint.y > mSelectEnd.y ) {
        
        usingCenter = mCenterSet;
        }
    else {
        usingCenter = false;
        }
    

    int startImX = (int)( mSelectStart.x + mSheetW/2 - mSheetOffset.x );
    int startImY = (int)( mSheetH/2 - mSelectStart.y + mSheetOffset.y );
    
    int imW = (int)( mSelectEnd.x - mSelectStart.x );
    int imH = (int)( mSelectStart.y - mSelectEnd.y );
    
    if( startImX >= mImportedSheet->getWidth() ) {
        imW -= startImX - ( mImportedSheet->getWidth() - 1 );
        startImX = mImportedSheet->getWidth() - 1;
        }
    if( startImY >= mImportedSheet->getHeight() ) {
        imH -= startImY - ( mImportedSheet->getHeight() - 1 );
        startImY = mImportedSheet->getHeight() - 1;
        }
    
    if( startImX < 0 ) {
        imW -= 0 - startImX;
        startImX = 0;
        }
    if( startImY < 0 ) {
        imH -= 0 - startImY;
        startImY = 0;
        }

    Image *cutImage = 
        mImportedSheet->getSubImage( startImX, startImY, imW, imH );
        
    
    int w = cutImage->getWidth();
    int h = cutImage->getHeight();
    
    int numPixels = w * h;
    
    char *doneMap = new char[numPixels];
    memset( doneMap, 0, numPixels );

    // -1 for unvisited
    // 0 for non-white
    // 1 for white
    char *whiteMap = new char[numPixels];
    memset( whiteMap, -1, numPixels );

    // assume top left pixel is white
    whiteMap[ 0 ] = 1;
    
    // how dark do all channels have to be before we say it's not white.
    double threshold = 0.1;


    double *r = cutImage->getChannel( 0 );
    double *g = cutImage->getChannel( 1 );
    double *b = cutImage->getChannel( 2 );
    double *a = cutImage->getChannel( 3 );
    
    

    char done = false;
    
    while( !done ) {
        
        int foundI = -1;
        for( int i=0; i<numPixels; i++ ) {
            if( whiteMap[i] == 1 && !doneMap[i] ) {
                foundI = i;
                break;
                }
            }
        
        if( foundI == -1 ) {
            done = true;
            }
        else {
            // found a white pixel that has unexplored neighbors
            
            int x = foundI % w;
            int y = foundI / w;
            
            int nDX[4] = { 0, 1, 0, -1 };
            int nDY[4] = { 1, 0, -1, 0 };
            

            char foundUnvisited = false;
            
            for( int n=0; n<4; n++ ) {
                
                int nX = x + nDX[n];
                int nY = y + nDY[n];
                
                
                if( nX >= 0 && nX < w
                    &&
                    nY >= 0 && nY < h ) {
                    

                    int nI = nY * w + nX;
                    
                    if( whiteMap[nI] == -1 ) {
                        
                        foundUnvisited = true;
                        
                        // is it white?
                        if( r[nI] > threshold &&
                            g[nI] > threshold &&
                            b[nI] > threshold ) {
                            
                            whiteMap[nI] = 1;
                            }
                        else {
                            whiteMap[nI] = 0;
                            }
                        
                        break;
                        }
                    }
                }
            
            if( !foundUnvisited ) {
                // all neighbors visited
                doneMap[foundI] = 1;
                }
            }
        

        }


    double paperThreshold = 0.88;
    
    // first, clean up isolated noise points in paper
    
    for( int i=0; i<numPixels; i++ ) {
        if( whiteMap[i] == 1 ) {
            
            if( r[i] <= paperThreshold ) {
                // a point in white areas that we can't count as paper 
                
                int x = i % w;
                int y = i / w;

                // check if all neighbors over threshold
                // if so, this is an isolated noise pixel
                int nDX[4] = { 0, 1, 0, -1 };
                int nDY[4] = { 1, 0, -1, 0 };
            

                char foundNonPaperNeighbor = false;
                
                for( int n=0; n<4; n++ ) {
                    
                    int nX = x + nDX[n];
                    int nY = y + nDY[n];
                
                
                    if( nX >= 0 && nX < w
                        &&
                        nY >= 0 && nY < h ) {
                        
                        int nI = nY * w + nX;
                        
                        if( whiteMap[nI] != 1 ||
                            r[nI] <= paperThreshold ) {
                            
                            foundNonPaperNeighbor = true;
                            break;
                            }
                        }
                    }

                if( ! foundNonPaperNeighbor ) {
                    // isolated non-paper point in middle of paper
                    // noise
                    
                    // turn it full white so we make it transparent below
                    r[i] = 1;
                    printf( "Found noise point at %d, %d\n", x, y );
                    }
                }
            }
        }
    
    for( int i=0; i<numPixels; i++ ) {
        if( whiteMap[i] == 1 ) {
            
            // alpha based on inverted color level
            // so image is transparent where it is most white
            // this gives our black outlines a proper feathered edge
            a[i] = 1.0 - r[i];
            
            
            // however, make sure paper areas that aren't black at all
            // are totally transparent
            if( r[i] > paperThreshold ) {
                a[i] = 0;
                }            
            

            // and make it all black out beyond outer line
            // no white halos
            
            r[i] = 0;
            g[i] = 0;
            b[i] = 0;
            }
        }
   
    delete [] doneMap;
    delete [] whiteMap;




    if( mCurrentOverlay != NULL ) {
        // apply overlay as multiply to cut image
        
        double cosAngle = cos( - 2 * M_PI * mOverlayRotation );
        double sinAngle = sin( - 2 * M_PI * mOverlayRotation );

        int cutW = cutImage->getWidth();
        int cutH = cutImage->getHeight();
        
        int sheetW = mImportedSheet->getWidth();
        int sheetH = mImportedSheet->getHeight();
        
        int overW = mCurrentOverlay->image->getWidth();
        int overH = mCurrentOverlay->image->getHeight();

        // this is relative to our whole sheet
        int offsetW = (overW - sheetW)/2 - (int)mOverlayOffset.x 
            + (int)mSheetOffset.x;
        int offsetH = (overH - sheetH)/2 + (int)mOverlayOffset.y
            - (int)mSheetOffset.y;
        
        // this is relative to what we cut out
        offsetW = startImX + offsetW;
        offsetH = startImY + offsetH;
        

        for( int c=0; c<3; c++ ) {
            double *overC = mCurrentOverlay->image->getChannel( c );
            double *cutC = cutImage->getChannel( c );
            
            for( int y=0; y<cutH; y++ ) {
                int overY = y + offsetH;

                // scale and rotate relative to center
                double overScaledY = (overY - overH/2) / mOverlayScale;

                    
                for( int x=0; x<cutW; x++ ) {
                    
                    int overX = x + offsetW;
                    
                    // scale and rotate relative to center
                    double overScaledX = 
                        (overX - overW/2) / mOverlayScale;

                    double overScaledFinalX = overScaledX;
                    double overScaledFinalY = overScaledY;
                    

                    if( mOverlayRotation != 0 ) {
                        
                        double rotX = 
                            overScaledX * cosAngle - overScaledY * sinAngle;
                        double rotY = 
                            overScaledX * sinAngle + overScaledY * cosAngle;

                        overScaledFinalX = rotX;
                        overScaledFinalY = rotY;
                        }
                    
                    // relative to corner again
                    overScaledFinalX += overW/2 - 0.5;
                    overScaledFinalY += overH/2 - 0.5;
                    
                    if( mOverlayFlip ) {
                        overScaledFinalX = ( overW - 1 ) - overScaledFinalX;
                        overX = ( overW - 1 ) - overX;
                        }

                    if( overScaledFinalY >= 0 && 
                        overScaledFinalY < overH - 1 ) {

                        if( overScaledFinalX >= 0 && 
                            overScaledFinalX < overW - 1 ) {
                            // this cut pixel is hit by overlay
                            
                            int cutI = y * cutW + x;
                            
                            // interpolation?
                            
                            if( mOverlayScale == 1 &&
                                mOverlayRotation == 0 ) {
                                // no interp needed
                                
                                int overI = overY * overW + overX;
                            
                                cutC[ cutI ] *= overC[ overI ];
                                }
                            else {
                                // bilinear interp
                                
                                int floorX = (int)floor(overScaledFinalX);
                                int ceilX = (int)ceil(overScaledFinalX);
                                int floorY = (int)floor(overScaledFinalY);
                                int ceilY = (int)ceil(overScaledFinalY);
    

                                double cornerA1 = 
                                    overC[ floorX + floorY * overW ];
                                double cornerA2 = 
                                    overC[ ceilX + floorY * overW ];
                                
                                double cornerB1 = 
                                    overC[ floorX + ceilY * overW ];

                                double cornerB2 = 
                                    overC[ ceilX + ceilY * overW ];
                                

                                double xOffset = overScaledFinalX - floorX;
                                double yOffset = overScaledFinalY - floorY;
    
    
                                double topBlend = 
                                    cornerA2 * xOffset + 
                                    (1-xOffset) * cornerA1;
    
                                double bottomBlend = cornerB2 * xOffset + 
                                    (1-xOffset) * cornerB1;
    

                                double blend = 
                                    bottomBlend * yOffset + 
                                    (1-yOffset) * topBlend;
                                
                                cutC[ cutI ] *= blend;
                                }
                            }
                        }
                    }
                }
            }
        }



    // now trim down image just around non-transparent part so
    // it is properly centered

    int firstX = w;
    int lastX = -1;
    
    int firstY = h;
    int lastY = -1;
    
    for( int y=0; y<h; y++ ) {
        for( int x=0; x<w; x++ ) {
            int i = y * w + x;
            
            if( a[i] > 0.0 ) {
                //printf( "a at (%d,%d) = %f\n", x, y, a[i] );
                
                if( x < firstX ) {
                    firstX = x;
                    }
                if( x > lastX ) {
                    lastX = x;
                    }
                if( y < firstY ) {
                    firstY = y;
                    }
                if( y > lastY ) {
                    lastY = y;
                    }

                // for testing, highlight main area of image
                //a[i] = 1;
                //r[i] = 1;
                }
            }
        }
    
    Image *trimmedImage = NULL;
    
    if( lastX > firstX && lastY > firstY ) {
        
        trimmedImage = 
            cutImage->getSubImage( firstX, firstY, 
                                   1 + lastX - firstX, 1 + lastY - firstY );
        }
    else {
        // no non-trans areas, don't trim it
        trimmedImage = cutImage->copy();
        }
    
    delete cutImage;
    
    // make relative to new trim
    centerX -= firstX;
    centerY -= firstY;

    
    // expand to make it big enough for shadow
    w = trimmedImage->getWidth();
    h = trimmedImage->getHeight();
    
    Image *shadowImage = trimmedImage->expandImage( w + 20 + 20, h + 20 + 20 );
    
    delete trimmedImage;

    centerX += 20;
    centerY += 20;


    addShadow( shadowImage );    
    

    if( usingCenter ) {
        w = shadowImage->getWidth();
        h = shadowImage->getHeight();

        if( centerX != w / 2 ||
            centerY != h / 2 ) {
            
            // not currently centered

            // make room to center.
            Image *bigImage = 
                shadowImage->expandImage( 3 * w, 
                                          3 * h );

            int xRadA = centerX;
            int xRadB = w - centerX;

            int xRad = xRadA;
            
            if( xRadB > xRad ) {
                xRad = xRadB;
                }

            
            int yRadA = centerY;
            int yRadB = h - centerY;

            int yRad = yRadA;
            
            if( yRadB > yRad ) {
                yRad = yRadB;
                }
            


            delete shadowImage;
            
            centerX += w;
            centerY += h;

            

            shadowImage = 
                bigImage->getSubImage( centerX - xRad, centerY - yRad, 
                                       2 * xRad, 2 * yRad );

            delete bigImage;
            }
            
        }
    

    
    /*
      // for testing to check for non-transparent areas

    w = shadowImage->getWidth();
    h = shadowImage->getHeight();
    
    numPixels = w * h;
    r = shadowImage->getChannel( 0 );
    a = shadowImage->getChannel( 3 );
    for( int i=0; i<numPixels; i++ ) {
        if( a[i] > 0.002 ) {
            r[i] = 1;
            a[i] = 1;
            }
        }
    */


    mProcessedSelection = expandToPowersOfTwo( shadowImage );
    delete shadowImage;

    




    if( mProcessedSelectionSprite != NULL ) {
        freeSprite( mProcessedSelectionSprite );
        mProcessedSelectionSprite = NULL;
        }
    mProcessedSelectionSprite = fillSprite( mProcessedSelection, false );
    mSaveSpriteButton.setVisible( true );
    mSaveOverlayButton.setVisible( false );
    }
