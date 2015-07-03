#include "EditorImportPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;


#include "SpritePickable.h"

static SpritePickable spritePickable;



EditorImportPage::EditorImportPage()
        : mImportButton( mainFont, -210, 260, "Import" ),
          mSelect( false ),
          mImportedSheet( NULL ),
          mImportedSheetSprite( NULL ),
          mProcessedSelection( NULL ),
          mProcessedSelectionSprite( NULL ),
          mSpriteTagField( mainFont, 
                           0,  -260, 6,
                           false,
                           "Sprite Tag", NULL, " " ),
          mSaveSpriteButton( mainFont, 210, -260, "Save" ),
          mSpritePicker( &spritePickable, -310, 100 ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ) {

    addComponent( &mImportButton );
    addComponent( &mSpriteTagField );
    addComponent( &mSaveSpriteButton );
    addComponent( &mSpritePicker );
    addComponent( &mObjectEditorButton );

    mImportButton.addActionListener( this );
    mSaveSpriteButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );

    mSaveSpriteButton.setVisible( false );
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




void EditorImportPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mImportButton ) {
        
        char *importPath = 
            SettingsManager::getStringSetting( "editorImportPath" );
        
        if( importPath != NULL ) {
            PNGImageConverter converter;
            
            File importFile( NULL, importPath );
            
            char imported = false;
            
            if( importFile.exists() ) {
                
                FileInputStream stream( &importFile );
                
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
                char *message = autoSprintf( "Failed to import PNG from:##"
                                             "%s", importPath );
                
                setStatusDirect( message, true );
                delete message;
                }
            
            delete [] importPath;
            }
        else {
            setStatus( "Import path not set in settings folder", true );
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
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    

    }


float lastMouseX, lastMouseY;

        
void EditorImportPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    doublePair pos = { 0, 0 };
    
    if( mImportedSheetSprite != NULL ) {
        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mImportedSheetSprite, pos );


        if( mSelect ) {
            setDrawColor( 0, 0, 1, 0.25 );
            
            drawRect( mSelectStart.x, mSelectStart.y,
                      mSelectEnd.x, mSelectEnd.y );
            }
        }

    if( mProcessedSelectionSprite != NULL ) {
        pos.x = lastMouseX;
        pos.y = lastMouseY;
        
        setDrawColor( 1, 1, 1, 1 );
        
        //drawSquare( pos, 100 );

        setDrawColor( 1, 1, 1, 1 );
        drawSprite( mProcessedSelectionSprite, pos );
        }
    
    }


void EditorImportPage::step() {
    }




void EditorImportPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }


    }


void EditorImportPage::pointerMove( float inX, float inY ) {
    lastMouseX = inX;
    lastMouseY = inY;
    }


void EditorImportPage::pointerDown( float inX, float inY ) {
    if( mImportedSheetSprite == NULL ) {
        return;
        }

    if( inX > - mSheetW / 2
        &&
        inX < mSheetW / 2 
        &&
        inY > - mSheetH / 2
        &&
        inY < mSheetH / 2 ) {
        
        mSelectStart.x = inX;
        mSelectStart.y = inY;
        mSelectEnd.x = inX + 1;
        mSelectEnd.y = inY - 1;
        mSelect = true;
        }
    }



void EditorImportPage::pointerDrag( float inX, float inY ) {
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




void EditorImportPage::processSelection() {
    
    if( mProcessedSelection != NULL ) {
        delete mProcessedSelection;
        mProcessedSelection = NULL;
        }
    

    Image *cutImage = 
        mImportedSheet->getSubImage( (int)( mSelectStart.x + mSheetW/2 ), 
                                     (int)( mSheetH/2 - mSelectStart.y ), 
                                     (int)( mSelectEnd.x - mSelectStart.x ), 
                                     (int)( mSelectStart.y - mSelectEnd.y ) );

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
    
    Image *trimmedImage = 
        cutImage->getSubImage( firstX, firstY, 
                               1 + lastX - firstX, 1 + lastY - firstY );
    
    delete cutImage;
    
    
    // expand to make it big enough for shadow
    w = trimmedImage->getWidth();
    h = trimmedImage->getHeight();
    
    Image *shadowImage = trimmedImage->expandImage( w + 20 + 20, h + 20 + 20 );
    
    delete trimmedImage;



    addShadow( shadowImage );    
    
    
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
    }
