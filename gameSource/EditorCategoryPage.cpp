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
          mTransEditorButton( mainFont, 0, 260, "Transitions" ) {

    addComponent( &mObjectPicker );
    addComponent( &mCategoryPicker );
    addComponent( &mTransEditorButton );

    
    mObjectPicker.addActionListener( this );
    mCategoryPicker.addActionListener( this );
    
    mTransEditorButton.addActionListener( this );
    
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
    }





   

     
void EditorCategoryPage::draw( doublePair inViewCenter, 
                               double inViewSize ) {
    
    
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
