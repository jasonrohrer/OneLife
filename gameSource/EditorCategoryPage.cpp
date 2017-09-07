#include "EditorCategoryPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"



#include "message.h"







extern Font *mainFont;
extern Font *smallFont;



#include "ObjectPickable.h"

static ObjectPickable objectPickableParent;

static ObjectPickable objectPickableChild;




char parentUnpickable( int inID ) {
    if( getNumCategoriesForObject( inID ) > 0 ) {
        // already a child, can't be a parent
        return true;
        }
    return false;
    }


char childUnpickable( int inID ) {
    CategoryRecord *catR = getCategory( inID );
    
    if( catR != NULL && catR->objectIDSet.size() > 0 ) {
        // already a parent, can't be a child
        return true;
        }
    return false;
    }



EditorCategoryPage::EditorCategoryPage()
        : mObjectParentPicker( &objectPickableParent, -410, 90 ),
          mObjectChildPicker( &objectPickableChild, +410, 90 ),
          mTransEditorButton( mainFont, 0, 260, "Transitions" ) {
    
    mObjectChildPicker.addFilter( &childUnpickable );
    mObjectParentPicker.addFilter( &parentUnpickable );
    

    addComponent( &mObjectChildPicker );
    addComponent( &mObjectParentPicker );
    addComponent( &mTransEditorButton );


    mObjectChildPicker.addActionListener( this );
    mObjectParentPicker.addActionListener( this );
    
    mTransEditorButton.addActionListener( this );
    

    mCurrentObject = -1;
    mCurrentCategory = -1;
    
    mSelectionIndex = 0;


    addKeyClassDescription( &mKeyLegend, "Up/Down", "Change selection" );
    addKeyClassDescription( &mKeyLegend, "Pg Up/Down", "Change order" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyClassDescription( &mKeyLegend, "Bkspce", "Remv item" );
    }



EditorCategoryPage::~EditorCategoryPage() {
    }



void EditorCategoryPage::clearUseOfObject( int inObjectID ) {
    if( mCurrentObject == inObjectID ) {
        mCurrentObject = -1;
        }
    else if( mCurrentCategory == inObjectID ) {
        mCurrentCategory = -1;
        }
        
    mObjectParentPicker.redoSearch( false );
    mObjectChildPicker.redoSearch( false );
    }




void EditorCategoryPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectChildPicker ) {
        if( mCurrentCategory == -1 ) {
            mCurrentObject = mObjectChildPicker.getSelectedObject();
            }
        else {
            int obj = mObjectChildPicker.getSelectedObject();
            
            if( obj != -1 ) {
                addCategoryToObject( obj, mCurrentCategory );
                }
            }
        }
    else if( inTarget == &mObjectParentPicker ) {
        int parentID = mObjectParentPicker.getSelectedObject();

        if( mCurrentObject != -1 && parentID != -1 ) {
            
            addCategoryToObject( mCurrentObject, parentID );
            
            // just-added becomes selected
            int count = getNumCategoriesForObject( mCurrentObject );
            mSelectionIndex = count - 1;
            
            mCurrentCategory = -1;
            }
        else {
            mCurrentCategory = parentID;
            mSelectionIndex = 0;
            }
        }
    else if( inTarget == &mTransEditorButton ) {
        setSignal( "transEditor" );
        }    
    }



static void drawObjectList( SimpleVector<int> *inList,
                            int inSelectionIndex = -1 ) {

    int num = inList->size();
        
    FloatColor boxColor = { 0.25f, 0.25f, 0.25f, 1.0f };
    
    FloatColor altBoxColor = { 0.35f, 0.35f, 0.35f, 1.0f };
    
    FloatColor selBoxColor = { 0.35f, 0.35f, 0.0f, 1.0f };

    doublePair pos;
    
    pos.x = -100;
    pos.y = 150;
        
    double spacing = 30;

    if( inSelectionIndex > 7 && num > 12 ) {    
        pos.y += ( inSelectionIndex - 7 )  * spacing;
        }
        

    for( int i=0; i<num; i++ ) {
            
        int objID = inList->getElementDirect( i );

        if( inSelectionIndex == i ) {
            setDrawColor( selBoxColor );
            }
        else {

            setDrawColor( boxColor );
            }

        float fade = 1;
            
        if( num > 12 && pos.y < -130 ) {
            fade = 1 - ( -130 - pos.y ) / 95;
                
            if( fade < 0 ) {
                fade = 0;
                }
            }

        if( pos.y > 130 ) {
            fade = 1 - ( pos.y - 130 ) / 95;
                
            if( fade < 0 ) {
                fade = 0;
                }
            }
            
            
        setDrawFade( fade );
        drawRect( pos, 150, spacing / 2  );
            
        FloatColor tempColor = boxColor;
        boxColor = altBoxColor;
            
        altBoxColor = tempColor;
            

        if( inSelectionIndex == i ) {
            setDrawColor( 1, 1, 0, 1 );
            }
        else {
            setDrawColor( 1, 1, 1, 1 );
            }
            
        doublePair textPos = pos;
        textPos.x -= 140;
            
        setDrawFade( fade );

        smallFont->drawString( getObject( objID )->description, 
                               textPos, alignLeft );

        pos.y -= spacing;
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
        
        SimpleVector<int> cats;
        
        for( int i=0; i<numCats; i++ ) {
            cats.push_back( getCategoryForObject( mCurrentObject, i ) );
            }
    
        drawObjectList( &cats, mSelectionIndex );
        }
    else if( mCurrentCategory != -1 ) {
        CategoryRecord *cat = getCategory( mCurrentCategory );
        
        if( cat != NULL ) {
            drawObjectList( &( cat->objectIDSet ), mSelectionIndex );
            }
        }
    
    }



void EditorCategoryPage::step() {
    }




void EditorCategoryPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectChildPicker.redoSearch( false );
    mObjectParentPicker.redoSearch( false );
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
    if( TextField::isAnyFocused() ) {
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
        else if( mCurrentCategory != -1 ) {
            int objID = getCategory( mCurrentCategory )->
                objectIDSet.getElementDirect( mSelectionIndex );
            
            removeCategoryFromObject( objID, mCurrentCategory );
            
            int newMax =
                getCategory( mCurrentCategory )->objectIDSet.size() - 1;
                
            if( mSelectionIndex > newMax ) {
                mSelectionIndex = newMax;
                }
            if( mSelectionIndex < 0 ) {
                mSelectionIndex = 0;
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
            else if( mCurrentCategory != -1 ) {
                max = getCategory( mCurrentCategory )->objectIDSet.size() - 1;
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
