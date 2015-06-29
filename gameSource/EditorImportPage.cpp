#include "EditorImportPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;



EditorImportPage::EditorImportPage()
        : mImportButton( mainFont, -210, 260, "Import" ),
          mSelect( false ),
          mImportedSheet( NULL ),
          mImportedSheetSprite( NULL ),
          mProcessedSelection( NULL ),
          mProcessedSelectionSprite( NULL ) {

    addComponent( &mImportButton );
    

    mImportButton.addActionListener( this );
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
    
    
    for( int i=0; i<numPixels; i++ ) {
        if( whiteMap[i] == 1 ) {
            
            // alpha based on inverted color level
            // so image is transparent where it is most white
            // this gives our black outlines a proper feathered edge
            a[i] = 1.0 - r[i];
            
            
            // however, make sure paper areas that aren't black at all
            // are totally transparent
            if( r[i] > 0.92 ) {
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
    

    mProcessedSelection = expandToPowersOfTwo( cutImage );
    
    delete cutImage;

    if( mProcessedSelectionSprite != NULL ) {
        freeSprite( mProcessedSelectionSprite );
        mProcessedSelectionSprite = NULL;
        }
    mProcessedSelectionSprite = fillSprite( mProcessedSelection, false );
    }
