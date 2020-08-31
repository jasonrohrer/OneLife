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
    // if it's already a category itself, we must let it be picked
    // so that it can be viewed
    
    // this should only be able to happen for pattern categories (which
    // can also be part of other, non-pattern categories)

    // This is okay, because transitions for regular categories are generated
    // first, followed by transitions for patterns
    CategoryRecord *catR = getCategory( inID );
    
    if( catR != NULL && catR->objectIDSet.size() > 0 ) {
        return false;
        }
    
    

    int num = getNumCategoriesForObject( inID );
    
    for( int i=0; i<num; i++ ) {
        if( ! 
            getCategory( getCategoryForObject( inID, i ) )->isProbabilitySet ) {
            
            // already a child of a non-prob-set category, can't be a parent
            return true;
            }
        }
    
    return false;
    }


char childUnpickable( int inID ) {
    CategoryRecord *catR = getCategory( inID );
    
    if( catR != NULL && ! catR->isPattern && catR->objectIDSet.size() > 0 ) {
        // already a parent, can't be a child, unless it's a pattern category
        // as a parent
        return true;
        }
    return false;
    }



EditorCategoryPage::EditorCategoryPage()
        : mObjectParentPicker( &objectPickableParent, -410, 90 ),
          mObjectChildPicker( &objectPickableChild, +410, 90 ),
          mTransEditorButton( mainFont, 0, 260, "Transitions" ),
          mIsPatternCheckbox( 220, 0, 2 ),
          mIsProbSetCheckbox( 220, -20, 2 ),
          mMakeUniformButton( smallFont, 220, -80, "Make Uniform" ) {
    
    mObjectChildPicker.addFilter( &childUnpickable );
    mObjectParentPicker.addFilter( &parentUnpickable );
    

    addComponent( &mObjectChildPicker );
    addComponent( &mObjectParentPicker );
    addComponent( &mTransEditorButton );
    addComponent( &mIsPatternCheckbox );
    addComponent( &mIsProbSetCheckbox );
    addComponent( &mMakeUniformButton );
    

    mObjectChildPicker.addActionListener( this );
    mObjectParentPicker.addActionListener( this );
    
    mTransEditorButton.addActionListener( this );
    
    mIsPatternCheckbox.addActionListener( this );
    mIsProbSetCheckbox.addActionListener( this );
    
    mMakeUniformButton.addActionListener( this );

    mIsPatternCheckbox.setVisible( false );
    mIsProbSetCheckbox.setVisible( false );
    
    mMakeUniformButton.setVisible( false );

    mCurrentObject = -1;
    mCurrentCategory = -1;
    
    mSelectionIndex = 0;
    mCurrentWeightDigit = 0;

    addKeyClassDescription( &mKeyLegend, "Up/Down", "Change selection" );
    addKeyClassDescription( &mKeyLegend, "Pg Up/Down", "Change order" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyClassDescription( &mKeyLegend, "Bkspce", "Remv item" );

    addKeyDescription( &mKeyLegendPattern, 'd', "Duplicate item in place" );
    addKeyDescription( &mKeyLegendPattern, 'D', "Duplicate item to bottom" );
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
        updateCheckbox();
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
            
            if( mCurrentCategory != -1 ) {
               
                CategoryRecord *cr = getCategory( mCurrentCategory );
                
                if( cr != NULL ) {
                    int newMax =
                        getCategory( mCurrentCategory )->objectIDSet.size() - 1;
                    
                    if( newMax < mSelectionIndex ) {
                        mSelectionIndex = 0;
                        }
                    }
                else {
                    mSelectionIndex = 0;
                    }
                }
            else {
                mSelectionIndex = 0;
                }
            }
        updateCheckbox();
        }
    else if( inTarget == &mTransEditorButton ) {
        setSignal( "transEditor" );
        }    
    else if( inTarget == &mIsPatternCheckbox ) {
        char set = mIsPatternCheckbox.getToggled();
        
        if( mCurrentCategory != -1 ) {
            setCategoryIsPattern( mCurrentCategory, set );
            }
        if( set ) {
            mIsProbSetCheckbox.setToggled( false );
            mMakeUniformButton.setVisible( false );
            }
        }
    else if( inTarget == &mIsProbSetCheckbox ) {
        char set = mIsProbSetCheckbox.getToggled();
        
        if( mCurrentCategory != -1 ) {
            setCategoryIsProbabilitySet( mCurrentCategory, set );
            }
        if( set ) {
            mIsPatternCheckbox.setToggled( false );
            }
        mMakeUniformButton.setVisible( set && mCurrentCategory != -1);
        }
    else if( inTarget == &mMakeUniformButton ) {
        if( mCurrentCategory != -1 ) {
            makeWeightUniform( mCurrentCategory );
            }
        }
    }



static void drawObjectList( char inCategories,
                            char inPattern,
                            SimpleVector<int> *inList,
                            SimpleVector<float> *inWeights = NULL,
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

        doublePair weightTextPos = pos;
        
        if( inWeights != NULL ) {
            doublePair probBoxPos = pos;
            
            double probBoxWidth = 70;
            probBoxPos.x += 150 + probBoxWidth / 2 + 2;
            
            drawRect( probBoxPos, probBoxWidth/2, spacing / 2  );
            
            weightTextPos = probBoxPos;
            weightTextPos.x -= probBoxWidth / 2 - 10;
            }

            
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
        
        if( inWeights != NULL ) {
            float prob = inWeights->getElementDirect( i );
            char *probString = autoSprintf( "%.3f", prob );
            
            smallFont->drawString( probString, 
                                   weightTextPos, alignLeft );
            delete [] probString;
            }


        if( inCategories && getCategory( objID )->isPattern ) {
            
            textPos.x -= 20;
            
            smallFont->drawString( "Pat", 
                                   textPos, alignRight );
            }
        else if( inCategories && getCategory( objID )->isProbabilitySet ) {
            
            textPos.x -= 20;
            
            smallFont->drawString( "Prob", 
                                   textPos, alignRight );
            }
        else if( inPattern ) {
            textPos.x -= 20;

            char *iString = autoSprintf( "%d", i );
            
            smallFont->drawString( iString, 
                                   textPos, alignRight );
            
            delete [] iString;
            }
        
        pos.y -= spacing;
        }
    }


   

     
void EditorCategoryPage::draw( doublePair inViewCenter, 
                               double inViewSize ) {
    
    doublePair legendPos = mTransEditorButton.getPosition();
    
    legendPos.x = -350;
    legendPos.y += 50;
    
    drawKeyLegend( &mKeyLegend, legendPos );


    if( mIsPatternCheckbox.getToggled() ) {
        doublePair otherLegendPos = legendPos;
        
        otherLegendPos.x -= 250;
        
        drawKeyLegend( &mKeyLegendPattern, otherLegendPos );
        }


    doublePair pos = { 200, 150 };
                       
    setDrawColor( 0.75, 0.75, 0.75, 1 );
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
    
        drawObjectList( true, false, &cats, NULL, mSelectionIndex );
        }
    else if( mCurrentCategory != -1 ) {
        CategoryRecord *cat = getCategory( mCurrentCategory );
        
        if( cat != NULL ) {
            
            if( mIsPatternCheckbox.isVisible() ) {
                pos = mIsPatternCheckbox.getPosition();
                pos.x -= 12;
                smallFont->drawString( "Pattern", pos, alignRight );
                }
            if( mIsProbSetCheckbox.isVisible() ) {
                pos = mIsProbSetCheckbox.getPosition();
                pos.x -= 12;
                smallFont->drawString( "Prob Set", pos, alignRight );
                
                if( mIsProbSetCheckbox.getToggled() ) {
                    pos = mIsProbSetCheckbox.getPosition();
                    pos.y -= 20;
                    smallFont->drawString( "0-9 = enter prob digits", pos,
                                           alignCenter );
                    }
                }

            SimpleVector<float> *w = NULL;
            if( cat->isProbabilitySet ) {
                w = &( cat->objectWeights );
                }
            
            drawObjectList( false, cat->isPattern,
                            &( cat->objectIDSet ), w, mSelectionIndex );
            }
        else {
            mIsPatternCheckbox.setToggled( false );
            mIsPatternCheckbox.setVisible( false );

            mIsProbSetCheckbox.setToggled( false );
            mIsProbSetCheckbox.setVisible( false );
            }
        }
    
    }



void EditorCategoryPage::step() {
    }



void EditorCategoryPage::updateCheckbox() {
    char vis = false;
    
    if( mCurrentCategory != -1 ) {
        CategoryRecord *cat = getCategory( mCurrentCategory );
        
        if( cat != NULL ) {
            
            if( cat->objectIDSet.size() > 0 ) {
                mIsPatternCheckbox.setVisible( true );
                mIsPatternCheckbox.setToggled( cat->isPattern );

                mIsProbSetCheckbox.setVisible( true );
                mIsProbSetCheckbox.setToggled( cat->isProbabilitySet );
                mMakeUniformButton.setVisible( cat->isProbabilitySet );
                vis = true;
                }
            else {
                // category still exists, but it's not saved on 
                // disk if empty
                // hide checkbox to avoid implying that pattern
                // status is saved for an empty category
                vis = false;
                }
            }
        }

    if( vis == false ) {
        mIsPatternCheckbox.setVisible( false );
        mIsPatternCheckbox.setToggled( false );
        mIsProbSetCheckbox.setVisible( false );
        mIsProbSetCheckbox.setToggled( false );
        mMakeUniformButton.setVisible( false );
        }
    }




void EditorCategoryPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mCurrentWeightDigit = 0;
    
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
        mCurrentWeightDigit = 0;
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
            
            removeObjectFromCategory( mCurrentCategory, objID, 
                                      mSelectionIndex );
            
            int newMax =
                getCategory( mCurrentCategory )->objectIDSet.size() - 1;
                
            if( mSelectionIndex > newMax ) {
                mSelectionIndex = newMax;
                }
            if( mSelectionIndex < 0 ) {
                mSelectionIndex = 0;
                }
            }
        
        updateCheckbox();
        }

    if( inASCII >= '0' && inASCII <= '9' && mCurrentCategory != -1 ) {
        CategoryRecord *r = getCategory( mCurrentCategory );
        
        if( r->isProbabilitySet ) {
            
            float oldWeight = 
                r->objectWeights.getElementDirect( mSelectionIndex );
            
            char *oldString = autoSprintf( "%.3f", oldWeight );
            
            if( mCurrentWeightDigit > 2 ) {
                mCurrentWeightDigit = 0;
                }
            
            oldString[ mCurrentWeightDigit + 2 ] = inASCII;
            
            // zero out digits beyond this current one
            // because we're typing a new value
            for( int i=mCurrentWeightDigit + 1; i<3; i++ ) {
                oldString[ i + 2 ] = '0';
                }
            

            mCurrentWeightDigit++;
            
            float newWeight = 0.0f;
            
            sscanf( oldString, "%f", &newWeight );
            
            delete [] oldString;

            setMemberWeight( mCurrentCategory, 
                             r->objectIDSet.getElementDirect( mSelectionIndex ),
                             newWeight );
            }
        }
    
    if( inASCII == 'd' || inASCII == 'D' ) {
        CategoryRecord *r = getCategory( mCurrentCategory );

        if( r != NULL && r->objectIDSet.size() > mSelectionIndex ) {
            int objID = r->objectIDSet.getElementDirect( mSelectionIndex );
            addCategoryToObject( objID, mCurrentCategory );

            int oldIndex = mSelectionIndex;
            
            mSelectionIndex = r->objectIDSet.size() - 1;

            if( inASCII == 'd' ) {
                // insert in place, move up from bottom
                int targetIndex = oldIndex + 1;
                
                int offset = mSelectionIndex - targetIndex;
                
                for( int i=0; i<offset; i++ ) {
                    moveCategoryMemberUp( mCurrentCategory, objID, 
                                          mSelectionIndex );
                    mSelectionIndex --;
                    }
                }

            updateCheckbox();
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
            mCurrentWeightDigit = 0;
            
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
            mCurrentWeightDigit = 0;
            
            if( mSelectionIndex < 0 ) {
                mSelectionIndex = 0;
                }
            break;
            }
        case MG_KEY_PAGE_UP:
            mCurrentWeightDigit = 0;
            /*
              the underlying implementation of this is not working
              not written to disk
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
            */
            if( mCurrentCategory != -1 ) {
                
                CategoryRecord *r = getCategory( mCurrentCategory );

                if( r != NULL &&
                    r->objectIDSet.size() > mSelectionIndex ) {
                    
                    int objID = 
                        r->objectIDSet.getElementDirect( mSelectionIndex );
                    
                    for( int i=0; i<offset; i++ ) {
                        moveCategoryMemberUp( mCurrentCategory, objID, 
                                              mSelectionIndex );
                        mSelectionIndex --;
                        }
                    if( mSelectionIndex < 0 ) {
                        mSelectionIndex = 0;
                        }
                    }
                }
            break;
        case MG_KEY_PAGE_DOWN:
            mCurrentWeightDigit = 0;
            /*
              the underlying implementation of this is not working
              not written to disk
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
            */
            if( mCurrentCategory != -1 ) {
                
                CategoryRecord *r = getCategory( mCurrentCategory );

                if( r != NULL &&
                    r->objectIDSet.size() > mSelectionIndex ) {
                    
                    int objID = 
                        r->objectIDSet.getElementDirect( mSelectionIndex );
                    
                    for( int i=0; i<offset; i++ ) {
                        moveCategoryMemberDown( mCurrentCategory, objID, 
                                                mSelectionIndex );
                        mSelectionIndex ++;
                        }
                    if( mSelectionIndex > r->objectIDSet.size() - 1 ) {
                        mSelectionIndex = 
                            r->objectIDSet.size() - 1;
                        }
                    }
                }
            break;
        }

    
            
    }
