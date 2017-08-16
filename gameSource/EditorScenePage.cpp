#include "EditorScenePage.h"


#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"



extern Font *mainFont;
extern Font *smallFont;


#include "GroundPickable.h"

static GroundPickable groundPickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;




EditorScenePage::EditorScenePage()
        : mAnimEditorButton( mainFont, -210, 260, "Anim" ),
          mSaveNewButton( mainFont, 400, 64, "Save" ),
          mGroundPicker( &groundPickable, -410, 90 ),
          mObjectPicker( &objectPickable, 410, 90 ) {
    
    addComponent( &mAnimEditorButton );
    mAnimEditorButton.addActionListener( this );

    addComponent( &mGroundPicker );
    mGroundPicker.addActionListener( this );

    addComponent( &mObjectPicker );
    mObjectPicker.addActionListener( this );


    addComponent( &mSaveNewButton );
    mSaveNewButton.addActionListener( this );
    }



EditorScenePage::~EditorScenePage() {
    }




void EditorScenePage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mAnimEditorButton ) {
        setSignal( "animEditor" );
        }
    else if( inTarget == &mGroundPicker ) {
        
        }
    else if( inTarget == &mObjectPicker ) {
        
        }
    else if( inTarget == &mSaveNewButton ) {
        }
    }
        



void EditorScenePage::drawUnderComponents( doublePair inViewCenter, 
                                           double inViewSize ) {
    
    }



void EditorScenePage::makeActive( char inFresh ) {
    
    if( !inFresh ) {
        return;
        }

    mGroundPicker.redoSearch( true );
    mObjectPicker.redoSearch( true );
    }

