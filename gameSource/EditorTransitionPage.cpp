#include "EditorTransitionPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;
extern Font *smallFont;



#include "ObjectPickable.h"

static ObjectPickable objectPickable;



EditorTransitionPage::EditorTransitionPage()
        : mSaveTransitionButton( smallFont, 210, -260, "Save New" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ) {

    addComponent( &mSaveTransitionButton );
    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    

    mSaveTransitionButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mSaveTransitionButton.setVisible( false );
    }



EditorTransitionPage::~EditorTransitionPage() {
    }








void EditorTransitionPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mSaveTransitionButton ) {
        
        }
    else if( inTarget == &mObjectPicker ) {
        int objectID = mObjectPicker.getSelectedObject();

        if( objectID != -1 ) {
            ObjectRecord *pickedRecord = getObject( objectID );
            }
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    
    
    }

   

     
void EditorTransitionPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    }



void EditorTransitionPage::step() {
    }




void EditorTransitionPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch();

    }



void EditorTransitionPage::pointerMove( float inX, float inY ) {
    }


void EditorTransitionPage::pointerDown( float inX, float inY ) {

    }




void EditorTransitionPage::pointerDrag( float inX, float inY ) {
    doublePair pos = {inX, inY};
    }



void EditorTransitionPage::pointerUp( float inX, float inY ) {
    }



void EditorTransitionPage::keyDown( unsigned char inASCII ) {
        
    
        
    }



void EditorTransitionPage::specialKeyDown( int inKeyCode ) {
    
            
    }
