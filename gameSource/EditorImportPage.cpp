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
          mProcessedShadow( NULL ),
          mProcessedShadowSprite( NULL ),
          mSelectionMultiplicative( false ),
          mShadowSlider( smallFont, 0, 200, 2,
                         100, 20,
                         0, 1, "Shadow" ),
          mSolidCheckbox( 215, 200, 2 ),
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
          mCenterSet( true ),
          mClearRotButton( smallFont, -300, -280, "0 Rot" ),
          mClearScaleButton( smallFont, -300, -240, "1 Scale" ),
          mFlipOverlayButton( smallFont, -230, -280, "Flip H" ),
          mClearOverlayButton( smallFont, -230, -240, "X Ovly" ),
          mShowTagMessage( false ) {


    mCenterPoint.x = 0;
    mCenterPoint.y = 0;

    addComponent( &mShadowSlider );
    mShadowSlider.setValue( 1.0 );
    
    addComponent( &mSolidCheckbox );
    mSolidCheckbox.addActionListener( this );
    mSolidCheckbox.setToggled( true );
    
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


    mMovingSheet = false;
    doublePair offset = { 0, 0 };
    mSheetOffset = offset;
    mMovingSheetPointerStart = offset;


    mSettingSpriteCenter = false;
        
    mMovingOverlay = false;
    mScalingOverlay = false;
    mRotatingOverlay = false;
        


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

    if( mProcessedShadow != NULL ) {
        delete mProcessedShadow;
        }
    if( mProcessedShadowSprite != NULL ) {
        freeSprite( mProcessedShadowSprite );
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
    
    if( mCurrentOverlay.getLastElementDirect()->id == inOverlayID ) {
        actionPerformed( &mClearOverlayButton );
        }
    }



static void addShadow( Image *inImage, Image *inShadow, double inWeight ) {
    
    int w = inImage->getWidth();
    int h = inImage->getHeight();
    
    int numPixels = w * h;
    
    double *shadowAlpha = inShadow->getChannel( 3 );
    double *imageAlpha = inImage->getChannel( 3 );
    
    for( int i=0; i<numPixels; i++ ) {
        imageAlpha[i] += inWeight * shadowAlpha[i];
        if( imageAlpha[i] > 1.0 ) {
            imageAlpha[i] = 1.0;
            }
        }
    
    }





void EditorImportPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mImportButton ||
        inTarget == &mImportOverlayButton ) {
        
        if( mProcessedSelectionSprite != NULL ) {
            freeSprite( mProcessedSelectionSprite );
            mProcessedSelectionSprite = NULL;
            }

        if( mProcessedShadowSprite != NULL ) {
            freeSprite( mProcessedShadowSprite );
            mProcessedShadowSprite = NULL;
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
    
            mSheetOffset.x = 0;
            mSheetOffset.y = 0;
            mMovingSheet = false;


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
            
            if( mShadowSlider.isVisible() &&
                mShadowSlider.getValue() > 0 ) {
                
                addShadow( mProcessedSelection, mProcessedShadow,
                           mShadowSlider.getValue() );
                
                freeSprite( mProcessedSelectionSprite );
                
                mProcessedSelectionSprite = 
                    fillSprite( mProcessedSelection, false );
                }
            


            addSprite( tag, 
                       mProcessedSelectionSprite,
                       mProcessedSelection,
                       mSelectionMultiplicative );

            mSpritePicker.redoSearch();
            
            // don't let it get freed now
            mProcessedSelectionSprite = NULL;

            freeSprite( mProcessedShadowSprite );
            mProcessedShadowSprite = NULL;

            mSaveSpriteButton.setVisible( false );
            
            mShowTagMessage = false;
            }
        else {
            mShowTagMessage = true;
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

            mShowTagMessage = false;
            }
        else {
            mShowTagMessage = true;
            }
        

        delete [] tag;
        }
    else if( inTarget == &mClearRotButton ) {
        mClearRotButton.setVisible( false );
        *( mOverlayRotation.getLastElement() ) = 0;
        }
    else if( inTarget == &mClearScaleButton ) {
        mClearScaleButton.setVisible( false );
        *( mOverlayScale.getLastElement() ) = 1;
        }    
    else if( inTarget == &mFlipOverlayButton ) {
        *( mOverlayFlip.getLastElement() ) = 
            !( mOverlayFlip.getLastElementDirect() );
        }    
    else if( inTarget == &mClearOverlayButton ) {
        if( mCurrentOverlay.size() > 0 ) {
            int lastIndex = mCurrentOverlay.size() - 1;
            
            mCurrentOverlay.deleteElement( lastIndex );
            mOverlayRotation.deleteElement( lastIndex );
            mOverlayScale.deleteElement( lastIndex );
            mOverlayFlip.deleteElement( lastIndex );
            mOverlayOffset.deleteElement( lastIndex );
            }
        if( mCurrentOverlay.size() == 0 ) {
            mClearScaleButton.setVisible( false );
            mClearRotButton.setVisible( false );
            mFlipOverlayButton.setVisible( false );
            mClearOverlayButton.setVisible( false );
            }
        else {
            mClearScaleButton.setVisible( 
                mOverlayScale.getLastElementDirect() != 1 );
            mClearRotButton.setVisible( 
                mOverlayRotation.getLastElementDirect() != 0 );
            }
        }    
    else if( inTarget == &mOverlayPicker ) {
        int overlayID = mOverlayPicker.getSelectedObject();
    
        if( overlayID != -1 ) {
            mCurrentOverlay.push_back( getOverlay( overlayID ) );
            doublePair offset = { 0, 0 };
            
            mOverlayOffset.push_back( offset );

            mMovingOverlay = false;
            mScalingOverlay = false;
            mRotatingOverlay = false;
            
            mOverlayFlip.push_back( 0 );
            
            mFlipOverlayButton.setVisible( true );
            mClearOverlayButton.setVisible( true );

            mOverlayScale.push_back( 1.0 );
            mOverlayRotation.push_back( 0 );
            }
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mSolidCheckbox ) {
        if( mSolidCheckbox.getToggled() ) {
            mShadowSlider.setVisible( true );
            mSelectionMultiplicative = false;
            }
        else {
            mShadowSlider.setVisible( false );
            mSelectionMultiplicative = true;
            }
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

    setDrawColor( 1, 1, 1, 1 );
    toggleMultiplicativeBlend( true );
    for( int i=0; i<mCurrentOverlay.size(); i++ ) {
        
        drawSprite( mCurrentOverlay.getElementDirect(i)->thumbnailSprite, 
                    mOverlayOffset.getElementDirect(i),
                    mOverlayScale.getElementDirect(i), 
                    mOverlayRotation.getElementDirect(i), 
                    mOverlayFlip.getElementDirect(i) );
        }
    toggleMultiplicativeBlend( false );
    }



void EditorImportPage::draw( doublePair inViewCenter, 
                             double inViewSize ) {
    
    if( mProcessedSelectionSprite != NULL ) {
        doublePair pos;
        
        pos.x = lastMouseX;
        pos.y = lastMouseY;
        
        setDrawColor( 1, 1, 1, 1 );
        
        //drawSquare( pos, 100 );

        if( mShadowSlider.isVisible() ) {
            setDrawColor( 1, 1, 1, mShadowSlider.getValue() );
            drawSprite( mProcessedShadowSprite, pos );
            }
            
        setDrawColor( 1, 1, 1, 1 );
        
        if( mSelectionMultiplicative ) {
            toggleMultiplicativeBlend( true );
            }
        
        drawSprite( mProcessedSelectionSprite, pos );
        
        if( mSelectionMultiplicative ) {
            toggleMultiplicativeBlend( false );
            }
        }
    
    setDrawColor( 1, 1, 1, 1 );

    if( mOverlayScale.getLastElementDirect() != 1.0 ) {
        doublePair pos = { 300, -240 };
        char *string = 
            autoSprintf( "Scale: %.3f", 
                         mOverlayScale.getLastElementDirect() );
    
        smallFont->drawString( string, pos, alignLeft );
        
        delete [] string;
        }
    if( mOverlayRotation.getLastElementDirect() != 0 ) {
        doublePair pos = { 300, -260 };
        char *string = 
            autoSprintf( "Rot: %.3f", 
                         mOverlayRotation.getLastElementDirect() );
    
        smallFont->drawString( string, pos, alignLeft );
        
        delete [] string;
        }

    if( mCurrentOverlay.size() > 0 ) {
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
    
    if( mShowTagMessage ) {
        setDrawColor( 1, 0, 0, 1 );
        
        doublePair pos = mSpriteTagField.getPosition();
        
        pos.y += 40;
        
        smallFont->drawString( "Tag Required", pos, alignCenter );
        }


    setDrawColor( 1, 1, 1, 1 );
    doublePair pos = mSolidCheckbox.getPosition();
    pos.x -= 20;
    smallFont->drawString( "Solid", pos, alignRight );
    
    }



void EditorImportPage::step() {
    if( mCurrentOverlay.size() > 0 ) {        
        mClearRotButton.setVisible( 
            mOverlayRotation.getLastElementDirect() != 0 );
        mClearScaleButton.setVisible( 
            mOverlayScale.getLastElementDirect() != 1 );
        }
    }





void EditorImportPage::makeActive( char inFresh ) {
    mMovingOverlay = false;
    mScalingOverlay = false;
    mRotatingOverlay = false;
    
    if( !inFresh ) {
        return;
        }

    mShowTagMessage = false;

    mSpritePicker.redoSearch();
    mOverlayPicker.redoSearch();
    }


void EditorImportPage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;

    if( mCurrentOverlay.size() > 0  ) {
        
        if( mMovingOverlay ) {
            doublePair pos = { inX, inY };
            *( mOverlayOffset.getLastElement() ) = 
                sub( pos, mMovingOverlayPointerStart );
            }
        if( mScalingOverlay ) {
            doublePair pos = { inX, inY };
            *( mOverlayScale.getLastElement() ) = mMovingOverlayScaleStart + 
                0.75 * mMovingOverlayScaleStart * 
                ( pos.y - mMovingOverlayPointerStart.y ) / 100;
        
            if( mOverlayScale.getLastElement() < 0 ) {
                *( mOverlayScale.getLastElement() ) = 0;
                }
            }
        if( mRotatingOverlay ) {
            doublePair pos = { inX, inY };
            *( mOverlayRotation.getLastElement() ) = 
                mMovingOverlayRotationStart + 
                ( pos.x - mMovingOverlayPointerStart.x ) / 400;
            }
        }

    }


void EditorImportPage::pointerDown( float inX, float inY ) {
    if( mImportedSheetSprite == NULL ) {
        return;
        }

    if( inX > -210 && inX < 210 && 
        inY > -210 && inY < 190 ) {
        TextField::unfocusAll();
        }
    else {
        // pointer down outside center area
        return;
        }


    if( isLastMouseButtonRight() ) {
        mMovingSheet = true;
        mMovingSheetPointerStart.x = inX - mSheetOffset.x;
        mMovingSheetPointerStart.y = inY - mSheetOffset.y;
        return;
        }
    
    if( mSettingSpriteCenter ) {
    
        mCenterPoint.x = inX;
        mCenterPoint.y = inY;
        
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
        mCenterPoint.x = inX;
        mCenterPoint.y = inY;
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



#include "minorGems/graphics/filters/BoxBlurFilter.h"



static Image *getShadow( Image *inImage ) {
    
    int w = inImage->getWidth();
    int h = inImage->getHeight();
    
    Image *shadowImage = new Image( w, h, 4, true );
    
    shadowImage->pasteChannel( inImage->getChannel( 3 ), 3 );
    
    BoxBlurFilter filter( 4 );
        
    shadowImage->filter( &filter, 3 );
        
    
    double *shadowAlpha = shadowImage->getChannel( 3 );
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
    
    return shadowImage;
    }





void EditorImportPage::keyDown( unsigned char inASCII ) {

    if( TextField::isAnyFocused() ) {
        return;
        }

    if( inASCII == 't' ) {
        mMovingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX - 
            mOverlayOffset.getLastElementDirect().x;
        mMovingOverlayPointerStart.y = lastMouseY - 
            mOverlayOffset.getLastElementDirect().y;
        }
    else if( inASCII == 's' ) {
        mScalingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX;
        mMovingOverlayPointerStart.y = lastMouseY;
        if( mCurrentOverlay.size() > 0 ) {
            mMovingOverlayScaleStart = mOverlayScale.getLastElementDirect();
            }
        }
    else if( inASCII == 'r' ) {
        mRotatingOverlay = true;
        mMovingOverlayPointerStart.x = lastMouseX;
        mMovingOverlayPointerStart.y = lastMouseY;
        if( mCurrentOverlay.size() > 0 ) {
            mMovingOverlayRotationStart = 
                mOverlayRotation.getLastElementDirect();
            }
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
    if( mProcessedShadow != NULL ) {
        delete mProcessedShadow;
        mProcessedShadow = NULL;
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

    // -1 for unvisited
    // 0 for non-white
    // 1 for white
    char *whiteMap = new char[numPixels];
    memset( whiteMap, -1, numPixels );

    // assume top left pixel is white
    whiteMap[ 0 ] = 1;
    
    // how dark do all channels have to be before we say it's not white.
    double threshold = 0.2;


    double *r = cutImage->getChannel( 0 );
    double *g = cutImage->getChannel( 1 );
    double *b = cutImage->getChannel( 2 );
    double *a = cutImage->getChannel( 3 );
    
    

    char done = false;


    if( ! mSolidCheckbox.getToggled() ) {
        // don't search for solid areas at all
        done = true;
        memset( whiteMap, 1, numPixels );
        mSelectionMultiplicative = true;
        }
    else {
        mSelectionMultiplicative = false;
        }
    
    SimpleVector<int> whitePixelsWithUnexploredNeighbors;
    
    whitePixelsWithUnexploredNeighbors.push_back( 0 );
    
    while( !done ) {
        
        int foundI = -1;

        int numFrontier = whitePixelsWithUnexploredNeighbors.size();

        if( numFrontier > 0 ) {
            
            foundI = whitePixelsWithUnexploredNeighbors.getElementDirect(
                numFrontier - 1 );
            whitePixelsWithUnexploredNeighbors.deleteElement( 
                numFrontier - 1 );
            
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
            
            for( int n=0; n<4; n++ ) {
                
                int nX = x + nDX[n];
                int nY = y + nDY[n];
                
                
                if( nX >= 0 && nX < w
                    &&
                    nY >= 0 && nY < h ) {
                    

                    int nI = nY * w + nX;
                    
                    if( whiteMap[nI] == -1 ) {
                        
                        // is it white?
                        if( r[nI] > threshold &&
                            g[nI] > threshold &&
                            b[nI] > threshold ) {
                            
                            whiteMap[nI] = 1;
                            
                            whitePixelsWithUnexploredNeighbors.push_back( nI );
                            }
                        else {
                            whiteMap[nI] = 0;
                            }
                        }
                    }
                }
            
            }
        

        }


    double paperThreshold = 0.88;
    
    // first, clean up isolated noise points in paper
    
    for( int i=0; i<numPixels; i++ ) {
        if( whiteMap[i] == 1 ) {
            
            if( r[i] <= paperThreshold && g[i] <= paperThreshold &&
                b[i] <= paperThreshold ) {
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
                            ( r[nI] <= paperThreshold &&
                              g[nI] <= paperThreshold &&
                              b[nI] <= paperThreshold ) ) {
                            
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
                    g[i] = 1;
                    b[i] = 1;
                    printf( "Found noise point at %d, %d\n", x, y );
                    }
                }
            }
        }
    
    
    if( !mSelectionMultiplicative )
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
   
    delete [] whiteMap;



    for( int i=0; i<mCurrentOverlay.size(); i++ ) {
        // apply overlay as multiply to cut image
        
        double cosAngle = 
            cos( - 2 * M_PI * mOverlayRotation.getElementDirect(i) );
        double sinAngle = 
            sin( - 2 * M_PI * mOverlayRotation.getElementDirect(i) );

        int cutW = cutImage->getWidth();
        int cutH = cutImage->getHeight();
        
        int sheetW = mImportedSheet->getWidth();
        int sheetH = mImportedSheet->getHeight();
        
        int overW = mCurrentOverlay.getElementDirect(i)->image->getWidth();
        int overH = mCurrentOverlay.getElementDirect(i)->image->getHeight();

        // this is relative to our whole sheet
        int offsetW = (overW - sheetW)/2 
            - (int)mOverlayOffset.getElementDirect(i).x 
            + (int)mSheetOffset.x;
        int offsetH = (overH - sheetH)/2 
            + (int)mOverlayOffset.getElementDirect(i).y
            - (int)mSheetOffset.y;
        
        // this is relative to what we cut out
        offsetW = startImX + offsetW;
        offsetH = startImY + offsetH;
        

        for( int c=0; c<3; c++ ) {
            double *overC = 
                mCurrentOverlay.getElementDirect(i)->image->getChannel( c );
            double *cutC = cutImage->getChannel( c );
            
            for( int y=0; y<cutH; y++ ) {
                int overY = y + offsetH;

                // scale and rotate relative to center
                double overScaledY = (overY - overH/2) / 
                    mOverlayScale.getElementDirect(i);

                    
                for( int x=0; x<cutW; x++ ) {
                    
                    int overX = x + offsetW;
                    
                    // scale and rotate relative to center
                    double overScaledX = 
                        (overX - overW/2) / 
                        mOverlayScale.getElementDirect(i);

                    double overScaledFinalX = overScaledX;
                    double overScaledFinalY = overScaledY;
                    

                    if( mOverlayRotation.getElementDirect(i) != 0 ) {
                        
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
                    
                    if( mOverlayFlip.getElementDirect(i) ) {
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
                            
                            if( mOverlayScale.getElementDirect(i) == 1 &&
                                mOverlayRotation.getElementDirect(i) == 0 ) {
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
    

    if( mSelectionMultiplicative ) {
        // set all transparent areas to white (black border added by expansion)
        r = mProcessedSelection->getChannel( 0 );
        g = mProcessedSelection->getChannel( 1 );
        b = mProcessedSelection->getChannel( 2 );
        a = mProcessedSelection->getChannel( 3 );

        int numPixels = mProcessedSelection->getWidth() *
            mProcessedSelection->getHeight();
        
        for( int i=0; i<numPixels; i++ ) {
            if( a[i] == 0 ) {
                r[i] =  1;
                g[i] =  1;
                b[i] =  1;
                }
            else if( r[i] > paperThreshold &&
                     g[i] > paperThreshold &&
                     b[i] > paperThreshold ) {
                // also make areas white that are close enough to white
                // thus, we don't have square, slightly-darkening ghost
                // around image
                r[i] =  1;
                g[i] =  1;
                b[i] =  1;
                }

            double min = r[i];
            
            if( g[i] < min ) {
                min = g[i];
                }
            if( b[i] < min ) {
                min = b[i];
                }
            
            // alpha is set to inverse of whiteness
            // (for shadow making, in case user re-checks Solid after 
            //  extraction)
            // Note that if image remains multiplicative, alpha will be ignored
            // when it is applied.
            a[i] = 1 - min;
            }
        }
    

    mProcessedShadow = getShadow( mProcessedSelection );    
    

    delete shadowImage;

    




    if( mProcessedSelectionSprite != NULL ) {
        freeSprite( mProcessedSelectionSprite );
        mProcessedSelectionSprite = NULL;
        }
    mProcessedSelectionSprite = fillSprite( mProcessedSelection, false );
    
    if( mProcessedShadowSprite != NULL ) {
        freeSprite( mProcessedShadowSprite );
        mProcessedShadowSprite = NULL;
        }
    mProcessedShadowSprite = fillSprite( mProcessedShadow, false );
    

    mSaveSpriteButton.setVisible( true );
    mSaveOverlayButton.setVisible( false );
    }
