#include "EditorCategoryPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"



#include "message.h"







extern Font *mainFont;
extern Font *smallFont;



#include "ObjectPickable.h"

static ObjectPickable objectPickable;


#include "CategoryPickable.h"

static CategoryPickable categoryPickable;



EditorCategoryPage::EditorCategoryPage()
        : mObjectPicker( &objectPickable, +410, 90 ),
          mCategoryPicker( &categoryPickable, -410, 90 ),
          mTransEditorButton( mainFont, 0, 260, "Transitions" ),
          mNewCategoryButton( mainFont, 200, -250, "Add" ),
          mNewCategoryField( mainFont, 
                             -100,  -250, 10,
                             false,
                             "New Category", NULL, NULL ) {

    addComponent( &mObjectPicker );
    addComponent( &mCategoryPicker );
    addComponent( &mTransEditorButton );


    addComponent( &mNewCategoryButton );
    addComponent( &mNewCategoryField );
    
    
    mObjectPicker.addActionListener( this );
    mCategoryPicker.addActionListener( this );
    
    mTransEditorButton.addActionListener( this );
    
    mNewCategoryButton.addActionListener( this );

    mNewCategoryField.setFireOnAnyTextChange( true );
    mNewCategoryField.addActionListener( this );

    mCurrentObject = -1;

    mSelectionIndex = 0;


    addKeyClassDescription( &mKeyLegend, "Up/Down", "Change selection" );
    addKeyClassDescription( &mKeyLegend, "Pg Up/Down", "Change order" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyClassDescription( &mKeyLegend, "Bkspce", "Remv category" );
    }



EditorCategoryPage::~EditorCategoryPage() {
    }



void EditorCategoryPage::clearUseOfObject( int inObjectID ) {
    if( mCurrentObject == inObjectID ) {
        mCurrentObject = -1;
        }
    }




void EditorCategoryPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectPicker ) {
        mCurrentObject = mObjectPicker.getSelectedObject();
        }
    else if( inTarget == &mCategoryPicker ) {
        int cID = mCategoryPicker.getSelectedObject();

        if( mCurrentObject != -1 && cID != -1 ) {
            
            addCategoryToObject( mCurrentObject, cID );
            
            // just-added becomes selected
            int count = getNumCategoriesForObject( mCurrentObject );
            mSelectionIndex = count - 1;
            }
        }
    else if( inTarget == &mTransEditorButton ) {
        setSignal( "transEditor" );
        }
    else if( inTarget == &mNewCategoryField ) {
        char *text = mNewCategoryField.getText();
        
        char *trim = trimWhitespace( text );

        delete [] text;

        if( strcmp( trim, "" ) != 0 ) {
            mNewCategoryButton.setVisible( true );
            }
        else {
            mNewCategoryButton.setVisible( false );
            }
        
        delete [] trim;
        }
    else if( inTarget == &mNewCategoryButton ) {
        char *text = mNewCategoryField.getText();
        
        char *trim = trimWhitespace( text );

        delete [] text;
        
        addCategory( trim );
        
        delete [] trim;
        
        mNewCategoryButton.setVisible( false );
        
        mNewCategoryField.unfocus();

        mCategoryPicker.redoSearch();
        }
    
    }





   

     
void EditorCategoryPage::draw( doublePair inViewCenter, 
                               double inViewSize ) {
    
    doublePair legendPos = mTransEditorButton.getPosition();
    
    legendPos.x = -350;
    legendPos.y += 50;
    
    drawKeyLegend( &mKeyLegend, legendPos );



    doublePair pos = { 200, 150 };
                       
    setDrawColor( 1, 1, 1, 1 );
    drawSquare( pos, 50 );
    
    if( mCurrentObject != -1 ) {
        ObjectRecord *r = getObject( mCurrentObject );
        
        int maxD = getMaxDiameter( r );
        
        double zoom = 1;
        
        if( maxD > 128 ) {
            zoom = 128.0 / maxD;
            }
        
        pos = sub( pos, mult( getObjectCenterOffset( r ), zoom ) );

        drawObject( r, 2, pos, 0, false, false, 20, 0, false,
                    false,
                    getEmptyClothingSet(), zoom );

        setDrawColor( 1, 1, 1, 1 );

        pos.x = 200;
        pos.y = 150;
        pos.y -= 60;
        smallFont->drawString( r->description, pos, alignCenter );

        int numCats = getNumCategoriesForObject( mCurrentObject );
        
        FloatColor boxColor = { 0.25f, 0.25f, 0.25f, 1.0f };
        
        FloatColor altBoxColor = { 0.35f, 0.35f, 0.35f, 1.0f };

        FloatColor selBoxColor = { 0.35f, 0.35f, 0.0f, 1.0f };

        
        pos.x = -100;
        pos.y = 185;
        
        for( int i=0; i<numCats; i++ ) {
            
            int cID = getCategoryForObject( mCurrentObject, i );

            if( mSelectionIndex == i ) {
                setDrawColor( selBoxColor );
                }
            else {
                setDrawColor( boxColor );
                }

            drawRect( pos, 150, 15 );
            
            FloatColor tempColor = boxColor;
            boxColor = altBoxColor;
            
            altBoxColor = tempColor;
            

            if( mSelectionIndex == i ) {
                setDrawColor( 1, 1, 0, 1 );
                }
            else {
                setDrawColor( 1, 1, 1, 1 );
                }
            
            doublePair textPos = pos;
            textPos.x -= 140;
            
            smallFont->drawString( getCategory( cID )->description, 
                                   textPos, alignLeft );

            pos.y -= 30;
            }
        }
    }



void EditorCategoryPage::step() {
    }




void EditorCategoryPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch();
    mCategoryPicker.redoSearch();
    }



void EditorCategoryPage::pointerMove( float inX, float inY ) {
    }


void EditorCategoryPage::pointerDown( float inX, float inY ) {

    }




void EditorCategoryPage::pointerDrag( float inX, float inY ) {
    //doublePair pos = {inX, inY};
    }



void EditorCategoryPage::pointerUp( float inX, float inY ) {
    }



void EditorCategoryPage::keyDown( unsigned char inASCII ) {
    if( mNewCategoryField.isAnyFocused() ) {
        return;
        }
    
    if( inASCII == 8 ) {
        // backspace
        if( mCurrentObject != -1 ) {
            int catID = getCategoryForObject( mCurrentObject, mSelectionIndex );
            
            if( catID != -1 ) {
                removeCategoryFromObject( mCurrentObject, catID );

                int newMax = 
                    getNumCategoriesForObject( mCurrentObject ) - 1;
                
                if( mSelectionIndex > newMax ) {
                    mSelectionIndex = newMax;
                    }
                if( mSelectionIndex < 0 ) {
                    mSelectionIndex = 0;
                    }
                }
            }
        }
    
    }



void EditorCategoryPage::specialKeyDown( int inKeyCode ) {


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
        case MG_KEY_DOWN: {
            
            mSelectionIndex += offset;

            int max = 0;
            if( mCurrentObject != -1 ) {
                max = getNumCategoriesForObject( mCurrentObject ) - 1;
                }
            
            if( mSelectionIndex > max ) {
                mSelectionIndex = max;
                
                if( mSelectionIndex > max  || mSelectionIndex < 0 ) {
                    mSelectionIndex = 0;
                    }
                }

            break;
            }
        case MG_KEY_UP: {
            mSelectionIndex -= offset;
            if( mSelectionIndex < 0 ) {
                mSelectionIndex = 0;
                }
            break;
            }
        case MG_KEY_PAGE_UP:
            if( mCurrentObject != -1 ) {
                
                int curCat = getCategoryForObject( mCurrentObject, 
                                                   mSelectionIndex );
                if( curCat != -1 ) {
                    for( int i=0; i<offset; i++ ) {
                        mSelectionIndex --;
                        moveCategoryUp( mCurrentObject, curCat );
                        }
                    if( mSelectionIndex < 0 ) {
                        mSelectionIndex = 0;
                        }
                    }
                }    
            break;
        case MG_KEY_PAGE_DOWN:
            if( mCurrentObject != -1 ) {
                
                int curCat = getCategoryForObject( mCurrentObject, 
                                                   mSelectionIndex );
                if( curCat != -1 ) {
                    for( int i=0; i<offset; i++ ) {
                        mSelectionIndex ++;
                        moveCategoryDown( mCurrentObject, curCat );
                        }
                    if( mSelectionIndex > 
                        getNumCategoriesForObject( mCurrentObject ) - 1 ) {
                        
                        mSelectionIndex = 
                            getNumCategoriesForObject( mCurrentObject ) - 1;
                        }
                    }
                }
            break;
        }

    
            
    }
