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
    }



EditorCategoryPage::~EditorCategoryPage() {
    }




void EditorCategoryPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectPicker ) {
        mCurrentObject = mObjectPicker.getSelectedObject();
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
        
        mCategoryPicker.redoSearch();
        }
    
    }





   

     
void EditorCategoryPage::draw( doublePair inViewCenter, 
                               double inViewSize ) {
    
    doublePair pos = { 0, 150 };
                       
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

        pos.x = 0;
        pos.y = 150;
        pos.y -= 60;
        smallFont->drawString( r->description, pos, alignCenter );
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
        
    
        
    }



void EditorCategoryPage::specialKeyDown( int inKeyCode ) {
    
            
    }
