#include "EditorImportPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"


#include "whiteSprites.h"


extern Font *mainFont;
extern Font *smallFont;


#include "SpritePickable.h"
#include "OverlayPickable.h"

static SpritePickable spritePickable;

static OverlayPickable overlayPickable;


#include "EditorObjectPage.h"

extern EditorObjectPage *objectPage;


#include "zoomView.h"


// label distance from checkboxes
static int checkboxSep = 12;


EditorImportPage::EditorImportPage()
        : mImportButton( smallFont, +170, 280, "Sprite Import" ),
          mImportLinesButton( smallFont, +170, 240, "Lines Import" ),
          mNextSpriteImportButton( smallFont, +240, 280, ">" ),
          mPrevSpriteImportButton( smallFont, +100, 280, "<" ),
          mNextLinesImportButton( smallFont, +240, 240, ">" ),
          mPrevLinesImportButton( smallFont, +100, 240, "<" ),
          mCurrentSpriteImportCacheIndex( 0 ),
          mCurrentLinesImportCacheIndex( 0 ),
          mImportPathOverride( NULL ),
          mXTopLinesButton( smallFont, +280, 240, "X" ),
          mImportOverlayButton( smallFont, +370, 260, "Overlay Import" ),
          mSelect( false ),
          mImportedSheet( NULL ),
          mImportedSheetSprite( NULL ),
          mWhiteOutSheet( NULL ),
          mWhiteOutSheetSprite( NULL ),
          mProcessedSelection( NULL ),
          mProcessedSelectionSprite( NULL ),
          mProcessedShadow( NULL ),
          mProcessedShadowSprite( NULL ),
          mSelectionMultiplicative( false ),
          mShadowSlider( smallFont, 90, 200, 2,
                         100, 20,
                         0, 1, "Shadow" ),
          mSolidCheckbox( 305, 200, 2 ),
          mBlackLineThresholdSlider( smallFont, 90, 170, 2,
                                 100, 20,
                                 0, 1, "Black Threshold" ),
          mBlackLineThresholdDefaultButton( smallFont, 305, 170, "D" ),
          mSaturationSlider( smallFont, 90, 140, 2,
                             100, 20,
                             -1, 2, "Saturation" ),
          mSaturationDefaultButton( smallFont, 305, 140, "D" ),
          mSpriteTagField( mainFont, 
                           0,  -260, 6,
                           false,
                           "Tag", NULL, " " ),
          mSaveSpriteButton( mainFont, 210, -260, "Save" ),
          mSaveOverlayButton( smallFont, 310, -260, "Save Overlay" ),
          mSpritePicker( &spritePickable, -410, 90 ),
          mOverlayPicker( &overlayPickable, 410, 90 ),
          mSpriteTrimEditorButton( mainFont, -460, 260, "Trim" ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mCenterMarkSprite( loadSprite( "centerMark.tga" ) ),
          mInternalPaperMarkSprite( loadSprite( "internalPaperMark.tga" ) ),
          mCenterSet( true ),
          mClearRotButton( smallFont, -400, -280, "0 Rot" ),
          mClearScaleButton( smallFont, -400, -240, "1 Scale" ),
          mFlipOverlayButton( smallFont, -330, -280, "Flip H" ),
          mClearOverlayButton( smallFont, -330, -240, "X Ovly" ),
          mShowTagMessage( false ) {


    mCenterPoint.x = 0;
    mCenterPoint.y = 0;

    addComponent( &mShadowSlider );
    mShadowSlider.setValue( 0.5 );
    
    addComponent( &mBlackLineThresholdSlider );
    mBlackLineThresholdSlider.setValue( 0.2 );
    mBlackLineThresholdSlider.addActionListener( this );

    addComponent( &mBlackLineThresholdDefaultButton );
    mBlackLineThresholdDefaultButton.addActionListener( this );


    addComponent( &mSaturationSlider );
    mSaturationSlider.setValue( 0.5 );
    mSaturationSlider.addActionListener( this );

    addComponent( &mSaturationDefaultButton );
    mSaturationDefaultButton.addActionListener( this );


    addComponent( &mSolidCheckbox );
    mSolidCheckbox.addActionListener( this );
    mSolidCheckbox.setToggled( true );
    
    addComponent( &mImportButton );
    addComponent( &mImportLinesButton );
    
    addComponent( &mNextSpriteImportButton );
    addComponent( &mPrevSpriteImportButton );
    addComponent( &mNextLinesImportButton );
    addComponent( &mPrevLinesImportButton );
    

    addComponent( &mXTopLinesButton );
    addComponent( &mImportOverlayButton );
    addComponent( &mSpriteTagField );
    addComponent( &mSaveSpriteButton );
    addComponent( &mSaveOverlayButton );
    addComponent( &mSpritePicker );
    addComponent( &mOverlayPicker );

    addComponent( &mSpriteTrimEditorButton );
    addComponent( &mObjectEditorButton );

    addComponent( &mClearRotButton );
    addComponent( &mClearScaleButton );
    addComponent( &mFlipOverlayButton );
    addComponent( &mClearOverlayButton );
    

    mImportButton.addActionListener( this );
    mImportLinesButton.addActionListener( this );
    
    mNextSpriteImportButton.addActionListener( this );
    mPrevSpriteImportButton.addActionListener( this );
    mNextLinesImportButton.addActionListener( this );
    mPrevLinesImportButton.addActionListener( this );
   
    mNextSpriteImportButton.setVisible( false );
    mNextLinesImportButton.setVisible( false );
    

    mXTopLinesButton.addActionListener( this );
    mXTopLinesButton.setVisible( false );
    
    mImportOverlayButton.addActionListener( this );
    
    mSaveSpriteButton.addActionListener( this );
    mSaveOverlayButton.addActionListener( this );
    
    mSpriteTrimEditorButton.addActionListener( this );
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
    mWhitingOut = false;
    mAnyWhiteOutSet = false;
    
    mPlacingInternalPaper = false;

    mMovingOverlay = false;
    mScalingOverlay = false;
    mRotatingOverlay = false;
        


    addKeyClassDescription( &mSheetKeyLegend, "r-mouse", "Mv sheet" );
    addKeyDescription( &mSheetKeyLegend, 'c', "Mv sprite center" );
    addKeyDescription( &mSheetKeyLegend, 'x', "Copy pixel color" );
    addKeyClassDescription( &mSheetKeyLegend, "w-click", "white out" );
    addKeyDescription( &mSheetKeyLegend, 'W', "clear white out" );
    addKeyClassDescription( &mSheetKeyLegend, "ijkl", "Mv sheet" );
    addKeyClassDescription( &mSheetKeyLegend, "Ctr/Shft", "Bigger jumps" );

    addKeyClassDescription( &mSheetKeyLegendB, "p-click", "internal paper" );
    addKeyDescription( &mSheetKeyLegendB, 'P', "clear internal paper" );
    

    addKeyClassDescription( &mLinesKeyLegend, "arrows", "Mv lines" );
    

    addKeyDescription( &mOverlayKeyLegend, 't', "Mv overlay" );
    addKeyDescription( &mOverlayKeyLegend, 's', "Scale overlay" );
    addKeyDescription( &mOverlayKeyLegend, 'r', "Rot overlay" );
    
    }




EditorImportPage::~EditorImportPage() {
    if( mImportedSheet != NULL ) {
        delete mImportedSheet;
        }
    
    if( mWhiteOutSheet != NULL ) {
        delete mWhiteOutSheet;
        freeSprite( mWhiteOutSheetSprite );
        mWhiteOutSheetSprite = NULL;
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
    freeSprite( mInternalPaperMarkSprite );

    clearLines();
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
        inTarget == &mImportLinesButton ||
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
        char loadedFromCache = false;
        
        if( inTarget == &mImportButton || inTarget == &mImportLinesButton ) {
            
            char *importPath;
            
            if( mImportPathOverride != NULL ) {
                importPath = mImportPathOverride;
                mImportPathOverride = NULL;
                
                loadedFromCache = true;
                }
            else {
                importPath = 
                    SettingsManager::getStringSetting( "editorImportPath" );
                
                if( inTarget == &mImportButton ) {
                    mNextSpriteImportButton.setVisible( false );
                    mCurrentSpriteImportCacheIndex = 0;
                    }
                else {
                    mNextLinesImportButton.setVisible( false );
                    mCurrentLinesImportCacheIndex = 0;
                    }
                }
            
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
    
            mMovingSheet = false;


            char imported = false;
            
            if( importFile->exists() ) {
                PNGImageConverter converter;
                
                FileInputStream stream( importFile );
                
                Image *image = converter.deformatImage( &stream );
                
                if( image != NULL ) {
                    
                    if( inTarget == &mImportButton ||
                        inTarget == &mImportLinesButton ) {
                        
                        if( ! loadedFromCache ) {
                            
                            if( inTarget == &mImportButton ) {
                                mPrevSpriteImportButton.setVisible( true );
                                }
                            else {
                                mPrevLinesImportButton.setVisible( true );
                                }

                            // cache it
                        
                            const char *cacheName = "spriteImportCache";
                    
                            if( inTarget == &mImportLinesButton ) {
                                cacheName = "lineImportCache";
                                }
                        
                            File cacheDir( NULL, cacheName );
                            if( !cacheDir.exists() ) {
                                cacheDir.makeDirectory();
                                }
                    
                            if( cacheDir.exists() && 
                                cacheDir.isDirectory() ) {
                            
                                File *nextFile = 
                                    cacheDir.getChildFile( "next.txt" );
                            
                                int nextIndex = 0;
                            
                                if( nextFile->exists() ) {
                                    nextIndex = 
                                        nextFile->readFileIntContents( 0 );
                                    }

                                char *cacheName = autoSprintf( "%d.png",
                                                               nextIndex );
                                
                                File *cacheFile = 
                                    cacheDir.getChildFile( cacheName );
                            
                                delete [] cacheName;
                            
                                char alreadyThere = false;
                            
                            
                                if( nextIndex != 0 ) {
                                    char *lastCacheName = 
                                        autoSprintf( "%d.png",
                                                     nextIndex - 1 );
                            
                                    File *lastCacheFile = 
                                        cacheDir.getChildFile( lastCacheName );
                                
                                    delete [] lastCacheName;
                                
                                    if( importFile->contentsMatches( 
                                            lastCacheFile ) ) {
                                    
                                        alreadyThere = true;
                                        }

                                    delete lastCacheFile;
                                    }
                            
                            
                                if( ! alreadyThere ) {
                                    importFile->copy( cacheFile );

                                    if( inTarget == &mImportButton ) {
                                        mCurrentSpriteImportCacheIndex = 
                                            nextIndex;
                                        }
                                    else {
                                        mCurrentLinesImportCacheIndex = 
                                            nextIndex;
                                        }
                            
                                    nextIndex ++;
                            
                                    nextFile->writeToFile( nextIndex );
                                    }
                                else {
                                    if( inTarget == &mImportButton ) {
                                        mCurrentSpriteImportCacheIndex = 
                                            nextIndex - 1;
                                        }
                                    else {
                                        mCurrentLinesImportCacheIndex = 
                                            nextIndex - 1;
                                        }
                                    }
                                
                            
                                delete cacheFile;

                                delete nextFile;
                                }
                            }
                        }
                    

                    // expand to powers of 2
                    Image *expanded  = expandToPowersOfTwo( image );
                    
                    if( inTarget == &mImportButton || 
                        inTarget == &mImportOverlayButton ) {
                        
                        if( mImportedSheet != NULL ) {
                            delete mImportedSheet;
                            mImportedSheet = NULL;
                            }
                        
                        mSheetOffset.x = 0;
                        mSheetOffset.y = 0;
                        
                        mImportedSheet = expanded;
                        
                        mSheetW = mImportedSheet->getWidth();
                        mSheetH = mImportedSheet->getHeight();
                        
                    
                        if( mImportedSheetSprite != NULL ) {
                            freeSprite( mImportedSheetSprite );
                            mImportedSheetSprite = NULL;
                            }
                        mImportedSheetSprite = 
                            fillSprite( mImportedSheet, false );


                        if( mWhiteOutSheet != NULL ) {
                            delete mWhiteOutSheet;
                            freeSprite( mWhiteOutSheetSprite );
                            }
                        
                        mWhiteOutSheet = new Image( mSheetW, mSheetH,
                                                    1, true );
                        mWhiteOutSheetSprite = 
                            fillWhiteSprite( mWhiteOutSheet );
                        
                        mAnyWhiteOutSet = false;
                        }
                    else {
                        // convert to grayscale
                        // luminosity formula:
                        // 0.21 R + 0.72 G + 0.07 B
                        int numPixels = 
                            expanded->getWidth() * expanded->getHeight();
                        double *r = expanded->getChannel( 0 );
                        double *g = expanded->getChannel( 1 );
                        double *b = expanded->getChannel( 2 );
                        double *a = expanded->getChannel( 3 );
                        
                        for( int i=0; i<numPixels; i++ ) {
                            double l = 0.21 * r[i] + 0.72 * g[i] + 0.07 * b[i];
                            r[i] = l;
                            g[i] = l;
                            b[i] = l;
                            // any alpha-0 areas are white for multiplicative
                            // blend
                            // expanded border left black and transparent
                            if( a[i] == 0 ) {
                                r[i] = 1;
                                g[i] = 1;
                                b[i] = 1;
                                }
                            }
                        
                        
                        SpriteHandle sprite = fillSprite( expanded, false );
                        
                        doublePair offset = { 0, 0 };
                        
                        mLinesOffset.push_back( offset );
                        mLinesImages.push_back( expanded );
                        mLinesSprites.push_back( sprite );

                        mXTopLinesButton.setVisible( true );
                        }
                    
                    
                    imported = true;
                    delete image;
                    
                    setStatusDirect( NULL, false );
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
    else if( inTarget == &mPrevSpriteImportButton ||
             inTarget == &mNextSpriteImportButton ||
             inTarget == &mPrevLinesImportButton ||
             inTarget == &mNextLinesImportButton ) {
        
        int increment = -1;
        
        
        if( inTarget == &mNextSpriteImportButton ||
            inTarget == &mNextLinesImportButton ) {
            increment = +1;
            }
        
        const char *cacheName = "spriteImportCache";
        int *currentIndex = &mCurrentSpriteImportCacheIndex;

        if( inTarget == &mPrevLinesImportButton ||
            inTarget == &mNextLinesImportButton ) {
            cacheName = "lineImportCache";
            
            currentIndex = &mCurrentLinesImportCacheIndex;
            }

                        
        File cacheDir( NULL, cacheName );

        if( cacheDir.exists() || cacheDir.isDirectory() ) {
            
            *currentIndex += increment;
            
            if( mImportPathOverride != NULL ) {
                delete [] mImportPathOverride;
                
                mImportPathOverride = NULL;
                }
            
            char triedZero = false;

            while( true ) {

                if( *currentIndex < 0 ) {
                    
                    File *nextFile = cacheDir.getChildFile( "next.txt" );
                    
                    *currentIndex = nextFile->readFileIntContents( 1 );
                    
                    *currentIndex = *currentIndex - 1;
                    
                    delete nextFile;
                    }

                char *fileName = autoSprintf( "%d.png", *currentIndex );
                
                if( *currentIndex == 0 ) {
                    triedZero = true;
                    }
                

                File *pngFile = cacheDir.getChildFile( fileName );
                
                delete [] fileName;


                if( pngFile->exists() ) {

                    mImportPathOverride = pngFile->getFullFileName();
                    
                    delete pngFile;
                    break;
                    }
                else {
                    delete pngFile;
                    *currentIndex = 0;

                    if( triedZero ) {
                        break;
                        }
                    }
                }

            
            if( mImportPathOverride != NULL ) {
                
                if( inTarget == &mNextSpriteImportButton ||
                    inTarget == &mPrevSpriteImportButton ) {
                    
                    mNextSpriteImportButton.setVisible( true );
                    mPrevSpriteImportButton.setVisible( true );

                    actionPerformed( &mImportButton );
                    }
                else {
                    mNextLinesImportButton.setVisible( true );
                    mPrevLinesImportButton.setVisible( true );

                    actionPerformed( &mImportLinesButton );
                    }
                }
            else {
                if( inTarget == &mNextSpriteImportButton ||
                    inTarget == &mPrevSpriteImportButton ) {
                    
                    mNextSpriteImportButton.setVisible( false );
                    mPrevSpriteImportButton.setVisible( false );
                    }
                else {
                    mNextLinesImportButton.setVisible( false );
                    mPrevLinesImportButton.setVisible( false );
                    }
                }
            }
        }
    else if( inTarget == &mBlackLineThresholdSlider ) {
        processSelection();
        }
    else if( inTarget == &mBlackLineThresholdDefaultButton ) {
        mBlackLineThresholdSlider.setValue( 0.2 );
        processSelection();
        }
    else if( inTarget == &mSaturationSlider ) {
        processSelection();
        }
    else if( inTarget == &mSaturationDefaultButton ) {
        mSaturationSlider.setValue( 0.5 );
        processSelection();
        }
    else if( inTarget == &mXTopLinesButton ) {
        int numLines = mLinesOffset.size();
        
        if( numLines > 0 ) {
            int i = numLines - 1;
            
            mLinesOffset.deleteElement( i );
            delete mLinesImages.getElementDirect( i );
            mLinesImages.deleteElement( i );
            freeSprite( mLinesSprites.getElementDirect( i ) );
            mLinesSprites.deleteElement( i );
            }
        if( mLinesOffset.size() == 0 ) {
            mXTopLinesButton.setVisible( false );
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
            


            int newID = 
                addSprite( tag, 
                           mProcessedSelectionSprite,
                           mProcessedSelection,
                           mSelectionMultiplicative,
                           mProcessedCenterOffset.x,
                           mProcessedCenterOffset.y );
            
            spritePickable.usePickable( newID );
            
            mSpritePicker.redoSearch( false );
            
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
                                
            int newID = addOverlay( tag, mImportedSheet );

            overlayPickable.usePickable( newID );
            
            // don't let it get freed now
            mImportedSheet = NULL;
            
            mOverlayPicker.redoSearch( false );
            
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
    else if( inTarget == &mSpriteTrimEditorButton ) {
        setSignal( "spriteTrimEditor" );
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mSolidCheckbox ) {
        if( mSolidCheckbox.getToggled() ) {
            mShadowSlider.setVisible( true );
            mBlackLineThresholdSlider.setVisible( true );
            mBlackLineThresholdDefaultButton.setVisible( true );
            mSelectionMultiplicative = false;
            processSelection();
            }
        else {
            mShadowSlider.setVisible( false ); 
            mBlackLineThresholdSlider.setVisible( false );
            mBlackLineThresholdDefaultButton.setVisible( false );
            mSelectionMultiplicative = true;
            processSelection();
            }
        }
    }


float lastMouseX, lastMouseY;

        
void EditorImportPage::drawUnderComponents( doublePair inViewCenter, 
                                            double inViewSize ) {
    
    if( mImportedSheetSprite != NULL ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mImportedSheetSprite, mSheetOffset );

        toggleMultiplicativeBlend( true );
        
        for( int i=0; i<mLinesOffset.size(); i++ ) {
            doublePair pos = 
                add( mSheetOffset, mLinesOffset.getElementDirect( i ) );
            drawSprite( mLinesSprites.getElementDirect( i ), pos );
            }
        toggleMultiplicativeBlend( false );
        
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mWhiteOutSheetSprite, mSheetOffset );


        if( mSelect ) {
            setDrawColor( 0, 0, 1, 0.25 );
            
            drawRect( mSelectStart.x, mSelectStart.y,
                      mSelectEnd.x, mSelectEnd.y );
            }
        
        // always draw crosshairs
        setDrawColor( 0, 0, 1, 0.50 );
        
        doublePair mouseCenter = { lastMouseX + 1, lastMouseY - 1 };
        
        drawRect( mouseCenter, 1000, 0.5 );
        drawRect( mouseCenter, 0.5, 1000 );
        
            
        if( mCenterSet ) {
            setDrawColor( 1, 1, 1, 0.75 );
            drawSprite( mCenterMarkSprite, mCenterPoint );
            }
        
        setDrawColor( 1, 1, 1, 0.75 );
        for( int i=0; i<mInternalPaperPoints.size(); i++ ) {
            drawSprite( mInternalPaperMarkSprite, 
                        add( mSheetOffset, 
                             mInternalPaperPoints.getElementDirect( i ) ) );
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
        
        pos.x -= mProcessedCenterOffset.x;
        pos.y += mProcessedCenterOffset.y;
        
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
    

    if( mOverlayScale.getLastElementDirect() != 1.0 ) {
        doublePair pos = { 400, -240 };
        char *string = 
            autoSprintf( "Scale: %.3f", 
                         mOverlayScale.getLastElementDirect() );
    
        setDrawColor( .5, .5, .5, 1 );
        smallFont->drawString( string, pos, alignLeft );

        delete [] string;
        }
    if( mOverlayRotation.getLastElementDirect() != 0 ) {
        doublePair pos = { 400, -260 };
        char *string = 
            autoSprintf( "Rot: %.3f", 
                         mOverlayRotation.getLastElementDirect() );
    
        setDrawColor( .5, .5, .5, 1 );
        smallFont->drawString( string, pos, alignLeft );
        
        delete [] string;
        }


    setDrawColor( 1, 1, 1, 1 );

    if( mCurrentOverlay.size() > 0 ) {
        doublePair pos = mObjectEditorButton.getPosition();

        pos.y += 20;
        pos.x -= 380;
        
        drawKeyLegend( &mOverlayKeyLegend, pos );
        }
    if( mImportedSheetSprite != NULL ) {
        doublePair pos = mObjectEditorButton.getPosition();
        
        pos.y += 80;
        pos.x -= 240;
        
        drawKeyLegend( &mSheetKeyLegend, pos );

        if( mLinesOffset.size() > 0 ) {
            pos.y -= 112;
            drawKeyLegend( &mLinesKeyLegend, pos );
            }

        pos = mObjectEditorButton.getPosition();
        pos.y += 80;

        drawKeyLegend( &mSheetKeyLegendB, pos );
        }
    
    if( mShowTagMessage ) {
        setDrawColor( 1, 0, 0, 1 );
        
        doublePair pos = mSpriteTagField.getPosition();
        
        pos.y += 40;
        
        smallFont->drawString( "Tag Required", pos, alignCenter );
        }


    setDrawColor( 1, 1, 1, 1 );
    doublePair pos = mSolidCheckbox.getPosition();
    pos.x -= checkboxSep;
    smallFont->drawString( "Solid", pos, alignRight );



    
    doublePair zoomPos = { lastMouseX, lastMouseY };

    pos.x = -500;
    pos.y = -290;
    
    drawZoomView( zoomPos, 16, 4, pos );
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

    mSpritePicker.redoSearch( false );
    mOverlayPicker.redoSearch( false );
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

    
    if( mPlacingInternalPaper ) {
        
        doublePair pos = { inX - mSheetOffset.x,
                           inY - mSheetOffset.y };
        mInternalPaperPoints.push_back( pos );
        }
        

    if( mWhitingOut && mWhiteOutSheet != NULL ) {
        
        int imX = inX - mSheetOffset.x + mSheetW / 2;
        int imY = -inY + mSheetOffset.y + mSheetH / 2;
        
        double *red = mWhiteOutSheet->getChannel( 0 );
        
        int r = 8;
        
        if( imY > r && imY < mSheetH - r &&
            imX > r && imX < mSheetW - r ) {
            
            
            for( int dy=-r; dy<=r; dy++ ) {
                for( int dx=-r; dx<=r; dx++ ) {
                    
                    int pY = imY + dy;
                    int pX = imX + dx;
                    
                    int pI = pY * mSheetW + pX;
    
                    red[ pI ] = 1.0;
                    }
                }
            
            freeSprite( mWhiteOutSheetSprite );
            mWhiteOutSheetSprite = fillWhiteSprite( mWhiteOutSheet );
            mAnyWhiteOutSet = true;
            }
        
        return;
        }

    // middle of screen?
    if( ( inX > -310 && inX < 310 && 
          inY > -210 && inY < 120 ) 
        || 
        // or top-left middle of screen (no gui compoents up there
        ( inX > -310 && inX < 46  && inY > 0 ) ) {
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


    int offset = 1;
        
    if( isCommandKeyDown() ) {
        offset = 5;
        }
    if( isShiftKeyDown() ) {
        offset = 10;
        }
    if( isCommandKeyDown() && isShiftKeyDown() ) {
        offset = 20;
        }

    
    // check ASCII codes for ctrl-i, -j, -k, etc
    if( inASCII == 'i' || inASCII == 'I' || inASCII == 9 ) {
        mSheetOffset.y += offset;
        }
    else if( inASCII == 'j' || inASCII == 'J' || inASCII == 10 ) {
        mSheetOffset.x -= offset;
        }
    else if( inASCII == 'k' || inASCII == 'K' || inASCII == 11 ) {
        mSheetOffset.y -= offset;
        }
    else if( inASCII == 'l' || inASCII == 'L' || inASCII == 12 ) {
        mSheetOffset.x += offset;
        }
    else if( inASCII == 't' ) {
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
    else if( inASCII == 'w' ) {
        mWhitingOut = true;
        }
    else if( inASCII == 'W' ) {
        mWhitingOut = false;
        
        if( mWhiteOutSheet != NULL ) {
            delete mWhiteOutSheet;
            freeSprite( mWhiteOutSheetSprite );
                        
            mWhiteOutSheet = new Image( mSheetW, mSheetH, 1, true );
            mWhiteOutSheetSprite = fillWhiteSprite( mWhiteOutSheet );
            }
        mAnyWhiteOutSet = false;
        }
    else if( inASCII == 'p' ) {
        mPlacingInternalPaper = true;
        }
    else if( inASCII == 'P' ) {
        mPlacingInternalPaper = false;
        
        mInternalPaperPoints.deleteAll();
        }
    else if( inASCII == 'x' ) {
        if( mImportedSheet != NULL ) {
            
            int startImX = (int)( lastMouseX + mSheetW/2 - mSheetOffset.x );
            int startImY = (int)( mSheetH/2 - lastMouseY + mSheetOffset.y );
            
            if( startImX >= 0 && startImX < mSheetW &&
                startImY >= 0 && startImY < mSheetH ) {
                
                Color c = mImportedSheet->getColor( 
                    startImY * mSheetW + startImX );

                FloatRGB fC = { c.r, c.g, c.b };
                
                objectPage->setClipboardColor( fC );
                }            
            }
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
    else if( inASCII == 'w' ) {
        mWhitingOut = false;
        }
    else if( inASCII == 'p' ) {
        mPlacingInternalPaper = false;
        }
    }



void EditorImportPage::specialKeyDown( int inKeyCode ) {

    if( TextField::isAnyFocused() ) {
        return;
        }

    
    if( mLinesOffset.size() > 0 ) {
        int i = mLinesOffset.size() - 1;
        
        int offset = 1;
        
        if( isCommandKeyDown() ) {
            offset = 5;
            }
        if( isShiftKeyDown() ) {
            offset = 10;
            }
        if( isCommandKeyDown() && isShiftKeyDown() ) {
            offset = 20;
            }
    

        switch( inKeyCode ) {
            case MG_KEY_LEFT:
                mLinesOffset.getElement( i )->x -= offset;
            break;
        case MG_KEY_RIGHT:
                mLinesOffset.getElement( i )->x += offset;
            break;
        case MG_KEY_DOWN:
                mLinesOffset.getElement( i )->y -= offset;
            break;
        case MG_KEY_UP:
                mLinesOffset.getElement( i )->y += offset;
            break;
            }
        }
    }



void EditorImportPage::processSelection() {

    // surrounding paper area becomes totally transparent above this brightness 
    double paperThreshold = 0.88;

    
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
    
    if( startImX + imW > mSheetW ) {
        imW = mSheetW - startImX;
        }
    if( startImY + imH > mSheetH ) {
        imH = mSheetH - startImY;
        }
    
    if( imH <= 0 || imW <= 0 ) {
        return;
        }

    
    SimpleVector<doublePair> trimmedPaperPoints;
    
    for( int i=0; i<mInternalPaperPoints.size(); i++ ) {
        doublePair point = mInternalPaperPoints.getElementDirect(i);
        point.y *= -1;
        
        // point is relative to center of imported sheet
        int halfW = mImportedSheet->getWidth() / 2;
        int halfH = mImportedSheet->getHeight() / 2;
        
        int extraX = halfW - startImX;
        int extraY = halfH - startImY;
        
        // now point is relative to start of selection
        point.x += extraX;
        point.y += extraY;
        
        if( point.x >= 0 &&
            point.x < imW &&
            point.y >= 0 &&
            point.y < imH ) {
    
            // keep it
            trimmedPaperPoints.push_back( point );
            }
        }
    
        



    Image *cutImage = 
        mImportedSheet->getSubImage( startImX, startImY, imW, imH );

    
    Image *cutWhiteout = NULL;

    if( mAnyWhiteOutSet ) {
        
        // apply white-out to sheet
        
        cutWhiteout =
            mWhiteOutSheet->getSubImage( startImX, startImY, imW, imH );
        
        int numPixCut = imW * imH;

        double *whiteOutChan = cutWhiteout->getChannel( 0 );
        double *cutImR = cutImage->getChannel( 0 );
        double *cutImG = cutImage->getChannel( 1 );
        double *cutImB = cutImage->getChannel( 2 );
        
        for( int i=0; i<numPixCut; i++ ) {
            if( whiteOutChan[i] > 0 ) {
                cutImR[i] =  1;
                cutImG[i] =  1;
                cutImB[i] =  1;
                }
            }
        }
    
    

    // since this is grayscale, only deal with red channel
    Image *cutLinesImage = NULL;
    double *cutLinesR = NULL;
    
    if( mLinesOffset.size() > 0 ) {
    
        cutLinesImage = new Image( imW, imH, 4, false );
        
        cutLinesR = cutLinesImage->getChannel( 0 );
        //double *cutLinesG = cutLinesImage->getChannel( 1 );
        //double *cutLinesB = cutLinesImage->getChannel( 2 );
        double *cutLinesA = cutLinesImage->getChannel( 3 );

        // start white, then multiply line layers into it
        int numPixels = imW * imH;
        for( int i=0; i<numPixels; i++ ) {
            cutLinesR[i] = 1;
            //cutLinesG[i] = 0;
            //cutLinesB[i] = 0;
            cutLinesA[i] = 1;
            }
        
        // apply line overlays
        for( int i=0; i<mLinesOffset.size(); i++ ) {
            doublePair offset = mLinesOffset.getElementDirect( i );
            
            Image *image = mLinesImages.getElementDirect( i );
            
            int linesW = image->getWidth();
            int linesH = image->getHeight();
            
            int linesRawW = linesW;

            int startLinesImX = startImX - offset.x + ( linesW - mSheetW ) / 2;
            int startLinesImY = startImY + offset.y + ( linesH - mSheetH ) / 2;
            
            int imStartX = 0;
            int imStartY = 0;

            if( startLinesImX < linesW && startLinesImY < linesH ) {
                
                if( startLinesImX < 0 ) {
                    linesW += startLinesImX;
                    imStartX -= startLinesImX;
                    startLinesImX = 0;
                    }
                if( startLinesImY < 0 ) {
                    linesH += startLinesImY;
                    imStartY -= startLinesImY;
                    startLinesImY = 0;
                    }
                
                if( linesW - startLinesImX > imW - imStartX ) {
                    linesW = startLinesImX + ( imW - imStartX );
                    }
                if( linesH - startLinesImY > imH - imStartY ) {
                    linesH = startLinesImY + ( imH - imStartY );
                    }
                

                if( linesH > 0 && linesW > 0 ) {
                    double *linesR = image->getChannel( 0 );
                    
                    int imY = imStartY;
                    for( int y=startLinesImY; y<linesH; y++ ) {
                        int imX = imStartX;
                        for( int x=startLinesImX; x<linesW; x++ ) {
                            int imP = imY * imW + imX;
                            int linesP = y * linesRawW + x;
                            
                            if( linesR[linesP] <= paperThreshold ) {    
                                cutLinesR[imP] *= linesR[linesP];
                                }
                            // else, treat this part of lines as totally
                            // transparent

                            imX ++;
                            }
                        imY ++;
                        }
                    }
                // else skip
                }
            // else skip
            }

        
        if( mAnyWhiteOutSet ) {
            // apply white-out to merged lines
            
            int numPixCut = imW * imH;

            double *whiteOutChan = cutWhiteout->getChannel( 0 );
            
            for( int i=0; i<numPixCut; i++ ) {
                if( whiteOutChan[i] > 0 ) {
                    cutLinesR[i] =  1;
                    }
                }    
            }

        }
    
    
    if( cutWhiteout != NULL ) {
        delete cutWhiteout;
        }
    
        
    
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
    double threshold = mBlackLineThresholdSlider.getValue();


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
    
    for( int i=0; i<trimmedPaperPoints.size(); i++ ) {
        doublePair p = trimmedPaperPoints.getElementDirect( i );
        
        whitePixelsWithUnexploredNeighbors.push_back(
            lrint( p.y ) * imW + lrint( p.x ) );
        }
    


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
                            // Treat these borders as the last white pixel
                            // right at the frontier, but stop here
                            // without exploring their neighbors further.
                            // Thus, when we fill in black later for smooth
                            // outlines, we won't have any light points
                            // lingering in the outline.  This allows
                            // us to have a higher threshold without
                            // white points in the outline.
                            whiteMap[nI] = 1;
                            }
                        }
                    }
                }
            
            }
        

        }


    
    // first, clean up isolated noise points in paper
    
    double *rCopy = cutImage->copyChannel( 0 );
    double *gCopy = cutImage->copyChannel( 1 );
    double *bCopy = cutImage->copyChannel( 2 );    

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
                            ( rCopy[nI] <= paperThreshold &&
                              gCopy[nI] <= paperThreshold &&
                              bCopy[nI] <= paperThreshold ) ) {
                            
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
    
    delete [] rCopy;
    delete [] gCopy;
    delete [] bCopy;
    

    
    
    if( !mSelectionMultiplicative )
    for( int i=0; i<numPixels; i++ ) {
        if( whiteMap[i] == 1 ) {
            
            // alpha based on inverted color level
            // so image is transparent where it is most white
            // this gives our black outlines a proper feathered edge
            
            double maxColor = r[i];
            if( g[i] > maxColor ) {
                maxColor = g[i];
                }
            if( b[i] > maxColor ) {
                maxColor = b[i];
                }
            
            a[i] = 1.0 - maxColor;
            
            
            // however, make sure paper areas that aren't black at all
            // are totally transparent
            if( maxColor > paperThreshold ) {
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


    double satAdjust = mSaturationSlider.getValue();
    
    if( satAdjust != 0 ) {
        cutImage->adjustSaturation( satAdjust );
        }


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
            
            if( a[i] > 0.0 || ( cutLinesR != NULL && cutLinesR[i] < 1.0 ) ) {
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
    Image *trimmedLinesImage = NULL;
    
    if( lastX > firstX && lastY > firstY ) {
        
        trimmedImage = 
            cutImage->getSubImage( firstX, firstY, 
                                   1 + lastX - firstX, 1 + lastY - firstY );
        
        delete cutImage;
        
        if( cutLinesImage != NULL ) {
            trimmedLinesImage = 
                cutLinesImage->getSubImage( firstX, firstY, 
                                            1 + lastX - firstX, 
                                            1 + lastY - firstY );
            delete cutLinesImage;
            }
        }
    else {
        // no non-trans areas, don't trim it
        trimmedImage = cutImage;
        trimmedLinesImage = cutLinesImage;
        }

    
    // make relative to new trim
    centerX -= firstX;
    centerY -= firstY;

    
    // expand to make it big enough for shadow
    w = trimmedImage->getWidth();
    h = trimmedImage->getHeight();

    Image *shadowImage = NULL;
    Image *shadowLinesImage = NULL;
    
    if( ! mSelectionMultiplicative ) {
        
        shadowImage = trimmedImage->expandImage( w + 6 + 6, h + 6 + 6 );
        delete trimmedImage;
        
        if( trimmedLinesImage != NULL ) {
            
            shadowLinesImage = 
                trimmedLinesImage->expandImage( w + 6 + 6, h + 6 + 6 );
    
            delete trimmedLinesImage;
            }
        
        centerX += 6;
        centerY += 6;
        }
    else {
        shadowImage = trimmedImage;
        shadowLinesImage = trimmedLinesImage;
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

    w = shadowImage->getWidth();
    h = shadowImage->getHeight();

    mProcessedSelection = expandToPowersOfTwo( shadowImage );
    delete shadowImage;
    
    Image *expandedLines = NULL;

    if( shadowLinesImage != NULL ) {
        expandedLines = expandToPowersOfTwo( shadowLinesImage );
        delete shadowLinesImage;
        }
    

    int newW = mProcessedSelection->getWidth();
    int newH = mProcessedSelection->getHeight();
    
    centerX += ( newW - w ) / 2;
    centerY += ( newH - h ) / 2;
    
    
    if( expandedLines != NULL ) {
        // set all transparent areas to white (black border added by expansion)
        // grayscale, so only deal with red channel
        r = expandedLines->getChannel( 0 );
        a = expandedLines->getChannel( 3 );

        int numPixels = expandedLines->getWidth() *
            expandedLines->getHeight();
        
        for( int i=0; i<numPixels; i++ ) {
            if( a[i] == 0 ) {
                r[i] =  1;
                }
            }
        }



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
    



    if( expandedLines != NULL ) {
        r = mProcessedSelection->getChannel( 0 );
        g = mProcessedSelection->getChannel( 1 );
        b = mProcessedSelection->getChannel( 2 );
        a = mProcessedSelection->getChannel( 3 );
        
        double *linesR = expandedLines->getChannel( 0 );
        
        int pW = mProcessedSelection->getWidth();
        int pH = mProcessedSelection->getHeight();
        
        int numPixels = pW * pH;


        for( int i=0; i<numPixels; i++ ) {
            if( linesR[i] < 1.0 ) {

                // this will only make things darker
                // and areas outside paper cut-out are already black
                // so places where lines stick out from paper won't
                // be affected
                r[i] *= linesR[i];
                g[i] *= linesR[i];
                b[i] *= linesR[i];

                if( linesR[i] <= paperThreshold ) {
                    // change alpha outside paper cut-out
                    // but only if lines image is darker than
                    // paper
                    // this prevents dark halos


                    // also, ignore isolated noise points

                    int x = i % pW;
                    int y = i / pW;

                    // check if all neighbors over threshold
                    // if so, this is an isolated noise pixel
                    int nDX[4] = { 0, 1, 0, -1 };
                    int nDY[4] = { 1, 0, -1, 0 };
            

                    char foundNonPaperNeighbor = false;
                
                    for( int n=0; n<4; n++ ) {
                    
                        int nX = x + nDX[n];
                        int nY = y + nDY[n];
                        
                        
                        if( nX >= 0 && nX < pW
                            &&
                            nY >= 0 && nY < pH ) {
                            
                            int nI = nY * pW + nX;
                        
                            if( linesR[nI] <= paperThreshold ) {
                                foundNonPaperNeighbor = true;
                                break;
                                }
                            }
                        }

                    if( foundNonPaperNeighbor ) {
                        
                        // not an isolated noise point
                        
                        a[i] += 1.0 - linesR[i];
                
                        if( a[i] > 1.0 ) {
                            a[i] = 1.0;
                            }
                        }
                    }
                }
            }
        
        delete expandedLines;
        }
    
    

    if( usingCenter ) {    
        mProcessedCenterOffset.x = centerX - (newW/2);
        mProcessedCenterOffset.y = centerY - (newH/2);
        
        if( fabs( mProcessedCenterOffset.x ) >= newW/2 ) {
            mProcessedCenterOffset.x = 0;
            }
        if( fabs( mProcessedCenterOffset.y ) >= newH/2 ) {
            mProcessedCenterOffset.y = 0;
            }
        }
    else {
        mProcessedCenterOffset.x = 0;
        mProcessedCenterOffset.y = 0;
        }
    

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



void EditorImportPage::clearLines() {
    
    for( int i=0; i<mLinesOffset.size(); i++ ) {
        delete mLinesImages.getElementDirect(i);
        freeSprite( mLinesSprites.getElementDirect(i) );
        }
    mLinesOffset.deleteAll();
    mLinesImages.deleteAll();
    mLinesSprites.deleteAll();
    }
